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

#include <libradiocompat/RadioCompatBase.h>

#include <android-base/logging.h>

namespace android::hardware::radio::compat {

RadioCompatBase::RadioCompatBase(std::shared_ptr<DriverContext> context, sp<V1_5::IRadio> hidlHal,
                                 std::shared_ptr<CallbackManager> cbMgr)
    : mContext(context),
      mHal1_5(hidlHal),
      mHal1_6(V1_6::IRadio::castFrom(hidlHal)),
      mCallbackManager(cbMgr) {}

}  // namespace android::hardware::radio::compat
