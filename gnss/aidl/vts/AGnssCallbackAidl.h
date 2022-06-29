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

#pragma once

#include <android/hardware/gnss/BnAGnssCallback.h>

using AGnssType = android::hardware::gnss::IAGnssCallback::AGnssType;
using AGnssStatusValue = android::hardware::gnss::IAGnssCallback::AGnssStatusValue;

/** Implementation for IAGnssCallback. */
class AGnssCallbackAidl : public android::hardware::gnss::BnAGnssCallback {
  public:
    AGnssCallbackAidl(){};
    ~AGnssCallbackAidl(){};
    android::binder::Status agnssStatusCb(const AGnssType type,
                                          const AGnssStatusValue status) override;
};
