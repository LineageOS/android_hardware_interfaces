/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include "HidlUtils.h"

namespace android {
namespace hardware {
namespace audio {
namespace common {
namespace CPP_VERSION {
namespace implementation {

status_t HidlUtils::audioPortConfigsFromHal(unsigned int numHalConfigs,
                                            const struct audio_port_config* halConfigs,
                                            hidl_vec<AudioPortConfig>* configs) {
    status_t result = NO_ERROR;
    configs->resize(numHalConfigs);
    for (unsigned int i = 0; i < numHalConfigs; ++i) {
        if (status_t status = audioPortConfigFromHal(halConfigs[i], &(*configs)[i]);
            status != NO_ERROR) {
            result = status;
        }
    }
    return result;
}

status_t HidlUtils::audioPortConfigsToHal(const hidl_vec<AudioPortConfig>& configs,
                                          std::unique_ptr<audio_port_config[]>* halConfigs) {
    status_t result = NO_ERROR;
    halConfigs->reset(new audio_port_config[configs.size()]);
    for (size_t i = 0; i < configs.size(); ++i) {
        if (status_t status = audioPortConfigToHal(configs[i], &(*halConfigs)[i]);
            status != NO_ERROR) {
            result = status;
        }
    }
    return result;
}

}  // namespace implementation
}  // namespace CPP_VERSION
}  // namespace common
}  // namespace audio
}  // namespace hardware
}  // namespace android
