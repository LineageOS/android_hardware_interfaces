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

#include <gmock/gmock.h>
#include <nnapi/SharedMemory.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/hal/ResilientBuffer.h>
#include <memory>
#include <tuple>
#include <utility>
#include "MockBuffer.h"

namespace android::hardware::neuralnetworks::utils {
namespace {

using ::testing::_;
using ::testing::InvokeWithoutArgs;
using ::testing::Return;

constexpr auto kToken = nn::Request::MemoryDomainToken{1};

using SharedMockBuffer = std::shared_ptr<const nn::MockBuffer>;
using MockBufferFactory = ::testing::MockFunction<nn::GeneralResult<nn::SharedBuffer>()>;

SharedMockBuffer createConfiguredMockBuffer() {
    return std::make_shared<const nn::MockBuffer>();
}

std::tuple<std::shared_ptr<const nn::MockBuffer>, std::unique_ptr<MockBufferFactory>,
           std::shared_ptr<const ResilientBuffer>>
setup() {
    auto mockBuffer = std::make_shared<const nn::MockBuffer>();

    auto mockBufferFactory = std::make_unique<MockBufferFactory>();
    EXPECT_CALL(*mockBufferFactory, Call()).Times(1).WillOnce(Return(mockBuffer));

    auto buffer = ResilientBuffer::create(mockBufferFactory->AsStdFunction()).value();
    return std::make_tuple(std::move(mockBuffer), std::move(mockBufferFactory), std::move(buffer));
}

constexpr auto makeError = [](nn::ErrorStatus status) {
    return [status](const auto&... /*args*/) { return nn::error(status); };
};
const auto kReturnGeneralFailure = makeError(nn::ErrorStatus::GENERAL_FAILURE);
const auto kReturnDeadObject = makeError(nn::ErrorStatus::DEAD_OBJECT);

const auto kNoError = nn::GeneralResult<void>{};

}  // namespace

TEST(ResilientBufferTest, invalidBufferFactory) {
    // setup call
    const auto invalidBufferFactory = ResilientBuffer::Factory{};

    // run test
    const auto result = ResilientBuffer::create(invalidBufferFactory);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::INVALID_ARGUMENT);
}

TEST(ResilientBufferTest, bufferFactoryFailure) {
    // setup call
    const auto invalidBufferFactory = kReturnGeneralFailure;

    // run test
    const auto result = ResilientBuffer::create(invalidBufferFactory);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(ResilientBufferTest, getBuffer) {
    // setup call
    const auto [mockBuffer, mockBufferFactory, buffer] = setup();

    // run test
    const auto result = buffer->getBuffer();

    // verify result
    EXPECT_TRUE(result == mockBuffer);
}

TEST(ResilientBufferTest, getToken) {
    // setup call
    const auto [mockBuffer, mockBufferFactory, buffer] = setup();
    EXPECT_CALL(*mockBuffer, getToken()).Times(1).WillOnce(Return(kToken));

    // run test
    const auto token = buffer->getToken();

    // verify result
    EXPECT_EQ(token, kToken);
}

TEST(ResilientBufferTest, copyTo) {
    // setup call
    const auto [mockBuffer, mockBufferFactory, buffer] = setup();
    EXPECT_CALL(*mockBuffer, copyTo(_)).Times(1).WillOnce(Return(kNoError));

    // run test
    const nn::SharedMemory memory = std::make_shared<const nn::Memory>();
    const auto result = buffer->copyTo(memory);

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
}

TEST(ResilientBufferTest, copyToError) {
    // setup call
    const auto [mockBuffer, mockBufferFactory, buffer] = setup();
    EXPECT_CALL(*mockBuffer, copyTo(_)).Times(1).WillOnce(kReturnGeneralFailure);

    // run test
    const nn::SharedMemory memory = std::make_shared<const nn::Memory>();
    const auto result = buffer->copyTo(memory);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(ResilientBufferTest, copyToDeadObjectFailedRecovery) {
    // setup call
    const auto [mockBuffer, mockBufferFactory, buffer] = setup();
    EXPECT_CALL(*mockBuffer, copyTo(_)).Times(1).WillOnce(kReturnDeadObject);
    EXPECT_CALL(*mockBufferFactory, Call()).Times(1).WillOnce(kReturnGeneralFailure);

    // run test
    const nn::SharedMemory memory = std::make_shared<const nn::Memory>();
    const auto result = buffer->copyTo(memory);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(ResilientBufferTest, copyToDeadObjectSuccessfulRecovery) {
    // setup call
    const auto [mockBuffer, mockBufferFactory, buffer] = setup();
    EXPECT_CALL(*mockBuffer, copyTo(_)).Times(1).WillOnce(kReturnDeadObject);
    const auto recoveredMockBuffer = createConfiguredMockBuffer();
    EXPECT_CALL(*recoveredMockBuffer, copyTo(_)).Times(1).WillOnce(Return(kNoError));
    EXPECT_CALL(*mockBufferFactory, Call()).Times(1).WillOnce(Return(recoveredMockBuffer));

    // run test
    const nn::SharedMemory memory = std::make_shared<const nn::Memory>();
    const auto result = buffer->copyTo(memory);

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
}

TEST(ResilientBufferTest, copyFrom) {
    // setup call
    const auto [mockBuffer, mockBufferFactory, buffer] = setup();
    EXPECT_CALL(*mockBuffer, copyFrom(_, _)).Times(1).WillOnce(Return(kNoError));

    // run test
    const nn::SharedMemory memory = std::make_shared<const nn::Memory>();
    const auto result = buffer->copyFrom(memory, {});

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
}

TEST(ResilientBufferTest, copyFromError) {
    // setup call
    const auto [mockBuffer, mockBufferFactory, buffer] = setup();
    EXPECT_CALL(*mockBuffer, copyFrom(_, _)).Times(1).WillOnce(kReturnGeneralFailure);

    // run test
    const nn::SharedMemory memory = std::make_shared<const nn::Memory>();
    const auto result = buffer->copyFrom(memory, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(ResilientBufferTest, copyFromDeadObjectFailedRecovery) {
    // setup call
    const auto [mockBuffer, mockBufferFactory, buffer] = setup();
    EXPECT_CALL(*mockBuffer, copyFrom(_, _)).Times(1).WillOnce(kReturnDeadObject);
    EXPECT_CALL(*mockBufferFactory, Call()).Times(1).WillOnce(kReturnGeneralFailure);

    // run test
    const nn::SharedMemory memory = std::make_shared<const nn::Memory>();
    const auto result = buffer->copyFrom(memory, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(ResilientBufferTest, copyFromDeadObjectSuccessfulRecovery) {
    // setup call
    const auto [mockBuffer, mockBufferFactory, buffer] = setup();
    EXPECT_CALL(*mockBuffer, copyFrom(_, _)).Times(1).WillOnce(kReturnDeadObject);
    const auto recoveredMockBuffer = createConfiguredMockBuffer();
    EXPECT_CALL(*recoveredMockBuffer, copyFrom(_, _)).Times(1).WillOnce(Return(kNoError));
    EXPECT_CALL(*mockBufferFactory, Call()).Times(1).WillOnce(Return(recoveredMockBuffer));

    // run test
    const nn::SharedMemory memory = std::make_shared<const nn::Memory>();
    const auto result = buffer->copyFrom(memory, {});

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
}

TEST(ResilientBufferTest, recover) {
    // setup call
    const auto [mockBuffer, mockBufferFactory, buffer] = setup();
    const auto recoveredMockBuffer = createConfiguredMockBuffer();
    EXPECT_CALL(*mockBufferFactory, Call()).Times(1).WillOnce(Return(recoveredMockBuffer));

    // run test
    const auto result = buffer->recover(mockBuffer.get());

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
    EXPECT_TRUE(result.value() == recoveredMockBuffer);
}

TEST(ResilientBufferTest, recoverFailure) {
    // setup call
    const auto [mockBuffer, mockBufferFactory, buffer] = setup();
    const auto recoveredMockBuffer = createConfiguredMockBuffer();
    EXPECT_CALL(*mockBufferFactory, Call()).Times(1).WillOnce(kReturnGeneralFailure);

    // run test
    const auto result = buffer->recover(mockBuffer.get());

    // verify result
    EXPECT_FALSE(result.has_value());
}

TEST(ResilientBufferTest, someoneElseRecovered) {
    // setup call
    const auto [mockBuffer, mockBufferFactory, buffer] = setup();
    const auto recoveredMockBuffer = createConfiguredMockBuffer();
    EXPECT_CALL(*mockBufferFactory, Call()).Times(1).WillOnce(Return(recoveredMockBuffer));
    buffer->recover(mockBuffer.get());

    // run test
    const auto result = buffer->recover(mockBuffer.get());

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
    EXPECT_TRUE(result.value() == recoveredMockBuffer);
}

}  // namespace android::hardware::neuralnetworks::utils
