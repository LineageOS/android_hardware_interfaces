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

#include "PowerStats.h"

#include "FakeEnergyConsumer.h"
#include "FakeEnergyMeter.h"
#include "FakeStateResidencyDataProvider.h"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using aidl::android::hardware::power::stats::EnergyConsumerType;
using aidl::android::hardware::power::stats::FakeEnergyConsumer;
using aidl::android::hardware::power::stats::FakeEnergyMeter;
using aidl::android::hardware::power::stats::FakeStateResidencyDataProvider;
using aidl::android::hardware::power::stats::PowerStats;
using aidl::android::hardware::power::stats::State;

void setFakeEnergyMeter(std::shared_ptr<PowerStats> p) {
    p->setEnergyMeter(
            std::make_unique<FakeEnergyMeter>(std::vector<std::pair<std::string, std::string>>{
                    {"Rail1", "Display"},
                    {"Rail2", "CPU"},
                    {"Rail3", "Modem"},
            }));
}

void addFakeStateResidencyDataProvider1(std::shared_ptr<PowerStats> p) {
    p->addStateResidencyDataProvider(std::make_unique<FakeStateResidencyDataProvider>(
            "CPU", std::vector<State>{{0, "Idle"}, {1, "Active"}}));
}

void addFakeStateResidencyDataProvider2(std::shared_ptr<PowerStats> p) {
    p->addStateResidencyDataProvider(std::make_unique<FakeStateResidencyDataProvider>(
            "Display", std::vector<State>{{0, "Off"}, {1, "On"}}));
}

void addFakeEnergyConsumer1(std::shared_ptr<PowerStats> p) {
    p->addEnergyConsumer(std::make_unique<FakeEnergyConsumer>(EnergyConsumerType::OTHER, "GPU"));
}

void addFakeEnergyConsumer2(std::shared_ptr<PowerStats> p) {
    p->addEnergyConsumer(
            std::make_unique<FakeEnergyConsumer>(EnergyConsumerType::MOBILE_RADIO, "MODEM"));
}

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    std::shared_ptr<PowerStats> p = ndk::SharedRefBase::make<PowerStats>();

    setFakeEnergyMeter(p);

    addFakeStateResidencyDataProvider1(p);
    addFakeStateResidencyDataProvider2(p);

    addFakeEnergyConsumer1(p);
    addFakeEnergyConsumer2(p);

    const std::string instance = std::string() + PowerStats::descriptor + "/default";
    binder_status_t status = AServiceManager_addService(p->asBinder().get(), instance.c_str());
    CHECK(status == STATUS_OK);

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
