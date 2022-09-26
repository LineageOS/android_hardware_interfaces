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

package android.hardware.broadcastradio;

import android.hardware.broadcastradio.AnnouncementType;
import android.hardware.broadcastradio.ProgramSelector;
import android.hardware.broadcastradio.VendorKeyValue;

/**
 * Station broadcasting active announcement.
 */
@VintfStability
@JavaDerive(equals=true, toString=true)
parcelable Announcement {
    /**
     * Program selector to tune to the announcement.
     */
    ProgramSelector selector;

    /**
     * Announcement type.
     */
    AnnouncementType type = AnnouncementType.INVALID;

    /**
     * Vendor-specific information.
     *
     * It may be used for extra features, not supported by the platform,
     * for example: com.me.hdradio.urgency=100; com.me.hdradio.certainity=50.
     */
    VendorKeyValue[] vendorInfo;
}
