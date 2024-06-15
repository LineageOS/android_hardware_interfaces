/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <algorithm>
#include <cstddef>

#define LOG_TAG "AHAL_VolumeSw"
#include <android-base/logging.h>
#include <fmq/AidlMessageQueue.h>
#include <system/audio_effects/effect_uuid.h>

#include "VolumeSw.h"

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::getEffectImplUuidVolumeSw;
using aidl::android::hardware::audio::effect::getEffectTypeUuidVolume;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::State;
using aidl::android::hardware::audio::effect::VolumeSw;
using aidl::android::media::audio::common::AudioUuid;

extern "C" binder_exception_t createEffect(const AudioUuid* in_impl_uuid,
                                           std::shared_ptr<IEffect>* instanceSpp) {
    if (!in_impl_uuid || *in_impl_uuid != getEffectImplUuidVolumeSw()) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    if (instanceSpp) {
        *instanceSpp = ndk::SharedRefBase::make<VolumeSw>();
        LOG(DEBUG) << __func__ << " instance " << instanceSpp->get() << " created";
        return EX_NONE;
    } else {
        LOG(ERROR) << __func__ << " invalid input parameter!";
        return EX_ILLEGAL_ARGUMENT;
    }
}

extern "C" binder_exception_t queryEffect(const AudioUuid* in_impl_uuid, Descriptor* _aidl_return) {
    if (!in_impl_uuid || *in_impl_uuid != getEffectImplUuidVolumeSw()) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    *_aidl_return = VolumeSw::kDescriptor;
    return EX_NONE;
}

namespace aidl::android::hardware::audio::effect {

const std::string VolumeSw::kEffectName = "VolumeSw";

const std::vector<Range::VolumeRange> VolumeSw::kRanges = {MAKE_RANGE(Volume, levelDb, -9600, 0)};

const Capability VolumeSw::kCapability = {.range = Range::make<Range::volume>(VolumeSw::kRanges)};

const Descriptor VolumeSw::kDescriptor = {
        .common = {.id = {.type = getEffectTypeUuidVolume(),
                          .uuid = getEffectImplUuidVolumeSw(),
                          .proxy = std::nullopt},
                   .flags = {.type = Flags::Type::INSERT,
                             .insert = Flags::Insert::FIRST,
                             .volume = Flags::Volume::CTRL},
                   .name = VolumeSw::kEffectName,
                   .implementor = "The Android Open Source Project"},
        .capability = VolumeSw::kCapability};

ndk::ScopedAStatus VolumeSw::getDescriptor(Descriptor* _aidl_return) {
    LOG(DEBUG) << __func__ << kDescriptor.toString();
    *_aidl_return = kDescriptor;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus VolumeSw::setParameterSpecific(const Parameter::Specific& specific) {
    RETURN_IF(Parameter::Specific::volume != specific.getTag(), EX_ILLEGAL_ARGUMENT,
              "EffectNotSupported");

    auto& volParam = specific.get<Parameter::Specific::volume>();
    RETURN_IF(!inRange(volParam, kRanges), EX_ILLEGAL_ARGUMENT, "outOfRange");
    auto tag = volParam.getTag();

    switch (tag) {
        case Volume::levelDb: {
            RETURN_IF(mContext->setVolLevel(volParam.get<Volume::levelDb>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "LevelNotSupported");
            return ndk::ScopedAStatus::ok();
        }
        case Volume::mute: {
            RETURN_IF(mContext->setVolMute(volParam.get<Volume::mute>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "MuteNotSupported");
            return ndk::ScopedAStatus::ok();
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "VolumeTagNotSupported");
        }
    }
}

ndk::ScopedAStatus VolumeSw::getParameterSpecific(const Parameter::Id& id,
                                                  Parameter::Specific* specific) {
    auto tag = id.getTag();
    RETURN_IF(Parameter::Id::volumeTag != tag, EX_ILLEGAL_ARGUMENT, "wrongIdTag");
    auto volId = id.get<Parameter::Id::volumeTag>();
    auto volIdTag = volId.getTag();
    switch (volIdTag) {
        case Volume::Id::commonTag:
            return getParameterVolume(volId.get<Volume::Id::commonTag>(), specific);
        default:
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "VolumeTagNotSupported");
    }
}

ndk::ScopedAStatus VolumeSw::getParameterVolume(const Volume::Tag& tag,
                                                Parameter::Specific* specific) {
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");

    Volume volParam;
    switch (tag) {
        case Volume::levelDb: {
            volParam.set<Volume::levelDb>(mContext->getVolLevel());
            break;
        }
        case Volume::mute: {
            volParam.set<Volume::mute>(mContext->getVolMute());
            break;
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "VolumeTagNotSupported");
        }
    }

    specific->set<Parameter::Specific::volume>(volParam);
    return ndk::ScopedAStatus::ok();
}

std::shared_ptr<EffectContext> VolumeSw::createContext(const Parameter::Common& common) {
    if (mContext) {
        LOG(DEBUG) << __func__ << " context already exist";
    } else {
        mContext = std::make_shared<VolumeSwContext>(1 /* statusFmqDepth */, common);
    }

    return mContext;
}

RetCode VolumeSw::releaseContext() {
    if (mContext) {
        mContext.reset();
    }
    return RetCode::SUCCESS;
}

// Processing method running in EffectWorker thread.
IEffect::Status VolumeSw::effectProcessImpl(float* in, float* out, int samples) {
    // TODO: get data buffer and process.
    LOG(DEBUG) << __func__ << " in " << in << " out " << out << " samples " << samples;
    for (int i = 0; i < samples; i++) {
        *out++ = *in++;
    }
    return {STATUS_OK, samples, samples};
}

RetCode VolumeSwContext::setVolLevel(int level) {
    mLevel = level;
    return RetCode::SUCCESS;
}

RetCode VolumeSwContext::setVolMute(bool mute) {
    // TODO : Add implementation to modify mute
    mMute = mute;
    return RetCode::SUCCESS;
}

}  // namespace aidl::android::hardware::audio::effect
