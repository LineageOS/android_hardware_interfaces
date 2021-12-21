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

#include "CallbackManager.h"
#include "DriverContext.h"

#include <android/hardware/radio/1.6/IRadio.h>

namespace android::hardware::radio::compat {

class RadioCompatBase {
  protected:
    std::shared_ptr<DriverContext> mContext;

    sp<V1_5::IRadio> mHal1_5;
    sp<V1_6::IRadio> mHal1_6;

    std::shared_ptr<CallbackManager> mCallbackManager;

  public:
    RadioCompatBase(std::shared_ptr<DriverContext> context, sp<V1_5::IRadio> hidlHal,
                    std::shared_ptr<CallbackManager> cbMgr);
};

}  // namespace android::hardware::radio::compat
