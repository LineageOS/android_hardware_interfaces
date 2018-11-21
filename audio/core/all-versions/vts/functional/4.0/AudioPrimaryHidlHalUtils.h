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

#include PATH(android/hardware/audio/FILE_VERSION/IStream.h)
#include PATH(android/hardware/audio/FILE_VERSION/types.h)
#include PATH(android/hardware/audio/common/FILE_VERSION/types.h)
#include <hidl/HidlSupport.h>

using ::android::hardware::hidl_bitfield;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::audio::common::CPP_VERSION::AudioChannelMask;
using ::android::hardware::audio::common::CPP_VERSION::AudioFormat;
using ::android::hardware::audio::CPP_VERSION::IStream;
using ::android::hardware::audio::CPP_VERSION::ParameterValue;
using ::android::hardware::audio::CPP_VERSION::Result;

using namespace ::android::hardware::audio::common::test::utility;

using Rotation = ::android::hardware::audio::CPP_VERSION::IPrimaryDevice::Rotation;
using ::android::hardware::audio::common::CPP_VERSION::AudioContentType;
using ::android::hardware::audio::common::CPP_VERSION::AudioUsage;
using ::android::hardware::audio::CPP_VERSION::MicrophoneInfo;
#if MAJOR_VERSION < 5
using ::android::hardware::audio::CPP_VERSION::SinkMetadata;
using ::android::hardware::audio::CPP_VERSION::SourceMetadata;
#else
using ::android::hardware::audio::common::CPP_VERSION::SinkMetadata;
using ::android::hardware::audio::common::CPP_VERSION::SourceMetadata;
#endif

struct Parameters {
    template <class T, class ReturnIn>
    static auto get(T t, hidl_vec<hidl_string> keys, ReturnIn returnIn) {
        hidl_vec<ParameterValue> context;
        return t->getParameters(context, keys, returnIn);
    }
    template <class T>
    static auto set(T t, hidl_vec<ParameterValue> values) {
        hidl_vec<ParameterValue> context;
        return t->setParameters(context, values);
    }
};

struct GetSupported {
    static auto getFormat(IStream* stream) {
        auto ret = stream->getFormat();
        EXPECT_TRUE(ret.isOk());
        return ret.withDefault({});
    }
    static Result sampleRates(IStream* stream, hidl_vec<uint32_t>& rates) {
        Result res;
        EXPECT_OK(stream->getSupportedSampleRates(getFormat(stream), returnIn(res, rates)));
        return res;
    }

    static Result channelMasks(IStream* stream,
                               hidl_vec<hidl_bitfield<AudioChannelMask>>& channels) {
        Result res;
        EXPECT_OK(stream->getSupportedChannelMasks(getFormat(stream), returnIn(res, channels)));
        return res;
    }

    static Result formats(IStream* stream, hidl_vec<AudioFormat>& capabilities) {
        EXPECT_OK(stream->getSupportedFormats(returnIn(capabilities)));
        // TODO: this should be an optional function
        return Result::OK;
    }
};

template <class T>
auto dump(T t, hidl_handle handle) {
    return t->debug(handle, {/* options */});
}
