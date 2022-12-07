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

package android.hardware.audio.core;

/**
 * This interface is used to indicate completion of asynchronous operations.
 * See the state machines referenced by StreamDescriptor for details.
 */
@VintfStability
oneway interface IStreamCallback {
    /**
     * Indicate that the stream is ready for next data exchange.
     */
    void onTransferReady();
    /**
     * Indicate that an irrecoverable error has occurred during the last I/O
     * operation. After sending this callback, the stream enters the 'ERROR'
     * state.
     */
    void onError();
    /**
     * Indicate that the stream has finished draining. This is only used
     * for output streams because for input streams draining is performed
     * by the client.
     */
    void onDrainReady();
}
