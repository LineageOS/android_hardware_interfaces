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

package android.hardware.cas;

import android.hardware.cas.AidlCasPluginDescriptor;
import android.hardware.cas.ICas;
import android.hardware.cas.ICasListener;
import android.hardware.cas.IDescrambler;

/**
 * IMediaCasService is the main entry point for interacting with a vendor's
 * cas HAL to create cas and descrambler plugin instances. A cas plugin instance
 * opens cas sessions which are used to obtain keys for a descrambler session,
 * which can in turn be used to descramble protected video content.
 * @hide
 */
@VintfStability
interface IMediaCasService {
    /**
     * Construct a new instance of a DescramblerPlugin given a CA_system_id.
     *
     * @param CA_system_id the id of the CA system.
     * @return the newly created plugin interface.
     */
    IDescrambler createDescrambler(in int CA_system_id);

    /**
     * Construct a new instance of a CasPlugin given a CA_system_id.
     *
     * @param CA_system_id the id of the CA system.
     * @param listener the event listener to receive events coming from the CasPlugin.
     * @return the newly created CasPlugin interface.
     */
    ICas createPlugin(in int CA_system_id, in ICasListener listener);

    /**
     * List all available CA systems on the device.
     *
     * @return an array of descriptors for the available CA systems.
     */
    AidlCasPluginDescriptor[] enumeratePlugins();

    /**
     * Query if the descrambling scheme for a CA system is supported on this device.
     *
     * @param CA_system_id the id of the CA system.
     * @return whether the specified descrambling scheme is supported on this device.
     */
    boolean isDescramblerSupported(in int CA_system_id);

    /**
     * Query if a certain CA system is supported on this device.
     *
     * @param CA_system_id the id of the CA system.
     * @return whether the specified CA system is supported on this device.
     */
    boolean isSystemIdSupported(in int CA_system_id);
}
