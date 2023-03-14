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

package example.vib;

import android.app.Activity;
import android.hardware.vibrator.IVibrator;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;

public class MyActivity extends Activity {
    private static native IBinder gimme(String name);

    @Override
    public void onCreate(Bundle b) {
        super.onCreate(b);
        System.loadLibrary("example_vib_getter");

        // There is no API to get ahold of a Stable AIDL service from a vendor app
        // in Java. This is because this is not the recommended way to get ahold
        // of functionality in Android. The Android API Council recommendation is to
        // implement uses-library APIs in the system/system_ext partition which add
        // new APIs. AIDL as an API in Java is not recommended or supported way to
        // communicate by apps - the recommendation is to use Java APIs. However,
        // there also exists a large number of vendor apps which are coupled with
        // hardware-specific code, and are therefore on the vendor partition. A
        // large number of these use HIDL, and this is how they can continue to
        // use that structure with AIDL.
        IVibrator v =
                IVibrator.Stub.asInterface(gimme("android.hardware.vibrator.IVibrator/default"));

        try {
            v.on(100 /*ms*/, null /*cb*/);
        } catch (RemoteException e) {
            throw new RuntimeException(e);
        }

        finish();
    }
}
