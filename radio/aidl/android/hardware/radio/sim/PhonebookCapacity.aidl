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

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable PhonebookCapacity {
    /**
     * Maximum number of ADN records possible in the SIM phonebook. Needs to be non-negative.
     */
    int maxAdnRecords;
    /**
     * Used ADN records in the SIM phonebook. Needs to be non-negative.
     */
    int usedAdnRecords;
    /**
     * Maximum email records possible in the SIM phonebook. Needs to be non-negative.
     */
    int maxEmailRecords;
    /**
     * Used email records in the SIM phonebook. Needs to be non-negative.
     */
    int usedEmailRecords;
    /**
     * Maximum additional number records possible in the SIM phonebook. Needs to be non-negative.
     */
    int maxAdditionalNumberRecords;
    /**
     * Used additional number records in the SIM phonebook. Needs to be non-negative.
     */
    int usedAdditionalNumberRecords;
    /**
     * Maximum name length possible in the SIM phonebook. Needs to be non-negative.
     */
    int maxNameLen;
    /**
     * Maximum number length possible in the SIM phonebook. Needs to be non-negative.
     */
    int maxNumberLen;
    /**
     * Maximum email length possible in the SIM phonebook. Needs to be non-negative.
     */
    int maxEmailLen;
    /**
     * Maximum additional number length possible in the SIM phonebook. Needs to be non-negative.
     */
    int maxAdditionalNumberLen;
}
