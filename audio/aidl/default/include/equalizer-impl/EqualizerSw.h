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

#include "effect-impl/EffectThread.h"

namespace aidl::android::hardware::audio::effect {

class EqualizerSwWorker : public EffectThread {
    // EqualizerSwWorker(const std::string name){EffectThread(name)};
    void process() override;
};

class EqualizerSw : public BnEffect {
  public:
    EqualizerSw() {
        // create the worker
        mWorker = std::make_unique<EqualizerSwWorker>();
        LOG(DEBUG) << __func__;
    };
    ~EqualizerSw() {
        cleanUp();
        LOG(DEBUG) << __func__;
    };
    ndk::ScopedAStatus open(const Parameter::Common& common, const Parameter::Specific& specific,
                            OpenEffectReturn* _aidl_return) override;
    ndk::ScopedAStatus close() override;
    ndk::ScopedAStatus getDescriptor(Descriptor* _aidl_return) override;

    ndk::ScopedAStatus getState(State* _aidl_return) override;
    ndk::ScopedAStatus command(CommandId in_commandId) override;
    ndk::ScopedAStatus setParameter(const Parameter& in_param) override;
    ndk::ScopedAStatus getParameter(const Parameter::Id& in_paramId,
                                    Parameter* _aidl_return) override;

  private:
    // effect processing thread.
    std::unique_ptr<EqualizerSwWorker> mWorker;
    // Effect descriptor.
    const Descriptor mDesc = {
            .common = {.id = {.type = EqualizerTypeUUID, .uuid = EqualizerSwImplUUID}}};

    // Parameters.
    Parameter::Common mCommonParam;
    Equalizer mEqualizerParam;  // TODO: the equalizer parameter needs to update

    // Instance state INIT by default.
    State mState = State::INIT;

    typedef ::android::AidlMessageQueue<
            Status, ::aidl::android::hardware::common::fmq::SynchronizedReadWrite>
            StatusMQ;
    typedef ::android::AidlMessageQueue<
            int8_t, ::aidl::android::hardware::common::fmq::SynchronizedReadWrite>
            DataMQ;

    std::unique_ptr<StatusMQ> mStatusMQ;
    std::unique_ptr<DataMQ> mInputMQ;
    std::unique_ptr<DataMQ> mOutputMQ;

    ndk::ScopedAStatus setCommonParameter(const Parameter::Common& common_param);
    ndk::ScopedAStatus setSpecificParameter(const Parameter::Specific& specific);
    bool createFmq(int statusDepth, int inBufferSize, int outBufferSize, OpenEffectReturn* ret);
    void destroyFmq();
    void cleanUp();
};
}  // namespace aidl::android::hardware::audio::effect
