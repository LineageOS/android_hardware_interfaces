/*
 * Copyright 2024 The Android Open Source Project
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

#include <complex>
#include <cstdint>
#include <vector>

#include "Eigen/Dense"
#include "aidl/android/hardware/bluetooth/ranging/ChannelSoudingRawData.h"

enum AlgoType : int {
  kZpIfft = 0,
};

class ChannelSoundingAlgorithm {
 public:
  class DataCleaning {
   public:
    DataCleaning();

    void Run(ChannelSoundingAlgorithm& cs_algo);

   private:
    void MultiplyPCT(ChannelSoundingAlgorithm& cs_algo);

    void SortPCT(ChannelSoundingAlgorithm& cs_algo);

    void FixDoppler(ChannelSoundingAlgorithm& cs_algo);

    void UpdateDeltaF(ChannelSoundingAlgorithm& cs_algo);

    void CalculateAutocorr(ChannelSoundingAlgorithm& cs_algo);

    void CalculateCovarianceMatrix(ChannelSoundingAlgorithm& cs_algo);

    void AddPctToBuffer(ChannelSoundingAlgorithm& cs_algo);

    void AddAutocorrToBuffer(ChannelSoundingAlgorithm& cs_algo);

    std::vector<uint8_t> SortPermutation(const std::vector<uint8_t>& vec);
  };

  class RangingAlgorithm {
   public:
    class Fft {
     public:
      bool Init(const size_t);

      void ComputeComplexIfft(const std::vector<std::complex<double>>& input,
                              std::vector<std::complex<double>>& output);

     private:
      int size_;
      int log2_size_;
    };

    RangingAlgorithm();

    void Run(ChannelSoundingAlgorithm& cs_algo);

    size_t fft_size_ = 4096;
    Fft fft_;
    double noise_est_interval_ = 20;  // in meters
    double threshold_zp_ifft_ = 20;   // in dB
    double shift_distance_ = 1.0;     // in meters

   private:
    void PreCombiningAddAutocorr(ChannelSoundingAlgorithm& cs_algo);

    double EstimateDistanceZpIfft(ChannelSoundingAlgorithm& cs_algo);

    double PostCombiningChooseMin(ChannelSoundingAlgorithm& cs_algo);

    std::vector<std::pair<int, double>> FindPeaksInDb(
        std::vector<double> input, bool& valid,
        const double threshold_dB = -100, size_t cir_shift_size = 0);
  };

  ChannelSoundingAlgorithm();

  void ResetVariables();
  double EstimateDistance(const ::aidl::android::hardware::bluetooth::ranging::
                              ChannelSoudingRawData& channel_souding_raw_data);
  double GetConfidenceLevel();

  void ParseRawData(const ::aidl::android::hardware::bluetooth::ranging::
                        ChannelSoudingRawData& channel_souding_raw_data);

  // Objects.
  DataCleaning dataCleaning;
  RangingAlgorithm rangingAlgorithm;

  // Variables.
  std::vector<uint8_t> step_channel_;
  std::vector<uint8_t> step_channel_cleaned_;
  int reference_power_level_initiator_ = 0;  // dBm
  int reference_power_level_reflector_ = 0;  // dBm
  size_t n_ap_ = 1;
  std::vector<std::vector<std::complex<double>>> pct_initiator_;
  std::vector<std::vector<std::complex<double>>> pct_reflector_;
  std::vector<std::vector<std::complex<double>>> pct_cleaned_;
  std::vector<std::complex<double>> pct_cleaned_combined_;
  std::vector<std::vector<std::complex<double>>> pct_autocorr_;
  std::vector<std::complex<double>> pct_autocorr_combined_;
  std::vector<Eigen::MatrixXcd> pct_covmat_;
  uint8_t delta_f_ = 1;  // minimum channel space (MHz)
  std::vector<double> raw_distance_collection_;
  std::vector<double> confidence_level_collection_;

  // Output.
  double raw_distance_ = 0.0;
  double delay_spread_ = 0.0;
  double confidence_level_ = 0.0;
  std::vector<std::pair<double, std::complex<double>>>
      channel_impulse_response_;  // (distance (m), coeff (complex))

  AlgoType algo_type_ = AlgoType::kZpIfft;
  bool use_pre_combining_ = false;
  bool use_post_combining_ = true;
  uint8_t selected_ap_ = 0;
  uint8_t autocorr_K_ = 48;
};