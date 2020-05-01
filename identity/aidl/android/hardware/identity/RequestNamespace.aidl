/*
 * Copyright 2020 The Android Open Source Project
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

package android.hardware.identity;

import android.hardware.identity.RequestDataItem;

@VintfStability
parcelable RequestNamespace {
    /**
     * The name of the namespace that items are being requested from, for
     * example "org.iso.18013.5.1".
     */
    @utf8InCpp String namespaceName;

    /**
     * The data items requested.
     */
    RequestDataItem[] items;
}
