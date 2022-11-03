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

#define LOG_TAG "AHAL_EffectImpl"
#include "effect-impl/EffectImpl.h"
#include "effect-impl/EffectTypes.h"
#include "include/effect-impl/EffectTypes.h"

namespace aidl::android::hardware::audio::effect {

ndk::ScopedAStatus EffectImpl::open(const Parameter::Common& common,
                                    const std::optional<Parameter::Specific>& specific,
                                    OpenEffectReturn* ret) {
    LOG(DEBUG) << __func__;
    {
        std::lock_guard lg(mMutex);
        RETURN_OK_IF(mState != State::INIT);
        mContext = createContext(common);
        RETURN_IF(!mContext, EX_ILLEGAL_ARGUMENT, "createContextFailed");
        setContext(mContext);
    }

    RETURN_IF_ASTATUS_NOT_OK(setParameterCommon(common), "setCommParamErr");
    if (specific.has_value()) {
        RETURN_IF_ASTATUS_NOT_OK(setParameterSpecific(specific.value()), "setSpecParamErr");
    }

    RETURN_IF(createThread(LOG_TAG) != RetCode::SUCCESS, EX_UNSUPPORTED_OPERATION,
              "FailedToCreateWorker");

    {
        std::lock_guard lg(mMutex);
        mContext->dupeFmq(ret);
        mState = State::IDLE;
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EffectImpl::close() {
    std::lock_guard lg(mMutex);
    RETURN_OK_IF(mState == State::INIT);
    RETURN_IF(mState == State::PROCESSING, EX_ILLEGAL_STATE, "closeAtProcessing");

    // stop the worker thread, ignore the return code
    RETURN_IF(destroyThread() != RetCode::SUCCESS, EX_UNSUPPORTED_OPERATION,
              "FailedToDestroyWorker");
    RETURN_IF(releaseContext() != RetCode::SUCCESS, EX_UNSUPPORTED_OPERATION,
              "FailedToCreateWorker");
    mState = State::INIT;
    LOG(DEBUG) << __func__;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EffectImpl::setParameter(const Parameter& param) {
    LOG(DEBUG) << __func__ << " with: " << param.toString();

    auto tag = param.getTag();
    switch (tag) {
        case Parameter::common:
        case Parameter::deviceDescription:
        case Parameter::mode:
        case Parameter::source:
            FALLTHROUGH_INTENDED;
        case Parameter::volumeStereo:
            return setParameterCommon(param);
        case Parameter::specific: {
            return setParameterSpecific(param.get<Parameter::specific>());
        }
        default: {
            LOG(ERROR) << __func__ << " unsupportedParameterTag " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "ParameterNotSupported");
        }
    }
}

ndk::ScopedAStatus EffectImpl::getParameter(const Parameter::Id& id, Parameter* param) {
    LOG(DEBUG) << __func__ << id.toString();
    auto tag = id.getTag();
    switch (tag) {
        case Parameter::Id::commonTag: {
            RETURN_IF_ASTATUS_NOT_OK(getParameterCommon(id.get<Parameter::Id::commonTag>(), param),
                                     "CommonParamNotSupported");
            break;
        }
        case Parameter::Id::vendorEffectTag: {
            LOG(DEBUG) << __func__ << " noop for vendor tag";
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "vendortagNotSupported");
        }
        default: {
            Parameter::Specific specific;
            RETURN_IF_ASTATUS_NOT_OK(getParameterSpecific(id, &specific), "SpecParamNotSupported");
            param->set<Parameter::specific>(specific);
            break;
        }
    }
    LOG(DEBUG) << __func__ << param->toString();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EffectImpl::setParameterCommon(const Parameter& param) {
    std::lock_guard lg(mMutex);
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");
    auto tag = param.getTag();
    switch (tag) {
        case Parameter::common:
            RETURN_IF(mContext->setCommon(param.get<Parameter::common>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setCommFailed");
            break;
        case Parameter::deviceDescription:
            RETURN_IF(mContext->setOutputDevice(param.get<Parameter::deviceDescription>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setDeviceFailed");
            break;
        case Parameter::mode:
            RETURN_IF(mContext->setAudioMode(param.get<Parameter::mode>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setModeFailed");
            break;
        case Parameter::source:
            RETURN_IF(mContext->setAudioSource(param.get<Parameter::source>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setSourceFailed");
            break;
        case Parameter::volumeStereo:
            RETURN_IF(mContext->setVolumeStereo(param.get<Parameter::volumeStereo>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setVolumeStereoFailed");
            break;
        default: {
            LOG(ERROR) << __func__ << " unsupportedParameterTag " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "commonParamNotSupported");
        }
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EffectImpl::getParameterCommon(const Parameter::Tag& tag, Parameter* param) {
    std::lock_guard lg(mMutex);
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");
    switch (tag) {
        case Parameter::common: {
            param->set<Parameter::common>(mContext->getCommon());
            break;
        }
        case Parameter::deviceDescription: {
            param->set<Parameter::deviceDescription>(mContext->getOutputDevice());
            break;
        }
        case Parameter::mode: {
            param->set<Parameter::mode>(mContext->getAudioMode());
            break;
        }
        case Parameter::source: {
            param->set<Parameter::source>(mContext->getAudioSource());
            break;
        }
        case Parameter::volumeStereo: {
            param->set<Parameter::volumeStereo>(mContext->getVolumeStereo());
            break;
        }
        default: {
            LOG(DEBUG) << __func__ << " unsupported tag " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "tagNotSupported");
        }
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EffectImpl::getState(State* state) {
    std::lock_guard lg(mMutex);
    *state = mState;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EffectImpl::command(CommandId command) {
    std::lock_guard lg(mMutex);
    LOG(DEBUG) << __func__ << ": receive command: " << toString(command) << " at state "
               << toString(mState);
    RETURN_IF(mState == State::INIT, EX_ILLEGAL_STATE, "CommandStateError");
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");

    switch (command) {
        case CommandId::START:
            RETURN_IF(mState == State::INIT, EX_ILLEGAL_STATE, "instanceNotOpen");
            RETURN_OK_IF(mState == State::PROCESSING);
            RETURN_IF_ASTATUS_NOT_OK(commandStart(), "commandStartFailed");
            mState = State::PROCESSING;
            startThread();
            return ndk::ScopedAStatus::ok();
        case CommandId::STOP:
            RETURN_OK_IF(mState == State::IDLE);
            mState = State::IDLE;
            RETURN_IF_ASTATUS_NOT_OK(commandStop(), "commandStopFailed");
            stopThread();
            return ndk::ScopedAStatus::ok();
        case CommandId::RESET:
            RETURN_OK_IF(mState == State::IDLE);
            mState = State::IDLE;
            RETURN_IF_ASTATUS_NOT_OK(commandStop(), "commandStopFailed");
            stopThread();
            mContext->resetBuffer();
            return ndk::ScopedAStatus::ok();
        default:
            LOG(ERROR) << __func__ << " instance still processing";
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "CommandIdNotSupported");
    }
    LOG(DEBUG) << __func__ << " transfer to state: " << toString(mState);
    return ndk::ScopedAStatus::ok();
}

void EffectImpl::cleanUp() {
    command(CommandId::STOP);
    close();
}

IEffect::Status EffectImpl::status(binder_status_t status, size_t consumed, size_t produced) {
    IEffect::Status ret;
    ret.status = status;
    ret.fmqConsumed = consumed;
    ret.fmqProduced = produced;
    return ret;
}

// A placeholder processing implementation to copy samples from input to output
IEffect::Status EffectImpl::effectProcessImpl(float* in, float* out, int processSamples) {
    // lock before access context/parameters
    std::lock_guard lg(mMutex);
    IEffect::Status status = {EX_NULL_POINTER, 0, 0};
    RETURN_VALUE_IF(!mContext, status, "nullContext");
    auto frameSize = mContext->getInputFrameSize();
    RETURN_VALUE_IF(0 == frameSize, status, "frameSizeIs0");
    LOG(DEBUG) << __func__ << " in " << in << " out " << out << " samples " << processSamples
               << " frames " << processSamples * sizeof(float) / frameSize;
    for (int i = 0; i < processSamples; i++) {
        *out++ = *in++;
    }
    LOG(DEBUG) << __func__ << " done processing " << processSamples << " samples";
    return {STATUS_OK, processSamples, processSamples};
}
}  // namespace aidl::android::hardware::audio::effect
