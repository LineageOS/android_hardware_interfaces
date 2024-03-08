/******************************************************************************
 *
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************
 * Originally developed and contributed by Ittiam Systems Pvt. Ltd, Bangalore
 */

#ifndef __VEHICLE_MANAGER_FUZZER_H__
#define __VEHICLE_MANAGER_FUZZER_H__

#include <vhal_v2_0/VehicleHalManager.h>
#include <vhal_v2_0/VehiclePropertyStore.h>
#include <vhal_v2_0/VmsUtils.h>

#include <VehicleHalTestUtils.h>
#include <fuzzer/FuzzedDataProvider.h>

namespace android::hardware::automotive::vehicle::V2_0::fuzzer {

constexpr int kRetriableAttempts = 3;

class MockedVehicleHal : public VehicleHal {
  public:
    MockedVehicleHal()
        : mFuelCapacityAttemptsLeft(kRetriableAttempts),
          mMirrorFoldAttemptsLeft(kRetriableAttempts) {
        mConfigs.assign(std::begin(kVehicleProperties), std::end(kVehicleProperties));
    }

    std::vector<VehiclePropConfig> listProperties() override { return mConfigs; }

    VehiclePropValuePtr get(const VehiclePropValue& requestedPropValue,
                            StatusCode* outStatus) override;

    StatusCode set(const VehiclePropValue& propValue) override {
        if (toInt(VehicleProperty::MIRROR_FOLD) == propValue.prop &&
            mMirrorFoldAttemptsLeft-- > 0) {
            return StatusCode::TRY_AGAIN;
        }

        mValues[makeKey(propValue)] = propValue;
        return StatusCode::OK;
    }

    StatusCode subscribe(int32_t /* property */, float /* sampleRate */) override {
        return StatusCode::OK;
    }

    StatusCode unsubscribe(int32_t /* property */) override { return StatusCode::OK; }

    void sendPropEvent(recyclable_ptr<VehiclePropValue> value) { doHalEvent(std::move(value)); }

    void sendHalError(StatusCode error, int32_t property, int32_t areaId) {
        doHalPropertySetError(error, property, areaId);
    }

  private:
    int64_t makeKey(const VehiclePropValue& v) const { return makeKey(v.prop, v.areaId); }

    int64_t makeKey(int32_t prop, int32_t area) const {
        return (static_cast<int64_t>(prop) << 32) | area;
    }

  private:
    std::vector<VehiclePropConfig> mConfigs;
    std::unordered_map<int64_t, VehiclePropValue> mValues;
    int mFuelCapacityAttemptsLeft;
    int mMirrorFoldAttemptsLeft;
};

class VehicleHalManagerFuzzer {
  public:
    VehicleHalManagerFuzzer() {
        mHal.reset(new MockedVehicleHal);
        mManager.reset(new VehicleHalManager(mHal.get()));
        mObjectPool = mHal->getValuePool();
    }
    ~VehicleHalManagerFuzzer() {
        mManager.reset(nullptr);
        mHal.reset(nullptr);
        mObjectPool = nullptr;
        if (mFuzzedDataProvider) {
            delete mFuzzedDataProvider;
        }
    }
    void process(const uint8_t* data, size_t size);

    template <typename T>
    void fillParameter(size_t size, std::vector<T>& data) {
        for (size_t i = 0; i < size; ++i) {
            data.push_back(mFuzzedDataProvider->ConsumeIntegral<T>());
        }
    }

  private:
    FuzzedDataProvider* mFuzzedDataProvider = nullptr;
    VehiclePropValue mActualValue = VehiclePropValue{};
    StatusCode mActualStatusCode = StatusCode::OK;

    VehiclePropValuePool* mObjectPool = nullptr;
    std::unique_ptr<MockedVehicleHal> mHal;
    std::unique_ptr<VehicleHalManager> mManager;

    void invokeDebug();
    void initValue();
    void invokePropConfigs();
    void invokeSubscribe();
    void invokeSetAndGetValues();
    void invokeObd2SensorStore();
    void invokeVmsUtils();
    void invokeVehiclePropStore();
    void invokeWatchDogClient();
    void invokeGetSubscribedLayers(VmsMessageType type);
    void invokeGet(int32_t property, int32_t areaId);
};

}  // namespace android::hardware::automotive::vehicle::V2_0::fuzzer

#endif  // __VEHICLE_MANAGER_FUZZER_H__
