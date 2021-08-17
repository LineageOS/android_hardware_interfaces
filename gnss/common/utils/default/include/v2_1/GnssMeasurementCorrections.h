/*
 * Copyright (C) 2019 The Android Open Source Project
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

#pragma once

#include <android/hardware/gnss/measurement_corrections/1.1/IMeasurementCorrections.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android::hardware::gnss::measurement_corrections::V1_1::implementation {

struct GnssMeasurementCorrections : public IMeasurementCorrections {
    GnssMeasurementCorrections();
    ~GnssMeasurementCorrections();

    // Methods from V1_0::IMeasurementCorrections follow.
    Return<bool> setCorrections(const V1_0::MeasurementCorrections& corrections) override;
    Return<bool> setCallback(const sp<V1_0::IMeasurementCorrectionsCallback>& callback) override;

    // Methods from V1_1::IMeasurementCorrections follow.
    Return<bool> setCorrections_1_1(const V1_1::MeasurementCorrections& corrections) override;
};

}  // namespace android::hardware::gnss::measurement_corrections::V1_1::implementation
