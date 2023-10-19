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
#include <set>
#include <unordered_set>

#define LOG_TAG "AHAL_DynamicsProcessingSw"
#include <android-base/logging.h>
#include <fmq/AidlMessageQueue.h>
#include <system/audio_effects/effect_uuid.h>

#include "DynamicsProcessingSw.h"

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::DynamicsProcessingSw;
using aidl::android::hardware::audio::effect::getEffectImplUuidDynamicsProcessingSw;
using aidl::android::hardware::audio::effect::getEffectTypeUuidDynamicsProcessing;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::State;
using aidl::android::media::audio::common::AudioUuid;

extern "C" binder_exception_t createEffect(const AudioUuid* in_impl_uuid,
                                           std::shared_ptr<IEffect>* instanceSpp) {
    if (!in_impl_uuid || *in_impl_uuid != getEffectImplUuidDynamicsProcessingSw()) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    if (instanceSpp) {
        *instanceSpp = ndk::SharedRefBase::make<DynamicsProcessingSw>();
        LOG(DEBUG) << __func__ << " instance " << instanceSpp->get() << " created";
        return EX_NONE;
    } else {
        LOG(ERROR) << __func__ << " invalid input parameter!";
        return EX_ILLEGAL_ARGUMENT;
    }
}

extern "C" binder_exception_t queryEffect(const AudioUuid* in_impl_uuid, Descriptor* _aidl_return) {
    if (!in_impl_uuid || *in_impl_uuid != getEffectImplUuidDynamicsProcessingSw()) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    *_aidl_return = DynamicsProcessingSw::kDescriptor;
    return EX_NONE;
}

namespace aidl::android::hardware::audio::effect {

const std::string DynamicsProcessingSw::kEffectName = "DynamicsProcessingSw";
const DynamicsProcessing::EqBandConfig DynamicsProcessingSw::kEqBandConfigMin =
        DynamicsProcessing::EqBandConfig({.channel = 0,
                                          .band = 0,
                                          .enable = false,
                                          .cutoffFrequencyHz = 220,
                                          .gainDb = std::numeric_limits<float>::min()});
const DynamicsProcessing::EqBandConfig DynamicsProcessingSw::kEqBandConfigMax =
        DynamicsProcessing::EqBandConfig({.channel = std::numeric_limits<int>::max(),
                                          .band = std::numeric_limits<int>::max(),
                                          .enable = true,
                                          .cutoffFrequencyHz = 20000,
                                          .gainDb = std::numeric_limits<float>::max()});
const Range::DynamicsProcessingRange DynamicsProcessingSw::kPreEqBandRange = {
        .min = DynamicsProcessing::make<DynamicsProcessing::preEqBand>(
                {DynamicsProcessingSw::kEqBandConfigMin}),
        .max = DynamicsProcessing::make<DynamicsProcessing::preEqBand>(
                {DynamicsProcessingSw::kEqBandConfigMax})};
const Range::DynamicsProcessingRange DynamicsProcessingSw::kPostEqBandRange = {
        .min = DynamicsProcessing::make<DynamicsProcessing::postEqBand>(
                {DynamicsProcessingSw::kEqBandConfigMin}),
        .max = DynamicsProcessing::make<DynamicsProcessing::postEqBand>(
                {DynamicsProcessingSw::kEqBandConfigMax})};

const std::vector<Range::DynamicsProcessingRange> DynamicsProcessingSw::kRanges = {
        DynamicsProcessingSw::kPreEqBandRange, DynamicsProcessingSw::kPostEqBandRange};
const Capability DynamicsProcessingSw::kCapability = {.range = DynamicsProcessingSw::kRanges};

const Descriptor DynamicsProcessingSw::kDescriptor = {
        .common = {.id = {.type = getEffectTypeUuidDynamicsProcessing(),
                          .uuid = getEffectImplUuidDynamicsProcessingSw(),
                          .proxy = std::nullopt},
                   .flags = {.type = Flags::Type::POST_PROC,
                             .insert = Flags::Insert::FIRST,
                             .volume = Flags::Volume::CTRL},
                   .name = DynamicsProcessingSw::kEffectName,
                   .implementor = "The Android Open Source Project"},
        .capability = DynamicsProcessingSw::kCapability};

ndk::ScopedAStatus DynamicsProcessingSw::getDescriptor(Descriptor* _aidl_return) {
    LOG(DEBUG) << __func__ << kDescriptor.toString();
    *_aidl_return = kDescriptor;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus DynamicsProcessingSw::setParameterSpecific(const Parameter::Specific& specific) {
    RETURN_IF(Parameter::Specific::dynamicsProcessing != specific.getTag(), EX_ILLEGAL_ARGUMENT,
              "EffectNotSupported");

    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");

    LOG(INFO) << __func__ << specific.toString();
    auto& dpParam = specific.get<Parameter::Specific::dynamicsProcessing>();
    auto tag = dpParam.getTag();
    switch (tag) {
        case DynamicsProcessing::engineArchitecture: {
            RETURN_IF(mContext->setEngineArchitecture(
                              dpParam.get<DynamicsProcessing::engineArchitecture>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setEngineArchitectureFailed");
            return ndk::ScopedAStatus::ok();
        }
        case DynamicsProcessing::preEq: {
            RETURN_IF(mContext->setPreEqChannelCfgs(dpParam.get<DynamicsProcessing::preEq>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setPreEqChannelCfgsFailed");
            return ndk::ScopedAStatus::ok();
        }
        case DynamicsProcessing::postEq: {
            RETURN_IF(mContext->setPostEqChannelCfgs(dpParam.get<DynamicsProcessing::postEq>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setPostEqChannelCfgsFailed");
            return ndk::ScopedAStatus::ok();
        }
        case DynamicsProcessing::mbc: {
            RETURN_IF(mContext->setMbcChannelCfgs(dpParam.get<DynamicsProcessing::mbc>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setMbcChannelCfgsFailed");
            return ndk::ScopedAStatus::ok();
        }
        case DynamicsProcessing::preEqBand: {
            RETURN_IF(mContext->setPreEqBandCfgs(dpParam.get<DynamicsProcessing::preEqBand>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setPreEqBandCfgsFailed");
            return ndk::ScopedAStatus::ok();
        }
        case DynamicsProcessing::postEqBand: {
            RETURN_IF(mContext->setPostEqBandCfgs(dpParam.get<DynamicsProcessing::postEqBand>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setPostEqBandCfgsFailed");
            return ndk::ScopedAStatus::ok();
        }
        case DynamicsProcessing::mbcBand: {
            RETURN_IF(mContext->setMbcBandCfgs(dpParam.get<DynamicsProcessing::mbcBand>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setMbcBandCfgsFailed");
            return ndk::ScopedAStatus::ok();
        }
        case DynamicsProcessing::limiter: {
            RETURN_IF(mContext->setLimiterCfgs(dpParam.get<DynamicsProcessing::limiter>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "limiterCfgsFailed");
            return ndk::ScopedAStatus::ok();
        }
        case DynamicsProcessing::inputGain: {
            RETURN_IF(mContext->setInputGainCfgs(dpParam.get<DynamicsProcessing::inputGain>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "inputGainCfgFailed");
            return ndk::ScopedAStatus::ok();
        }
        case DynamicsProcessing::vendor: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "DynamicsProcessingTagNotSupported");
        }
    }
}

ndk::ScopedAStatus DynamicsProcessingSw::getParameterSpecific(const Parameter::Id& id,
                                                              Parameter::Specific* specific) {
    auto tag = id.getTag();
    RETURN_IF(Parameter::Id::dynamicsProcessingTag != tag, EX_ILLEGAL_ARGUMENT, "wrongIdTag");
    auto dpId = id.get<Parameter::Id::dynamicsProcessingTag>();
    auto dpIdTag = dpId.getTag();
    switch (dpIdTag) {
        case DynamicsProcessing::Id::commonTag:
            return getParameterDynamicsProcessing(dpId.get<DynamicsProcessing::Id::commonTag>(),
                                                  specific);
        case DynamicsProcessing::Id::vendorExtensionTag:
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(dpIdTag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "DynamicsProcessingTagNotSupported");
    }
}

ndk::ScopedAStatus DynamicsProcessingSw::getParameterDynamicsProcessing(
        const DynamicsProcessing::Tag& tag, Parameter::Specific* specific) {
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");

    DynamicsProcessing dpParam;
    switch (tag) {
        case DynamicsProcessing::Tag::engineArchitecture: {
            dpParam.set<DynamicsProcessing::engineArchitecture>(mContext->getEngineArchitecture());
            break;
        }
        case DynamicsProcessing::Tag::preEq: {
            dpParam.set<DynamicsProcessing::preEq>(mContext->getPreEqChannelCfgs());
            break;
        }
        case DynamicsProcessing::Tag::postEq: {
            dpParam.set<DynamicsProcessing::postEq>(mContext->getPostEqChannelCfgs());
            break;
        }
        case DynamicsProcessing::Tag::mbc: {
            dpParam.set<DynamicsProcessing::mbc>(mContext->getMbcChannelCfgs());
            break;
        }
        case DynamicsProcessing::Tag::preEqBand: {
            dpParam.set<DynamicsProcessing::preEqBand>(mContext->getPreEqBandCfgs());
            break;
        }
        case DynamicsProcessing::Tag::postEqBand: {
            dpParam.set<DynamicsProcessing::postEqBand>(mContext->getPostEqBandCfgs());
            break;
        }
        case DynamicsProcessing::Tag::mbcBand: {
            dpParam.set<DynamicsProcessing::mbcBand>(mContext->getMbcBandCfgs());
            break;
        }
        case DynamicsProcessing::Tag::limiter: {
            dpParam.set<DynamicsProcessing::limiter>(mContext->getLimiterCfgs());
            break;
        }
        case DynamicsProcessing::Tag::inputGain: {
            dpParam.set<DynamicsProcessing::inputGain>(mContext->getInputGainCfgs());
            break;
        }
        case DynamicsProcessing::vendor: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "DynamicsProcessingTagNotSupported");
        }
    }

    specific->set<Parameter::Specific::dynamicsProcessing>(dpParam);
    LOG(INFO) << __func__ << specific->toString();
    return ndk::ScopedAStatus::ok();
}

std::shared_ptr<EffectContext> DynamicsProcessingSw::createContext(
        const Parameter::Common& common) {
    if (mContext) {
        LOG(DEBUG) << __func__ << " context already exist";
    } else {
        mContext = std::make_shared<DynamicsProcessingSwContext>(1 /* statusFmqDepth */, common);
    }
    return mContext;
}

std::shared_ptr<EffectContext> DynamicsProcessingSw::getContext() {
    return mContext;
}

RetCode DynamicsProcessingSw::releaseContext() {
    if (mContext) {
        mContext.reset();
    }
    return RetCode::SUCCESS;
}

// Processing method running in EffectWorker thread.
IEffect::Status DynamicsProcessingSw::effectProcessImpl(float* in, float* out, int samples) {
    // TODO: get data buffer and process.
    LOG(DEBUG) << __func__ << " in " << in << " out " << out << " samples " << samples;
    for (int i = 0; i < samples; i++) {
        *out++ = *in++;
    }
    return {STATUS_OK, samples, samples};
}

RetCode DynamicsProcessingSwContext::setCommon(const Parameter::Common& common) {
    mCommon = common;
    mChannelCount = ::aidl::android::hardware::audio::common::getChannelCount(
            common.input.base.channelMask);
    resizeChannels();
    resizeBands();
    LOG(INFO) << __func__ << mCommon.toString();
    return RetCode::SUCCESS;
}

RetCode DynamicsProcessingSwContext::setEngineArchitecture(
        const DynamicsProcessing::EngineArchitecture& cfg) {
    RETURN_VALUE_IF(!validateEngineConfig(cfg), RetCode::ERROR_ILLEGAL_PARAMETER,
                    "illegalEngineConfig");

    if (mEngineSettings == cfg) {
        LOG(INFO) << __func__ << " not change in engine, do nothing";
        return RetCode::SUCCESS;
    }
    mEngineSettings = cfg;
    resizeBands();
    return RetCode::SUCCESS;
}

RetCode DynamicsProcessingSwContext::setChannelCfgs(
        const std::vector<DynamicsProcessing::ChannelConfig>& cfgs,
        std::vector<DynamicsProcessing::ChannelConfig>& targetCfgs,
        const DynamicsProcessing::StageEnablement& stage) {
    RETURN_VALUE_IF(!stage.inUse, RetCode::ERROR_ILLEGAL_PARAMETER, "stageNotInUse");

    RetCode ret = RetCode::SUCCESS;
    std::unordered_set<int> channelSet;
    for (auto& cfg : cfgs) {
        if (cfg.channel < 0 || (size_t)cfg.channel >= mChannelCount) {
            LOG(ERROR) << __func__ << " skip illegal channel config " << cfg.toString();
            ret = RetCode::ERROR_ILLEGAL_PARAMETER;
            continue;
        }
        if (0 != channelSet.count(cfg.channel)) {
            LOG(WARNING) << __func__ << " duplicated channel " << cfg.channel;
            ret = RetCode::ERROR_ILLEGAL_PARAMETER;
        } else {
            channelSet.insert(cfg.channel);
        }
        targetCfgs[cfg.channel] = cfg;
    }
    return ret;
}

RetCode DynamicsProcessingSwContext::setPreEqChannelCfgs(
        const std::vector<DynamicsProcessing::ChannelConfig>& cfgs) {
    return setChannelCfgs(cfgs, mPreEqChCfgs, mEngineSettings.preEqStage);
}

RetCode DynamicsProcessingSwContext::setPostEqChannelCfgs(
        const std::vector<DynamicsProcessing::ChannelConfig>& cfgs) {
    return setChannelCfgs(cfgs, mPostEqChCfgs, mEngineSettings.postEqStage);
}

RetCode DynamicsProcessingSwContext::setMbcChannelCfgs(
        const std::vector<DynamicsProcessing::ChannelConfig>& cfgs) {
    return setChannelCfgs(cfgs, mMbcChCfgs, mEngineSettings.mbcStage);
}

RetCode DynamicsProcessingSwContext::setEqBandCfgs(
        const std::vector<DynamicsProcessing::EqBandConfig>& cfgs,
        std::vector<DynamicsProcessing::EqBandConfig>& targetCfgs,
        const DynamicsProcessing::StageEnablement& stage,
        const std::vector<DynamicsProcessing::ChannelConfig>& channelConfig) {
    RETURN_VALUE_IF(!stage.inUse, RetCode::ERROR_ILLEGAL_PARAMETER, "eqStageNotInUse");

    RetCode ret = RetCode::SUCCESS;
    std::set<std::pair<int /* channel */, int /* band */>> bandSet;

    for (auto& cfg : cfgs) {
        if (0 != bandSet.count({cfg.channel, cfg.band})) {
            LOG(WARNING) << __func__ << " duplicated band " << cfg.toString();
            ret = RetCode::ERROR_ILLEGAL_PARAMETER;
        } else {
            bandSet.insert({cfg.channel, cfg.band});
        }
        if (!validateEqBandConfig(cfg, mChannelCount, stage.bandCount, channelConfig)) {
            LOG(WARNING) << __func__ << " skip invalid band " << cfg.toString();
            ret = RetCode::ERROR_ILLEGAL_PARAMETER;
            continue;
        }
        targetCfgs[cfg.channel * stage.bandCount + cfg.band] = cfg;
    }
    return ret;
}

RetCode DynamicsProcessingSwContext::setPreEqBandCfgs(
        const std::vector<DynamicsProcessing::EqBandConfig>& cfgs) {
    return setEqBandCfgs(cfgs, mPreEqChBands, mEngineSettings.preEqStage, mPreEqChCfgs);
}

RetCode DynamicsProcessingSwContext::setPostEqBandCfgs(
        const std::vector<DynamicsProcessing::EqBandConfig>& cfgs) {
    return setEqBandCfgs(cfgs, mPostEqChBands, mEngineSettings.postEqStage, mPostEqChCfgs);
}

RetCode DynamicsProcessingSwContext::setMbcBandCfgs(
        const std::vector<DynamicsProcessing::MbcBandConfig>& cfgs) {
    RETURN_VALUE_IF(!mEngineSettings.mbcStage.inUse, RetCode::ERROR_ILLEGAL_PARAMETER,
                    "mbcNotInUse");

    RetCode ret = RetCode::SUCCESS;
    std::set<std::pair<int /* channel */, int /* band */>> bandSet;

    int bandCount = mEngineSettings.mbcStage.bandCount;
    std::vector<bool> filled(mChannelCount * bandCount, false);
    for (auto& it : cfgs) {
        if (0 != bandSet.count({it.channel, it.band})) {
            LOG(WARNING) << __func__ << " duplicated band " << it.toString();
            ret = RetCode::ERROR_ILLEGAL_PARAMETER;
        } else {
            bandSet.insert({it.channel, it.band});
        }
        if (!validateMbcBandConfig(it, mChannelCount, mEngineSettings.mbcStage.bandCount,
                                   mMbcChCfgs)) {
            LOG(WARNING) << __func__ << " skip invalid band " << it.toString();
            ret = RetCode::ERROR_ILLEGAL_PARAMETER;
            continue;
        }
        mMbcChBands[it.channel * bandCount + it.band] = it;
    }
    return ret;
}

RetCode DynamicsProcessingSwContext::setLimiterCfgs(
        const std::vector<DynamicsProcessing::LimiterConfig>& cfgs) {
    RETURN_VALUE_IF(!mEngineSettings.limiterInUse, RetCode::ERROR_ILLEGAL_PARAMETER,
                    "limiterNotInUse");

    RetCode ret = RetCode::SUCCESS;
    std::unordered_set<int> channelSet;

    for (auto& it : cfgs) {
        if (0 != channelSet.count(it.channel)) {
            LOG(WARNING) << __func__ << " duplicated channel " << it.channel;
            ret = RetCode::ERROR_ILLEGAL_PARAMETER;
        } else {
            channelSet.insert(it.channel);
        }
        if (!validateLimiterConfig(it, mChannelCount)) {
            LOG(WARNING) << __func__ << " skip invalid limiter " << it.toString();
            ret = RetCode::ERROR_ILLEGAL_PARAMETER;
            continue;
        }
        mLimiterCfgs[it.channel] = it;
    }
    return ret;
}

void DynamicsProcessingSwContext::resizeChannels() {
    if (mPreEqChCfgs.size() != mChannelCount) {
        mPreEqChCfgs.resize(mChannelCount, {.channel = kInvalidChannelId});
    }
    if (mPostEqChCfgs.size() != mChannelCount) {
        mPostEqChCfgs.resize(mChannelCount, {.channel = kInvalidChannelId});
    }
    if (mMbcChCfgs.size() != mChannelCount) {
        mMbcChCfgs.resize(mChannelCount, {.channel = kInvalidChannelId});
    }
    if (mLimiterCfgs.size() != mChannelCount) {
        mLimiterCfgs.resize(mChannelCount, {.channel = kInvalidChannelId});
    }
    if (mInputGainCfgs.size() != mChannelCount) {
        mInputGainCfgs.resize(mChannelCount, {.channel = kInvalidChannelId});
    }
}

void DynamicsProcessingSwContext::resizeBands() {
    if (mPreEqChBands.size() != (size_t)(mChannelCount * mEngineSettings.preEqStage.bandCount)) {
        mPreEqChBands.resize(mChannelCount * mEngineSettings.preEqStage.bandCount,
                             {.channel = kInvalidChannelId});
    }
    if (mPostEqChBands.size() != (size_t)(mChannelCount * mEngineSettings.postEqStage.bandCount)) {
        mPostEqChBands.resize(mChannelCount * mEngineSettings.postEqStage.bandCount,
                              {.channel = kInvalidChannelId});
    }
    if (mMbcChBands.size() != (size_t)(mChannelCount * mEngineSettings.mbcStage.bandCount)) {
        mMbcChBands.resize(mChannelCount * mEngineSettings.mbcStage.bandCount,
                           {.channel = kInvalidChannelId});
    }
}

RetCode DynamicsProcessingSwContext::setInputGainCfgs(
        const std::vector<DynamicsProcessing::InputGain>& cfgs) {
    for (const auto& cfg : cfgs) {
        RETURN_VALUE_IF(cfg.channel < 0 || (size_t)cfg.channel >= mChannelCount,
                        RetCode::ERROR_ILLEGAL_PARAMETER, "invalidChannel");
        mInputGainCfgs[cfg.channel] = cfg;
    }
    return RetCode::SUCCESS;
}

std::vector<DynamicsProcessing::InputGain> DynamicsProcessingSwContext::getInputGainCfgs() {
    std::vector<DynamicsProcessing::InputGain> ret;
    std::copy_if(mInputGainCfgs.begin(), mInputGainCfgs.end(), std::back_inserter(ret),
                 [&](const auto& gain) { return gain.channel != kInvalidChannelId; });
    return ret;
}

bool DynamicsProcessingSwContext::validateStageEnablement(
        const DynamicsProcessing::StageEnablement& enablement) {
    return !enablement.inUse || (enablement.inUse && enablement.bandCount > 0);
}

bool DynamicsProcessingSwContext::validateEngineConfig(
        const DynamicsProcessing::EngineArchitecture& engine) {
    return engine.preferredProcessingDurationMs >= 0 &&
           validateStageEnablement(engine.preEqStage) &&
           validateStageEnablement(engine.postEqStage) && validateStageEnablement(engine.mbcStage);
}

bool DynamicsProcessingSwContext::validateEqBandConfig(
        const DynamicsProcessing::EqBandConfig& band, int maxChannel, int maxBand,
        const std::vector<DynamicsProcessing::ChannelConfig>& channelConfig) {
    return band.channel >= 0 && band.channel < maxChannel &&
           (size_t)band.channel < channelConfig.size() && channelConfig[band.channel].enable &&
           band.band >= 0 && band.band < maxBand;
}

bool DynamicsProcessingSwContext::validateMbcBandConfig(
        const DynamicsProcessing::MbcBandConfig& band, int maxChannel, int maxBand,
        const std::vector<DynamicsProcessing::ChannelConfig>& channelConfig) {
    return band.channel >= 0 && band.channel < maxChannel &&
           (size_t)band.channel < channelConfig.size() && channelConfig[band.channel].enable &&
           band.band >= 0 && band.band < maxBand && band.attackTimeMs >= 0 &&
           band.releaseTimeMs >= 0 && band.ratio >= 0 && band.thresholdDb <= 0 &&
           band.kneeWidthDb <= 0 && band.noiseGateThresholdDb <= 0 && band.expanderRatio >= 0;
}

bool DynamicsProcessingSwContext::validateLimiterConfig(
        const DynamicsProcessing::LimiterConfig& limiter, int maxChannel) {
    return limiter.channel >= 0 && limiter.channel < maxChannel && limiter.attackTimeMs >= 0 &&
           limiter.releaseTimeMs >= 0 && limiter.ratio >= 0 && limiter.thresholdDb <= 0;
}

}  // namespace aidl::android::hardware::audio::effect
