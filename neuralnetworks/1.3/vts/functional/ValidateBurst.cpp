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

#define LOG_TAG "neuralnetworks_hidl_hal_test"

#include "VtsHalNeuralnetworks.h"

#include "1.2/Callbacks.h"
#include "ExecutionBurstController.h"
#include "ExecutionBurstServer.h"
#include "GeneratedTestHarness.h"
#include "TestHarness.h"

#include <android-base/logging.h>
#include <chrono>
#include <cstring>

namespace android::hardware::neuralnetworks::V1_3::vts::functional {

using nn::ExecutionBurstController;
using nn::RequestChannelSender;
using nn::ResultChannelReceiver;
using V1_0::Request;
using V1_2::FmqRequestDatum;
using V1_2::FmqResultDatum;
using V1_2::IBurstCallback;
using V1_2::IBurstContext;
using V1_2::MeasureTiming;
using V1_2::Timing;
using ExecutionBurstCallback = ExecutionBurstController::ExecutionBurstCallback;

using BurstExecutionMutation = std::function<void(std::vector<FmqRequestDatum>*)>;

// This constant value represents the length of an FMQ that is large enough to
// return a result from a burst execution for all of the generated test cases.
constexpr size_t kExecutionBurstChannelLength = 1024;

// This constant value represents a length of an FMQ that is not large enough
// to return a result from a burst execution for some of the generated test
// cases.
constexpr size_t kExecutionBurstChannelSmallLength = 8;

///////////////////////// UTILITY FUNCTIONS /////////////////////////

static bool badTiming(Timing timing) {
    return timing.timeOnDevice == UINT64_MAX && timing.timeInDriver == UINT64_MAX;
}

static void createBurst(const sp<IPreparedModel>& preparedModel, const sp<IBurstCallback>& callback,
                        std::unique_ptr<RequestChannelSender>* sender,
                        std::unique_ptr<ResultChannelReceiver>* receiver,
                        sp<IBurstContext>* context,
                        size_t resultChannelLength = kExecutionBurstChannelLength) {
    ASSERT_NE(nullptr, preparedModel.get());
    ASSERT_NE(nullptr, sender);
    ASSERT_NE(nullptr, receiver);
    ASSERT_NE(nullptr, context);

    // create FMQ objects
    auto [fmqRequestChannel, fmqRequestDescriptor] =
            RequestChannelSender::create(kExecutionBurstChannelLength);
    auto [fmqResultChannel, fmqResultDescriptor] =
            ResultChannelReceiver::create(resultChannelLength, std::chrono::microseconds{0});
    ASSERT_NE(nullptr, fmqRequestChannel.get());
    ASSERT_NE(nullptr, fmqResultChannel.get());
    ASSERT_NE(nullptr, fmqRequestDescriptor);
    ASSERT_NE(nullptr, fmqResultDescriptor);

    // configure burst
    V1_0::ErrorStatus errorStatus;
    sp<IBurstContext> burstContext;
    const Return<void> ret = preparedModel->configureExecutionBurst(
            callback, *fmqRequestDescriptor, *fmqResultDescriptor,
            [&errorStatus, &burstContext](V1_0::ErrorStatus status,
                                          const sp<IBurstContext>& context) {
                errorStatus = status;
                burstContext = context;
            });
    ASSERT_TRUE(ret.isOk());
    ASSERT_EQ(V1_0::ErrorStatus::NONE, errorStatus);
    ASSERT_NE(nullptr, burstContext.get());

    // return values
    *sender = std::move(fmqRequestChannel);
    *receiver = std::move(fmqResultChannel);
    *context = burstContext;
}

static void createBurstWithResultChannelLength(
        const sp<IPreparedModel>& preparedModel, size_t resultChannelLength,
        std::shared_ptr<ExecutionBurstController>* controller) {
    ASSERT_NE(nullptr, preparedModel.get());
    ASSERT_NE(nullptr, controller);

    // create FMQ objects
    std::unique_ptr<RequestChannelSender> sender;
    std::unique_ptr<ResultChannelReceiver> receiver;
    sp<ExecutionBurstCallback> callback = new ExecutionBurstCallback();
    sp<IBurstContext> context;
    ASSERT_NO_FATAL_FAILURE(createBurst(preparedModel, callback, &sender, &receiver, &context,
                                        resultChannelLength));
    ASSERT_NE(nullptr, sender.get());
    ASSERT_NE(nullptr, receiver.get());
    ASSERT_NE(nullptr, context.get());

    // return values
    *controller = std::make_shared<ExecutionBurstController>(std::move(sender), std::move(receiver),
                                                             context, callback);
}

// Primary validation function. This function will take a valid serialized
// request, apply a mutation to it to invalidate the serialized request, then
// pass it to interface calls that use the serialized request.
static void validate(RequestChannelSender* sender, ResultChannelReceiver* receiver,
                     const std::string& message,
                     const std::vector<FmqRequestDatum>& originalSerialized,
                     const BurstExecutionMutation& mutate) {
    std::vector<FmqRequestDatum> serialized = originalSerialized;
    mutate(&serialized);

    // skip if packet is too large to send
    if (serialized.size() > kExecutionBurstChannelLength) {
        return;
    }

    SCOPED_TRACE(message);

    // send invalid packet
    ASSERT_TRUE(sender->sendPacket(serialized));

    // receive error
    auto results = receiver->getBlocking();
    ASSERT_TRUE(results.has_value());
    const auto [status, outputShapes, timing] = std::move(*results);
    EXPECT_NE(V1_0::ErrorStatus::NONE, status);
    EXPECT_EQ(0u, outputShapes.size());
    EXPECT_TRUE(badTiming(timing));
}

// For validation, valid packet entries are mutated to invalid packet entries,
// or invalid packet entries are inserted into valid packets. This function
// creates pre-set invalid packet entries for convenience.
static std::vector<FmqRequestDatum> createBadRequestPacketEntries() {
    const FmqRequestDatum::PacketInformation packetInformation = {
            /*.packetSize=*/10, /*.numberOfInputOperands=*/10, /*.numberOfOutputOperands=*/10,
            /*.numberOfPools=*/10};
    const FmqRequestDatum::OperandInformation operandInformation = {
            /*.hasNoValue=*/false, /*.location=*/{}, /*.numberOfDimensions=*/10};
    const int32_t invalidPoolIdentifier = std::numeric_limits<int32_t>::max();
    std::vector<FmqRequestDatum> bad(7);
    bad[0].packetInformation(packetInformation);
    bad[1].inputOperandInformation(operandInformation);
    bad[2].inputOperandDimensionValue(0);
    bad[3].outputOperandInformation(operandInformation);
    bad[4].outputOperandDimensionValue(0);
    bad[5].poolIdentifier(invalidPoolIdentifier);
    bad[6].measureTiming(MeasureTiming::YES);
    return bad;
}

// For validation, valid packet entries are mutated to invalid packet entries,
// or invalid packet entries are inserted into valid packets. This function
// retrieves pre-set invalid packet entries for convenience. This function
// caches these data so they can be reused on subsequent validation checks.
static const std::vector<FmqRequestDatum>& getBadRequestPacketEntries() {
    static const std::vector<FmqRequestDatum> bad = createBadRequestPacketEntries();
    return bad;
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
    const std::vector<FmqRequestDatum>& extra = getBadRequestPacketEntries();
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
    const bool sameDiscriminator = (lhs.getDiscriminator() == rhs.getDiscriminator());
    const auto discriminator = rhs.getDiscriminator();
    const bool isDimensionValue = (discriminator == Discriminator::inputOperandDimensionValue ||
                                   discriminator == Discriminator::outputOperandDimensionValue);

    return differentValues && !(sameDiscriminator && isDimensionValue);
}

static void mutateDatumTest(RequestChannelSender* sender, ResultChannelReceiver* receiver,
                            const std::vector<FmqRequestDatum>& serialized) {
    const std::vector<FmqRequestDatum>& change = getBadRequestPacketEntries();
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
                                       const Request& request) {
    // create burst
    std::unique_ptr<RequestChannelSender> sender;
    std::unique_ptr<ResultChannelReceiver> receiver;
    sp<ExecutionBurstCallback> callback = new ExecutionBurstCallback();
    sp<IBurstContext> context;
    ASSERT_NO_FATAL_FAILURE(createBurst(preparedModel, callback, &sender, &receiver, &context));
    ASSERT_NE(nullptr, sender.get());
    ASSERT_NE(nullptr, receiver.get());
    ASSERT_NE(nullptr, context.get());

    // load memory into callback slots
    std::vector<intptr_t> keys;
    keys.reserve(request.pools.size());
    std::transform(request.pools.begin(), request.pools.end(), std::back_inserter(keys),
                   [](const auto& pool) { return reinterpret_cast<intptr_t>(&pool); });
    const std::vector<int32_t> slots = callback->getSlots(request.pools, keys);

    // ensure slot std::numeric_limits<int32_t>::max() doesn't exist (for
    // subsequent slot validation testing)
    ASSERT_TRUE(std::all_of(slots.begin(), slots.end(), [](int32_t slot) {
        return slot != std::numeric_limits<int32_t>::max();
    }));

    // serialize the request
    const auto serialized = android::nn::serialize(request, MeasureTiming::YES, slots);

    // validations
    removeDatumTest(sender.get(), receiver.get(), serialized);
    addDatumTest(sender.get(), receiver.get(), serialized);
    mutateDatumTest(sender.get(), receiver.get(), serialized);
}

// This test validates that when the Result message size exceeds length of the
// result FMQ, the service instance gracefully fails and returns an error.
static void validateBurstFmqLength(const sp<IPreparedModel>& preparedModel,
                                   const Request& request) {
    // create regular burst
    std::shared_ptr<ExecutionBurstController> controllerRegular;
    ASSERT_NO_FATAL_FAILURE(createBurstWithResultChannelLength(
            preparedModel, kExecutionBurstChannelLength, &controllerRegular));
    ASSERT_NE(nullptr, controllerRegular.get());

    // create burst with small output channel
    std::shared_ptr<ExecutionBurstController> controllerSmall;
    ASSERT_NO_FATAL_FAILURE(createBurstWithResultChannelLength(
            preparedModel, kExecutionBurstChannelSmallLength, &controllerSmall));
    ASSERT_NE(nullptr, controllerSmall.get());

    // load memory into callback slots
    std::vector<intptr_t> keys(request.pools.size());
    for (size_t i = 0; i < keys.size(); ++i) {
        keys[i] = reinterpret_cast<intptr_t>(&request.pools[i]);
    }

    // collect serialized result by running regular burst
    const auto [nRegular, outputShapesRegular, timingRegular, fallbackRegular] =
            controllerRegular->compute(request, MeasureTiming::NO, keys);
    const V1_0::ErrorStatus statusRegular = nn::legacyConvertResultCodeToErrorStatus(nRegular);
    EXPECT_FALSE(fallbackRegular);

    // skip test if regular burst output isn't useful for testing a failure
    // caused by having too small of a length for the result FMQ
    const std::vector<FmqResultDatum> serialized =
            android::nn::serialize(statusRegular, outputShapesRegular, timingRegular);
    if (statusRegular != V1_0::ErrorStatus::NONE ||
        serialized.size() <= kExecutionBurstChannelSmallLength) {
        return;
    }

    // by this point, execution should fail because the result channel isn't
    // large enough to return the serialized result
    const auto [nSmall, outputShapesSmall, timingSmall, fallbackSmall] =
            controllerSmall->compute(request, MeasureTiming::NO, keys);
    const V1_0::ErrorStatus statusSmall = nn::legacyConvertResultCodeToErrorStatus(nSmall);
    EXPECT_NE(V1_0::ErrorStatus::NONE, statusSmall);
    EXPECT_EQ(0u, outputShapesSmall.size());
    EXPECT_TRUE(badTiming(timingSmall));
    EXPECT_FALSE(fallbackSmall);
}

static bool isSanitized(const FmqResultDatum& datum) {
    using Discriminator = FmqResultDatum::hidl_discriminator;

    // check to ensure the padding values in the returned
    // FmqResultDatum::OperandInformation are initialized to 0
    if (datum.getDiscriminator() == Discriminator::operandInformation) {
        static_assert(
                offsetof(FmqResultDatum::OperandInformation, isSufficient) == 0,
                "unexpected value for offset of FmqResultDatum::OperandInformation::isSufficient");
        static_assert(
                sizeof(FmqResultDatum::OperandInformation::isSufficient) == 1,
                "unexpected value for size of FmqResultDatum::OperandInformation::isSufficient");
        static_assert(offsetof(FmqResultDatum::OperandInformation, numberOfDimensions) == 4,
                      "unexpected value for offset of "
                      "FmqResultDatum::OperandInformation::numberOfDimensions");
        static_assert(sizeof(FmqResultDatum::OperandInformation::numberOfDimensions) == 4,
                      "unexpected value for size of "
                      "FmqResultDatum::OperandInformation::numberOfDimensions");
        static_assert(sizeof(FmqResultDatum::OperandInformation) == 8,
                      "unexpected value for size of "
                      "FmqResultDatum::OperandInformation");

        constexpr size_t paddingOffset =
                offsetof(FmqResultDatum::OperandInformation, isSufficient) +
                sizeof(FmqResultDatum::OperandInformation::isSufficient);
        constexpr size_t paddingSize =
                offsetof(FmqResultDatum::OperandInformation, numberOfDimensions) - paddingOffset;

        FmqResultDatum::OperandInformation initialized{};
        std::memset(&initialized, 0, sizeof(initialized));

        const char* initializedPaddingStart =
                reinterpret_cast<const char*>(&initialized) + paddingOffset;
        const char* datumPaddingStart =
                reinterpret_cast<const char*>(&datum.operandInformation()) + paddingOffset;

        return std::memcmp(datumPaddingStart, initializedPaddingStart, paddingSize) == 0;
    }

    // there are no other padding initialization checks required, so return true
    // for any sum-type that isn't FmqResultDatum::OperandInformation
    return true;
}

static void validateBurstSanitized(const sp<IPreparedModel>& preparedModel,
                                   const Request& request) {
    // create burst
    std::unique_ptr<RequestChannelSender> sender;
    std::unique_ptr<ResultChannelReceiver> receiver;
    sp<ExecutionBurstCallback> callback = new ExecutionBurstCallback();
    sp<IBurstContext> context;
    ASSERT_NO_FATAL_FAILURE(createBurst(preparedModel, callback, &sender, &receiver, &context));
    ASSERT_NE(nullptr, sender.get());
    ASSERT_NE(nullptr, receiver.get());
    ASSERT_NE(nullptr, context.get());

    // load memory into callback slots
    std::vector<intptr_t> keys;
    keys.reserve(request.pools.size());
    std::transform(request.pools.begin(), request.pools.end(), std::back_inserter(keys),
                   [](const auto& pool) { return reinterpret_cast<intptr_t>(&pool); });
    const std::vector<int32_t> slots = callback->getSlots(request.pools, keys);

    // send valid request
    ASSERT_TRUE(sender->send(request, MeasureTiming::YES, slots));

    // receive valid result
    auto serialized = receiver->getPacketBlocking();
    ASSERT_TRUE(serialized.has_value());

    // sanitize result
    ASSERT_TRUE(std::all_of(serialized->begin(), serialized->end(), isSanitized))
            << "The result serialized data is not properly sanitized";
}

///////////////////////////// ENTRY POINT //////////////////////////////////

void validateBurst(const sp<IPreparedModel>& preparedModel, const Request& request) {
    ASSERT_NO_FATAL_FAILURE(validateBurstSerialization(preparedModel, request));
    ASSERT_NO_FATAL_FAILURE(validateBurstFmqLength(preparedModel, request));
    ASSERT_NO_FATAL_FAILURE(validateBurstSanitized(preparedModel, request));
}

}  // namespace android::hardware::neuralnetworks::V1_3::vts::functional
