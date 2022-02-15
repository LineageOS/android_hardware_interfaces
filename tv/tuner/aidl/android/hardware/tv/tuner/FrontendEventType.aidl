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

/**
 * Frontend Event Type.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum FrontendEventType {
    /**
     * The frontend has locked to the signal specified by the tune method. It can also be notified
     * after signal is locked if the signal attributes transmission parameter of the signal is
     * changed (e.g., Modulation).
     */
    LOCKED,

    /**
     * The frontend is unable to lock to the signal specified by the tune method.
     */
    NO_SIGNAL,

    /**
     * The frontend has lost the lock to the signal specified by the tune method.
     */
    LOST_LOCK,
}
