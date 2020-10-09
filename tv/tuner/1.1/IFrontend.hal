/*
 * Copyright 2020 The Android Open Source Project
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

package android.hardware.tv.tuner@1.1;

import @1.0::FrontendScanType;
import @1.0::FrontendSettings;
import @1.0::IFrontend;
import @1.0::Result;

/**
 * A Tuner Frontend is used to tune to a frequency and lock signal.
 *
 * IFrontend provides a bit stream to the Tuner Demux interface.
 */
interface IFrontend extends @1.0::IFrontend {
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
     * @param settingsExt1_1 v1_1 Extended information that would be used in the 1.1 Frontend to
     * search and lock the signal in a better way.
     *
     * @return result Result status of the operation.
     *         SUCCESS if successful,
     *         INVALID_STATE if tuning can't be applied at current stage,
     *         UNKNOWN_ERROR if tuning failed for other reasons.
     */
    tune_1_1(FrontendSettings settings, FrontendSettingsExt1_1 settingsExt1_1)
        generates (Result result);

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
     * @param settingsExt1_1 v1_1 Extended information that would be used in the 1.1 Frontend to
     * search and lock the signal in a better way.
     * @return result Result status of the operation.
     *         SUCCESS if successful,
     *         INVALID_STATE if tuning can't be applied at current stage,
     *         UNKNOWN_ERROR if tuning failed for other reasons.
     */
    scan_1_1(FrontendSettings settings, FrontendScanType type,
        FrontendSettingsExt1_1 settingsExt1_1) generates (Result result);

    /**
     * Link Conditional Access Modules (CAM) to Frontend support Common Interface (CI) bypass mode.
     *
     * The client may use this to link CI-CAM to a frontend. CI bypass mode requires that the
     * CICAM also receives the TS concurrently from the frontend when the Demux is receiving the TS
     * directly from the frontend.
     *
     * @param ciCamId specify CI-CAM Id to link.
     * @return ltsId Local Transport Stream Id.
     * @return result Result status of the operation.
     *         SUCCESS if successful,
     *         UNKNOWN_ERROR if failed for other reasons.
     */
    linkCiCam(uint32_t ciCamId) generates (Result result, uint32_t ltsId);

    /**
     * Unlink Conditional Access Modules (CAM) to Frontend.
     *
     * @param ciCamId specify CI-CAM Id to unlink.
     * @return result Result status of the operation.
     *         SUCCESS if successful,
     *         UNKNOWN_ERROR if failed for other reasons.
     */
    unlinkCiCam(uint32_t ciCamId) generates (Result result);

    /**
     * Get the v1_1 extended statuses of the frontend.
     *
     * This retrieve the extended statuses of the frontend for given extended status types.
     *
     * @param statusTypes an array of the extended status types which the caller request.
     *
     * @return result Result status of the operation.
     *         SUCCESS if successful,
     *         INVALID_STATE if tuning can't be applied at current stage,
     *         UNKNOWN_ERROR if tuning failed for other reasons.
     * @return statuses an array of extended statuses the caller requests for.
     */
    getStatusExt1_1(vec<FrontendStatusTypeExt1_1> statusTypes)
        generates (Result result, vec<FrontendStatusExt1_1> statuses);
};
