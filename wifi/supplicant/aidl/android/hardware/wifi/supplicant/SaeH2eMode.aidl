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

package android.hardware.wifi.supplicant;

/**
 * SAE Hash-to-Element mode.
 */
@VintfStability
@Backing(type="byte")
enum SaeH2eMode {
    /**
     * Hash-to-Element is disabled, only Hunting & Pecking is allowed.
     */
    DISABLED,
    /**
     * Both Hash-to-Element and Hunting & Pecking are allowed.
     */
    H2E_OPTIONAL,
    /**
     * Only Hash-to-Element is allowed.
     */
    H2E_MANDATORY,
}
