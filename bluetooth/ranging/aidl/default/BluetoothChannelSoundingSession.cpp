/*
 * Copyright (C) 2023 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "BluetoothChannelSoundingSession.h"

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <cutils/trace.h>

#include "bluetooth_channel_sounding_algorithm.h"

namespace aidl::android::hardware::bluetooth::ranging::impl {
// A table that maps the maximum valid permutation index based on
// num_antenna_paths. The total number of permutations for N items is N! (index
// start from 0).
static constexpr uint8_t kMaxValidPermutationIndexTable[4] = {0, 1, 5, 23};
// Antenna path permutations. See Channel Sounding CR_PR for the details.
static constexpr uint8_t kCsAntennaPermutationArray[24][4] = {
    {1, 2, 3, 4}, {2, 1, 3, 4}, {1, 3, 2, 4}, {3, 1, 2, 4}, {3, 2, 1, 4},
    {2, 3, 1, 4}, {1, 2, 4, 3}, {2, 1, 4, 3}, {1, 4, 2, 3}, {4, 1, 2, 3},
    {4, 2, 1, 3}, {2, 4, 1, 3}, {1, 4, 3, 2}, {4, 1, 3, 2}, {1, 3, 4, 2},
    {3, 1, 4, 2}, {3, 4, 1, 2}, {4, 3, 1, 2}, {4, 2, 3, 1}, {2, 4, 3, 1},
    {4, 3, 2, 1}, {3, 4, 2, 1}, {3, 2, 4, 1}, {2, 3, 4, 1}};
static std::unique_ptr<ChannelSoundingAlgorithm> channel_sounding_algorithm =
    nullptr;

BluetoothChannelSoundingSession::BluetoothChannelSoundingSession(
    std::shared_ptr<IBluetoothChannelSoundingSessionCallback> callback,
    Reason reason) {
  callback_ = callback;
  if (channel_sounding_algorithm == nullptr) {
    channel_sounding_algorithm = std::make_unique<ChannelSoundingAlgorithm>();
  }
  callback_->onOpened(reason);
}

ndk::ScopedAStatus BluetoothChannelSoundingSession::getVendorSpecificReplies(
    std::optional<
        std::vector<std::optional<VendorSpecificData>>>* /*_aidl_return*/) {
  return ::ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus BluetoothChannelSoundingSession::getSupportedResultTypes(
    std::vector<ResultType>* _aidl_return) {
  std::vector<ResultType> supported_result_types = {ResultType::RESULT_METERS};
  *_aidl_return = supported_result_types;
  return ::ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus BluetoothChannelSoundingSession::isAbortedProcedureRequired(
    bool* _aidl_return) {
  *_aidl_return = false;
  return ::ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus BluetoothChannelSoundingSession::writeRawData(
    const ChannelSoudingRawData& in_rawData) {
  if (in_rawData.stepChannels.empty()) {
    LOG(WARNING) << __func__ << " in_rawData.stepChannels is empty, skip";
    return ::ndk::ScopedAStatus::ok();
  }

  RangingResult ranging_result;
  channel_sounding_algorithm->ResetVariables();
  ATRACE_BEGIN("CS EstimateDistance");
  ranging_result.resultMeters =
      channel_sounding_algorithm->EstimateDistance(in_rawData);
  ATRACE_END();
  ranging_result.confidenceLevel =
      channel_sounding_algorithm->GetConfidenceLevel() * 100;

  LOG(DEBUG) << "result: " << ranging_result.resultMeters;
  callback_->onResult(ranging_result);
  return ::ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus BluetoothChannelSoundingSession::close(Reason in_reason) {
  callback_->onClose(in_reason);
  return ::ndk::ScopedAStatus::ok();
}

template <int BITS>
static int16_t convert_to_signed(uint16_t num) {
  unsigned msb_mask = 1 << (BITS - 1);  // setup a mask for most significant bit
  int16_t num_signed = num;
  if ((num_signed & msb_mask) != 0) {
    num_signed |= ~(msb_mask - 1);  // extend the MSB
  }
  return num_signed;
}

static double get_iq_value(uint16_t sample) {
  int16_t signed_sample = convert_to_signed<12>(sample);
  double value = 1.0 * signed_sample / 2048;
  return value;
}

static ComplexNumber get_complex_number(const PctIQSample& pct_iq_sample) {
  ComplexNumber cn;
  cn.real = get_iq_value(pct_iq_sample.iSample);
  cn.imaginary = get_iq_value(pct_iq_sample.qSample);
  return cn;
}

static bool is_valid_antenna_permutation_data(uint8_t permutation_index,
                                              uint8_t num_antenna_paths) {
  if (num_antenna_paths < 1 || num_antenna_paths > 4) {
    return false;
  }
  uint8_t max_valid_permutation_index =
      kMaxValidPermutationIndexTable[num_antenna_paths - 1];
  return permutation_index <= max_valid_permutation_index;
}

/**
 * Populates the relevant vectors in ChannelSoundingSingleSideData from
 * ModeOneData.
 */
static void populate_mode_one_data(
    const ModeOneData& data, ChannelSoundingSingleSideData& single_side_data,
    std::vector<int32_t>& toa_tod_values) {
  single_side_data.packetQuality->push_back(data.packetQuality);
  single_side_data.packetRssiDbm->push_back(data.packetRssiDbm);
  single_side_data.packetNadm->push_back(data.packetNadm);
  if (data.packetPct1.has_value()) {
    single_side_data.packetPct1->push_back(
        get_complex_number(*data.packetPct1));
  }
  if (data.packetPct2.has_value()) {
    single_side_data.packetPct2->push_back(
        get_complex_number(*data.packetPct2));
  }

  if (data.rttToaTodData.getTag() == RttToaTodData::Tag::toaTodInitiator) {
    toa_tod_values.push_back(
        data.rttToaTodData.get<RttToaTodData::Tag::toaTodInitiator>());
  } else {
    toa_tod_values.push_back(
        data.rttToaTodData.get<RttToaTodData::Tag::todToaReflector>());
  }
}

static void populate_mode_two_data(
    const ModeTwoData& data, int8_t num_antenna_paths,
    ChannelSoundingSingleSideData& single_side_data) {
  uint8_t permutation_index = data.antennaPermutationIndex;
  if (!is_valid_antenna_permutation_data(permutation_index,
                                         num_antenna_paths)) {
    LOG(WARNING) << __func__ << " Invalid antenna permutation data (index: "
                 << (int)permutation_index
                 << ", paths: " << (int)num_antenna_paths << ")";
    return;
  }

  uint16_t num_tone_data = data.tonePctIQSamples.size();
  for (uint16_t k = 0; k < num_tone_data; k++) {
    // The last tone is the extension tone and is not part of the permutation.
    uint8_t antenna_path_idx =
        (k == num_antenna_paths)
            ? num_antenna_paths
            : kCsAntennaPermutationArray[permutation_index][k] -
                  1;  // -1 for 0-based index

    if (antenna_path_idx > num_antenna_paths) {
      LOG(ERROR) << __func__ << " Calculated antenna path "
                 << (int)antenna_path_idx << " is out of bounds for "
                 << (int)num_antenna_paths << " antenna paths.";
      continue;
    }

    // Get the correct StepTonePct object for this antenna path
    auto& target_step_tone_pct =
        single_side_data.stepTonePcts->at(antenna_path_idx).value();

    // Append the data for this step to the correct antenna's vectors.
    target_step_tone_pct.tonePcts.push_back(
        get_complex_number(data.tonePctIQSamples[k]));
    if (k < data.toneQualityIndicators.size()) {
      target_step_tone_pct.toneQualityIndicator.push_back(
          data.toneQualityIndicators[k]);
    }
  }
}

static void populate_single_side_data(
    const std::vector<SubeventResultData>& subevent_results, bool is_initiator,
    ChannelSoundingSingleSideData& single_side_data,
    std::vector<uint8_t>& all_step_channels,
    std::vector<int32_t>& toa_tod_values) {
  // Initialize optional vectors
  single_side_data.packetQuality.emplace();
  single_side_data.packetRssiDbm.emplace();
  single_side_data.packetNadm.emplace();
  single_side_data.measuredFreqOffset.emplace();
  single_side_data.packetPct1.emplace();
  single_side_data.packetPct2.emplace();
  single_side_data.stepTonePcts.emplace();

  for (const auto& subevent : subevent_results) {
    single_side_data.referencePowerDbm = subevent.referencePowerLevelDbm;
    uint8_t num_antenna_paths = subevent.numAntennaPaths;
    // Pre-initialize the per-antenna path structure for stepTonePcts
    single_side_data.stepTonePcts.emplace();
    if (num_antenna_paths > 0) {
      single_side_data.stepTonePcts->resize(num_antenna_paths +
                                            1);  // one more extent pct
      for (int i = 0; i <= num_antenna_paths; ++i) {
        single_side_data.stepTonePcts->at(i)
            .emplace();  // Create the StepTonePct objects
      }
    }
    for (const auto& step : subevent.stepData) {
      switch (step.stepMode) {
        case ModeType::ZERO: {
          const auto& data =
              step.stepModeData.get<ModeData::Tag::modeZeroData>();
          single_side_data.packetQuality->push_back(data.packetQuality);
          single_side_data.packetRssiDbm->push_back(data.packetRssiDbm);

          if (is_initiator) {
            single_side_data.measuredFreqOffset->push_back(
                data.initiatorMeasuredFreqOffset);
          }
          break;
        }
        case ModeType::ONE: {
          const auto& data =
              step.stepModeData.get<ModeData::Tag::modeOneData>();
          populate_mode_one_data(data, single_side_data, toa_tod_values);
          if (is_initiator) {
            all_step_channels.push_back(step.stepChannel);
          }
          break;
        }
        case ModeType::TWO: {
          const auto& data =
              step.stepModeData.get<ModeData::Tag::modeTwoData>();
          populate_mode_two_data(data, num_antenna_paths, single_side_data);

          if (is_initiator) {
            all_step_channels.push_back(step.stepChannel);
          }
          break;
        }
        case ModeType::THREE: {
          const auto& data =
              step.stepModeData.get<ModeData::Tag::modeThreeData>();
          // ModeThree is a combination of ModeOne and ModeTwo.
          populate_mode_one_data(data.modeOneData, single_side_data,
                                 toa_tod_values);

          populate_mode_two_data(data.modeTwoData, num_antenna_paths,
                                 single_side_data);

          if (is_initiator) {
            all_step_channels.push_back(step.stepChannel);
          }
          break;
        }
      }
    }
  }
}

static ChannelSoudingRawData convert_procedure_data_to_Raw(
    const ChannelSoundingProcedureData& procedure_data) {
  ChannelSoudingRawData raw_data;

  // Direct and Simple Mappings
  raw_data.procedureCounter = procedure_data.procedureCounter;

  // The old 'aborted' flag is true if either side aborted for any reason.
  raw_data.aborted = (procedure_data.initiatorProcedureAbortReason !=
                      ProcedureAbortReason::SUCCESS) ||
                     (procedure_data.reflectorProcedureAbortReason !=
                      ProcedureAbortReason::SUCCESS);

  if (!procedure_data.initiatorSubeventResultData.empty()) {
    raw_data.timestampMs =
        procedure_data.initiatorSubeventResultData[0].timestampNanos / 1000000;
    raw_data.numAntennaPaths =
        procedure_data.initiatorSubeventResultData[0].numAntennaPaths;
  }

  // Complex Mappings using Helper
  std::vector<int32_t> initiator_toa_tod_values;
  std::vector<int32_t> reflector_toa_tod_values;

  // Process initiator data
  populate_single_side_data(procedure_data.initiatorSubeventResultData,
                            /*is_initiator=*/true, raw_data.initiatorData,
                            raw_data.stepChannels, initiator_toa_tod_values);

  // Process reflector data
  populate_single_side_data(procedure_data.reflectorSubeventResultData,
                            /*is_initiator=*/false, raw_data.reflectorData,
                            raw_data.stepChannels, reflector_toa_tod_values);

  if (!initiator_toa_tod_values.empty()) {
    raw_data.toaTodInitiator.emplace(initiator_toa_tod_values);
  }
  if (!reflector_toa_tod_values.empty()) {
    raw_data.todToaReflector.emplace(reflector_toa_tod_values);
  }

  return raw_data;
}

ndk::ScopedAStatus BluetoothChannelSoundingSession::writeProcedureData(
    const ChannelSoundingProcedureData& procedure_data) {
  return writeRawData(convert_procedure_data_to_Raw(procedure_data));
}

ndk::ScopedAStatus BluetoothChannelSoundingSession::updateChannelSoundingConfig(
    const Config& /*in_config*/) {
  return ::ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus BluetoothChannelSoundingSession::updateProcedureEnableConfig(
    const ProcedureEnableConfig& /*in_procedureEnableConfig*/) {
  return ::ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus BluetoothChannelSoundingSession::updateBleConnInterval(
    int /*in_bleConnInterval*/) {
  return ::ndk::ScopedAStatus::ok();
}
}  // namespace aidl::android::hardware::bluetooth::ranging::impl
