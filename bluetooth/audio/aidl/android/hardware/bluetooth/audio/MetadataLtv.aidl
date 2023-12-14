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

package android.hardware.bluetooth.audio;

import android.hardware.bluetooth.audio.AudioContext;

/**
 * Used to exchange generic metadata between the stack and the provider.
 * As defined in Bluetooth Assigned Numbers, Sec. 6.12.6.
 */
@VintfStability
union MetadataLtv {
    parcelable PreferredAudioContexts {
        AudioContext values;
    }
    parcelable StreamingAudioContexts {
        AudioContext values;
    }
    /* This is an opaque container for passing metadata between the provider and
     * the remote device. It must not be interpreted by the BT stack.
     */
    parcelable VendorSpecific {
        int companyId;
        byte[] opaqueValue;
    }

    PreferredAudioContexts preferredAudioContexts;
    StreamingAudioContexts streamingAudioContexts;
    VendorSpecific vendorSpecific;
}
