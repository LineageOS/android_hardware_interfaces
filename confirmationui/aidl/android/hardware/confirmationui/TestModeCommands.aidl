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

package android.hardware.confirmationui;

/**
 * Test mode commands.
 *
 * IConfirmationUI::deliverSecureInputEvent can be used to test certain code paths.
 * To that end, the caller passes an auth token that has an HMAC keyed with the test key
 * (see IConfirmationUI::TEST_KEY_BYTE). Implementations first check the HMAC against test key.
 * If the test key produces a matching HMAC, the implementation evaluates the challenge field
 * of the auth token against the values defined in TestModeCommand.
 * If the command indicates that a confirmation token is to be generated the test key MUST be used
 * to generate this confirmation token.
 *
 * See command code for individual test command descriptions.
 */
@VintfStability
@Backing(type="int")
enum TestModeCommands {
    /**
     * Simulates the user pressing the OK button on the UI. If no operation is pending
     * IConfirmationUI::IGNORED must be returned. A pending operation is finalized successfully
     * see IConfirmationResultCallback::result, however, the test key
     * (see IConfirmationUI::TEST_KEY_BYTE) MUST be used to generate the confirmation token.
     */
    OK_EVENT = 0,
    /**
     * Simulates the user pressing the CANCEL button on the UI. If no operation is pending
     * IConfirmationUI::IGNORED must be returned. A pending operation is finalized as specified in
     * IConfirmationResultCallback.aidl.
     */
    CANCEL_EVENT = 1,
}
