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

package android.hardware.automotive.remoteaccess;

@VintfStability
parcelable ApState {
    /**
     * Whether AP (application processor) is ready to receive remote tasks.
     *
     * If this is true. AP is powered on and the car service is ready to handle
     * remote tasks.
     */
    boolean isReadyForRemoteTask;
    /**
     * Whether AP (application processor) needs to be woken up.
     *
     * While the AP is shutting down, this will be set to false to prevent the
     * wakeup signal to interrupt the shutdown process. At the last step of the
     * shutdown process, this will be set to true so that AP will be waken
     * up when task arrives. After AP starts up, this will be set to false
     * to prevent unnecessary wakeup signal.
     */
    boolean isWakeupRequired;
}
