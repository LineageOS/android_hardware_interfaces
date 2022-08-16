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
 * Used by ELECTRONIC_TOLL_COLLECTION_CARD_TYPE.
 */
@VintfStability
@Backing(type="int")
enum ElectronicTollCollectionCardType {
    UNKNOWN = 0,
    JP_ELECTRONIC_TOLL_COLLECTION_CARD = 1,
    JP_ELECTRONIC_TOLL_COLLECTION_CARD_V2 = 2,
}
