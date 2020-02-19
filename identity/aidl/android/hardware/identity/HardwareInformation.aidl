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

@VintfStability
parcelable HardwareInformation {
    /**
     * credentialStoreName is the name of the credential store implementation.
     */
    @utf8InCpp String credentialStoreName;

    /**
     * credentialStoreAuthorName is the name of the credential store author.
     */
    @utf8InCpp String credentialStoreAuthorName;

    /**
     * dataChunkSize is the size of data chunks to be used when sending and recieving data
     * entries. All data chunks for a data item must be this size except for the last.
     */
    int dataChunkSize;

    /**
     * isDirectAccess specifies whether the provisioned credential is available through
     * direct access. Credentials provisioned in credential stores with this set
     * to true, should use reader authentication on all data elements.
     */
    boolean isDirectAccess;

    /**
     * supportedDocTypes if empty, then any document type is supported, otherwise
     * only the document types returned are supported.
     *
     * Document types are defined in the relevant standard for the document, for example for the
     * for Mobile Driving License as defined by ISO 18013-5 the document type is defined to
     * be "org.iso.18013.5.1.mDL".
     *
     */
    @utf8InCpp String[] supportedDocTypes;
}
