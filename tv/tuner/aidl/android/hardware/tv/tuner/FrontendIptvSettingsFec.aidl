/*
 * Copyright 2023 The Android Open Source Project
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

import android.hardware.tv.tuner.FrontendIptvSettingsFecType;

/**
 * Configuration details for IPTV FEC.
 * @hide
 */
@VintfStability
parcelable FrontendIptvSettingsFec {
    FrontendIptvSettingsFecType type;

    /**
     * The number of columns in the source block as well as the type of the repair packet.
     */
    int fecColNum;

    /**
     * The number of rows in the source block as well as the type of the repair packet.
     */
    int fecRowNum;
}
