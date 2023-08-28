/*
 * Copyright 2022, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <aidl/android/hardware/automotive/can/Result.h>

#include <android-base/macros.h>
#include <utils/Mutex.h>

#include <atomic>
#include <mutex>

namespace aidl::android::hardware::automotive::can {

class CanBus {
  public:
    /**
     * Some interface types (such as SLCAN) don't get an interface name until after being
     * initialized, hence ifname is optional.
     *
     * You MUST ensure mIfname is initialized prior to the completion of preUp().
     */
    CanBus(std::string_view ifname = std::string_view{""});

    virtual ~CanBus();

    Result up();
    Result down();
    std::string getIfaceName();

  protected:
    /**
     * Prepare the SocketCAN interface.
     *
     * After calling this method, mIfname network interface is available and ready to be brought up.
     *
     * \return true upon success and false upon failure
     */
    virtual Result preUp();

    /**
     * Cleanup after bringing the interface down.
     *
     * This is a counterpart to preUp().
     *
     * \return true upon success and false upon failure
     */
    virtual bool postDown();

    /** Network interface name. */
    std::string mIfname;

  private:
    /**
     * Guard for up flag is required to be held for entire time when the interface is being used
     * because we don't want the interface to be torn down while executing that operation.
     */
    std::mutex mIsUpGuard;
    bool mIsUp GUARDED_BY(mIsUpGuard) = false;

    bool mDownAfterUse;
};

}  // namespace aidl::android::hardware::automotive::can
