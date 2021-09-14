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

package android.hardware.automotive.vehicle;

/**
 * Used by ELECTRONIC_TOLL_COLLECTION_CARD_STATUS.
 */
@VintfStability
@Backing(type="int")
enum ElectronicTollCollectionCardStatus {
    UNKNOWN = 0,
    ELECTRONIC_TOLL_COLLECTION_CARD_VALID = 1,
    ELECTRONIC_TOLL_COLLECTION_CARD_INVALID = 2,
    ELECTRONIC_TOLL_COLLECTION_CARD_NOT_INSERTED = 3,
}
