/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Copyright 2021 NXP.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.uwb;

/**
 * Asynchronous events sent from the UWB subsystem.
 */
@VintfStability
@Backing(type="int")
enum UwbEvent {
    /** Open request processing completed. */
    OPEN_CPLT = 0,
    /** Close request processing completed. */
    CLOSE_CPLT = 1,
    /** Post initialization processing completed. */
    POST_INIT_CPLT = 2,
    /** Fatal error encountered. */
    ERROR = 3,
}
