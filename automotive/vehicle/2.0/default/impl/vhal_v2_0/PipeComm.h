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

#ifndef android_hardware_automotive_vehicle_V2_0_impl_PipeComm_H_
#define android_hardware_automotive_vehicle_V2_0_impl_PipeComm_H_

#include <mutex>
#include <vector>
#include "CommConn.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

/**
 * PipeComm opens a qemu pipe to connect to the emulator, allowing the emulator UI to access the
 * Vehicle HAL and simulate changing properties.
 *
 * Since the pipe is a client, it directly implements CommConn, and only one PipeComm can be open
 * at a time.
 */
class PipeComm : public CommConn {
   public:
    PipeComm(MessageProcessor* messageProcessor);

    void start() override;
    void stop() override;

    std::vector<uint8_t> read() override;
    int write(const std::vector<uint8_t>& data) override;

    inline bool isOpen() override { return mPipeFd > 0; }

   private:
    int mPipeFd;
};

}  // impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android


#endif  // android_hardware_automotive_vehicle_V2_0_impl_PipeComm_H_
