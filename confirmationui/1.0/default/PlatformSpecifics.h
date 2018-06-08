/*
**
** Copyright 2017, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef CONFIRMATIONUI_1_0_DEFAULT_PLATFORMSPECIFICS_H_
#define CONFIRMATIONUI_1_0_DEFAULT_PLATFORMSPECIFICS_H_

#include <stdint.h>
#include <time.h>

#include <android/hardware/confirmationui/1.0/IConfirmationResultCallback.h>
#include <android/hardware/confirmationui/1.0/generic/GenericOperation.h>
#include <android/hardware/confirmationui/support/confirmationui_utils.h>

namespace android {
namespace hardware {
namespace confirmationui {
namespace V1_0 {
namespace implementation {

struct MonotonicClockTimeStamper {
    class TimeStamp {
       public:
        explicit TimeStamp(uint64_t ts) : timestamp_(ts), ok_(true) {}
        TimeStamp() : timestamp_(0), ok_(false) {}
        bool isOk() const { return ok_; }
        operator const uint64_t() const { return timestamp_; }

       private:
        uint64_t timestamp_;
        bool ok_;
    };
    static TimeStamp now();
};

class HMacImplementation {
   public:
    static support::NullOr<support::hmac_t> hmac256(
        const support::auth_token_key_t& key,
        std::initializer_list<support::ByteBufferProxy> buffers);
};

class MyOperation : public generic::Operation<sp<IConfirmationResultCallback>,
                                              MonotonicClockTimeStamper, HMacImplementation> {
   public:
    static MyOperation& get() {
        static MyOperation op;
        return op;
    }
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace confirmationui
}  // namespace hardware
}  // namespace android

#endif  // CONFIRMATIONUI_1_0_DEFAULT_PLATFORMSPECIFICS_H_
