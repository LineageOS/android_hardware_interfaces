/*
 * Copyright (C) 2021 The Android Open Source Project
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

#define LOG_TAG "neuralnetworks_aidl_hal_test"
#include "VtsHalNeuralnetworks.h"

#include <android-base/logging.h>
#include <android/binder_auto_utils.h>
#include <android/binder_interface_utils.h>
#include <android/binder_manager.h>
#include <android/binder_status.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <utility>

#include <TestHarness.h>
#include <aidl/Vintf.h>
#include <nnapi/hal/aidl/Conversions.h>

#include "Callbacks.h"
#include "GeneratedTestHarness.h"
#include "Utils.h"

namespace aidl::android::hardware::neuralnetworks::vts::functional {

using implementation::PreparedModelCallback;

// internal helper function
void createPreparedModel(const std::shared_ptr<IDevice>& device, const Model& model,
                         std::shared_ptr<IPreparedModel>* preparedModel, bool reportSkipping) {
    ASSERT_NE(nullptr, preparedModel);
    *preparedModel = nullptr;

    // see if service can handle model
    std::vector<bool> supportedOperations;
    const auto supportedCallStatus = device->getSupportedOperations(model, &supportedOperations);
    ASSERT_TRUE(supportedCallStatus.isOk());
    ASSERT_NE(0ul, supportedOperations.size());
    const bool fullySupportsModel = std::all_of(
            supportedOperations.begin(), supportedOperations.end(), [](bool v) { return v; });

    // launch prepare model
    const std::shared_ptr<PreparedModelCallback> preparedModelCallback =
            ndk::SharedRefBase::make<PreparedModelCallback>();
    const auto prepareLaunchStatus =
            device->prepareModel(model, ExecutionPreference::FAST_SINGLE_ANSWER, kDefaultPriority,
                                 kNoDeadline, {}, {}, kEmptyCacheToken, preparedModelCallback);
    ASSERT_TRUE(prepareLaunchStatus.isOk()) << prepareLaunchStatus.getDescription();

    // retrieve prepared model
    preparedModelCallback->wait();
    const ErrorStatus prepareReturnStatus = preparedModelCallback->getStatus();
    *preparedModel = preparedModelCallback->getPreparedModel();

    // The getSupportedOperations call returns a list of operations that are guaranteed not to fail
    // if prepareModel is called, and 'fullySupportsModel' is true i.f.f. the entire model is
    // guaranteed. If a driver has any doubt that it can prepare an operation, it must return false.
    // So here, if a driver isn't sure if it can support an operation, but reports that it
    // successfully prepared the model, the test can continue.
    if (!fullySupportsModel && prepareReturnStatus != ErrorStatus::NONE) {
        ASSERT_EQ(nullptr, preparedModel->get());
        if (!reportSkipping) {
            return;
        }
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

void NeuralNetworksAidlTest::SetUp() {
    testing::TestWithParam<NeuralNetworksAidlTestParam>::SetUp();
    ASSERT_NE(kDevice, nullptr);
    const bool deviceIsResponsive =
            ndk::ScopedAStatus::fromStatus(AIBinder_ping(kDevice->asBinder().get())).isOk();
    ASSERT_TRUE(deviceIsResponsive);
}

static NamedDevice makeNamedDevice(const std::string& name) {
    ndk::SpAIBinder binder(AServiceManager_waitForService(name.c_str()));
    return {name, IDevice::fromBinder(binder)};
}

static std::vector<NamedDevice> getNamedDevicesImpl() {
    // Retrieves the name of all service instances that implement IDevice,
    // including any Lazy HAL instances.
    const std::vector<std::string> names = ::android::getAidlHalInstanceNames(IDevice::descriptor);

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

std::string printNeuralNetworksAidlTest(
        const testing::TestParamInfo<NeuralNetworksAidlTestParam>& info) {
    return gtestCompliantName(getName(info.param));
}

INSTANTIATE_DEVICE_TEST(NeuralNetworksAidlTest);

// Forward declaration from ValidateModel.cpp
void validateModel(const std::shared_ptr<IDevice>& device, const Model& model);
// Forward declaration from ValidateRequest.cpp
void validateRequest(const std::shared_ptr<IPreparedModel>& preparedModel, const Request& request);
// Forward declaration from ValidateRequest.cpp
void validateBurst(const std::shared_ptr<IPreparedModel>& preparedModel, const Request& request);
// Forward declaration from ValidateRequest.cpp
void validateRequestFailure(const std::shared_ptr<IPreparedModel>& preparedModel,
                            const Request& request);

void validateEverything(const std::shared_ptr<IDevice>& device, const Model& model,
                        const Request& request) {
    validateModel(device, model);

    // Create IPreparedModel.
    std::shared_ptr<IPreparedModel> preparedModel;
    createPreparedModel(device, model, &preparedModel);
    if (preparedModel == nullptr) return;

    validateRequest(preparedModel, request);
    validateBurst(preparedModel, request);
    // HIDL also had test that expected executeFenced to fail on received null fd (-1). This is not
    // allowed in AIDL and will result in EX_TRANSACTION_FAILED.
}

void validateFailure(const std::shared_ptr<IDevice>& device, const Model& model,
                     const Request& request) {
    // TODO: Should this always succeed?
    //       What if the invalid input is part of the model (i.e., a parameter).
    validateModel(device, model);

    // Create IPreparedModel.
    std::shared_ptr<IPreparedModel> preparedModel;
    createPreparedModel(device, model, &preparedModel);
    if (preparedModel == nullptr) return;

    validateRequestFailure(preparedModel, request);
}

TEST_P(ValidationTest, Test) {
    const Model model = createModel(kTestModel);
    ExecutionContext context;
    const Request request = context.createRequest(kTestModel);
    if (kTestModel.expectFailure) {
        validateFailure(kDevice, model, request);
    } else {
        validateEverything(kDevice, model, request);
    }
}

INSTANTIATE_GENERATED_TEST(ValidationTest, [](const std::string& testName) {
    // Skip validation for the "inputs_as_internal" and "all_tensors_as_inputs"
    // generated tests.
    return testName.find("inputs_as_internal") == std::string::npos &&
           testName.find("all_tensors_as_inputs") == std::string::npos;
});

std::string toString(Executor executor) {
    switch (executor) {
        case Executor::SYNC:
            return "SYNC";
        case Executor::BURST:
            return "BURST";
        case Executor::FENCED:
            return "FENCED";
        default:
            CHECK(false);
    }
}

}  // namespace aidl::android::hardware::neuralnetworks::vts::functional
