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

package android.hardware.wifi;

/**
 * NAN API response codes used in request notifications and events.
 */
@VintfStability
@Backing(type="int")
enum NanStatusCode {
    SUCCESS = 0,
    /**
     * NAN Discovery Engine/Host driver failures.
     */
    INTERNAL_FAILURE = 1,
    /**
     * NAN OTA failures.
     */
    PROTOCOL_FAILURE = 2,
    /**
     * The publish/subscribe discovery session id is invalid.
     */
    INVALID_SESSION_ID = 3,
    /**
     * Out of resources to fufill request.
     */
    NO_RESOURCES_AVAILABLE = 4,
    /**
     * Invalid arguments passed.
     */
    INVALID_ARGS = 5,
    /**
     * Invalid peer id.
     */
    INVALID_PEER_ID = 6,
    /**
     * Invalid NAN data-path (ndp) id.
     */
    INVALID_NDP_ID = 7,
    /**
     * Attempting to enable NAN when not available, e.g. wifi is disabled.
     */
    NAN_NOT_ALLOWED = 8,
    /**
     * Over the air ACK not received.
     */
    NO_OTA_ACK = 9,
    /**
     * Attempting to enable NAN when already enabled.
     */
    ALREADY_ENABLED = 10,
    /**
     * Can't queue tx followup message for transmission.
     */
    FOLLOWUP_TX_QUEUE_FULL = 11,
    /**
     * Unsupported concurrency of NAN and another feature - NAN disabled.
     */
    UNSUPPORTED_CONCURRENCY_NAN_DISABLED = 12,
    /**
     * If the pairing id is invalid
     */
    INVALID_PAIRING_ID = 13,
    /**
     * If the bootstrapping id is invalid
     */
    INVALID_BOOTSTRAPPING_ID = 14,
    /*
     * If same request is received again
     */
    REDUNDANT_REQUEST = 15,
    /*
     * If current request is not supported
     */
    NOT_SUPPORTED = 16,
    /*
     * If no Wifi Aware connection is active
     */
    NO_CONNECTION = 17
}
