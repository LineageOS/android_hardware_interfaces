/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <iostream>
#include <string>

#include <android-base/logging.h>
#include <android/hardware/light/2.0/ILight.h>

void error(const std::string& msg) {
    LOG(ERROR) << msg;
    std::cerr << msg << std::endl;
}

int main() {
    using ::android::hardware::hidl_vec;
    using ::android::hardware::light::V2_0::Brightness;
    using ::android::hardware::light::V2_0::Flash;
    using ::android::hardware::light::V2_0::ILight;
    using ::android::hardware::light::V2_0::LightState;
    using ::android::hardware::light::V2_0::Status;
    using ::android::hardware::light::V2_0::Type;
    using ::android::sp;

    sp<ILight> service = ILight::getService();
    if (service == nullptr) {
        error("Could not retrieve light service.");
        return -1;
    }

    const static LightState off = {
        .color = 0u, .flashMode = Flash::NONE, .brightnessMode = Brightness::USER,
    };

    service->getSupportedTypes([&](const hidl_vec<Type>& types) {
        for (Type type : types) {
            Status ret = service->setLight(type, off);
            if (ret != Status::SUCCESS) {
                error("Failed to shut off screen for type " +
                      std::to_string(static_cast<int>(type)));
            }
        }
    });

    // b/77934844: Avoid running static destructors.
    _exit(1);
}
