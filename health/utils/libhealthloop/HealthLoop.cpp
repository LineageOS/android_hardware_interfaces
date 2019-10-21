/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "HealthLoop"
#define KLOG_LEVEL 6

#include <health/HealthLoop.h>

#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include <android-base/logging.h>
#include <batteryservice/BatteryService.h>
#include <cutils/klog.h>
#include <cutils/uevent.h>
#include <healthd/healthd.h>
#include <utils/Errors.h>

#include <health/utils.h>

using namespace android;
using namespace std::chrono_literals;

#define POWER_SUPPLY_SUBSYSTEM "power_supply"

namespace android {
namespace hardware {
namespace health {

HealthLoop::HealthLoop() {
    InitHealthdConfig(&healthd_config_);
    awake_poll_interval_ = -1;
    wakealarm_wake_interval_ = healthd_config_.periodic_chores_interval_fast;
}

HealthLoop::~HealthLoop() {
    LOG(FATAL) << "HealthLoop cannot be destroyed";
}

int HealthLoop::RegisterEvent(int fd, BoundFunction func, EventWakeup wakeup) {
    CHECK(!reject_event_register_);

    auto* event_handler =
            event_handlers_
                    .emplace_back(std::make_unique<EventHandler>(EventHandler{this, fd, func}))
                    .get();

    struct epoll_event ev;

    ev.events = EPOLLIN;

    if (wakeup == EVENT_WAKEUP_FD) ev.events |= EPOLLWAKEUP;

    ev.data.ptr = reinterpret_cast<void*>(event_handler);

    if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
        KLOG_ERROR(LOG_TAG, "epoll_ctl failed; errno=%d\n", errno);
        return -1;
    }

    return 0;
}

void HealthLoop::WakeAlarmSetInterval(int interval) {
    struct itimerspec itval;

    if (wakealarm_fd_ == -1) return;

    wakealarm_wake_interval_ = interval;

    if (interval == -1) interval = 0;

    itval.it_interval.tv_sec = interval;
    itval.it_interval.tv_nsec = 0;
    itval.it_value.tv_sec = interval;
    itval.it_value.tv_nsec = 0;

    if (timerfd_settime(wakealarm_fd_, 0, &itval, NULL) == -1)
        KLOG_ERROR(LOG_TAG, "wakealarm_set_interval: timerfd_settime failed\n");
}

void HealthLoop::AdjustWakealarmPeriods(bool charger_online) {
    // Fast wake interval when on charger (watch for overheat);
    // slow wake interval when on battery (watch for drained battery).

    int new_wake_interval = charger_online ? healthd_config_.periodic_chores_interval_fast
                                           : healthd_config_.periodic_chores_interval_slow;

    if (new_wake_interval != wakealarm_wake_interval_) WakeAlarmSetInterval(new_wake_interval);

    // During awake periods poll at fast rate.  If wake alarm is set at fast
    // rate then just use the alarm; if wake alarm is set at slow rate then
    // poll at fast rate while awake and let alarm wake up at slow rate when
    // asleep.

    if (healthd_config_.periodic_chores_interval_fast == -1)
        awake_poll_interval_ = -1;
    else
        awake_poll_interval_ = new_wake_interval == healthd_config_.periodic_chores_interval_fast
                                       ? -1
                                       : healthd_config_.periodic_chores_interval_fast * 1000;
}

void HealthLoop::PeriodicChores() {
    ScheduleBatteryUpdate();
}

// TODO(b/140330870): Use BPF instead.
#define UEVENT_MSG_LEN 2048
void HealthLoop::UeventEvent(uint32_t /*epevents*/) {
    // No need to lock because uevent_fd_ is guaranteed to be initialized.

    char msg[UEVENT_MSG_LEN + 2];
    char* cp;
    int n;

    n = uevent_kernel_multicast_recv(uevent_fd_, msg, UEVENT_MSG_LEN);
    if (n <= 0) return;
    if (n >= UEVENT_MSG_LEN) /* overflow -- discard */
        return;

    msg[n] = '\0';
    msg[n + 1] = '\0';
    cp = msg;

    while (*cp) {
        if (!strcmp(cp, "SUBSYSTEM=" POWER_SUPPLY_SUBSYSTEM)) {
            ScheduleBatteryUpdate();
            break;
        }

        /* advance to after the next \0 */
        while (*cp++)
            ;
    }
}

void HealthLoop::UeventInit(void) {
    uevent_fd_.reset(uevent_open_socket(64 * 1024, true));

    if (uevent_fd_ < 0) {
        KLOG_ERROR(LOG_TAG, "uevent_init: uevent_open_socket failed\n");
        return;
    }

    fcntl(uevent_fd_, F_SETFL, O_NONBLOCK);
    if (RegisterEvent(uevent_fd_, &HealthLoop::UeventEvent, EVENT_WAKEUP_FD))
        KLOG_ERROR(LOG_TAG, "register for uevent events failed\n");
}

void HealthLoop::WakeAlarmEvent(uint32_t /*epevents*/) {
    // No need to lock because wakealarm_fd_ is guaranteed to be initialized.

    unsigned long long wakeups;

    if (read(wakealarm_fd_, &wakeups, sizeof(wakeups)) == -1) {
        KLOG_ERROR(LOG_TAG, "wakealarm_event: read wakealarm fd failed\n");
        return;
    }

    PeriodicChores();
}

void HealthLoop::WakeAlarmInit(void) {
    wakealarm_fd_.reset(timerfd_create(CLOCK_BOOTTIME_ALARM, TFD_NONBLOCK));
    if (wakealarm_fd_ == -1) {
        KLOG_ERROR(LOG_TAG, "wakealarm_init: timerfd_create failed\n");
        return;
    }

    if (RegisterEvent(wakealarm_fd_, &HealthLoop::WakeAlarmEvent, EVENT_WAKEUP_FD))
        KLOG_ERROR(LOG_TAG, "Registration of wakealarm event failed\n");

    WakeAlarmSetInterval(healthd_config_.periodic_chores_interval_fast);
}

void HealthLoop::MainLoop(void) {
    int nevents = 0;
    while (1) {
        reject_event_register_ = true;
        size_t eventct = event_handlers_.size();
        struct epoll_event events[eventct];
        int timeout = awake_poll_interval_;

        int mode_timeout;

        /* Don't wait for first timer timeout to run periodic chores */
        if (!nevents) PeriodicChores();

        Heartbeat();

        mode_timeout = PrepareToWait();
        if (timeout < 0 || (mode_timeout > 0 && mode_timeout < timeout)) timeout = mode_timeout;
        nevents = epoll_wait(epollfd_, events, eventct, timeout);
        if (nevents == -1) {
            if (errno == EINTR) continue;
            KLOG_ERROR(LOG_TAG, "healthd_mainloop: epoll_wait failed\n");
            break;
        }

        for (int n = 0; n < nevents; ++n) {
            if (events[n].data.ptr) {
                auto* event_handler = reinterpret_cast<EventHandler*>(events[n].data.ptr);
                event_handler->func(event_handler->object, events[n].events);
            }
        }
    }

    return;
}

int HealthLoop::InitInternal() {
    epollfd_.reset(epoll_create1(EPOLL_CLOEXEC));
    if (epollfd_ == -1) {
        KLOG_ERROR(LOG_TAG, "epoll_create1 failed; errno=%d\n", errno);
        return -1;
    }

    // Call subclass's init for any additional init steps.
    // Note that healthd_config_ is initialized before wakealarm_fd_; see
    // AdjustUeventWakealarmPeriods().
    Init(&healthd_config_);

    WakeAlarmInit();
    UeventInit();

    return 0;
}

int HealthLoop::StartLoop() {
    int ret;

    klog_set_level(KLOG_LEVEL);

    ret = InitInternal();
    if (ret) {
        KLOG_ERROR(LOG_TAG, "Initialization failed, exiting\n");
        return 2;
    }

    MainLoop();
    KLOG_ERROR(LOG_TAG, "Main loop terminated, exiting\n");
    return 3;
}

}  // namespace health
}  // namespace hardware
}  // namespace android
