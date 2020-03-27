/*
 * Copyright (C) 2018 The Android Open Source Project
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

#define LOG_TAG "neuralnetworks_hidl_hal_test"

#include "VtsHalNeuralnetworks.h"
#include "1.0/Callbacks.h"
#include "GeneratedTestHarness.h"
#include "TestHarness.h"

#include <android-base/logging.h>
#include <hidl/ServiceManagement.h>
#include <string>
#include <utility>

namespace android::hardware::neuralnetworks::V1_0::vts::functional {

using implementation::PreparedModelCallback;

void createPreparedModel(const sp<IDevice>& device, const Model& model,
                         sp<IPreparedModel>* preparedModel) {
    ASSERT_NE(nullptr, preparedModel);
    *preparedModel = nullptr;

    // see if service can handle model
    bool fullySupportsModel = false;
    const Return<void> supportedCall = device->getSupportedOperations(
            model, [&fullySupportsModel](ErrorStatus status, const hidl_vec<bool>& supported) {
                ASSERT_EQ(ErrorStatus::NONE, status);
                ASSERT_NE(0ul, supported.size());
                fullySupportsModel = std::all_of(supported.begin(), supported.end(),
                                                 [](bool valid) { return valid; });
            });
    ASSERT_TRUE(supportedCall.isOk());

    // launch prepare model
    const sp<PreparedModelCallback> preparedModelCallback = new PreparedModelCallback();
    const Return<ErrorStatus> prepareLaunchStatus =
            device->prepareModel(model, preparedModelCallback);
    ASSERT_TRUE(prepareLaunchStatus.isOk());
    ASSERT_EQ(ErrorStatus::NONE, static_cast<ErrorStatus>(prepareLaunchStatus));

    // retrieve prepared model
    preparedModelCallback->wait();
    const ErrorStatus prepareReturnStatus = preparedModelCallback->getStatus();
    *preparedModel = preparedModelCallback->getPreparedModel();

    // The getSupportedOperations call returns a list of operations that are
    // guaranteed not to fail if prepareModel is called, and
    // 'fullySupportsModel' is true i.f.f. the entire model is guaranteed.
    // If a driver has any doubt that it can prepare an operation, it must
    // return false. So here, if a driver isn't sure if it can support an
    // operation, but reports that it successfully prepared the model, the test
    // can continue.
    if (!fullySupportsModel && prepareReturnStatus != ErrorStatus::NONE) {
        ASSERT_EQ(nullptr, preparedModel->get());
        LOG(INFO) << "NN VTS: Early termination of test because vendor service cannot prepare "
                     "model that it does not support.";
        std::cout << "[          ]   Early termination of test because vendor service cannot "
                     "prepare model that it does not support."
                  << std::endl;
        GTEST_SKIP();
    }
    ASSERT_EQ(ErrorStatus::NONE, prepareReturnStatus);
    ASSERT_NE(nullptr, preparedModel->get());
}

void NeuralnetworksHidlTest::SetUp() {
    testing::TestWithParam<NeuralnetworksHidlTestParam>::SetUp();
    ASSERT_NE(kDevice, nullptr);
}

static NamedDevice makeNamedDevice(const std::string& name) {
    return {name, IDevice::getService(name)};
}

static std::vector<NamedDevice> getNamedDevicesImpl() {
    // Retrieves the name of all service instances that implement IDevice,
    // including any Lazy HAL instances.
    const std::vector<std::string> names = hardware::getAllHalInstanceNames(IDevice::descriptor);

    // Get a handle to each device and pair it with its name.
    std::vector<NamedDevice> namedDevices;
    namedDevices.reserve(names.size());
    std::transform(names.begin(), names.end(), std::back_inserter(namedDevices), makeNamedDevice);
    return namedDevices;
}

const std::vector<NamedDevice>& getNamedDevices() {
    const static std::vector<NamedDevice> devices = getNamedDevicesImpl();
    return devices;
}

std::string printNeuralnetworksHidlTest(
        const testing::TestParamInfo<NeuralnetworksHidlTestParam>& info) {
    return gtestCompliantName(getName(info.param));
}

INSTANTIATE_DEVICE_TEST(NeuralnetworksHidlTest);

// Forward declaration from ValidateModel.cpp
void validateModel(const sp<IDevice>& device, const Model& model);
// Forward declaration from ValidateRequest.cpp
void validateRequest(const sp<IPreparedModel>& preparedModel, const Request& request);

void validateEverything(const sp<IDevice>& device, const Model& model, const Request& request) {
    validateModel(device, model);

    // Create IPreparedModel.
    sp<IPreparedModel> preparedModel;
    createPreparedModel(device, model, &preparedModel);
    if (preparedModel == nullptr) return;

    validateRequest(preparedModel, request);
}

TEST_P(ValidationTest, Test) {
    const Model model = createModel(kTestModel);
    ExecutionContext context;
    const Request request = context.createRequest(kTestModel);
    ASSERT_FALSE(kTestModel.expectFailure);
    validateEverything(kDevice, model, request);
}

INSTANTIATE_GENERATED_TEST(ValidationTest, [](const std::string& testName) {
    // Skip validation for the "inputs_as_internal" and "all_tensors_as_inputs"
    // generated tests.
    return testName.find("inputs_as_internal") == std::string::npos &&
           testName.find("all_tensors_as_inputs") == std::string::npos;
});

}  // namespace android::hardware::neuralnetworks::V1_0::vts::functional
