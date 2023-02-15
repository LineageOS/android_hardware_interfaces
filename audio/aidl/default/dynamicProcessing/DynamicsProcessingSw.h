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

#include "effect-impl/EffectImpl.h"
#include "effect-impl/EffectUUID.h"

namespace aidl::android::hardware::audio::effect {

class DynamicsProcessingSwContext final : public EffectContext {
  public:
    DynamicsProcessingSwContext(int statusDepth, const Parameter::Common& common)
        : EffectContext(statusDepth, common),
          mChannelCount(::android::hardware::audio::common::getChannelCount(
                  common.input.base.channelMask)),
          mPreEqChCfgs(mChannelCount, {.channel = kInvalidChannelId}),
          mPostEqChCfgs(mChannelCount, {.channel = kInvalidChannelId}),
          mMbcChCfgs(mChannelCount, {.channel = kInvalidChannelId}),
          mLimiterCfgs(mChannelCount, {.channel = kInvalidChannelId}) {
        LOG(DEBUG) << __func__;
    }

    // utils
    RetCode setChannelCfgs(const std::vector<DynamicsProcessing::ChannelConfig>& cfgs,
                           std::vector<DynamicsProcessing::ChannelConfig>& targetCfgs,
                           const DynamicsProcessing::StageEnablement& engineSetting);

    RetCode setEqBandCfgs(const std::vector<DynamicsProcessing::EqBandConfig>& cfgs,
                          std::vector<DynamicsProcessing::EqBandConfig>& targetCfgs,
                          const DynamicsProcessing::StageEnablement& stage,
                          const std::vector<DynamicsProcessing::ChannelConfig>& channelConfig);

    // set params
    RetCode setCommon(const Parameter::Common& common) override;
    RetCode setEngineArchitecture(const DynamicsProcessing::EngineArchitecture& cfg);
    RetCode setPreEqChannelCfgs(const std::vector<DynamicsProcessing::ChannelConfig>& cfgs);
    RetCode setPostEqChannelCfgs(const std::vector<DynamicsProcessing::ChannelConfig>& cfgs);
    RetCode setMbcChannelCfgs(const std::vector<DynamicsProcessing::ChannelConfig>& cfgs);
    RetCode setPreEqBandCfgs(const std::vector<DynamicsProcessing::EqBandConfig>& cfgs);
    RetCode setPostEqBandCfgs(const std::vector<DynamicsProcessing::EqBandConfig>& cfgs);
    RetCode setMbcBandCfgs(const std::vector<DynamicsProcessing::MbcBandConfig>& cfgs);
    RetCode setLimiterCfgs(const std::vector<DynamicsProcessing::LimiterConfig>& cfgs);
    RetCode setInputGainCfgs(const std::vector<DynamicsProcessing::InputGain>& cfgs);

    // get params
    DynamicsProcessing::EngineArchitecture getEngineArchitecture() { return mEngineSettings; }
    std::vector<DynamicsProcessing::ChannelConfig> getPreEqChannelCfgs() { return mPreEqChCfgs; }
    std::vector<DynamicsProcessing::ChannelConfig> getPostEqChannelCfgs() { return mPostEqChCfgs; }
    std::vector<DynamicsProcessing::ChannelConfig> getMbcChannelCfgs() { return mMbcChCfgs; }
    std::vector<DynamicsProcessing::EqBandConfig> getPreEqBandCfgs() { return mPreEqChBands; }
    std::vector<DynamicsProcessing::EqBandConfig> getPostEqBandCfgs() { return mPostEqChBands; }
    std::vector<DynamicsProcessing::MbcBandConfig> getMbcBandCfgs() { return mMbcChBands; }
    std::vector<DynamicsProcessing::LimiterConfig> getLimiterCfgs() { return mLimiterCfgs; }
    std::vector<DynamicsProcessing::InputGain> getInputGainCfgs();

  private:
    static constexpr int32_t kInvalidChannelId = -1;
    size_t mChannelCount = 0;
    DynamicsProcessing::EngineArchitecture mEngineSettings;
    // Channel config vector with size of mChannelCount
    std::vector<DynamicsProcessing::ChannelConfig> mPreEqChCfgs;
    std::vector<DynamicsProcessing::ChannelConfig> mPostEqChCfgs;
    std::vector<DynamicsProcessing::ChannelConfig> mMbcChCfgs;
    std::vector<DynamicsProcessing::LimiterConfig> mLimiterCfgs;
    std::vector<DynamicsProcessing::InputGain> mInputGainCfgs;
    // Band config vector with size of mChannelCount * bandCount
    std::vector<DynamicsProcessing::EqBandConfig> mPreEqChBands;
    std::vector<DynamicsProcessing::EqBandConfig> mPostEqChBands;
    std::vector<DynamicsProcessing::MbcBandConfig> mMbcChBands;
    bool validateStageEnablement(const DynamicsProcessing::StageEnablement& enablement);
    bool validateEngineConfig(const DynamicsProcessing::EngineArchitecture& engine);
    bool validateEqBandConfig(const DynamicsProcessing::EqBandConfig& band, int maxChannel,
                              int maxBand,
                              const std::vector<DynamicsProcessing::ChannelConfig>& channelConfig);
    bool validateMbcBandConfig(const DynamicsProcessing::MbcBandConfig& band, int maxChannel,
                               int maxBand,
                               const std::vector<DynamicsProcessing::ChannelConfig>& channelConfig);
    bool validateLimiterConfig(const DynamicsProcessing::LimiterConfig& limiter, int maxChannel);
    void resizeChannels();
    void resizeBands();
};  // DynamicsProcessingSwContext

class DynamicsProcessingSw final : public EffectImpl {
  public:
    static const std::string kEffectName;
    static const Capability kCapability;
    static const Descriptor kDescriptor;
    DynamicsProcessingSw() { LOG(DEBUG) << __func__; }
    ~DynamicsProcessingSw() {
        cleanUp();
        LOG(DEBUG) << __func__;
    }

    ndk::ScopedAStatus getDescriptor(Descriptor* _aidl_return) override;
    ndk::ScopedAStatus setParameterSpecific(const Parameter::Specific& specific) override;
    ndk::ScopedAStatus getParameterSpecific(const Parameter::Id& id,
                                            Parameter::Specific* specific) override;

    std::shared_ptr<EffectContext> createContext(const Parameter::Common& common) override;
    std::shared_ptr<EffectContext> getContext() override;
    RetCode releaseContext() override;

    IEffect::Status effectProcessImpl(float* in, float* out, int samples) override;
    std::string getEffectName() override { return kEffectName; };

  private:
    static const DynamicsProcessing::EqBandConfig kEqBandConfigMin;
    static const DynamicsProcessing::EqBandConfig kEqBandConfigMax;
    static const Range::DynamicsProcessingRange kPreEqBandRange;
    static const Range::DynamicsProcessingRange kPostEqBandRange;
    static const std::vector<Range::DynamicsProcessingRange> kRanges;
    std::shared_ptr<DynamicsProcessingSwContext> mContext;
    ndk::ScopedAStatus getParameterDynamicsProcessing(const DynamicsProcessing::Tag& tag,
                                                      Parameter::Specific* specific);

};  // DynamicsProcessingSw

}  // namespace aidl::android::hardware::audio::effect
