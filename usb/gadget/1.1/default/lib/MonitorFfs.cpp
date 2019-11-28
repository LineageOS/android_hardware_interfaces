/*
 * Copyright (C) 2020 The Android Open Source Project
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

#define LOG_TAG "libusbconfigfs"

#include "include/UsbGadgetCommon.h"

namespace android {
namespace hardware {
namespace usb {
namespace gadget {

static volatile bool gadgetPullup;

MonitorFfs::MonitorFfs(const char* const gadget)
    : mWatchFd(),
      mEndpointList(),
      mLock(),
      mCv(),
      mLockFd(),
      mCurrentUsbFunctionsApplied(false),
      mMonitor(),
      mCallback(NULL),
      mPayload(NULL),
      mGadgetName(gadget),
      mMonitorRunning(false) {
    unique_fd eventFd(eventfd(0, 0));
    if (eventFd == -1) {
        ALOGE("mEventFd failed to create %d", errno);
        abort();
    }

    unique_fd epollFd(epoll_create(2));
    if (epollFd == -1) {
        ALOGE("mEpollFd failed to create %d", errno);
        abort();
    }

    unique_fd inotifyFd(inotify_init());
    if (inotifyFd < 0) {
        ALOGE("inotify init failed");
        abort();
    }

    if (addEpollFd(epollFd, inotifyFd) == -1) abort();

    if (addEpollFd(epollFd, eventFd) == -1) abort();

    mEpollFd = move(epollFd);
    mInotifyFd = move(inotifyFd);
    mEventFd = move(eventFd);
    gadgetPullup = false;
}

static void displayInotifyEvent(struct inotify_event* i) {
    ALOGE("    wd =%2d; ", i->wd);
    if (i->cookie > 0) ALOGE("cookie =%4d; ", i->cookie);

    ALOGE("mask = ");
    if (i->mask & IN_ACCESS) ALOGE("IN_ACCESS ");
    if (i->mask & IN_ATTRIB) ALOGE("IN_ATTRIB ");
    if (i->mask & IN_CLOSE_NOWRITE) ALOGE("IN_CLOSE_NOWRITE ");
    if (i->mask & IN_CLOSE_WRITE) ALOGE("IN_CLOSE_WRITE ");
    if (i->mask & IN_CREATE) ALOGE("IN_CREATE ");
    if (i->mask & IN_DELETE) ALOGE("IN_DELETE ");
    if (i->mask & IN_DELETE_SELF) ALOGE("IN_DELETE_SELF ");
    if (i->mask & IN_IGNORED) ALOGE("IN_IGNORED ");
    if (i->mask & IN_ISDIR) ALOGE("IN_ISDIR ");
    if (i->mask & IN_MODIFY) ALOGE("IN_MODIFY ");
    if (i->mask & IN_MOVE_SELF) ALOGE("IN_MOVE_SELF ");
    if (i->mask & IN_MOVED_FROM) ALOGE("IN_MOVED_FROM ");
    if (i->mask & IN_MOVED_TO) ALOGE("IN_MOVED_TO ");
    if (i->mask & IN_OPEN) ALOGE("IN_OPEN ");
    if (i->mask & IN_Q_OVERFLOW) ALOGE("IN_Q_OVERFLOW ");
    if (i->mask & IN_UNMOUNT) ALOGE("IN_UNMOUNT ");
    ALOGE("\n");

    if (i->len > 0) ALOGE("        name = %s\n", i->name);
}

void* MonitorFfs::startMonitorFd(void* param) {
    MonitorFfs* monitorFfs = (MonitorFfs*)param;
    char buf[kBufferSize];
    bool writeUdc = true, stopMonitor = false;
    struct epoll_event events[kEpollEvents];
    steady_clock::time_point disconnect;

    bool descriptorWritten = true;
    for (int i = 0; i < static_cast<int>(monitorFfs->mEndpointList.size()); i++) {
        if (access(monitorFfs->mEndpointList.at(i).c_str(), R_OK)) {
            descriptorWritten = false;
            break;
        }
    }

    // notify here if the endpoints are already present.
    if (descriptorWritten) {
        usleep(kPullUpDelay);
        if (!!WriteStringToFile(monitorFfs->mGadgetName, PULLUP_PATH)) {
            lock_guard<mutex> lock(monitorFfs->mLock);
            monitorFfs->mCurrentUsbFunctionsApplied = true;
            monitorFfs->mCallback(monitorFfs->mCurrentUsbFunctionsApplied, monitorFfs->mPayload);
            gadgetPullup = true;
            writeUdc = false;
            ALOGI("GADGET pulled up");
            monitorFfs->mCv.notify_all();
        }
    }

    while (!stopMonitor) {
        int nrEvents = epoll_wait(monitorFfs->mEpollFd, events, kEpollEvents, -1);

        if (nrEvents <= 0) {
            ALOGE("epoll wait did not return descriptor number");
            continue;
        }

        for (int i = 0; i < nrEvents; i++) {
            ALOGI("event=%u on fd=%d\n", events[i].events, events[i].data.fd);

            if (events[i].data.fd == monitorFfs->mInotifyFd) {
                // Process all of the events in buffer returned by read().
                int numRead = read(monitorFfs->mInotifyFd, buf, kBufferSize);
                for (char* p = buf; p < buf + numRead;) {
                    struct inotify_event* event = (struct inotify_event*)p;
                    if (kDebug) displayInotifyEvent(event);

                    p += sizeof(struct inotify_event) + event->len;

                    bool descriptorPresent = true;
                    for (int j = 0; j < static_cast<int>(monitorFfs->mEndpointList.size()); j++) {
                        if (access(monitorFfs->mEndpointList.at(j).c_str(), R_OK)) {
                            if (kDebug) ALOGI("%s absent", monitorFfs->mEndpointList.at(j).c_str());
                            descriptorPresent = false;
                            break;
                        }
                    }

                    if (!descriptorPresent && !writeUdc) {
                        if (kDebug) ALOGI("endpoints not up");
                        writeUdc = true;
                        disconnect = std::chrono::steady_clock::now();
                    } else if (descriptorPresent && writeUdc) {
                        steady_clock::time_point temp = steady_clock::now();

                        if (std::chrono::duration_cast<microseconds>(temp - disconnect).count() <
                            kPullUpDelay)
                            usleep(kPullUpDelay);

                        if (!!WriteStringToFile(monitorFfs->mGadgetName, PULLUP_PATH)) {
                            lock_guard<mutex> lock(monitorFfs->mLock);
                            monitorFfs->mCurrentUsbFunctionsApplied = true;
                            monitorFfs->mCallback(monitorFfs->mCurrentUsbFunctionsApplied,
                                                  monitorFfs->mPayload);
                            ALOGI("GADGET pulled up");
                            writeUdc = false;
                            gadgetPullup = true;
                            // notify the main thread to signal userspace.
                            monitorFfs->mCv.notify_all();
                        }
                    }
                }
            } else {
                uint64_t flag;
                read(monitorFfs->mEventFd, &flag, sizeof(flag));
                if (flag == 100) {
                    stopMonitor = true;
                    break;
                }
            }
        }
    }
    return NULL;
}

void MonitorFfs::reset() {
    lock_guard<mutex> lock(mLockFd);
    uint64_t flag = 100;
    unsigned long ret;

    if (mMonitorRunning) {
        // Stop the monitor thread by writing into signal fd.
        ret = TEMP_FAILURE_RETRY(write(mEventFd, &flag, sizeof(flag)));
        if (ret < 0) ALOGE("Error writing eventfd errno=%d", errno);

        ALOGI("mMonitor signalled to exit");
        mMonitor->join();
        ALOGI("mMonitor destroyed");
        mMonitorRunning = false;
    }

    for (std::vector<int>::size_type i = 0; i != mWatchFd.size(); i++)
        inotify_rm_watch(mInotifyFd, mWatchFd[i]);

    mEndpointList.clear();
    gadgetPullup = false;
    mCallback = NULL;
    mPayload = NULL;
}

bool MonitorFfs::startMonitor() {
    mMonitor = unique_ptr<thread>(new thread(this->startMonitorFd, this));
    mMonitorRunning = true;
    return true;
}

bool MonitorFfs::isMonitorRunning() {
    return mMonitorRunning;
}

bool MonitorFfs::waitForPullUp(int timeout_ms) {
    std::unique_lock<std::mutex> lk(mLock);

    if (gadgetPullup) return true;

    if (mCv.wait_for(lk, timeout_ms * 1ms, [] { return gadgetPullup; })) {
        ALOGI("monitorFfs signalled true");
        return true;
    } else {
        ALOGI("monitorFfs signalled error");
        // continue monitoring as the descriptors might be written at a later
        // point.
        return false;
    }
}

bool MonitorFfs::addInotifyFd(string fd) {
    lock_guard<mutex> lock(mLockFd);
    int wfd;

    wfd = inotify_add_watch(mInotifyFd, fd.c_str(), IN_ALL_EVENTS);
    if (wfd == -1)
        return false;
    else
        mWatchFd.push_back(wfd);

    return true;
}

void MonitorFfs::addEndPoint(string ep) {
    lock_guard<mutex> lock(mLockFd);

    mEndpointList.push_back(ep);
}

void MonitorFfs::registerFunctionsAppliedCallback(void (*callback)(bool functionsApplied,
                                                                   void* payload),
                                                  void* payload) {
    mCallback = callback;
    mPayload = payload;
}

}  // namespace gadget
}  // namespace usb
}  // namespace hardware
}  // namespace android
