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

#pragma once
#include <aidl/android/hardware/audio/effect/BnEffect.h>
#include <fmq/AidlMessageQueue.h>
#include <cstdlib>
#include <memory>
#include <mutex>

#include "EffectTypes.h"
#include "effect-impl/EffectContext.h"
#include "effect-impl/EffectTypes.h"
#include "effect-impl/EffectWorker.h"

namespace aidl::android::hardware::audio::effect {

class EffectImpl : public BnEffect, public EffectWorker {
  public:
    EffectImpl() { LOG(DEBUG) << __func__; }
    ~EffectImpl() {
        cleanUp();
        LOG(DEBUG) << __func__;
    }

    /**
     * Each effect implementation CAN override these methods if necessary
     * If you would like implement IEffect::open completely, override EffectImpl::open(), if you
     * want to keep most of EffectImpl logic but have a little customize, try override openImpl().
     * openImpl() will be called at the beginning of EffectImpl::open() without lock protection.
     *
     * Same for closeImpl().
     */
    virtual ndk::ScopedAStatus open(const Parameter::Common& common,
                                    const std::optional<Parameter::Specific>& specific,
                                    OpenEffectReturn* ret) override;
    virtual ndk::ScopedAStatus close() override;
    virtual ndk::ScopedAStatus command(CommandId id) override;
    virtual ndk::ScopedAStatus commandStart() { return ndk::ScopedAStatus::ok(); }
    virtual ndk::ScopedAStatus commandStop() { return ndk::ScopedAStatus::ok(); }
    virtual ndk::ScopedAStatus commandReset() { return ndk::ScopedAStatus::ok(); }

    virtual ndk::ScopedAStatus getState(State* state) override;
    virtual ndk::ScopedAStatus setParameter(const Parameter& param) override;
    virtual ndk::ScopedAStatus getParameter(const Parameter::Id& id, Parameter* param) override;
    virtual IEffect::Status effectProcessImpl(float* in, float* out, int process) override;

    virtual ndk::ScopedAStatus setParameterCommon(const Parameter& param);
    virtual ndk::ScopedAStatus getParameterCommon(const Parameter::Tag& tag, Parameter* param);

    /* Methods MUST be implemented by each effect instances */
    virtual ndk::ScopedAStatus getDescriptor(Descriptor* desc) = 0;
    virtual ndk::ScopedAStatus setParameterSpecific(const Parameter::Specific& specific) = 0;
    virtual ndk::ScopedAStatus getParameterSpecific(const Parameter::Id& id,
                                                    Parameter::Specific* specific) = 0;
    virtual std::shared_ptr<EffectContext> createContext(const Parameter::Common& common) = 0;
    virtual RetCode releaseContext() = 0;

  protected:
    /*
     * Lock is required if effectProcessImpl (which is running in an independent thread) needs to
     * access state and parameters.
     */
    std::mutex mMutex;
    State mState GUARDED_BY(mMutex) = State::INIT;

    IEffect::Status status(binder_status_t status, size_t consumed, size_t produced);

  private:
    void cleanUp();
    std::shared_ptr<EffectContext> mContext GUARDED_BY(mMutex);
};
}  // namespace aidl::android::hardware::audio::effect
