/*
 * Copyright (C) 2019 The Android Open Source Project
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
package android.hardware.tests.extension.vibrator;

// it's fine to use types from other interfaces
import android.hardware.vibrator.IVibratorCallback;
import android.hardware.tests.extension.vibrator.Directionality;
import android.hardware.tests.extension.vibrator.VendorEffect;

/**
 * This is an example of an AIDL interface extension. Notice that it does not
 * inherit from any other extension. Instead, it will be tagged onto that
 * extension at runtime.
 */
@VintfStability
interface ICustomVibrator {
    /**
     * Avoid conflicting with vendor properties. Giving this as an example
     * because the core vibrator interface uses capabilities. In reality,
     * since this is only one capability, it's probably not needed to construct
     * a bitfield.
     *
     * This is for longitudinal/transverse waves, see setDirectionality.
     */
    const int CAP_VENDOR_DIRECTIONALITY = 1 << 0;

    /**
     * Any new methods can be added, this returns CAP_VENDOR_*.
     */
    int getVendorCapabilities();

    /**
     * Arbitrary new functionality can be added.
     */
    void setDirectionality(Directionality directionality);

    /**
     * Perform a custom vendor effect. Note, this is a separate effect enum to
     * avoid conflicting with core types.
     */
    int perform(VendorEffect effect, IVibratorCallback callback);
}
