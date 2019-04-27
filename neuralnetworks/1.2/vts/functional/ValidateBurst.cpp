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

#include "Callbacks.h"
#include "ExecutionBurstController.h"
#include "ExecutionBurstServer.h"
#include "TestHarness.h"
#include "Utils.h"

#include <android-base/logging.h>

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_2 {
namespace vts {
namespace functional {

using ::android::nn::ExecutionBurstController;
using ::android::nn::RequestChannelSender;
using ::android::nn::ResultChannelReceiver;
using ExecutionBurstCallback = ::android::nn::ExecutionBurstController::ExecutionBurstCallback;

constexpr size_t kExecutionBurstChannelLength = 1024;
constexpr size_t kExecutionBurstChannelSmallLength = 8;

///////////////////////// UTILITY FUNCTIONS /////////////////////////

static bool badTiming(Timing timing) {
    return timing.timeOnDevice == UINT64_MAX && timing.timeInDriver == UINT64_MAX;
}

static void createBurst(const sp<IPreparedModel>& preparedModel, const sp<IBurstCallback>& callback,
                        std::unique_ptr<RequestChannelSender>* sender,
                        std::unique_ptr<ResultChannelReceiver>* receiver,
                        sp<IBurstContext>* context) {
    ASSERT_NE(nullptr, preparedModel.get());
    ASSERT_NE(nullptr, sender);
    ASSERT_NE(nullptr, receiver);
    ASSERT_NE(nullptr, context);

    // create FMQ objects
    auto [fmqRequestChannel, fmqRequestDescriptor] =
            RequestChannelSender::create(kExecutionBurstChannelLength, /*blocking=*/true);
    auto [fmqResultChannel, fmqResultDescriptor] =
            ResultChannelReceiver::create(kExecutionBurstChannelLength, /*blocking=*/true);
    ASSERT_NE(nullptr, fmqRequestChannel.get());
    ASSERT_NE(nullptr, fmqResultChannel.get());
    ASSERT_NE(nullptr, fmqRequestDescriptor);
    ASSERT_NE(nullptr, fmqResultDescriptor);

    // configure burst
    ErrorStatus errorStatus;
    sp<IBurstContext> burstContext;
    const Return<void> ret = preparedModel->configureExecutionBurst(
            callback, *fmqRequestDescriptor, *fmqResultDescriptor,
            [&errorStatus, &burstContext](ErrorStatus status, const sp<IBurstContext>& context) {
                errorStatus = status;
                burstContext = context;
            });
    ASSERT_TRUE(ret.isOk());
    ASSERT_EQ(ErrorStatus::NONE, errorStatus);
    ASSERT_NE(nullptr, burstContext.get());

    // return values
    *sender = std::move(fmqRequestChannel);
    *receiver = std::move(fmqResultChannel);
    *context = burstContext;
}

static void createBurstWithResultChannelLength(
        const sp<IPreparedModel>& preparedModel,
        std::shared_ptr<ExecutionBurstController>* controller, size_t resultChannelLength) {
    ASSERT_NE(nullptr, preparedModel.get());
    ASSERT_NE(nullptr, controller);

    // create FMQ objects
    auto [fmqRequestChannel, fmqRequestDescriptor] =
            RequestChannelSender::create(kExecutionBurstChannelLength, /*blocking=*/true);
    auto [fmqResultChannel, fmqResultDescriptor] =
            ResultChannelReceiver::create(resultChannelLength, /*blocking=*/true);
    ASSERT_NE(nullptr, fmqRequestChannel.get());
    ASSERT_NE(nullptr, fmqResultChannel.get());
    ASSERT_NE(nullptr, fmqRequestDescriptor);
    ASSERT_NE(nullptr, fmqResultDescriptor);

    // configure burst
    sp<ExecutionBurstCallback> callback = new ExecutionBurstCallback();
    ErrorStatus errorStatus;
    sp<IBurstContext> burstContext;
    const Return<void> ret = preparedModel->configureExecutionBurst(
            callback, *fmqRequestDescriptor, *fmqResultDescriptor,
            [&errorStatus, &burstContext](ErrorStatus status, const sp<IBurstContext>& context) {
                errorStatus = status;
                burstContext = context;
            });
    ASSERT_TRUE(ret.isOk());
    ASSERT_EQ(ErrorStatus::NONE, errorStatus);
    ASSERT_NE(nullptr, burstContext.get());

    // return values
    *controller = std::make_shared<ExecutionBurstController>(
            std::move(fmqRequestChannel), std::move(fmqResultChannel), burstContext, callback);
}

// Primary validation function. This function will take a valid serialized
// request, apply a mutation to it to invalidate the serialized request, then
// pass it to interface calls that use the serialized request. Note that the
// serialized request here is passed by value, and any mutation to the
// serialized request does not leave this function.
static void validate(RequestChannelSender* sender, ResultChannelReceiver* receiver,
                     const std::string& message, std::vector<FmqRequestDatum> serialized,
                     const std::function<void(std::vector<FmqRequestDatum>*)>& mutation) {
    mutation(&serialized);

    // skip if packet is too large to send
    if (serialized.size() > kExecutionBurstChannelLength) {
        return;
    }

    SCOPED_TRACE(message);

    // send invalid packet
    sender->sendPacket(serialized);

    // receive error
    auto results = receiver->getBlocking();
    ASSERT_TRUE(results.has_value());
    const auto [status, outputShapes, timing] = std::move(*results);
    EXPECT_NE(ErrorStatus::NONE, status);
    EXPECT_EQ(0u, outputShapes.size());
    EXPECT_TRUE(badTiming(timing));
}

static std::vector<FmqRequestDatum> createUniqueDatum() {
    const FmqRequestDatum::PacketInformation packetInformation = {
            /*.packetSize=*/10, /*.numberOfInputOperands=*/10, /*.numberOfOutputOperands=*/10,
            /*.numberOfPools=*/10};
    const FmqRequestDatum::OperandInformation operandInformation = {
            /*.hasNoValue=*/false, /*.location=*/{}, /*.numberOfDimensions=*/10};
    const int32_t invalidPoolIdentifier = std::numeric_limits<int32_t>::max();
    std::vector<FmqRequestDatum> unique(7);
    unique[0].packetInformation(packetInformation);
    unique[1].inputOperandInformation(operandInformation);
    unique[2].inputOperandDimensionValue(0);
    unique[3].outputOperandInformation(operandInformation);
    unique[4].outputOperandDimensionValue(0);
    unique[5].poolIdentifier(invalidPoolIdentifier);
    unique[6].measureTiming(MeasureTiming::YES);
    return unique;
}

static const std::vector<FmqRequestDatum>& getUniqueDatum() {
    static const std::vector<FmqRequestDatum> unique = createUniqueDatum();
    return unique;
}

///////////////////////// REMOVE DATUM ////////////////////////////////////

static void removeDatumTest(RequestChannelSender* sender, ResultChannelReceiver* receiver,
                            const std::vector<FmqRequestDatum>& serialized) {
    for (size_t index = 0; index < serialized.size(); ++index) {
        const std::string message = "removeDatum: removed datum at index " + std::to_string(index);
        validate(sender, receiver, message, serialized,
                 [index](std::vector<FmqRequestDatum>* serialized) {
                     serialized->erase(serialized->begin() + index);
                 });
    }
}

///////////////////////// ADD DATUM ////////////////////////////////////

static void addDatumTest(RequestChannelSender* sender, ResultChannelReceiver* receiver,
                         const std::vector<FmqRequestDatum>& serialized) {
    const std::vector<FmqRequestDatum>& extra = getUniqueDatum();
    for (size_t index = 0; index <= serialized.size(); ++index) {
        for (size_t type = 0; type < extra.size(); ++type) {
            const std::string message = "addDatum: added datum type " + std::to_string(type) +
                                        " at index " + std::to_string(index);
            validate(sender, receiver, message, serialized,
                     [index, type, &extra](std::vector<FmqRequestDatum>* serialized) {
                         serialized->insert(serialized->begin() + index, extra[type]);
                     });
        }
    }
}

///////////////////////// MUTATE DATUM ////////////////////////////////////

static bool interestingCase(const FmqRequestDatum& lhs, const FmqRequestDatum& rhs) {
    using Discriminator = FmqRequestDatum::hidl_discriminator;

    const bool differentValues = (lhs != rhs);
    const bool sameSumType = (lhs.getDiscriminator() == rhs.getDiscriminator());
    const auto discriminator = rhs.getDiscriminator();
    const bool isDimensionValue = (discriminator == Discriminator::inputOperandDimensionValue ||
                                   discriminator == Discriminator::outputOperandDimensionValue);

    return differentValues && !(sameSumType && isDimensionValue);
}

static void mutateDatumTest(RequestChannelSender* sender, ResultChannelReceiver* receiver,
                            const std::vector<FmqRequestDatum>& serialized) {
    const std::vector<FmqRequestDatum>& change = getUniqueDatum();
    for (size_t index = 0; index < serialized.size(); ++index) {
        for (size_t type = 0; type < change.size(); ++type) {
            if (interestingCase(serialized[index], change[type])) {
                const std::string message = "mutateDatum: changed datum at index " +
                                            std::to_string(index) + " to datum type " +
                                            std::to_string(type);
                validate(sender, receiver, message, serialized,
                         [index, type, &change](std::vector<FmqRequestDatum>* serialized) {
                             (*serialized)[index] = change[type];
                         });
            }
        }
    }
}

///////////////////////// BURST VALIATION TESTS ////////////////////////////////////

static void validateBurstSerialization(const sp<IPreparedModel>& preparedModel,
                                       const std::vector<Request>& requests) {
    // create burst
    std::unique_ptr<RequestChannelSender> sender;
    std::unique_ptr<ResultChannelReceiver> receiver;
    sp<ExecutionBurstCallback> callback = new ExecutionBurstCallback();
    sp<IBurstContext> context;
    ASSERT_NO_FATAL_FAILURE(createBurst(preparedModel, callback, &sender, &receiver, &context));
    ASSERT_NE(nullptr, sender.get());
    ASSERT_NE(nullptr, receiver.get());
    ASSERT_NE(nullptr, context.get());

    // validate each request
    for (const Request& request : requests) {
        // load memory into callback slots
        std::vector<intptr_t> keys(request.pools.size());
        for (size_t i = 0; i < keys.size(); ++i) {
            keys[i] = reinterpret_cast<intptr_t>(&request.pools[i]);
        }
        const std::vector<int32_t> slots = callback->getSlots(request.pools, keys);

        // ensure slot std::numeric_limits<int32_t>::max() doesn't exist (for
        // subsequent slot validation testing)
        const auto maxElement = std::max_element(slots.begin(), slots.end());
        ASSERT_NE(slots.end(), maxElement);
        ASSERT_NE(std::numeric_limits<int32_t>::max(), *maxElement);

        // serialize the request
        const auto serialized = ::android::nn::serialize(request, MeasureTiming::YES, slots);

        // validations
        removeDatumTest(sender.get(), receiver.get(), serialized);
        addDatumTest(sender.get(), receiver.get(), serialized);
        mutateDatumTest(sender.get(), receiver.get(), serialized);
    }
}

static void validateBurstFmqLength(const sp<IPreparedModel>& preparedModel,
                                   const std::vector<Request>& requests) {
    // create regular burst
    std::shared_ptr<ExecutionBurstController> controllerRegular;
    ASSERT_NO_FATAL_FAILURE(createBurstWithResultChannelLength(preparedModel, &controllerRegular,
                                                               kExecutionBurstChannelLength));
    ASSERT_NE(nullptr, controllerRegular.get());

    // create burst with small output channel
    std::shared_ptr<ExecutionBurstController> controllerSmall;
    ASSERT_NO_FATAL_FAILURE(createBurstWithResultChannelLength(preparedModel, &controllerSmall,
                                                               kExecutionBurstChannelSmallLength));
    ASSERT_NE(nullptr, controllerSmall.get());

    // validate each request
    for (const Request& request : requests) {
        // load memory into callback slots
        std::vector<intptr_t> keys(request.pools.size());
        for (size_t i = 0; i < keys.size(); ++i) {
            keys[i] = reinterpret_cast<intptr_t>(&request.pools[i]);
        }

        // collect serialized result by running regular burst
        const auto [status1, outputShapes1, timing1] =
                controllerRegular->compute(request, MeasureTiming::NO, keys);

        // skip test if synchronous output isn't useful
        const std::vector<FmqResultDatum> serialized =
                ::android::nn::serialize(status1, outputShapes1, timing1);
        if (status1 != ErrorStatus::NONE ||
            serialized.size() <= kExecutionBurstChannelSmallLength) {
            continue;
        }

        // by this point, execution should fail because the result channel isn't
        // large enough to return the serialized result
        const auto [status2, outputShapes2, timing2] =
                controllerSmall->compute(request, MeasureTiming::NO, keys);
        EXPECT_NE(ErrorStatus::NONE, status2);
        EXPECT_EQ(0u, outputShapes2.size());
        EXPECT_TRUE(badTiming(timing2));
    }
}

///////////////////////////// ENTRY POINT //////////////////////////////////

void ValidationTest::validateBurst(const sp<IPreparedModel>& preparedModel,
                                   const std::vector<Request>& requests) {
    ASSERT_NO_FATAL_FAILURE(validateBurstSerialization(preparedModel, requests));
    ASSERT_NO_FATAL_FAILURE(validateBurstFmqLength(preparedModel, requests));
}

}  // namespace functional
}  // namespace vts
}  // namespace V1_2
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
