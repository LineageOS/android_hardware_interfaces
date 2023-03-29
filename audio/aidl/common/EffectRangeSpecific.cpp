/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include <aidl/android/hardware/audio/effect/DynamicsProcessing.h>
#include <aidl/android/hardware/audio/effect/Range.h>

#include "EffectRangeSpecific.h"
#include "effect-impl/EffectRange.h"

namespace aidl::android::hardware::audio::effect {

namespace DynamicsProcessingRanges {

static bool isInputGainConfigInRange(const std::vector<DynamicsProcessing::InputGain>& cfgs,
                                     const DynamicsProcessing::InputGain& min,
                                     const DynamicsProcessing::InputGain& max) {
    auto func = [](const DynamicsProcessing::InputGain& arg) {
        return std::make_tuple(arg.channel, arg.gainDb);
    };
    return isTupleInRange(cfgs, min, max, func);
}

static bool isLimiterConfigInRange(const std::vector<DynamicsProcessing::LimiterConfig>& cfgs,
                                   const DynamicsProcessing::LimiterConfig& min,
                                   const DynamicsProcessing::LimiterConfig& max) {
    auto func = [](const DynamicsProcessing::LimiterConfig& arg) {
        return std::make_tuple(arg.channel, arg.enable, arg.linkGroup, arg.attackTimeMs,
                               arg.releaseTimeMs, arg.ratio, arg.thresholdDb, arg.postGainDb);
    };
    return isTupleInRange(cfgs, min, max, func);
}

static bool isMbcBandConfigInRange(const std::vector<DynamicsProcessing::MbcBandConfig>& cfgs,
                                   const DynamicsProcessing::MbcBandConfig& min,
                                   const DynamicsProcessing::MbcBandConfig& max) {
    auto func = [](const DynamicsProcessing::MbcBandConfig& arg) {
        return std::make_tuple(arg.channel, arg.band, arg.enable, arg.cutoffFrequencyHz,
                               arg.attackTimeMs, arg.releaseTimeMs, arg.ratio, arg.thresholdDb,
                               arg.kneeWidthDb, arg.noiseGateThresholdDb, arg.expanderRatio,
                               arg.preGainDb, arg.postGainDb);
    };
    return isTupleInRange(cfgs, min, max, func);
}

static bool isEqBandConfigInRange(const std::vector<DynamicsProcessing::EqBandConfig>& cfgs,
                                  const DynamicsProcessing::EqBandConfig& min,
                                  const DynamicsProcessing::EqBandConfig& max) {
    auto func = [](const DynamicsProcessing::EqBandConfig& arg) {
        return std::make_tuple(arg.channel, arg.band, arg.enable, arg.cutoffFrequencyHz,
                               arg.gainDb);
    };
    return isTupleInRange(cfgs, min, max, func);
}

static bool isChannelConfigInRange(const std::vector<DynamicsProcessing::ChannelConfig>& cfgs,
                                   const DynamicsProcessing::ChannelConfig& min,
                                   const DynamicsProcessing::ChannelConfig& max) {
    auto func = [](const DynamicsProcessing::ChannelConfig& arg) {
        return std::make_tuple(arg.channel, arg.enable);
    };
    return isTupleInRange(cfgs, min, max, func);
}

static bool isEngineConfigInRange(const DynamicsProcessing::EngineArchitecture& cfg,
                                  const DynamicsProcessing::EngineArchitecture& min,
                                  const DynamicsProcessing::EngineArchitecture& max) {
    auto func = [](const DynamicsProcessing::EngineArchitecture& arg) {
        return std::make_tuple(arg.resolutionPreference, arg.preferredProcessingDurationMs,
                               arg.preEqStage.inUse, arg.preEqStage.bandCount,
                               arg.postEqStage.inUse, arg.postEqStage.bandCount, arg.mbcStage.inUse,
                               arg.mbcStage.bandCount, arg.limiterInUse);
    };
    return isTupleInRange(func(cfg), func(min), func(max));
}

static int locateMinMaxForTag(DynamicsProcessing::Tag tag,
                              const std::vector<Range::DynamicsProcessingRange>& ranges) {
    for (int i = 0; i < (int)ranges.size(); i++) {
        if (tag == ranges[i].min.getTag() && tag == ranges[i].max.getTag()) {
            return i;
        }
    }
    return -1;
}

bool isParamInRange(const DynamicsProcessing& dp,
                    const std::vector<Range::DynamicsProcessingRange>& ranges) {
    auto tag = dp.getTag();
    int i = locateMinMaxForTag(tag, ranges);
    if (i == -1) return true;

    switch (tag) {
        case DynamicsProcessing::engineArchitecture: {
            return isEngineConfigInRange(
                    dp.get<DynamicsProcessing::engineArchitecture>(),
                    ranges[i].min.get<DynamicsProcessing::engineArchitecture>(),
                    ranges[i].max.get<DynamicsProcessing::engineArchitecture>());
        }
        case DynamicsProcessing::preEq: {
            return isChannelConfigInRange(dp.get<DynamicsProcessing::preEq>(),
                                          ranges[i].min.get<DynamicsProcessing::preEq>()[0],
                                          ranges[i].max.get<DynamicsProcessing::preEq>()[0]);
        }
        case DynamicsProcessing::postEq: {
            return isChannelConfigInRange(dp.get<DynamicsProcessing::postEq>(),
                                          ranges[i].min.get<DynamicsProcessing::postEq>()[0],
                                          ranges[i].max.get<DynamicsProcessing::postEq>()[0]);
        }
        case DynamicsProcessing::mbc: {
            return isChannelConfigInRange(dp.get<DynamicsProcessing::mbc>(),
                                          ranges[i].min.get<DynamicsProcessing::mbc>()[0],
                                          ranges[i].max.get<DynamicsProcessing::mbc>()[0]);
        }
        case DynamicsProcessing::preEqBand: {
            return isEqBandConfigInRange(dp.get<DynamicsProcessing::preEqBand>(),
                                         ranges[i].min.get<DynamicsProcessing::preEqBand>()[0],
                                         ranges[i].max.get<DynamicsProcessing::preEqBand>()[0]);
        }
        case DynamicsProcessing::postEqBand: {
            return isEqBandConfigInRange(dp.get<DynamicsProcessing::postEqBand>(),
                                         ranges[i].min.get<DynamicsProcessing::postEqBand>()[0],
                                         ranges[i].max.get<DynamicsProcessing::postEqBand>()[0]);
        }
        case DynamicsProcessing::mbcBand: {
            return isMbcBandConfigInRange(dp.get<DynamicsProcessing::mbcBand>(),
                                          ranges[i].min.get<DynamicsProcessing::mbcBand>()[0],
                                          ranges[i].max.get<DynamicsProcessing::mbcBand>()[0]);
        }
        case DynamicsProcessing::limiter: {
            return isLimiterConfigInRange(dp.get<DynamicsProcessing::limiter>(),
                                          ranges[i].min.get<DynamicsProcessing::limiter>()[0],
                                          ranges[i].max.get<DynamicsProcessing::limiter>()[0]);
        }
        case DynamicsProcessing::inputGain: {
            return isInputGainConfigInRange(dp.get<DynamicsProcessing::inputGain>(),
                                            ranges[i].min.get<DynamicsProcessing::inputGain>()[0],
                                            ranges[i].max.get<DynamicsProcessing::inputGain>()[0]);
        }
        default: {
            return true;
        }
    }
    return true;
}

}  // namespace DynamicsProcessingRanges

}  // namespace aidl::android::hardware::audio::effect