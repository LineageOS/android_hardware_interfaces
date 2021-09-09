/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.radio;

/**
 * The allowed failure modes on an IWLAN handover failure.
 */
@VintfStability
@Backing(type="byte")
enum HandoverFailureMode {
    /**
     * On data handover failure, fallback to the source data transport when the fail cause is due
     * to a hand off preference change.
     */
    LEGACY,
    /**
     * On data handover failure, fallback to the source data transport.
     */
    DO_FALLBACK,
    /**
     * On data handover failure, retry the handover instead of falling back to the source data
     * transport.
     */
    NO_FALLBACK_RETRY_HANDOVER,
    /**
     * On data handover failure, setup a new data connection by sending a normal request to the
     * underlying data service.
     */
    NO_FALLBACK_RETRY_SETUP_NORMAL,
}
