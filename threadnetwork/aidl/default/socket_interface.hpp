/*
 * Copyright (C) 2024 The Android Open Source Project
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

/**
 * @file
 *   This file includes definitions for the Socket interface interface to radio
 * (RCP).
 */

#include "lib/spinel/spinel_interface.hpp"
#include "lib/url/url.hpp"

namespace aidl {
namespace android {
namespace hardware {
namespace threadnetwork {

/**
 * Defines a Socket interface to the Radio Co-processor (RCP)
 *
 */
class SocketInterface : public ot::Spinel::SpinelInterface {
  public:
    /**
     * Initializes the object.
     *
     * @param[in] aRadioUrl  RadioUrl parsed from radio url.
     *
     */
    explicit SocketInterface(const ot::Url::Url& aRadioUrl);

    /**
     * This destructor deinitializes the object.
     *
     */
    ~SocketInterface();

    /**
     * Returns the RCP interface metrics.
     *
     * @return The RCP interface metrics.
     *
     */
    const otRcpInterfaceMetrics* GetRcpInterfaceMetrics(void) const { return &mInterfaceMetrics; }

    /**
     * Indicates whether or not the given interface matches this interface name.
     *
     * @param[in] aInterfaceName A pointer to the interface name.
     *
     * @retval TRUE   The given interface name matches this interface name.
     * @retval FALSE  The given interface name doesn't match this interface
     * name.
     */
    static bool IsInterfaceNameMatch(const char* aInterfaceName) {
        static const char kInterfaceName[] = "spinel+socket";
        return (strncmp(aInterfaceName, kInterfaceName, strlen(kInterfaceName)) == 0);
    }

  private:
    ReceiveFrameCallback mReceiveFrameCallback;
    void* mReceiveFrameContext;
    RxFrameBuffer* mReceiveFrameBuffer;

    int mSockFd;
    const ot::Url::Url& mRadioUrl;

    otRcpInterfaceMetrics mInterfaceMetrics;

    // Non-copyable, intentionally not implemented.
    SocketInterface(const SocketInterface&);
    SocketInterface& operator=(const SocketInterface&);
};

}  // namespace threadnetwork
}  // namespace hardware
}  // namespace android
}  // namespace aidl
