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
package com.android.tests.security.identity;

import static org.junit.Assert.fail;
import static org.junit.Assume.assumeTrue;

import android.platform.test.annotations.RequiresDevice;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;
import com.android.tradefed.testtype.junit4.BaseHostJUnit4Test;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(DeviceJUnit4ClassRunner.class)
public class IdentityCredentialImplementedTest extends BaseHostJUnit4Test {
    // Returns the ro.vendor.api_level or 0 if not set.
    //
    // Throws NumberFormatException if ill-formatted.
    //
    // Throws DeviceNotAvailableException if device is not available.
    //
    private int getVendorApiLevel() throws NumberFormatException, DeviceNotAvailableException {
        String vendorApiLevelString =
                getDevice().executeShellCommand("getprop ro.vendor.api_level").trim();
        if (vendorApiLevelString.isEmpty()) {
            return 0;
        }
        return Integer.parseInt(vendorApiLevelString);
    }

    // As of Android 13 (API level 31), Identity Credential is required at feature version 202201
    // or newer.
    //
    @RequiresDevice
    @Test
    public void testIdentityCredentialIsImplemented() throws Exception {
        int vendorApiLevel = getVendorApiLevel();
        assumeTrue(vendorApiLevel >= 31);

        final String minimumFeatureVersionNeeded = "202201";

        String result = getDevice().executeShellCommand(
                "pm has-feature android.hardware.identity_credential "
                + minimumFeatureVersionNeeded);
        if (!result.trim().equals("true")) {
            fail("Identity Credential feature version " + minimumFeatureVersionNeeded
                    + " required but not found");
        }
    }
}
