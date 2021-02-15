/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.hardware.neuralnetworks;

import android.hardware.neuralnetworks.ErrorStatus;
import android.hardware.neuralnetworks.IPreparedModel;

/**
 * IPreparedModelCallback must be used to return a prepared model produced by an asynchronous task
 * launched from IDevice::prepareModel*.
 */
@VintfStability
interface IPreparedModelCallback {
    /**
     * Notify must be invoked immediately after the asynchronous task holding this callback has
     * finished preparing the model. If the model was successfully prepared, the method must be
     * invoked with ErrorStatus::NONE and the prepared model. If the model was not able to be
     * successfully prepared, the method must be invoked with the appropriate ErrorStatus and
     * nullptr as the IPreparedModel. If the asynchronous task holding this callback fails to launch
     * or if the model provided to IDevice::prepareModel is invalid, notify method must be invoked
     * with the appropriate error as well as nullptr for the IPreparedModel.
     *
     * @param status Error status returned from the asynchronous model preparation task; must be:
     *               - NONE if the asynchronous task successfully prepared the model
     *               - DEVICE_UNAVAILABLE if driver is offline or busy
     *               - GENERAL_FAILURE if the asynchronous task resulted in an unspecified error
     *               - INVALID_ARGUMENT if one of the input arguments to prepareModel is invalid
     *               - MISSED_DEADLINE_* if the preparation is aborted because the model cannot be
     *                 prepared by the deadline
     *               - RESOURCE_EXHAUSTED_* if the task was aborted by the driver
     * @param preparedModel A model that has been asynchronously prepared for execution. If the
     *                      model was unable to be prepared due to an error, nullptr must be passed
     *                      in place of the IPreparedModel object.
     */
    void notify(in ErrorStatus status, in IPreparedModel preparedModel);
}
