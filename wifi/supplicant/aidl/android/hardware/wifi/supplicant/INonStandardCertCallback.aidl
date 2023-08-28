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

package android.hardware.wifi.supplicant;

/**
 * Callback to allow supplicant to retrieve non-standard certificate types
 * from the framework. Certificates can be stored in the framework using
 * the WifiKeystore#put system API.
 *
 * Must be registered by the client at initialization, so that
 * supplicant can call into the client to retrieve any values.
 */
@VintfStability
interface INonStandardCertCallback {
    /**
     * Requests a binary blob from the certificate key-value store.
     *
     * @param alias Key into the key-value mapping.
     * @return Value associated with |alias| in the certificate store.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    byte[] getBlob(in String alias);

    /**
     * List the aliases currently stored in the database.
     *
     * @param prefix Prefix to filter the aliases by.
     * @return List of alias strings in the certificate store.
               The resulting strings will each exclude the prefix.
     */
    String[] listAliases(in String prefix);
}
