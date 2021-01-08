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

#include "MockBuffer.h"

#include <android/hardware/neuralnetworks/1.3/IBuffer.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <nnapi/IBuffer.h>
#include <nnapi/SharedMemory.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/hal/1.3/Buffer.h>

#include <functional>
#include <memory>

namespace android::hardware::neuralnetworks::V1_3::utils {
namespace {

using ::testing::_;
using ::testing::Invoke;
using ::testing::InvokeWithoutArgs;
using ::testing::Return;

const auto kMemory = nn::createSharedMemory(4).value();
const sp<V1_3::IBuffer> kInvalidBuffer;
constexpr auto kInvalidToken = nn::Request::MemoryDomainToken{0};
constexpr auto kToken = nn::Request::MemoryDomainToken{1};

std::function<hardware::Return<V1_3::ErrorStatus>()> makeFunctionReturn(V1_3::ErrorStatus status) {
    return [status]() -> hardware::Return<V1_3::ErrorStatus> { return status; };
}

std::function<hardware::Status()> makeTransportFailure(status_t status) {
    return [status] { return hardware::Status::fromStatusT(status); };
}

const auto makeSuccessful = makeFunctionReturn(V1_3::ErrorStatus::NONE);
const auto makeGeneralError = makeFunctionReturn(V1_3::ErrorStatus::GENERAL_FAILURE);
const auto makeGeneralTransportFailure = makeTransportFailure(NO_MEMORY);
const auto makeDeadObjectFailure = makeTransportFailure(DEAD_OBJECT);

}  // namespace

TEST(BufferTest, invalidBuffer) {
    // run test
    const auto result = Buffer::create(kInvalidBuffer, kToken);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(BufferTest, invalidToken) {
    // setup call
    const auto mockBuffer = MockBuffer::create();

    // run test
    const auto result = Buffer::create(mockBuffer, kInvalidToken);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(BufferTest, create) {
    // setup call
    const auto mockBuffer = MockBuffer::create();
    const auto buffer = Buffer::create(mockBuffer, kToken).value();

    // run test
    const auto token = buffer->getToken();

    // verify result
    EXPECT_EQ(token, kToken);
}

TEST(BufferTest, copyTo) {
    // setup call
    const auto mockBuffer = MockBuffer::create();
    const auto buffer = Buffer::create(mockBuffer, kToken).value();
    EXPECT_CALL(*mockBuffer, copyTo(_)).Times(1).WillOnce(InvokeWithoutArgs(makeSuccessful));

    // run test
    const auto result = buffer->copyTo(kMemory);

    // verify result
    EXPECT_TRUE(result.has_value()) << result.error().message;
}

TEST(BufferTest, copyToError) {
    // setup test
    const auto mockBuffer = MockBuffer::create();
    const auto buffer = Buffer::create(mockBuffer, kToken).value();
    EXPECT_CALL(*mockBuffer, copyTo(_)).Times(1).WillOnce(InvokeWithoutArgs(makeGeneralError));

    // run test
    const auto result = buffer->copyTo(kMemory);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(BufferTest, copyToTransportFailure) {
    // setup test
    const auto mockBuffer = MockBuffer::create();
    const auto buffer = Buffer::create(mockBuffer, kToken).value();
    EXPECT_CALL(*mockBuffer, copyTo(_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));

    // run test
    const auto result = buffer->copyTo(kMemory);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(BufferTest, copyToDeadObject) {
    // setup test
    const auto mockBuffer = MockBuffer::create();
    const auto buffer = Buffer::create(mockBuffer, kToken).value();
    EXPECT_CALL(*mockBuffer, copyTo(_)).Times(1).WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // run test
    const auto result = buffer->copyTo(kMemory);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(BufferTest, copyFrom) {
    // setup call
    const auto mockBuffer = MockBuffer::create();
    const auto buffer = Buffer::create(mockBuffer, kToken).value();
    EXPECT_CALL(*mockBuffer, copyFrom(_, _)).Times(1).WillOnce(InvokeWithoutArgs(makeSuccessful));

    // run test
    const auto result = buffer->copyFrom(kMemory, {});

    // verify result
    EXPECT_TRUE(result.has_value());
}

TEST(BufferTest, copyFromError) {
    // setup test
    const auto mockBuffer = MockBuffer::create();
    const auto buffer = Buffer::create(mockBuffer, kToken).value();
    EXPECT_CALL(*mockBuffer, copyFrom(_, _)).Times(1).WillOnce(InvokeWithoutArgs(makeGeneralError));

    // run test
    const auto result = buffer->copyFrom(kMemory, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(BufferTest, copyFromTransportFailure) {
    // setup test
    const auto mockBuffer = MockBuffer::create();
    const auto buffer = Buffer::create(mockBuffer, kToken).value();
    EXPECT_CALL(*mockBuffer, copyFrom(_, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));

    // run test
    const auto result = buffer->copyFrom(kMemory, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(BufferTest, copyFromDeadObject) {
    // setup test
    const auto mockBuffer = MockBuffer::create();
    const auto buffer = Buffer::create(mockBuffer, kToken).value();
    EXPECT_CALL(*mockBuffer, copyFrom(_, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // run test
    const auto result = buffer->copyFrom(kMemory, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

}  // namespace android::hardware::neuralnetworks::V1_3::utils
