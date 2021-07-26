/*
 * Copyright 2021 The Android Open Source Project
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

package android.hardware.tv.tuner;

import android.hardware.tv.tuner.DemuxCapabilities;
import android.hardware.tv.tuner.FrontendInfo;
import android.hardware.tv.tuner.IDemux;
import android.hardware.tv.tuner.IDescrambler;
import android.hardware.tv.tuner.IFrontend;
import android.hardware.tv.tuner.ILnb;
import android.hardware.tv.tuner.FrontendDtmbCapabilities;

/**
 * Top level interface to manage Frontend, Demux and Decrambler hardware
 * resources which are needed for Android TV.
 * @hide
 */
@VintfStability
@SuppressWarnings(value={"out-array"})
interface ITuner {
    /**
     * Get Frontend IDs
     *
     * It is used by the client to get all available frontends' IDs.
     *
     * @return an array of IDs for the available Frontends.
     */
    int[] getFrontendIds();

    /**
     * Create a new instance of Frontend given a frontendId.
     *
     * It is used by the client to create a frontend instance.
     *
     * @param frontendId the id of the frontend to be opened.
     *
     * @return the newly created frontend interface.
     */
    IFrontend openFrontendById(in int frontendId);

    /**
     * Create a new instance of Demux.
     *
     * It is used by the client to create a Demux instance.
     *
     * @param out demuxId the newly created demux id will be the first
     *        element of the array.
     *
     * @return the newly created demux interface.
     */
    IDemux openDemux(out int[] demuxId);

    /**
     * Retrieve the Demux's Capabilities.
     *
     * @return the Demux's Capabilities.
     */
    DemuxCapabilities getDemuxCaps();

    /**
     * Create a new instance of Descrambler.
     *
     * It is used by the client to create a Descrambler instance.
     *
     * @return the newly created descrambler interface.
     */
    IDescrambler openDescrambler();

    /**
     * Retrieve the frontend's information.
     *
     * @return the frontend's information.
     */
    FrontendInfo getFrontendInfo(in int frontendId);

    /**
     * Get low-noise block downconverter (LNB) IDs.
     *
     * It is used by the client to get all available LNBs' IDs.
     *
     * @return an array of LnbId for the available LNBs.
     */
    int[] getLnbIds();

    /**
     * Create a new instance of Lnb given a lnbId.
     *
     * It is used by the client to create a Lnb instance for satellite Frontend.
     *
     * @param lnbId the id of the LNB to be opened.
     *
     * @return the newly created Lnb interface.
     */
    ILnb openLnbById(in int lnbId);

    /**
     * Create a new instance of Lnb given a LNB name.
     *
     * It is used by the client to create a LNB instance for external device.
     *
     * @param lnbName the name for an external LNB to be opened. The app
     *        provides the name. Frammework doesn't depend on the name, instead
     *        use lnbId return from this call.
     * @param out demuxId the newly opened Lnb id will be the first element of
     *        the array.
     *
     * @return the newly opened Lnb iterface.
     */
    ILnb openLnbByName(in String lnbName, out int[] lnbId);
}
