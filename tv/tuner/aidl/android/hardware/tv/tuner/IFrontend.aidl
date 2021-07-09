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

import android.hardware.tv.tuner.FrontendScanType;
import android.hardware.tv.tuner.FrontendSettings;
import android.hardware.tv.tuner.FrontendStatus;
import android.hardware.tv.tuner.FrontendStatusType;
import android.hardware.tv.tuner.IFrontendCallback;

/**
 * A Tuner Frontend is used to tune to a frequency and lock signal.
 *
 * IFrontend provides a bit stream to the Tuner Demux interface.
 * @hide
 */
@VintfStability
interface IFrontend {
    /**
     * Set the frontend callback.
     *
     * IFrontendCallback is used by the client to receive events from the Frontend.
     * Only one callback per IFrontend instance is supported. The callback
     * will be replaced if it's set again.
     *
     * @param callback Callback object to pass Frontend events to the system.
     *        The previously registered callback must be replaced with this one.
     *        It can be null.
     */
    void setCallback(in IFrontendCallback callback);

    /**
     * Tunes the frontend to using the settings given.
     *
     * This locks the frontend to a frequency by providing signal
     * delivery information. If previous tuning isn't completed, this call MUST
     * stop previous tuning, and start a new tuning.
     * Tune is an async call, with LOCKED or NO_SIGNAL events sent via callback.
     *
     * @param settings Signal delivery information the frontend uses to
     * search and lock the signal.
     */
    void tune(in FrontendSettings settings);

    /**
     * Stops a previous tuning.
     *
     * If the method completes successfully the frontend is no longer tuned and no data
     * will be sent to attached demuxes.
     */
    void stopTune();

    /**
     * Releases the Frontend instance
     *
     * Associated resources are released.  close may be called more than once.
     * Calls to any other method after this will return an error
     */
    void close();

    /**
     * Scan the frontend to use the settings given.
     *
     * This uses the frontend to start a scan from signal delivery information.
     * If previous scan isn't completed, this call MUST stop previous scan,
     * and start a new scan.
     * Scan is an async call, with FrontendScanMessage sent via callback.
     *
     * @param settings Signal delivery information which the frontend uses to
     * scan the signal.
     * @param type the type which the frontend uses to scan the signal.
     */
    void scan(in FrontendSettings settings, in FrontendScanType type);

    /**
     * Stops a previous scanning.
     *
     * If the method completes successfully, the frontend stop previous
     * scanning.
     */
    void stopScan();

    /**
     * Gets the statuses of the frontend.
     *
     * This retrieve the statuses of the frontend for given status types.
     *
     * @param statusTypes an array of status type which the caller request.
     *
     * @return an array of statuses which response the caller's request.
     */
    FrontendStatus[] getStatus(in FrontendStatusType[] statusTypes);

    /**
     * Sets Low-Noise Block downconverter (LNB) for satellite frontend.
     *
     * This assigns a hardware LNB resource to the satellite frontend. It can be
     * called multiple times to update LNB assignment. The LNB resource must be
     * released when the frontend is closed.
     *
     * @param lnbId the Id of assigned LNB resource.
     */
    void setLnb(in int lnbId);

    /**
     * Enable or Disable Low Noise Amplifier (LNA).
     *
     * @param bEnable true if activate LNA module; false if deactivate LNA
     */
    void setLna(in boolean bEnable);

    /**
     * Link Conditional Access Modules (CAM) to Frontend support Common
     * Interface (CI) bypass mode.
     *
     * The client may use this to link CI-CAM to a frontend. CI bypass mode
     * requires that the CICAM also receives the TS concurrently from the
     * frontend when the Demux is receiving the TS directly from the frontend.
     *
     * @param ciCamId specify CI-CAM Id to link.
     *
     * @return the Local Transport Stream Id.
     */
    int linkCiCam(in int ciCamId);

    /**
     * Unlink Conditional Access Modules (CAM) to Frontend.
     *
     * @param ciCamId specify CI-CAM Id to unlink.
     */
    void unlinkCiCam(in int ciCamId);
}
