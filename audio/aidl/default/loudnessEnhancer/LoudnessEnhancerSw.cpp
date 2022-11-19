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

#include <cstddef>
#define LOG_TAG "AHAL_LoudnessEnhancerSw"
#include <Utils.h>
#include <algorithm>
#include <unordered_set>

#include <android-base/logging.h>
#include <fmq/AidlMessageQueue.h>

#include "LoudnessEnhancerSw.h"

using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::kLoudnessEnhancerSwImplUUID;
using aidl::android::hardware::audio::effect::LoudnessEnhancerSw;
using aidl::android::hardware::audio::effect::State;
using aidl::android::media::audio::common::AudioUuid;

extern "C" binder_exception_t createEffect(const AudioUuid* in_impl_uuid,
                                           std::shared_ptr<IEffect>* instanceSpp) {
    if (!in_impl_uuid || *in_impl_uuid != kLoudnessEnhancerSwImplUUID) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    if (instanceSpp) {
        *instanceSpp = ndk::SharedRefBase::make<LoudnessEnhancerSw>();
        LOG(DEBUG) << __func__ << " instance " << instanceSpp->get() << " created";
        return EX_NONE;
    } else {
        LOG(ERROR) << __func__ << " invalid input parameter!";
        return EX_ILLEGAL_ARGUMENT;
    }
}

extern "C" binder_exception_t destroyEffect(const std::shared_ptr<IEffect>& instanceSp) {
    if (!instanceSp) {
        return EX_NONE;
    }
    State state;
    ndk::ScopedAStatus status = instanceSp->getState(&state);
    if (!status.isOk() || State::INIT != state) {
        LOG(ERROR) << __func__ << " instance " << instanceSp.get()
                   << " in state: " << toString(state) << ", status: " << status.getDescription();
        return EX_ILLEGAL_STATE;
    }
    LOG(DEBUG) << __func__ << " instance " << instanceSp.get() << " destroyed";
    return EX_NONE;
}

namespace aidl::android::hardware::audio::effect {

ndk::ScopedAStatus LoudnessEnhancerSw::getDescriptor(Descriptor* _aidl_return) {
    LOG(DEBUG) << __func__ << kDescriptor.toString();
    *_aidl_return = kDescriptor;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus LoudnessEnhancerSw::setParameterSpecific(const Parameter::Specific& specific) {
    RETURN_IF(Parameter::Specific::loudnessEnhancer != specific.getTag(), EX_ILLEGAL_ARGUMENT,
              "EffectNotSupported");
    std::lock_guard lg(mMutex);
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");

    auto& leParam = specific.get<Parameter::Specific::loudnessEnhancer>();
    auto tag = leParam.getTag();

    switch (tag) {
        case LoudnessEnhancer::gainMb: {
            RETURN_IF(mContext->setLeGainMb(leParam.get<LoudnessEnhancer::gainMb>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setGainMbFailed");
            return ndk::ScopedAStatus::ok();
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "LoudnessEnhancerTagNotSupported");
        }
    }
}

ndk::ScopedAStatus LoudnessEnhancerSw::getParameterSpecific(const Parameter::Id& id,
                                                            Parameter::Specific* specific) {
    auto tag = id.getTag();
    RETURN_IF(Parameter::Id::loudnessEnhancerTag != tag, EX_ILLEGAL_ARGUMENT, "wrongIdTag");
    auto leId = id.get<Parameter::Id::loudnessEnhancerTag>();
    auto leIdTag = leId.getTag();
    switch (leIdTag) {
        case LoudnessEnhancer::Id::commonTag:
            return getParameterLoudnessEnhancer(leId.get<LoudnessEnhancer::Id::commonTag>(),
                                                specific);
        default:
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(leIdTag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "LoudnessEnhancerTagNotSupported");
    }
}

ndk::ScopedAStatus LoudnessEnhancerSw::getParameterLoudnessEnhancer(
        const LoudnessEnhancer::Tag& tag, Parameter::Specific* specific) {
    std::lock_guard lg(mMutex);
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");

    LoudnessEnhancer leParam;
    switch (tag) {
        case LoudnessEnhancer::gainMb: {
            leParam.set<LoudnessEnhancer::gainMb>(mContext->getLeGainMb());
            break;
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "LoudnessEnhancerTagNotSupported");
        }
    }

    specific->set<Parameter::Specific::loudnessEnhancer>(leParam);
    return ndk::ScopedAStatus::ok();
}

std::shared_ptr<EffectContext> LoudnessEnhancerSw::createContext(const Parameter::Common& common) {
    if (mContext) {
        LOG(DEBUG) << __func__ << " context already exist";
        return mContext;
    }
    mContext = std::make_shared<LoudnessEnhancerSwContext>(1 /* statusFmqDepth */, common);
    return mContext;
}

RetCode LoudnessEnhancerSw::releaseContext() {
    if (mContext) {
        mContext.reset();
    }
    return RetCode::SUCCESS;
}

// Processing method running in EffectWorker thread.
IEffect::Status LoudnessEnhancerSw::effectProcessImpl(float* in, float* out, int process) {
    // TODO: get data buffer and process.
    LOG(DEBUG) << __func__ << " in " << in << " out " << out << " process " << process;
    for (int i = 0; i < process; i++) {
        *out++ = *in++;
    }
    return {STATUS_OK, process, process};
}

}  // namespace aidl::android::hardware::audio::effect
