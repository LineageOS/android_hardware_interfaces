/*
 * Copyright (C) 2024 The Android Open Source Project
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

// C++ reimplementation of f/b/telephony/java/com/android/internal/telephony/uicc/IccUtils.java

#include <libminradio/sim/IccUtils.h>

#include <android-base/logging.h>
#include <libminradio/sim/IccConstants.h>

namespace android::hardware::radio::minimal::sim {

using namespace ::android::hardware::radio::minimal::sim::constants;
namespace aidl = ::aidl::android::hardware::radio::sim;

// frameworks/opt/telephony/src/java/com/android/internal/telephony/uicc/AdnRecord.java
// 3GPP TS 31.102 4.2.26
constexpr int ADN_FOOTER_SIZE_BYTES = 14;
constexpr uint8_t ADN_UNUSED = 0xFF;
constexpr int ADN_BCD_NUMBER_LENGTH = 0;
constexpr int ADN_TON_AND_NPI = 1;
constexpr int ADN_DIALING_NUMBER_START = 2;
constexpr int ADN_DIALING_NUMBER_END = 11;

// com.android.internal.telephony.uicc.IccUtils.charToByte
// com.android.internal.telephony.uicc.IccUtils.hexCharToInt
static uint8_t charToByte(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    LOG(FATAL) << "IccUtils.charToByte: invalid hex character: " << static_cast<int>(c);
    return 0;
}

static constexpr char kHexChars[] = "0123456789ABCDEF";

static aidl::IccIoResult toIccIoResult(uint16_t errorCode, std::string_view simResponse) {
    return {
            .sw1 = errorCode >> 8,
            .sw2 = errorCode & 0xFF,
            .simResponse = std::string(simResponse),
    };
}

aidl::IccIoResult toIccIoResult(std::span<uint8_t const> bytes) {
    return toIccIoResult(IO_RESULT_SUCCESS, sim::bytesToHexString(bytes));
}

aidl::IccIoResult toIccIoResult(std::vector<uint8_t>&& bytes) {
    return toIccIoResult(IO_RESULT_SUCCESS, sim::bytesToHexString(bytes));
}

aidl::IccIoResult toIccIoResult(std::string_view simResponse) {
    return toIccIoResult(IO_RESULT_SUCCESS, simResponse);
}

aidl::IccIoResult toIccIoResult(uint16_t errorCode) {
    return toIccIoResult(errorCode, "");
}

// com.android.internal.telephony.uicc.IccUtils.hexStringToBytes
std::vector<uint8_t> hexStringToBytes(std::string_view str) {
    CHECK(str.size() % 2 == 0) << "Hex string length not even";
    std::vector<uint8_t> bytes(str.size() / 2);
    for (size_t i = 0; i < bytes.size(); i++) {
        bytes[i] = charToByte(str[i * 2]) << 4 | charToByte(str[i * 2 + 1]);
    }
    return bytes;
}

// com.android.internal.telephony.uicc.IccUtils.bchToString (inversion)
// com.android.internal.telephony.uicc.euicc.padTrailingFs
// NOTE: BCH is a nibble-swizzled bytes reprezentation
std::vector<uint8_t> hexStringToBch(std::string_view str) {
    std::vector<uint8_t> bch((str.size() + 1) / 2);
    for (size_t i = 0; i < bch.size(); i++) {
        size_t inpos = i * 2;
        uint8_t encoded = charToByte(str[inpos]);
        if (inpos + 1 < str.size()) {
            encoded |= charToByte(str[inpos + 1]) << 4;
        } else {
            encoded |= 0xF0;
        }
        bch[i] = encoded;
    }
    return bch;
}

// com.android.internal.telephony.uicc.IccUtils.bytesToHexString
std::string bytesToHexString(std::span<uint8_t const> bytes) {
    std::string ret(bytes.size() * 2, '0');
    for (size_t i = 0; i < bytes.size(); i++) {
        ret[i * 2 + 0] = kHexChars[0x0F & (bytes[i] >> 4)];
        ret[i * 2 + 1] = kHexChars[0x0F & (bytes[i])];
    }
    return ret;
}

std::string bytesToHexString(std::vector<uint8_t>&& bytes) {
    std::span<uint8_t> bytesSpan(bytes);
    return bytesToHexString(bytesSpan);
}

// com.android.internal.telephony.uicc.IccUtils.bchToString
std::string bchToHexString(std::span<uint8_t const> bytes) {
    std::string ret(bytes.size() * 2, '0');
    for (size_t i = 0; i < bytes.size(); i++) {
        ret[i * 2 + 0] = kHexChars[0x0F & (bytes[i])];
        ret[i * 2 + 1] = kHexChars[0x0F & (bytes[i] >> 4)];
    }
    return ret;
}

std::vector<uint8_t> uint8ToBytes(uint8_t val) {
    return {val};
}

std::vector<uint8_t> uint16ToBytes(uint16_t val) {
    return {
            static_cast<uint8_t>(val >> 8),
            static_cast<uint8_t>(val & 0xFF),
    };
}

// com.android.internal.telephony.uicc.IccUtils.bcdToString (inversion)
// integerString is a number with possible leading zeros
static std::vector<uint8_t> stringToBcd(std::string_view intString) {
    // Note: 3GPP TS 31.102 Table 4.4 describes BCD coding for characters * and # (not implemented)
    bool isOdd = intString.size() % 2 == 1;
    std::vector<uint8_t> ret(intString.size() / 2 + (isOdd ? 1 : 0), 0);
    for (size_t i = 0; i < intString.size(); i++) {
        const char digitC = intString[i];
        CHECK(digitC >= '0' && digitC <= '9') << "Invalid numeric string: " << intString;
        uint8_t digit = digitC - '0';

        if (i % 2 == 1) digit <<= 4;
        ret[i / 2] |= digit;
    }
    if (isOdd) {
        *ret.rbegin() |= 0xF0;
    }
    return ret;
}

// com.android.internal.telephony.uicc.IccUtils.stringToBcdPlmn
static void stringToBcdPlmn(std::string_view plmn, std::vector<uint8_t>& data, size_t offset) {
    char digit6 = plmn.length() > 5 ? plmn[5] : 'F';
    data[offset] = (charToByte(plmn[1]) << 4) | charToByte(plmn[0]);
    data[offset + 1] = (charToByte(digit6) << 4) | charToByte(plmn[2]);
    data[offset + 2] = (charToByte(plmn[4]) << 4) | charToByte(plmn[3]);
}

// com.android.internal.telephony.uicc.IccUtils.encodeFplmns
std::vector<uint8_t> encodeFplmns(std::span<std::string_view> fplmns) {
    // 3GPP TS 31.102 4.2.16
    auto recordsCount = std::max<size_t>(fplmns.size(), 4);
    std::vector<uint8_t> serializedFplmns(recordsCount * FPLMN_BYTE_SIZE, 0xFF);

    size_t record = 0;
    for (auto&& fplmn : fplmns) {
        stringToBcdPlmn(fplmn, serializedFplmns, FPLMN_BYTE_SIZE * record++);
    }
    return serializedFplmns;
}

std::vector<uint8_t> encodeMsisdn(std::string_view phoneNumber) {
    // 3GPP TS 31.102 4.2.26
    std::vector<uint8_t> msisdn(ADN_FOOTER_SIZE_BYTES, ADN_UNUSED);
    bool isInternational = phoneNumber.size() >= 1 && phoneNumber[0] == '+';
    if (isInternational) phoneNumber = phoneNumber.substr(1);

    auto encodedNumber = stringToBcd(phoneNumber);
    constexpr int numberMaxSize = ADN_DIALING_NUMBER_END - ADN_DIALING_NUMBER_START + 1;
    if (encodedNumber.size() > numberMaxSize) {
        encodedNumber.resize(numberMaxSize);
    }

    msisdn[ADN_BCD_NUMBER_LENGTH] = 1 + encodedNumber.size();

    // 3GPP TS 24.008 Table 10.5.91:
    // 0b1xxxxxx - mandatory bit
    // ton (type of number):
    //  - 0bx001xxxx - international number (with +)
    //  - 0bx010xxxx - national number
    // npi (numbering plan identification):
    //  - 0bxxxx0001 - ISDN/telephony numbering plan
    msisdn[ADN_TON_AND_NPI] = isInternational ? 0b10010001 : 0b10100001;

    std::copy(encodedNumber.begin(), encodedNumber.end(),
              std::next(msisdn.begin(), ADN_DIALING_NUMBER_START));

    return msisdn;
}

std::vector<uint8_t> encodeAd(uint8_t mncLength) {
    // ETSI TS 131 102 4.2.18
    CHECK(mncLength == 2 || mncLength == 3) << "Invalid MNC length: " << mncLength;

    std::vector<uint8_t> ad(4);
    ad[3] = mncLength;
    return ad;
}

}  // namespace android::hardware::radio::minimal::sim
