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

import android.hardware.automotive.can.BusConfig;
import android.hardware.automotive.can.InterfaceType;
import android.hardware.automotive.can.Result;

/**
 * Represents a CAN controller that's capable of configuring CAN bus interfaces.
 *
 * The goal of this service is to configure and manage CAN interfaces.
 *
 * Providing an ICanController interface to configure CAN buses is optional.
 * A system can elect to configure CAN buses manually if the hardware is
 * dedicated to a specific application.
 */
@VintfStability
interface ICanController {
    /**
     * Fetches the list of interface types supported by this HAL server.
     *
     * @return iftypes The list of supported interface types.
     */
    InterfaceType[] getSupportedInterfaceTypes();

    /**
     * Gets the interface name given the name of the bus. This will
     *
     * @param busName Name of the CAN bus who's interface name we would like
     * (e.x. BCAN, CCAN, HS3, BodyCAN, ...)
     * @return name of the socketcan network interface corresponding to busName
     * (e.x. can0, vcan5, ...)
     */
    String getInterfaceName(in String busName);

    /**
     * Bring up a CAN bus.
     *
     * @param config Configuration for the CAN bus.
     * @return name of iface if successful
     */
    String upBus(in BusConfig config);

    /**
     * Bring down a CAN bus.
     *
     * @param name Name of the bus (@see BusConfig#name} to bring down.
     */
    void downBus(in String name);
}
