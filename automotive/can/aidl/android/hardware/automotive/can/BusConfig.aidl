/*
 * Copyright (C) 2022 The Android Open Source Project
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
package android.hardware.automotive.can;

import android.hardware.automotive.can.IndexedInterface;
import android.hardware.automotive.can.NativeInterface;
import android.hardware.automotive.can.SlcanInterface;
import android.hardware.automotive.can.VirtualInterface;

/**
 * Configuration of the (physical or virtual) CAN bus.
 *
 * ISO TP and CAN FD support is dependent upon the hardware.
 */
@VintfStability
parcelable BusConfig {
    /**
     * Name by which a given bus may be referenced.
     *
     * It must consist of only alphanumeric characters and underscore
     * (a-z, A-Z, 0-9, '_'), at least 1 and at most 32 characters long.
     *
     * This field is *not* meant to distinguish between hardware interfaces
     * nor preselect parameters like bitrate.
     *
     * This field represents a more human-friendly name for a CAN bus:
     * e.x. rather than /some/dev/can1234, "name" might be "BodyCAN" or "CCAN"
     */
    String name;

    /**
     * Hardware interface configuration.
     *
     * This union's discriminator has an equivalent enum {@see InterfaceType} to
     * express compatibility via getSupportedInterfaceTypes().
     */
    union InterfaceId {
        /** Virtual SocketCAN interface. */
        VirtualInterface virtualif;

        /** Native SocketCAN interface. */
        NativeInterface nativeif;

        /** Serial line CAN interface. */
        SlcanInterface slcan;

        /**
         * Proprietary, device-specific interface.
         *
         * Non-SocketCAN interfaces should use this variant.
         */
        IndexedInterface indexed;
    }

    InterfaceId interfaceId;

    /**
     * Bit rate for CAN communication.
     *
     * Typical bit rates are: 100000, 125000, 250000, 500000.
     *
     * For {@see interfaceId#virtual} interfaces, this value is ignored.
     */
    int bitrate;
}
