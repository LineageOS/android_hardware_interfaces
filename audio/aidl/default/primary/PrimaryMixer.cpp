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

#define LOG_TAG "AHAL_PrimaryMixer"

#include <android-base/properties.h>

#include "PrimaryMixer.h"

using android::base::GetIntProperty;

namespace aidl::android::hardware::audio::core::primary {

namespace {

static constexpr int kAlsaCard = 0;
static constexpr int kAlsaDevice = 0;
static constexpr char kAlsaCardProp[] = "persist.vendor.audio.primary.alsa_card";
static constexpr char kAlsaDeviceProp[] = "persist.vendor.audio.primary.alsa_device";

}  // namespace

PrimaryMixer::PrimaryMixer() : alsa::Mixer(getAlsaCard()) {}

// static
PrimaryMixer& PrimaryMixer::getInstance() {
    static PrimaryMixer gInstance;
    return gInstance;
}

int PrimaryMixer::getAlsaCard() {
    if (AlsaCard == -1) AlsaCard = GetIntProperty(kAlsaCardProp, kAlsaCard);
    return AlsaCard;
}

int PrimaryMixer::getAlsaDevice() {
    if (AlsaDevice == -1) AlsaDevice = GetIntProperty(kAlsaDeviceProp, kAlsaDevice);
    return AlsaDevice;
}

int PrimaryMixer::AlsaCard = -1;
int PrimaryMixer::AlsaDevice = -1;

}  // namespace aidl::android::hardware::audio::core::primary
