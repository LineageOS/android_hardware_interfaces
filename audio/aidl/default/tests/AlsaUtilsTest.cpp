/*
 * Copyright (C) 2024 The Android Open Source Project
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

#define LOG_TAG "AlsaUtilsTest"

#include <alsa/Utils.h>
#include <android-base/macros.h>
#include <audio_utils/primitives.h>
#include <gtest/gtest.h>
#include <log/log.h>

extern "C" {
#include <tinyalsa/pcm.h>
}

namespace alsa = ::aidl::android::hardware::audio::core::alsa;

namespace {

const static constexpr float kInt16tTolerance = 4;
const static constexpr float kIntTolerance = 1;
const static constexpr float kFloatTolerance = 1e-4;
const static constexpr float kUnityGain = 1;
const static constexpr int32_t kInt24Min = -(1 << 23);
const static constexpr int32_t kInt24Max = (1 << 23) - 1;
const static constexpr float kFloatMin = -1;
const static constexpr float kFloatMax = 1;
const static int32_t kQ8_23Min = 0x80000000;
const static int32_t kQ8_23Max = 0x7FFFFFFF;
const static std::vector<int16_t> kInt16Buffer = {10000,     100,   0,    INT16_MAX,
                                                  INT16_MIN, -2500, 1000, -5800};
const static std::vector<float> kFloatBuffer = {0.5, -0.6, kFloatMin, 0.01, kFloatMax, 0.0};
const static std::vector<int32_t> kInt32Buffer = {100, 0, 8000, INT32_MAX, INT32_MIN, -300};
const static std::vector<int32_t> kQ8_23Buffer = {
        kQ8_23Min, kQ8_23Max, 0x00000000, 0x00000001, 0x00400000, static_cast<int32_t>(0xFFD33333)};
const static std::vector<int32_t> kInt24Buffer = {200, 10, -100, 0, kInt24Min, kInt24Max};

template <typename T>
void* CopyToBuffer(int& bytesToTransfer, std::vector<T>& destBuffer,
                   const std::vector<T>& srcBuffer) {
    bytesToTransfer = srcBuffer.size() * sizeof(T);
    destBuffer = srcBuffer;
    return destBuffer.data();
}

template <typename T>
void VerifyTypedBufferResults(const std::vector<T>& bufferWithGain, const std::vector<T>& srcBuffer,
                              float gain, float tolerance) {
    for (size_t i = 0; i < srcBuffer.size(); i++) {
        EXPECT_NEAR(srcBuffer[i] * gain, static_cast<float>(bufferWithGain[i]), tolerance);
    }
}

template <typename T>
void VerifyTypedBufferResultsWithClamp(const std::vector<T>& bufferWithGain,
                                       const std::vector<T>& srcBuffer, float gain, float tolerance,
                                       T minValue, T maxValue) {
    for (size_t i = 0; i < srcBuffer.size(); i++) {
        float expectedResult = std::clamp(srcBuffer[i] * gain, static_cast<float>(minValue),
                                          static_cast<float>(maxValue));
        EXPECT_NEAR(expectedResult, static_cast<float>(bufferWithGain[i]), tolerance);
    }
}

}  // namespace

using ApplyGainTestParameters = std::tuple<pcm_format, int, float>;
enum { INDEX_PCM_FORMAT, INDEX_CHANNEL_COUNT, INDEX_GAIN };

class ApplyGainTest : public ::testing::TestWithParam<ApplyGainTestParameters> {
  protected:
    void SetUp() override;
    void VerifyBufferResult(const pcm_format pcmFormat, const float gain);
    void VerifyBufferResultWithClamp(const pcm_format pcmFormat, const float gain);

    pcm_format mPcmFormat;
    int mBufferSizeBytes;
    void* mBuffer;

  private:
    std::vector<int16_t> mInt16BufferToConvert;
    std::vector<float> mFloatBufferToConvert;
    std::vector<int32_t> mInt32BufferToConvert;
    std::vector<int32_t> mQ8_23BufferToConvert;
    std::vector<int32_t> mInt24BufferToConvert;
};

void ApplyGainTest::SetUp() {
    mPcmFormat = std::get<INDEX_PCM_FORMAT>(GetParam());
    switch (mPcmFormat) {
        case PCM_FORMAT_S16_LE:
            mBuffer = CopyToBuffer(mBufferSizeBytes, mInt16BufferToConvert, kInt16Buffer);
            break;
        case PCM_FORMAT_FLOAT_LE:
            mBuffer = CopyToBuffer(mBufferSizeBytes, mFloatBufferToConvert, kFloatBuffer);
            break;
        case PCM_FORMAT_S32_LE:
            mBuffer = CopyToBuffer(mBufferSizeBytes, mInt32BufferToConvert, kInt32Buffer);
            break;
        case PCM_FORMAT_S24_LE:
            mBuffer = CopyToBuffer(mBufferSizeBytes, mQ8_23BufferToConvert, kQ8_23Buffer);
            break;
        case PCM_FORMAT_S24_3LE: {
            std::vector<int32_t> original32BitBuffer(kInt24Buffer.begin(), kInt24Buffer.end());
            for (auto& val : original32BitBuffer) {
                val <<= 8;
            }
            mInt24BufferToConvert = std::vector<int32_t>(kInt24Buffer.size());
            mBufferSizeBytes = kInt24Buffer.size() * 3 * sizeof(uint8_t);
            memcpy_to_p24_from_i32(reinterpret_cast<uint8_t*>(mInt24BufferToConvert.data()),
                                   original32BitBuffer.data(), kInt24Buffer.size());
            mBuffer = mInt24BufferToConvert.data();
        } break;
        default:
            FAIL() << "Unsupported pcm format: " << mPcmFormat;
            return;
    }
}

void ApplyGainTest::VerifyBufferResult(const pcm_format pcmFormat, const float gain) {
    switch (pcmFormat) {
        case PCM_FORMAT_S16_LE:
            VerifyTypedBufferResults(mInt16BufferToConvert, kInt16Buffer, gain, kInt16tTolerance);
            break;
        case PCM_FORMAT_FLOAT_LE:
            VerifyTypedBufferResults(mFloatBufferToConvert, kFloatBuffer, gain, kFloatTolerance);
            break;
        case PCM_FORMAT_S32_LE:
            VerifyTypedBufferResults(mInt32BufferToConvert, kInt32Buffer, gain, kIntTolerance);
            break;
        case PCM_FORMAT_S24_LE: {
            for (size_t i = 0; i < kQ8_23Buffer.size(); i++) {
                EXPECT_NEAR(float_from_q8_23(kQ8_23Buffer[i]) * gain,
                            static_cast<float>(float_from_q8_23(mQ8_23BufferToConvert[i])),
                            kFloatTolerance);
            }
        } break;
        case PCM_FORMAT_S24_3LE: {
            size_t bufferSize = kInt24Buffer.size();
            std::vector<int32_t> result32BitBuffer(bufferSize);
            memcpy_to_i32_from_p24(result32BitBuffer.data(),
                                   reinterpret_cast<uint8_t*>(mInt24BufferToConvert.data()),
                                   bufferSize);
            for (size_t i = 0; i < bufferSize; i++) {
                EXPECT_NEAR(kInt24Buffer[i] * gain, result32BitBuffer[i] >> 8, kIntTolerance);
            }
        } break;
        default:
            return;
    }
}

void ApplyGainTest::VerifyBufferResultWithClamp(const pcm_format pcmFormat, const float gain) {
    switch (pcmFormat) {
        case PCM_FORMAT_S16_LE:
            VerifyTypedBufferResultsWithClamp(mInt16BufferToConvert, kInt16Buffer, gain,
                                              kInt16tTolerance, static_cast<int16_t>(INT16_MIN),
                                              static_cast<int16_t>(INT16_MAX));
            break;
        case PCM_FORMAT_FLOAT_LE:
            VerifyTypedBufferResultsWithClamp(mFloatBufferToConvert, kFloatBuffer, gain,
                                              kFloatTolerance, kFloatMin, kFloatMax);
            break;
        case PCM_FORMAT_S32_LE:
            VerifyTypedBufferResultsWithClamp(mInt32BufferToConvert, kInt32Buffer, gain,
                                              kIntTolerance, INT32_MIN, INT32_MAX);
            break;
        case PCM_FORMAT_S24_LE: {
            for (size_t i = 0; i < kQ8_23Buffer.size(); i++) {
                float expectedResult =
                        std::clamp(float_from_q8_23(kQ8_23Buffer[i]) * gain,
                                   float_from_q8_23(kQ8_23Min), float_from_q8_23(kQ8_23Max));
                EXPECT_NEAR(expectedResult,
                            static_cast<float>(float_from_q8_23(mQ8_23BufferToConvert[i])),
                            kFloatTolerance);
            }
        } break;
        case PCM_FORMAT_S24_3LE: {
            size_t bufferSize = kInt24Buffer.size();
            std::vector<int32_t> result32BitBuffer(bufferSize);
            memcpy_to_i32_from_p24(result32BitBuffer.data(),
                                   reinterpret_cast<uint8_t*>(mInt24BufferToConvert.data()),
                                   bufferSize);
            for (size_t i = 0; i < bufferSize; i++) {
                result32BitBuffer[i] >>= 8;
            }
            VerifyTypedBufferResultsWithClamp(result32BitBuffer, kInt24Buffer, gain, kIntTolerance,
                                              kInt24Min, kInt24Max);
        } break;
        default:
            return;
    }
}

TEST_P(ApplyGainTest, ApplyGain) {
    float gain = std::get<INDEX_GAIN>(GetParam());
    int channelCount = std::get<INDEX_CHANNEL_COUNT>(GetParam());

    alsa::applyGain(mBuffer, gain, mBufferSizeBytes, mPcmFormat, channelCount);

    if (gain <= kUnityGain) {
        VerifyBufferResult(mPcmFormat, gain);
    } else {
        VerifyBufferResultWithClamp(mPcmFormat, gain);
    }
}

std::string GetApplyGainTestName(const testing::TestParamInfo<ApplyGainTestParameters>& info) {
    std::string testNameStr;
    switch (std::get<INDEX_PCM_FORMAT>(info.param)) {
        case PCM_FORMAT_S16_LE:
            testNameStr = "S16_LE";
            break;
        case PCM_FORMAT_FLOAT_LE:
            testNameStr = "Float_LE";
            break;
        case PCM_FORMAT_S32_LE:
            testNameStr = "S32_LE";
            break;
        case PCM_FORMAT_S24_LE:
            testNameStr = "S24_LE";
            break;
        case PCM_FORMAT_S24_3LE:
            testNameStr = "S24_3LE";
            break;
        default:
            testNameStr = "UnsupportedPcmFormat";
            break;
    }
    testNameStr += std::get<INDEX_CHANNEL_COUNT>(info.param) == 1 ? "_Mono_" : "_Stereo_";
    testNameStr += std::get<INDEX_GAIN>(info.param) <= kUnityGain ? "WithoutClamp" : "WithClamp";
    return testNameStr;
}

INSTANTIATE_TEST_SUITE_P(PerPcmFormat, ApplyGainTest,
                         testing::Combine(testing::Values(PCM_FORMAT_S16_LE, PCM_FORMAT_FLOAT_LE,
                                                          PCM_FORMAT_S32_LE, PCM_FORMAT_S24_LE,
                                                          PCM_FORMAT_S24_3LE),
                                          testing::Values(1, 2), testing::Values(0.6f, 1.5f)),
                         GetApplyGainTestName);
