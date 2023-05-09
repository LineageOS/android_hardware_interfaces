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

package android.hardware.radio.sim;

/**
 * Phonebook-record-information specified by EF_ADN (Abbreviated dialing numbers) record of SIM
 * as per 3GPP spec 31.102 v15 Section-4.4.2.3.
 * @hide
 */
@VintfStability
@JavaDerive(toString=true)
parcelable PhonebookRecordInfo {
    /**
     * Record index. 0 is used to insert a record
     */
    int recordId;
    /**
     * Alpha identifier, empty string if no value
     */
    String name;
    /**
     * Dialling number, empty string if no value
     */
    String number;
    /**
     * Email addresses
     */
    String[] emails;
    /**
     * Additional numbers
     */
    String[] additionalNumbers;
}
