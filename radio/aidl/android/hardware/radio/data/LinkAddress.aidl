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

package android.hardware.radio.data;

/**
 * Describes a data link address for mobile data connection.
 * @hide
 */
@VintfStability
@JavaDerive(toString=true)
parcelable LinkAddress {
    const int ADDRESS_PROPERTY_NONE = 0;
    /**
     * Indicates this address is deprecated
     */
    const int ADDRESS_PROPERTY_DEPRECATED = 0x20;
    /**
     * The format is IP address with optional "/" prefix length (The format is defined in RFC-4291
     * section 2.3). For example, "192.0.1.3", "192.0.1.11/16", or "2001:db8::1/64". Typically one
     * IPv4 or one IPv6 or one of each. If the prefix length is absent, then the addresses are
     * assumed to be point to point with IPv4 with prefix length 32 or IPv6 with prefix length 128.
     */
    String address;
    /**
     * The properties of the link address, as defined in if_addr.h in the Linux kernel.
     * Values are ADDRESS_PROPERTY_
     */
    int addressProperties;
    /**
     * The time, as reported by SystemClock.elapsedRealtime(), when this link address will be or
     * was deprecated. -1 indicates this information is not available. At the time existing
     * connections can still use this address until it expires, but new connections should use the
     * new address. LONG_MAX(0x7FFFFFFFFFFFFFFF) indicates this link address will never be
     * deprecated.
     */
    long deprecationTime;
    /**
     * The time, as reported by SystemClock.elapsedRealtime(), when this link address will expire
     * and be removed from the interface. -1 indicates this information is not available.
     * LONG_MAX(0x7FFFFFFFFFFFFFFF) indicates this link address will never expire.
     */
    long expirationTime;
}
