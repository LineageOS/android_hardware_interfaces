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

package android.hardware.tv.tuner@1.1;

import @1.0::IFrontendCallback;
import FrontendScanMessageExt1_1;
import FrontendScanMessageTypeExt1_1;

interface IFrontendCallback extends @1.0::IFrontendCallback {
    /**
     * The callback function that must be called by HAL implementation to notify
     * the client of the v1_1 extended scan messages.
     *
     * @param type the type of v1_1 extended scan message.
     * @param message the v1_1 extended scan message sent by HAL to the client.
     */
    onScanMessageExt1_1(FrontendScanMessageTypeExt1_1 type, FrontendScanMessageExt1_1 messageExt);
};
