/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include <libradiocompat/RadioIndication.h>

// TODO(b/203699028): remove when fully implemented
#pragma GCC diagnostic ignored "-Wunused-parameter"

namespace android::hardware::radio::compat {

Return<void> RadioIndication::radioStateChanged(V1_0::RadioIndicationType type,
                                                V1_0::RadioState radioState) {
    return {};
}

Return<void> RadioIndication::rilConnected(V1_0::RadioIndicationType type) {
    return {};
}

Return<void> RadioIndication::hardwareConfigChanged(V1_0::RadioIndicationType type,
                                                    const hidl_vec<V1_0::HardwareConfig>& configs) {
    return {};
}

Return<void> RadioIndication::radioCapabilityIndication(V1_0::RadioIndicationType type,
                                                        const V1_0::RadioCapability& rc) {
    return {};
}

Return<void> RadioIndication::modemReset(V1_0::RadioIndicationType type,
                                         const hidl_string& reason) {
    return {};
}

}  // namespace android::hardware::radio::compat
