/*
 * Copyright (C) 2020 The Android Open Source Project
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

#define LOG_TAG "keymint_1_test"
#include <cutils/log.h>

#include <signal.h>

#include <algorithm>
#include <iostream>

#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/mem.h>
#include <openssl/x509v3.h>

#include <cutils/properties.h>

#include <android/binder_manager.h>

#include <aidl/android/hardware/security/keymint/IRemotelyProvisionedComponent.h>
#include <aidl/android/hardware/security/keymint/KeyFormat.h>

#include <keymint_support/key_param_output.h>
#include <keymint_support/openssl_utils.h>

#include "KeyMintAidlTestBase.h"

using aidl::android::hardware::security::keymint::AuthorizationSet;
using aidl::android::hardware::security::keymint::KeyCharacteristics;
using aidl::android::hardware::security::keymint::KeyFormat;

namespace std {

using namespace aidl::android::hardware::security::keymint;

template <>
struct std::equal_to<KeyCharacteristics> {
    bool operator()(const KeyCharacteristics& a, const KeyCharacteristics& b) const {
        if (a.securityLevel != b.securityLevel) return false;

        // this isn't very efficient. Oh, well.
        AuthorizationSet a_auths(a.authorizations);
        AuthorizationSet b_auths(b.authorizations);

        a_auths.Sort();
        b_auths.Sort();

        return a_auths == b_auths;
    }
};

}  // namespace std

namespace aidl::android::hardware::security::keymint::test {

namespace {

bool check_patchLevels = false;

template <TagType tag_type, Tag tag, typename ValueT>
bool contains(const vector<KeyParameter>& set, TypedTag<tag_type, tag> ttag,
              ValueT expected_value) {
    auto it = std::find_if(set.begin(), set.end(), [&](const KeyParameter& param) {
        if (auto p = authorizationValue(ttag, param)) {
            return *p == expected_value;
        }
        return false;
    });
    return (it != set.end());
}

template <TagType tag_type, Tag tag>
bool contains(const vector<KeyParameter>& set, TypedTag<tag_type, tag>) {
    auto it = std::find_if(set.begin(), set.end(),
                           [&](const KeyParameter& param) { return param.tag == tag; });
    return (it != set.end());
}

constexpr char hex_value[256] = {0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 1,  2,  3,  4,  5,  6,  7, 8, 9, 0, 0, 0, 0, 0, 0,  // '0'..'9'
                                 0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 'A'..'F'
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 'a'..'f'
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0};

string hex2str(string a) {
    string b;
    size_t num = a.size() / 2;
    b.resize(num);
    for (size_t i = 0; i < num; i++) {
        b[i] = (hex_value[a[i * 2] & 0xFF] << 4) + (hex_value[a[i * 2 + 1] & 0xFF]);
    }
    return b;
}

string rsa_key = hex2str(
        // RFC 5208 s5
        "30820275"            // SEQUENCE length 0x275 (PrivateKeyInfo) {
        "020100"              // INTEGER length 1 value 0x00 (version)
        "300d"                // SEQUENCE length 0x0d (AlgorithmIdentifier) {
        "0609"                // OBJECT IDENTIFIER length 9 (algorithm)
        "2a864886f70d010101"  // 1.2.840.113549.1.1.1 (rsaEncryption)
        "0500"                // NULL (parameters)
        // } end SEQUENCE (AlgorithmIdentifier)
        "0482025f"  // OCTET STRING length 0x25f (privateKey) holding...
        // RFC 8017 A.1.2
        "3082025b"  // SEQUENCE length 0x25b (RSAPrivateKey) {
        "020100"    // INTEGER length 1 value 0x00 (version)
        "028181"    // INTEGER length 0x81 value (modulus) ...
        "00c6095409047d8634812d5a218176e4"
        "5c41d60a75b13901f234226cffe77652"
        "1c5a77b9e389417b71c0b6a44d13afe4"
        "e4a2805d46c9da2935adb1ff0c1f24ea"
        "06e62b20d776430a4d435157233c6f91"
        "6783c30e310fcbd89b85c2d567711697"
        "85ac12bca244abda72bfb19fc44d27c8"
        "1e1d92de284f4061edfd99280745ea6d"
        "25"
        "0203010001"  // INTEGER length 3 value 0x10001 (publicExponent)
        "028180"      // INTEGER length 0x80 (privateExponent) value...
        "1be0f04d9cae3718691f035338308e91"
        "564b55899ffb5084d2460e6630257e05"
        "b3ceab02972dfabcd6ce5f6ee2589eb6"
        "7911ed0fac16e43a444b8c861e544a05"
        "93365772f8baf6b22fc9e3c5f1024b06"
        "3ac080a7b2234cf8aee8f6c47bbf4fd3"
        "ace7240290bef16c0b3f7f3cdd64ce3a"
        "b5912cf6e32f39ab188358afcccd8081"
        "0241"  // INTEGER length 0x41 (prime1)
        "00e4b49ef50f765d3b24dde01aceaaf1"
        "30f2c76670a91a61ae08af497b4a82be"
        "6dee8fcdd5e3f7ba1cfb1f0c926b88f8"
        "8c92bfab137fba2285227b83c342ff7c"
        "55"
        "0241"  // INTEGER length 0x41 (prime2)
        "00ddabb5839c4c7f6bf3d4183231f005"
        "b31aa58affdda5c79e4cce217f6bc930"
        "dbe563d480706c24e9ebfcab28a6cdef"
        "d324b77e1bf7251b709092c24ff501fd"
        "91"
        "0240"  // INTEGER length 0x40 (exponent1)
        "23d4340eda3445d8cd26c14411da6fdc"
        "a63c1ccd4b80a98ad52b78cc8ad8beb2"
        "842c1d280405bc2f6c1bea214a1d742a"
        "b996b35b63a82a5e470fa88dbf823cdd"
        "0240"  // INTEGER length 0x40 (exponent2)
        "1b7b57449ad30d1518249a5f56bb9829"
        "4d4b6ac12ffc86940497a5a5837a6cf9"
        "46262b494526d328c11e1126380fde04"
        "c24f916dec250892db09a6d77cdba351"
        "0240"  // INTEGER length 0x40 (coefficient)
        "7762cd8f4d050da56bd591adb515d24d"
        "7ccd32cca0d05f866d583514bd7324d5"
        "f33645e8ed8b4a1cb3cc4a1d67987399"
        "f2a09f5b3fb68c88d5e5d90ac33492d6"
        // } end SEQUENCE (PrivateKey)
        // } end SEQUENCE (PrivateKeyInfo)
);

/*
 * DER-encoded PKCS#8 format RSA key. Generated using:
 *
 * openssl genrsa 2048 | openssl pkcs8 -topk8 -nocrypt -outform der | hexdump -e '30/1  "%02X" "\n"'
 */
string rsa_2048_key = hex2str(
        // RFC 5208 s5
        "308204BD"            // SEQUENCE length 0x4bd (PrivateKeyInfo) {
        "020100"              // INTEGER length 1 value 0x00 (version)
        "300D"                // SEQUENCE length 0x0d (AlgorithmIdentifier) {
        "0609"                // OBJECT IDENTIFIER length 9 (algorithm)
        "2A864886F70D010101"  // 1.2.840.113549.1.1.1 (rsaEncryption)
        "0500"                // NULL (parameters)
        // } end SEQUENCE (AlgorithmIdentifier)
        "048204A7"  // OCTET STRING length 0x25f (privateKey) holding...
        // RFC 8017 A.1.2
        "308204A3"  // SEQUENCE length 0x4a3 (RSAPrivateKey) {
        "020100"    // INTEGER length 1 value 0x00 (version)
        "02820101"  // INTEGER length 0x101 value (modulus) ...
        "00BEBC342B56D443B1299F9A6A7056E8"
        "0A897E318476A5A18029E63B2ED739A6"
        "1791D339F58DC763D9D14911F2EDEC38"
        "3DEE11F6319B44510E7A3ECD9B79B973"
        "82E49500ACF8117DC89CAF0E621F7775"
        "6554A2FD4664BFE7AB8B59AB48340DBF"
        "A27B93B5A81F6ECDEB02D0759307128D"
        "F3E3BAD4055C8B840216DFAA5700670E"
        "6C5126F0962FCB70FF308F25049164CC"
        "F76CC2DA66A7DD9A81A714C2809D6918"
        "6133D29D84568E892B6FFBF3199BDB14"
        "383EE224407F190358F111A949552ABA"
        "6714227D1BD7F6B20DD0CB88F9467B71"
        "9339F33BFF35B3870B3F62204E4286B0"
        "948EA348B524544B5F9838F29EE643B0"
        "79EEF8A713B220D7806924CDF7295070"
        "C5"
        "0203010001"  // INTEGER length 3 value 0x10001 (publicExponent)
        "02820100"    // INTEGER length 0x100 (privateExponent) value...
        "69F377F35F2F584EF075353CCD1CA997"
        "38DB3DBC7C7FF35F9366CE176DFD1B13"
        "5AB10030344ABF5FBECF1D4659FDEF1C"
        "0FC430834BE1BE3911951377BB3D563A"
        "2EA9CA8F4AD9C48A8CE6FD516A735C66"
        "2686C7B4B3C09A7B8354133E6F93F790"
        "D59EAEB92E84C9A4339302CCE28FDF04"
        "CCCAFA7DE3F3A827D4F6F7D38E68B0EC"
        "6AB706645BF074A4E4090D06FB163124"
        "365FD5EE7A20D350E9958CC30D91326E"
        "1B292E9EF5DB408EC42DAF737D201497"
        "04D0A678A0FB5B5446863B099228A352"
        "D604BA8091A164D01D5AB05397C71EAD"
        "20BE2A08FC528FE442817809C787FEE4"
        "AB97F97B9130D022153EDC6EB6CBE7B0"
        "F8E3473F2E901209B5DB10F93604DB01"
        "028181"  // INTEGER length 0x81 (prime1)
        "00E83C0998214941EA4F9293F1B77E2E"
        "99E6CF305FAF358238E126124FEAF2EB"
        "9724B2EA7B78E6032343821A80E55D1D"
        "88FB12D220C3F41A56142FEC85796D19"
        "17F1E8C774F142B67D3D6E7B7E6B4383"
        "E94DB5929089DBB346D5BDAB40CC2D96"
        "EE0409475E175C63BF78CFD744136740"
        "838127EA723FF3FE7FA368C1311B4A4E"
        "05"
        "028181"  // INTEGER length 0x81 (prime2)
        "00D240FCC0F5D7715CDE21CB2DC86EA1"
        "46132EA3B06F61FF2AF54BF38473F59D"
        "ADCCE32B5F4CC32DD0BA6F509347B4B5"
        "B1B58C39F95E4798CCBB43E83D0119AC"
        "F532F359CA743C85199F0286610E2009"
        "97D7312917179AC9B67558773212EC96"
        "1E8BCE7A3CC809BC5486A96E4B0E6AF3"
        "94D94E066A0900B7B70E82A44FB30053"
        "C1"
        "028181"  // INTEGER length 0x81 (exponent1)
        "00AD15DA1CBD6A492B66851BA8C316D3"
        "8AB700E2CFDDD926A658003513C54BAA"
        "152B30021D667D20078F500F8AD3E7F3"
        "945D74A891ED1A28EAD0FEEAEC8C14A8"
        "E834CF46A13D1378C99D18940823CFDD"
        "27EC5810D59339E0C34198AC638E09C8"
        "7CBB1B634A9864AE9F4D5EB2D53514F6"
        "7B4CAEC048C8AB849A02E397618F3271"
        "35"
        "028180"  // INTEGER length 0x80 (exponent2)
        "1FA2C1A5331880A92D8F3E281C617108"
        "BF38244F16E352E69ED417C7153F9EC3"
        "18F211839C643DCF8B4DD67CE2AC312E"
        "95178D5D952F06B1BF779F4916924B70"
        "F582A23F11304E02A5E7565AE22A35E7"
        "4FECC8B6FDC93F92A1A37703E4CF0E63"
        "783BD02EB716A7ECBBFA606B10B74D01"
        "579522E7EF84D91FC522292108D902C1"
        "028180"  // INTEGER length 0x80 (coefficient)
        "796FE3825F9DCC85DF22D58690065D93"
        "898ACD65C087BEA8DA3A63BF4549B795"
        "E2CD0E3BE08CDEBD9FCF1720D9CDC507"
        "0D74F40DED8E1102C52152A31B6165F8"
        "3A6722AECFCC35A493D7634664B888A0"
        "8D3EB034F12EA28BFEE346E205D33482"
        "7F778B16ED40872BD29FCB36536B6E93"
        "FFB06778696B4A9D81BB0A9423E63DE5"
        // } end SEQUENCE (PrivateKey)
        // } end SEQUENCE (PrivateKeyInfo)
);

string ec_256_key = hex2str(
        // RFC 5208 s5
        "308187"            // SEQUENCE length 0x87 (PrivateKeyInfo) {
        "020100"            // INTEGER length 1 value 0 (version)
        "3013"              // SEQUENCE length 0x13 (AlgorithmIdentifier) {
        "0607"              // OBJECT IDENTIFIER length 7 (algorithm)
        "2a8648ce3d0201"    // 1.2.840.10045.2.1 (ecPublicKey)
        "0608"              // OBJECT IDENTIFIER length 8 (param)
        "2a8648ce3d030107"  //  1.2.840.10045.3.1.7 (secp256r1)
        // } end SEQUENCE (AlgorithmIdentifier)
        "046d"    // OCTET STRING length 0x6d (privateKey) holding...
        "306b"    // SEQUENCE length 0x6b (ECPrivateKey)
        "020101"  // INTEGER length 1 value 1 (version)
        "0420"    // OCTET STRING length 0x20 (privateKey)
        "737c2ecd7b8d1940bf2930aa9b4ed3ff"
        "941eed09366bc03299986481f3a4d859"
        "a144"  // TAG [1] len 0x44 (publicKey) {
        "03420004bf85d7720d07c25461683bc6"
        "48b4778a9a14dd8a024e3bdd8c7ddd9a"
        "b2b528bbc7aa1b51f14ebbbb0bd0ce21"
        "bcc41c6eb00083cf3376d11fd44949e0"
        "b2183bfe"
        // } end SEQUENCE (ECPrivateKey)
        // } end SEQUENCE (PrivateKeyInfo)
);

string ec_521_key = hex2str(
        // RFC 5208 s5
        "3081EE"          // SEQUENCE length 0xee (PrivateKeyInfo) {
        "020100"          // INTEGER length 1 value 0 (version)
        "3010"            // SEQUENCE length 0x10 (AlgorithmIdentifier) {
        "0607"            // OBJECT IDENTIFIER length 7 (algorithm)
        "2A8648CE3D0201"  // 1.2.840.10045.2.1 (ecPublicKey)
        "0605"            // OBJECT IDENTIFIER length 5 (param)
        "2B81040023"      //  1.3.132.0.35 (secp521r1)
        // } end SEQUENCE (AlgorithmIdentifier)
        "0481D6"  // OCTET STRING length 0xd6 (privateKey) holding...
        "3081D3"  // SEQUENCE length 0xd3 (ECPrivateKey)
        "020101"  // INTEGER length 1 value 1 (version)
        "0442"    // OCTET STRING length 0x42 (privateKey)
        "0011458C586DB5DAA92AFAB03F4FE46A"
        "A9D9C3CE9A9B7A006A8384BEC4C78E8E"
        "9D18D7D08B5BCFA0E53C75B064AD51C4"
        "49BAE0258D54B94B1E885DED08ED4FB2"
        "5CE9"
        "A18189"  // TAG [1] len 0x89 (publicKey) {
        "03818600040149EC11C6DF0FA122C6A9"
        "AFD9754A4FA9513A627CA329E349535A"
        "5629875A8ADFBE27DCB932C051986377"
        "108D054C28C6F39B6F2C9AF81802F9F3"
        "26B842FF2E5F3C00AB7635CFB36157FC"
        "0882D574A10D839C1A0C049DC5E0D775"
        "E2EE50671A208431BB45E78E70BEFE93"
        "0DB34818EE4D5C26259F5C6B8E28A652"
        "950F9F88D7B4B2C9D9"
        // } end SEQUENCE (ECPrivateKey)
        // } end SEQUENCE (PrivateKeyInfo)
);

string ec_256_key_rfc5915 = hex2str(
        // RFC 5208 s5
        "308193"            // SEQUENCE length 0x93 (PrivateKeyInfo) {
        "020100"            // INTEGER length 1 value 0 (version)
        "3013"              // SEQUENCE length 0x13 (AlgorithmIdentifier) {
        "0607"              // OBJECT IDENTIFIER length 7 (algorithm)
        "2a8648ce3d0201"    // 1.2.840.10045.2.1 (ecPublicKey)
        "0608"              // OBJECT IDENTIFIER length 8 (param)
        "2a8648ce3d030107"  //  1.2.840.10045.3.1.7 (secp256r1)
        // } end SEQUENCE (AlgorithmIdentifier)
        "0479"  // OCTET STRING length 0x79 (privateKey) holding...
        // RFC 5915 s3
        "3077"    // SEQUENCE length 0x77 (ECPrivateKey)
        "020101"  // INTEGER length 1 value 1 (version)
        "0420"    // OCTET STRING length 0x42 (privateKey)
        "782370a8c8ce5537baadd04dcff079c8"
        "158cfa9c67b818b38e8d21c9fa750c1d"
        "a00a"              // TAG [0] length 0xa (parameters)
        "0608"              // OBJECT IDENTIFIER length 8
        "2a8648ce3d030107"  // 1.2.840.10045.3.1.7 (secp256r1)
        // } end TAG [0]
        "a144"  // TAG [1] length 0x44 (publicKey) {
        "0342"  // BIT STRING length 0x42
        "00"    // no pad bits
        "04e2cc561ee701da0ad0ef0d176bb0c9"
        "19d42e79c393fdc1bd6c4010d85cf2cf"
        "8e68c905464666f98dad4f01573ba810"
        "78b3428570a439ba3229fbc026c55068"
        "2f"
        // } end SEQUENCE (ECPrivateKey)
        // } end SEQUENCE (PrivateKeyInfo)
);

string ec_256_key_sec1 = hex2str(
        // RFC 5208 s5
        "308187"            // SEQUENCE length 0x87 (PrivateKeyInfo) {
        "020100"            // INTEGER length 1 value 0 (version)
        "3013"              // SEQUENCE length 0x13 (AlgorithmIdentifier) {
        "0607"              // OBJECT IDENTIFIER length 7 (algorithm)
        "2a8648ce3d0201"    // 1.2.840.10045.2.1 (ecPublicKey)
        "0608"              // OBJECT IDENTIFIER length 8 (param)
        "2a8648ce3d030107"  // 1.2.840.10045.3.1.7 (secp256r1)
        // } end SEQUENCE (AlgorithmIdentifier)
        "046d"  // OCTET STRING length 0x6d (privateKey) holding...
        // SEC1-v2 C.4
        "306b"    // SEQUENCE length 0x6b (ECPrivateKey)
        "020101"  // INTEGER length 1 value 0x01 (version)
        "0420"    // OCTET STRING length 0x20 (privateKey)
        "782370a8c8ce5537baadd04dcff079c8"
        "158cfa9c67b818b38e8d21c9fa750c1d"
        "a144"  // TAG [1] length 0x44 (publicKey) {
        "0342"  // BIT STRING length 0x42
        "00"    // no pad bits
        "04e2cc561ee701da0ad0ef0d176bb0c9"
        "19d42e79c393fdc1bd6c4010d85cf2cf"
        "8e68c905464666f98dad4f01573ba810"
        "78b3428570a439ba3229fbc026c55068"
        "2f"
        // } end TAG [1] (publicKey)
        // } end SEQUENCE (PrivateKeyInfo)
);

struct RSA_Delete {
    void operator()(RSA* p) { RSA_free(p); }
};

std::string make_string(const uint8_t* data, size_t length) {
    return std::string(reinterpret_cast<const char*>(data), length);
}

template <size_t N>
std::string make_string(const uint8_t (&a)[N]) {
    return make_string(a, N);
}

class AidlBuf : public vector<uint8_t> {
    typedef vector<uint8_t> super;

  public:
    AidlBuf() {}
    AidlBuf(const super& other) : super(other) {}
    AidlBuf(super&& other) : super(std::move(other)) {}
    explicit AidlBuf(const std::string& other) : AidlBuf() { *this = other; }

    AidlBuf& operator=(const super& other) {
        super::operator=(other);
        return *this;
    }

    AidlBuf& operator=(super&& other) {
        super::operator=(std::move(other));
        return *this;
    }

    AidlBuf& operator=(const string& other) {
        resize(other.size());
        for (size_t i = 0; i < other.size(); ++i) {
            (*this)[i] = static_cast<uint8_t>(other[i]);
        }
        return *this;
    }

    string to_string() const { return string(reinterpret_cast<const char*>(data()), size()); }
};

string device_suffix(const string& name) {
    size_t pos = name.find('/');
    if (pos == string::npos) {
        return name;
    }
    return name.substr(pos + 1);
}

bool matching_rp_instance(const string& km_name,
                          std::shared_ptr<IRemotelyProvisionedComponent>* rp) {
    string km_suffix = device_suffix(km_name);

    vector<string> rp_names =
            ::android::getAidlHalInstanceNames(IRemotelyProvisionedComponent::descriptor);
    for (const string& rp_name : rp_names) {
        // If the suffix of the RemotelyProvisionedComponent instance equals the suffix of the
        // KeyMint instance, assume they match.
        if (device_suffix(rp_name) == km_suffix && AServiceManager_isDeclared(rp_name.c_str())) {
            ::ndk::SpAIBinder binder(AServiceManager_waitForService(rp_name.c_str()));
            *rp = IRemotelyProvisionedComponent::fromBinder(binder);
            return true;
        }
    }
    return false;
}

}  // namespace

class NewKeyGenerationTest : public KeyMintAidlTestBase {
  protected:
    void CheckBaseParams(const vector<KeyCharacteristics>& keyCharacteristics) {
        AuthorizationSet auths = CheckCommonParams(keyCharacteristics);
        EXPECT_TRUE(auths.Contains(TAG_PURPOSE, KeyPurpose::SIGN));

        // Check that some unexpected tags/values are NOT present.
        EXPECT_FALSE(auths.Contains(TAG_PURPOSE, KeyPurpose::ENCRYPT));
        EXPECT_FALSE(auths.Contains(TAG_PURPOSE, KeyPurpose::DECRYPT));
    }

    void CheckSymmetricParams(const vector<KeyCharacteristics>& keyCharacteristics) {
        AuthorizationSet auths = CheckCommonParams(keyCharacteristics);
        EXPECT_TRUE(auths.Contains(TAG_PURPOSE, KeyPurpose::ENCRYPT));
        EXPECT_TRUE(auths.Contains(TAG_PURPOSE, KeyPurpose::DECRYPT));

        EXPECT_FALSE(auths.Contains(TAG_PURPOSE, KeyPurpose::SIGN));
    }

    AuthorizationSet CheckCommonParams(const vector<KeyCharacteristics>& keyCharacteristics) {
        // TODO(swillden): Distinguish which params should be in which auth list.
        AuthorizationSet auths;
        for (auto& entry : keyCharacteristics) {
            auths.push_back(AuthorizationSet(entry.authorizations));
        }
        EXPECT_TRUE(auths.Contains(TAG_ORIGIN, KeyOrigin::GENERATED));

        // Verify that App data, ROT and auth timeout are NOT included.
        EXPECT_FALSE(auths.Contains(TAG_ROOT_OF_TRUST));
        EXPECT_FALSE(auths.Contains(TAG_APPLICATION_DATA));
        EXPECT_FALSE(auths.Contains(TAG_AUTH_TIMEOUT, 301U));

        // None of the tests specify CREATION_DATETIME so check that the KeyMint implementation
        // never adds it.
        EXPECT_FALSE(auths.Contains(TAG_CREATION_DATETIME));

        // Check OS details match the original hardware info.
        auto os_ver = auths.GetTagValue(TAG_OS_VERSION);
        EXPECT_TRUE(os_ver);
        EXPECT_EQ(*os_ver, os_version());
        auto os_pl = auths.GetTagValue(TAG_OS_PATCHLEVEL);
        EXPECT_TRUE(os_pl);
        EXPECT_EQ(*os_pl, os_patch_level());

        if (check_patchLevels) {
            // Should include vendor and boot patchlevels.
            auto vendor_pl = auths.GetTagValue(TAG_VENDOR_PATCHLEVEL);
            EXPECT_TRUE(vendor_pl);
            EXPECT_EQ(*vendor_pl, vendor_patch_level());
            auto boot_pl = auths.GetTagValue(TAG_BOOT_PATCHLEVEL);
            EXPECT_TRUE(boot_pl);
        }

        return auths;
    }
};

/*
 * NewKeyGenerationTest.Aes
 *
 * Verifies that keymint can generate all required AES key sizes, and that the resulting keys
 * have correct characteristics.
 */
TEST_P(NewKeyGenerationTest, Aes) {
    for (auto key_size : ValidKeySizes(Algorithm::AES)) {
        for (auto block_mode : ValidBlockModes(Algorithm::AES)) {
            for (auto padding_mode : ValidPaddingModes(Algorithm::AES, block_mode)) {
                SCOPED_TRACE(testing::Message()
                             << "AES-" << key_size << "-" << block_mode << "-" << padding_mode);
                vector<uint8_t> key_blob;
                vector<KeyCharacteristics> key_characteristics;
                auto builder = AuthorizationSetBuilder()
                                       .AesEncryptionKey(key_size)
                                       .BlockMode(block_mode)
                                       .Padding(padding_mode)
                                       .SetDefaultValidity();
                if (block_mode == BlockMode::GCM) {
                    builder.Authorization(TAG_MIN_MAC_LENGTH, 128);
                }
                ASSERT_EQ(ErrorCode::OK, GenerateKey(builder, &key_blob, &key_characteristics));

                EXPECT_GT(key_blob.size(), 0U);
                CheckSymmetricParams(key_characteristics);
                CheckCharacteristics(key_blob, key_characteristics);

                AuthorizationSet crypto_params = SecLevelAuthorizations(key_characteristics);

                EXPECT_TRUE(crypto_params.Contains(TAG_ALGORITHM, Algorithm::AES));
                EXPECT_TRUE(crypto_params.Contains(TAG_KEY_SIZE, key_size))
                        << "Key size " << key_size << "missing";

                CheckedDeleteKey(&key_blob);
            }
        }
    }
}

/*
 * NewKeyGenerationTest.AesInvalidSize
 *
 * Verifies that specifying an invalid key size for AES key generation returns
 * UNSUPPORTED_KEY_SIZE.
 */
TEST_P(NewKeyGenerationTest, AesInvalidSize) {
    for (auto key_size : InvalidKeySizes(Algorithm::AES)) {
        for (auto block_mode : ValidBlockModes(Algorithm::AES)) {
            for (auto padding_mode : ValidPaddingModes(Algorithm::AES, block_mode)) {
                SCOPED_TRACE(testing::Message()
                             << "AES-" << key_size << "-" << block_mode << "-" << padding_mode);
                vector<uint8_t> key_blob;
                vector<KeyCharacteristics> key_characteristics;
                auto builder = AuthorizationSetBuilder()
                                       .AesEncryptionKey(key_size)
                                       .BlockMode(block_mode)
                                       .Padding(padding_mode)
                                       .SetDefaultValidity();
                if (block_mode == BlockMode::GCM) {
                    builder.Authorization(TAG_MIN_MAC_LENGTH, 128);
                }
                EXPECT_EQ(ErrorCode::UNSUPPORTED_KEY_SIZE,
                          GenerateKey(builder, &key_blob, &key_characteristics));
            }
        }
    }

    for (auto block_mode : ValidBlockModes(Algorithm::AES)) {
        for (auto padding_mode : ValidPaddingModes(Algorithm::AES, block_mode)) {
            vector<uint8_t> key_blob;
            vector<KeyCharacteristics> key_characteristics;
            // No key size specified
            auto builder = AuthorizationSetBuilder()
                                   .Authorization(TAG_ALGORITHM, Algorithm::AES)
                                   .BlockMode(block_mode)
                                   .Padding(padding_mode)
                                   .SetDefaultValidity();
            if (block_mode == BlockMode::GCM) {
                builder.Authorization(TAG_MIN_MAC_LENGTH, 128);
            }
            EXPECT_EQ(ErrorCode::UNSUPPORTED_KEY_SIZE,
                      GenerateKey(builder, &key_blob, &key_characteristics));
        }
    }
}

/*
 * NewKeyGenerationTest.AesInvalidPadding
 *
 * Verifies that specifying an invalid padding on AES keys gives a failure
 * somewhere along the way.
 */
TEST_P(NewKeyGenerationTest, AesInvalidPadding) {
    for (auto key_size : ValidKeySizes(Algorithm::AES)) {
        for (auto block_mode : ValidBlockModes(Algorithm::AES)) {
            for (auto padding_mode : InvalidPaddingModes(Algorithm::AES, block_mode)) {
                SCOPED_TRACE(testing::Message()
                             << "AES-" << key_size << "-" << block_mode << "-" << padding_mode);
                auto builder = AuthorizationSetBuilder()
                                       .Authorization(TAG_NO_AUTH_REQUIRED)
                                       .AesEncryptionKey(key_size)
                                       .BlockMode(block_mode)
                                       .Padding(padding_mode)
                                       .SetDefaultValidity();
                if (block_mode == BlockMode::GCM) {
                    builder.Authorization(TAG_MIN_MAC_LENGTH, 128);
                }

                auto result = GenerateKey(builder);
                if (result == ErrorCode::OK) {
                    // Key creation was OK but has generated a key that cannot be used.
                    auto params =
                            AuthorizationSetBuilder().BlockMode(block_mode).Padding(padding_mode);
                    if (block_mode == BlockMode::GCM) {
                        params.Authorization(TAG_MAC_LENGTH, 128);
                    }
                    auto result = Begin(KeyPurpose::ENCRYPT, params);
                    EXPECT_TRUE(result == ErrorCode::INCOMPATIBLE_PADDING_MODE ||
                                result == ErrorCode::INVALID_KEY_BLOB)
                            << "unexpected result: " << result;
                } else {
                    // The KeyMint implementation detected that the generated key
                    // is unusable.
                    EXPECT_EQ(ErrorCode::INCOMPATIBLE_PADDING_MODE, result);
                }
            }
        }
    }
}

/*
 * NewKeyGenerationTest.AesGcmMissingMinMac
 *
 * Verifies that specifying an invalid key size for AES key generation returns
 * UNSUPPORTED_KEY_SIZE.
 */
TEST_P(NewKeyGenerationTest, AesGcmMissingMinMac) {
    for (auto key_size : ValidKeySizes(Algorithm::AES)) {
        BlockMode block_mode = BlockMode::GCM;
        for (auto padding_mode : ValidPaddingModes(Algorithm::AES, block_mode)) {
            SCOPED_TRACE(testing::Message()
                         << "AES-" << key_size << "-" << block_mode << "-" << padding_mode);
            vector<uint8_t> key_blob;
            vector<KeyCharacteristics> key_characteristics;
            // No MIN_MAC_LENGTH provided.
            auto builder = AuthorizationSetBuilder()
                                   .AesEncryptionKey(key_size)
                                   .BlockMode(block_mode)
                                   .Padding(padding_mode)
                                   .SetDefaultValidity();
            EXPECT_EQ(ErrorCode::MISSING_MIN_MAC_LENGTH,
                      GenerateKey(builder, &key_blob, &key_characteristics));
        }
    }
}

/*
 * NewKeyGenerationTest.AesGcmMinMacOutOfRange
 *
 * Verifies that specifying an invalid min MAC size for AES key generation returns
 * UNSUPPORTED_MIN_MAC_LENGTH.
 */
TEST_P(NewKeyGenerationTest, AesGcmMinMacOutOfRange) {
    for (size_t min_mac_len : {88, 136}) {
        for (auto key_size : ValidKeySizes(Algorithm::AES)) {
            BlockMode block_mode = BlockMode::GCM;
            for (auto padding_mode : ValidPaddingModes(Algorithm::AES, block_mode)) {
                SCOPED_TRACE(testing::Message()
                             << "AES-" << key_size << "-" << block_mode << "-" << padding_mode);
                vector<uint8_t> key_blob;
                vector<KeyCharacteristics> key_characteristics;
                auto builder = AuthorizationSetBuilder()
                                       .AesEncryptionKey(key_size)
                                       .BlockMode(block_mode)
                                       .Padding(padding_mode)
                                       .Authorization(TAG_MIN_MAC_LENGTH, min_mac_len)
                                       .SetDefaultValidity();
                EXPECT_EQ(ErrorCode::UNSUPPORTED_MIN_MAC_LENGTH,
                          GenerateKey(builder, &key_blob, &key_characteristics));
            }
        }
    }
}

/*
 * NewKeyGenerationTest.TripleDes
 *
 * Verifies that keymint can generate all required 3DES key sizes, and that the resulting keys
 * have correct characteristics.
 */
TEST_P(NewKeyGenerationTest, TripleDes) {
    for (auto key_size : ValidKeySizes(Algorithm::TRIPLE_DES)) {
        for (auto block_mode : ValidBlockModes(Algorithm::TRIPLE_DES)) {
            for (auto padding_mode : ValidPaddingModes(Algorithm::AES, block_mode)) {
                SCOPED_TRACE(testing::Message()
                             << "3DES-" << key_size << "-" << block_mode << "-" << padding_mode);
                vector<uint8_t> key_blob;
                vector<KeyCharacteristics> key_characteristics;
                ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                             .TripleDesEncryptionKey(key_size)
                                                             .BlockMode(block_mode)
                                                             .Padding(padding_mode)
                                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                                             .SetDefaultValidity(),
                                                     &key_blob, &key_characteristics));

                EXPECT_GT(key_blob.size(), 0U);
                CheckSymmetricParams(key_characteristics);
                CheckCharacteristics(key_blob, key_characteristics);

                AuthorizationSet crypto_params = SecLevelAuthorizations(key_characteristics);

                EXPECT_TRUE(crypto_params.Contains(TAG_ALGORITHM, Algorithm::TRIPLE_DES));
                EXPECT_TRUE(crypto_params.Contains(TAG_KEY_SIZE, key_size))
                        << "Key size " << key_size << "missing";

                CheckedDeleteKey(&key_blob);
            }
        }
    }
}

/*
 * NewKeyGenerationTest.TripleDesWithAttestation
 *
 * Verifies that keymint can generate all required 3DES key sizes, and that the resulting keys
 * have correct characteristics.
 *
 * Request attestation, which doesn't help for symmetric keys (as there is no public key to
 * put in a certificate) but which isn't an error.
 */
TEST_P(NewKeyGenerationTest, TripleDesWithAttestation) {
    for (auto key_size : ValidKeySizes(Algorithm::TRIPLE_DES)) {
        for (auto block_mode : ValidBlockModes(Algorithm::TRIPLE_DES)) {
            for (auto padding_mode : ValidPaddingModes(Algorithm::AES, block_mode)) {
                SCOPED_TRACE(testing::Message()
                             << "3DES-" << key_size << "-" << block_mode << "-" << padding_mode);

                auto challenge = "hello";
                auto app_id = "foo";

                vector<uint8_t> key_blob;
                vector<KeyCharacteristics> key_characteristics;
                ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                             .TripleDesEncryptionKey(key_size)
                                                             .BlockMode(block_mode)
                                                             .Padding(padding_mode)
                                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                                             .AttestationChallenge(challenge)
                                                             .AttestationApplicationId(app_id)
                                                             .SetDefaultValidity(),
                                                     &key_blob, &key_characteristics));

                EXPECT_GT(key_blob.size(), 0U);
                CheckSymmetricParams(key_characteristics);
                CheckCharacteristics(key_blob, key_characteristics);

                AuthorizationSet crypto_params = SecLevelAuthorizations(key_characteristics);

                EXPECT_TRUE(crypto_params.Contains(TAG_ALGORITHM, Algorithm::TRIPLE_DES));
                EXPECT_TRUE(crypto_params.Contains(TAG_KEY_SIZE, key_size))
                        << "Key size " << key_size << "missing";

                CheckedDeleteKey(&key_blob);
            }
        }
    }
}

/*
 * NewKeyGenerationTest.TripleDesInvalidSize
 *
 * Verifies that specifying an invalid key size for 3-DES key generation returns
 * UNSUPPORTED_KEY_SIZE.
 */
TEST_P(NewKeyGenerationTest, TripleDesInvalidSize) {
    for (auto key_size : InvalidKeySizes(Algorithm::TRIPLE_DES)) {
        for (auto block_mode : ValidBlockModes(Algorithm::TRIPLE_DES)) {
            for (auto padding_mode : ValidPaddingModes(Algorithm::AES, block_mode)) {
                SCOPED_TRACE(testing::Message()
                             << "3DES-" << key_size << "-" << block_mode << "-" << padding_mode);
                vector<uint8_t> key_blob;
                vector<KeyCharacteristics> key_characteristics;
                EXPECT_EQ(ErrorCode::UNSUPPORTED_KEY_SIZE,
                          GenerateKey(AuthorizationSetBuilder()
                                              .TripleDesEncryptionKey(key_size)
                                              .BlockMode(block_mode)
                                              .Padding(padding_mode)
                                              .Authorization(TAG_NO_AUTH_REQUIRED)
                                              .SetDefaultValidity(),
                                      &key_blob, &key_characteristics));
            }
        }
    }

    // Omitting the key size fails.
    for (auto block_mode : ValidBlockModes(Algorithm::TRIPLE_DES)) {
        for (auto padding_mode : ValidPaddingModes(Algorithm::AES, block_mode)) {
            SCOPED_TRACE(testing::Message()
                         << "3DES-default-" << block_mode << "-" << padding_mode);
            vector<uint8_t> key_blob;
            vector<KeyCharacteristics> key_characteristics;
            ASSERT_EQ(ErrorCode::UNSUPPORTED_KEY_SIZE,
                      GenerateKey(AuthorizationSetBuilder()
                                          .Authorization(TAG_ALGORITHM, Algorithm::TRIPLE_DES)
                                          .BlockMode(block_mode)
                                          .Padding(padding_mode)
                                          .Authorization(TAG_NO_AUTH_REQUIRED)
                                          .SetDefaultValidity(),
                                  &key_blob, &key_characteristics));
        }
    }
}

/*
 * NewKeyGenerationTest.Rsa
 *
 * Verifies that keymint can generate all required RSA key sizes, and that the resulting keys
 * have correct characteristics.
 */
TEST_P(NewKeyGenerationTest, Rsa) {
    for (auto key_size : ValidKeySizes(Algorithm::RSA)) {
        vector<uint8_t> key_blob;
        vector<KeyCharacteristics> key_characteristics;
        ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                     .RsaSigningKey(key_size, 65537)
                                                     .Digest(Digest::NONE)
                                                     .Padding(PaddingMode::NONE)
                                                     .SetDefaultValidity(),
                                             &key_blob, &key_characteristics));

        ASSERT_GT(key_blob.size(), 0U);
        CheckBaseParams(key_characteristics);
        CheckCharacteristics(key_blob, key_characteristics);

        AuthorizationSet crypto_params = SecLevelAuthorizations(key_characteristics);

        EXPECT_TRUE(crypto_params.Contains(TAG_ALGORITHM, Algorithm::RSA));
        EXPECT_TRUE(crypto_params.Contains(TAG_KEY_SIZE, key_size))
                << "Key size " << key_size << "missing";
        EXPECT_TRUE(crypto_params.Contains(TAG_RSA_PUBLIC_EXPONENT, 65537U));

        CheckedDeleteKey(&key_blob);
    }
}

/*
 * NewKeyGenerationTest.RsaWithAttestation
 *
 * Verifies that keymint can generate all required RSA key sizes with attestation, and that the
 * resulting keys have correct characteristics.
 */
TEST_P(NewKeyGenerationTest, RsaWithAttestation) {
    auto challenge = "hello";
    auto app_id = "foo";

    auto subject = "cert subj 2";
    vector<uint8_t> subject_der(make_name_from_str(subject));

    uint64_t serial_int = 66;
    vector<uint8_t> serial_blob(build_serial_blob(serial_int));

    for (auto key_size : ValidKeySizes(Algorithm::RSA)) {
        vector<uint8_t> key_blob;
        vector<KeyCharacteristics> key_characteristics;
        ASSERT_EQ(ErrorCode::OK,
                  GenerateKey(AuthorizationSetBuilder()
                                      .RsaSigningKey(key_size, 65537)
                                      .Digest(Digest::NONE)
                                      .Padding(PaddingMode::NONE)
                                      .AttestationChallenge(challenge)
                                      .AttestationApplicationId(app_id)
                                      .Authorization(TAG_NO_AUTH_REQUIRED)
                                      .Authorization(TAG_CERTIFICATE_SERIAL, serial_blob)
                                      .Authorization(TAG_CERTIFICATE_SUBJECT, subject_der)
                                      .SetDefaultValidity(),
                              &key_blob, &key_characteristics));

        ASSERT_GT(key_blob.size(), 0U);
        CheckBaseParams(key_characteristics);
        CheckCharacteristics(key_blob, key_characteristics);

        AuthorizationSet crypto_params = SecLevelAuthorizations(key_characteristics);

        EXPECT_TRUE(crypto_params.Contains(TAG_ALGORITHM, Algorithm::RSA));
        EXPECT_TRUE(crypto_params.Contains(TAG_KEY_SIZE, key_size))
                << "Key size " << key_size << "missing";
        EXPECT_TRUE(crypto_params.Contains(TAG_RSA_PUBLIC_EXPONENT, 65537U));

        verify_subject_and_serial(cert_chain_[0], serial_int, subject, false);
        EXPECT_TRUE(ChainSignaturesAreValid(cert_chain_));
        ASSERT_GT(cert_chain_.size(), 0);

        AuthorizationSet hw_enforced = HwEnforcedAuthorizations(key_characteristics);
        AuthorizationSet sw_enforced = SwEnforcedAuthorizations(key_characteristics);
        EXPECT_TRUE(verify_attestation_record(challenge, app_id,  //
                                              sw_enforced, hw_enforced, SecLevel(),
                                              cert_chain_[0].encodedCertificate));

        CheckedDeleteKey(&key_blob);
    }
}

/*
 * NewKeyGenerationTest.RsaWithRpkAttestation
 *
 * Verifies that keymint can generate all required RSA key sizes, using an attestation key
 * that has been generated using an associate IRemotelyProvisionedComponent.
 *
 * This test is disabled because the KeyMint specification does not require that implementations
 * of the first version of KeyMint have to also implement IRemotelyProvisionedComponent.
 * However, the test is kept in the code because KeyMint v2 will impose this requirement.
 */
TEST_P(NewKeyGenerationTest, DISABLED_RsaWithRpkAttestation) {
    // There should be an IRemotelyProvisionedComponent instance associated with the KeyMint
    // instance.
    std::shared_ptr<IRemotelyProvisionedComponent> rp;
    ASSERT_TRUE(matching_rp_instance(GetParam(), &rp))
            << "No IRemotelyProvisionedComponent found that matches KeyMint device " << GetParam();

    // Generate a P-256 keypair to use as an attestation key.
    MacedPublicKey macedPubKey;
    std::vector<uint8_t> privateKeyBlob;
    auto status =
            rp->generateEcdsaP256KeyPair(/* testMode= */ false, &macedPubKey, &privateKeyBlob);
    ASSERT_TRUE(status.isOk());
    vector<uint8_t> coseKeyData;
    check_maced_pubkey(macedPubKey, /* testMode= */ false, &coseKeyData);

    AttestationKey attestation_key;
    attestation_key.keyBlob = std::move(privateKeyBlob);
    attestation_key.issuerSubjectName = make_name_from_str("Android Keystore Key");

    for (auto key_size : ValidKeySizes(Algorithm::RSA)) {
        auto challenge = "hello";
        auto app_id = "foo";

        vector<uint8_t> key_blob;
        vector<KeyCharacteristics> key_characteristics;
        ASSERT_EQ(ErrorCode::OK,
                  GenerateKey(AuthorizationSetBuilder()
                                      .RsaSigningKey(key_size, 65537)
                                      .Digest(Digest::NONE)
                                      .Padding(PaddingMode::NONE)
                                      .AttestationChallenge(challenge)
                                      .AttestationApplicationId(app_id)
                                      .Authorization(TAG_NO_AUTH_REQUIRED)
                                      .SetDefaultValidity(),
                              attestation_key, &key_blob, &key_characteristics, &cert_chain_));

        ASSERT_GT(key_blob.size(), 0U);
        CheckBaseParams(key_characteristics);
        CheckCharacteristics(key_blob, key_characteristics);

        AuthorizationSet crypto_params = SecLevelAuthorizations(key_characteristics);

        EXPECT_TRUE(crypto_params.Contains(TAG_ALGORITHM, Algorithm::RSA));
        EXPECT_TRUE(crypto_params.Contains(TAG_KEY_SIZE, key_size))
                << "Key size " << key_size << "missing";
        EXPECT_TRUE(crypto_params.Contains(TAG_RSA_PUBLIC_EXPONENT, 65537U));

        // Attestation by itself is not valid (last entry is not self-signed).
        EXPECT_FALSE(ChainSignaturesAreValid(cert_chain_));

        // The signature over the attested key should correspond to the P256 public key.
        X509_Ptr key_cert(parse_cert_blob(cert_chain_[0].encodedCertificate));
        ASSERT_TRUE(key_cert.get());
        EVP_PKEY_Ptr signing_pubkey;
        p256_pub_key(coseKeyData, &signing_pubkey);
        ASSERT_TRUE(signing_pubkey.get());

        ASSERT_TRUE(X509_verify(key_cert.get(), signing_pubkey.get()))
                << "Verification of attested certificate failed "
                << "OpenSSL error string: " << ERR_error_string(ERR_get_error(), NULL);

        CheckedDeleteKey(&key_blob);
    }
}

/*
 * NewKeyGenerationTest.RsaEncryptionWithAttestation
 *
 * Verifies that keymint attestation for RSA encryption keys with challenge and
 * app id is also successful.
 */
TEST_P(NewKeyGenerationTest, RsaEncryptionWithAttestation) {
    auto key_size = 2048;
    auto challenge = "hello";
    auto app_id = "foo";

    auto subject = "subj 2";
    vector<uint8_t> subject_der(make_name_from_str(subject));

    uint64_t serial_int = 111166;
    vector<uint8_t> serial_blob(build_serial_blob(serial_int));

    vector<uint8_t> key_blob;
    vector<KeyCharacteristics> key_characteristics;
    ASSERT_EQ(ErrorCode::OK,
              GenerateKey(AuthorizationSetBuilder()
                                  .RsaEncryptionKey(key_size, 65537)
                                  .Padding(PaddingMode::NONE)
                                  .AttestationChallenge(challenge)
                                  .AttestationApplicationId(app_id)
                                  .Authorization(TAG_NO_AUTH_REQUIRED)
                                  .Authorization(TAG_CERTIFICATE_SERIAL, serial_blob)
                                  .Authorization(TAG_CERTIFICATE_SUBJECT, subject_der)
                                  .SetDefaultValidity(),
                          &key_blob, &key_characteristics));

    ASSERT_GT(key_blob.size(), 0U);
    AuthorizationSet auths;
    for (auto& entry : key_characteristics) {
        auths.push_back(AuthorizationSet(entry.authorizations));
    }

    EXPECT_TRUE(auths.Contains(TAG_ORIGIN, KeyOrigin::GENERATED));
    EXPECT_TRUE(auths.Contains(TAG_PURPOSE, KeyPurpose::DECRYPT));

    // Verify that App data and ROT are NOT included.
    EXPECT_FALSE(auths.Contains(TAG_ROOT_OF_TRUST));
    EXPECT_FALSE(auths.Contains(TAG_APPLICATION_DATA));

    // Check that some unexpected tags/values are NOT present.
    EXPECT_FALSE(auths.Contains(TAG_PURPOSE, KeyPurpose::SIGN));
    EXPECT_FALSE(auths.Contains(TAG_PURPOSE, KeyPurpose::VERIFY));

    EXPECT_FALSE(auths.Contains(TAG_AUTH_TIMEOUT, 301U));

    auto os_ver = auths.GetTagValue(TAG_OS_VERSION);
    ASSERT_TRUE(os_ver);
    EXPECT_EQ(*os_ver, os_version());

    AuthorizationSet crypto_params = SecLevelAuthorizations(key_characteristics);

    EXPECT_TRUE(crypto_params.Contains(TAG_ALGORITHM, Algorithm::RSA));
    EXPECT_TRUE(crypto_params.Contains(TAG_KEY_SIZE, key_size))
            << "Key size " << key_size << "missing";
    EXPECT_TRUE(crypto_params.Contains(TAG_RSA_PUBLIC_EXPONENT, 65537U));

    verify_subject_and_serial(cert_chain_[0], serial_int, subject, false);
    EXPECT_TRUE(ChainSignaturesAreValid(cert_chain_));
    ASSERT_GT(cert_chain_.size(), 0);

    AuthorizationSet hw_enforced = HwEnforcedAuthorizations(key_characteristics);
    AuthorizationSet sw_enforced = SwEnforcedAuthorizations(key_characteristics);
    EXPECT_TRUE(verify_attestation_record(challenge, app_id,  //
                                          sw_enforced, hw_enforced, SecLevel(),
                                          cert_chain_[0].encodedCertificate));

    CheckedDeleteKey(&key_blob);
}

/*
 * NewKeyGenerationTest.RsaWithSelfSign
 *
 * Verifies that attesting to RSA key generation is successful, and returns
 * self signed certificate if no challenge is provided.  And signing etc
 * works as expected.
 */
TEST_P(NewKeyGenerationTest, RsaWithSelfSign) {
    auto subject = "cert subj subj subj subj subj subj 22222222222222222222";
    vector<uint8_t> subject_der(make_name_from_str(subject));

    uint64_t serial_int = 0;
    vector<uint8_t> serial_blob(build_serial_blob(serial_int));

    for (auto key_size : ValidKeySizes(Algorithm::RSA)) {
        vector<uint8_t> key_blob;
        vector<KeyCharacteristics> key_characteristics;
        ASSERT_EQ(ErrorCode::OK,
                  GenerateKey(AuthorizationSetBuilder()
                                      .RsaSigningKey(key_size, 65537)
                                      .Digest(Digest::NONE)
                                      .Padding(PaddingMode::NONE)
                                      .Authorization(TAG_NO_AUTH_REQUIRED)
                                      .Authorization(TAG_CERTIFICATE_SERIAL, serial_blob)
                                      .Authorization(TAG_CERTIFICATE_SUBJECT, subject_der)
                                      .SetDefaultValidity(),
                              &key_blob, &key_characteristics));

        ASSERT_GT(key_blob.size(), 0U);
        CheckBaseParams(key_characteristics);
        CheckCharacteristics(key_blob, key_characteristics);

        AuthorizationSet crypto_params = SecLevelAuthorizations(key_characteristics);

        EXPECT_TRUE(crypto_params.Contains(TAG_ALGORITHM, Algorithm::RSA));
        EXPECT_TRUE(crypto_params.Contains(TAG_KEY_SIZE, key_size))
                << "Key size " << key_size << "missing";
        EXPECT_TRUE(crypto_params.Contains(TAG_RSA_PUBLIC_EXPONENT, 65537U));

        verify_subject_and_serial(cert_chain_[0], serial_int, subject, false);
        EXPECT_TRUE(ChainSignaturesAreValid(cert_chain_));
        ASSERT_EQ(cert_chain_.size(), 1);

        CheckedDeleteKey(&key_blob);
    }
}

/*
 * NewKeyGenerationTest.RsaWithAttestationMissAppId
 *
 * Verifies that attesting to RSA checks for missing app ID.
 */
TEST_P(NewKeyGenerationTest, RsaWithAttestationMissAppId) {
    auto challenge = "hello";
    vector<uint8_t> key_blob;
    vector<KeyCharacteristics> key_characteristics;

    ASSERT_EQ(ErrorCode::ATTESTATION_APPLICATION_ID_MISSING,
              GenerateKey(AuthorizationSetBuilder()
                                  .RsaSigningKey(2048, 65537)
                                  .Digest(Digest::NONE)
                                  .Padding(PaddingMode::NONE)
                                  .AttestationChallenge(challenge)
                                  .Authorization(TAG_NO_AUTH_REQUIRED)
                                  .SetDefaultValidity(),
                          &key_blob, &key_characteristics));
}

/*
 * NewKeyGenerationTest.RsaWithAttestationAppIdIgnored
 *
 * Verifies that attesting to RSA ignores app id if challenge is missing.
 */
TEST_P(NewKeyGenerationTest, RsaWithAttestationAppIdIgnored) {
    auto key_size = 2048;
    auto app_id = "foo";

    auto subject = "cert subj 2";
    vector<uint8_t> subject_der(make_name_from_str(subject));

    uint64_t serial_int = 1;
    vector<uint8_t> serial_blob(build_serial_blob(serial_int));

    vector<uint8_t> key_blob;
    vector<KeyCharacteristics> key_characteristics;
    ASSERT_EQ(ErrorCode::OK,
              GenerateKey(AuthorizationSetBuilder()
                                  .RsaSigningKey(key_size, 65537)
                                  .Digest(Digest::NONE)
                                  .Padding(PaddingMode::NONE)
                                  .AttestationApplicationId(app_id)
                                  .Authorization(TAG_NO_AUTH_REQUIRED)
                                  .Authorization(TAG_CERTIFICATE_SERIAL, serial_blob)
                                  .Authorization(TAG_CERTIFICATE_SUBJECT, subject_der)
                                  .SetDefaultValidity(),
                          &key_blob, &key_characteristics));

    ASSERT_GT(key_blob.size(), 0U);
    CheckBaseParams(key_characteristics);
    CheckCharacteristics(key_blob, key_characteristics);

    AuthorizationSet crypto_params = SecLevelAuthorizations(key_characteristics);

    EXPECT_TRUE(crypto_params.Contains(TAG_ALGORITHM, Algorithm::RSA));
    EXPECT_TRUE(crypto_params.Contains(TAG_KEY_SIZE, key_size))
            << "Key size " << key_size << "missing";
    EXPECT_TRUE(crypto_params.Contains(TAG_RSA_PUBLIC_EXPONENT, 65537U));

    verify_subject_and_serial(cert_chain_[0], serial_int, subject, false);
    EXPECT_TRUE(ChainSignaturesAreValid(cert_chain_));
    ASSERT_EQ(cert_chain_.size(), 1);

    CheckedDeleteKey(&key_blob);
}

/*
 * NewKeyGenerationTest.LimitedUsageRsa
 *
 * Verifies that KeyMint can generate all required RSA key sizes with limited usage, and that the
 * resulting keys have correct characteristics.
 */
TEST_P(NewKeyGenerationTest, LimitedUsageRsa) {
    for (auto key_size : ValidKeySizes(Algorithm::RSA)) {
        vector<uint8_t> key_blob;
        vector<KeyCharacteristics> key_characteristics;
        ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                     .RsaSigningKey(key_size, 65537)
                                                     .Digest(Digest::NONE)
                                                     .Padding(PaddingMode::NONE)
                                                     .Authorization(TAG_USAGE_COUNT_LIMIT, 1)
                                                     .SetDefaultValidity(),
                                             &key_blob, &key_characteristics));

        ASSERT_GT(key_blob.size(), 0U);
        CheckBaseParams(key_characteristics);
        CheckCharacteristics(key_blob, key_characteristics);

        AuthorizationSet crypto_params = SecLevelAuthorizations(key_characteristics);

        EXPECT_TRUE(crypto_params.Contains(TAG_ALGORITHM, Algorithm::RSA));
        EXPECT_TRUE(crypto_params.Contains(TAG_KEY_SIZE, key_size))
                << "Key size " << key_size << "missing";
        EXPECT_TRUE(crypto_params.Contains(TAG_RSA_PUBLIC_EXPONENT, 65537U));

        // Check the usage count limit tag appears in the authorizations.
        AuthorizationSet auths;
        for (auto& entry : key_characteristics) {
            auths.push_back(AuthorizationSet(entry.authorizations));
        }
        EXPECT_TRUE(auths.Contains(TAG_USAGE_COUNT_LIMIT, 1U))
                << "key usage count limit " << 1U << " missing";

        CheckedDeleteKey(&key_blob);
    }
}

/*
 * NewKeyGenerationTest.LimitedUsageRsaWithAttestation
 *
 * Verifies that KeyMint can generate all required RSA key sizes with limited usage, and that the
 * resulting keys have correct characteristics and attestation.
 */
TEST_P(NewKeyGenerationTest, LimitedUsageRsaWithAttestation) {
    auto challenge = "hello";
    auto app_id = "foo";

    auto subject = "cert subj 2";
    vector<uint8_t> subject_der(make_name_from_str(subject));

    uint64_t serial_int = 66;
    vector<uint8_t> serial_blob(build_serial_blob(serial_int));

    for (auto key_size : ValidKeySizes(Algorithm::RSA)) {
        vector<uint8_t> key_blob;
        vector<KeyCharacteristics> key_characteristics;
        ASSERT_EQ(ErrorCode::OK,
                  GenerateKey(AuthorizationSetBuilder()
                                      .RsaSigningKey(key_size, 65537)
                                      .Digest(Digest::NONE)
                                      .Padding(PaddingMode::NONE)
                                      .AttestationChallenge(challenge)
                                      .AttestationApplicationId(app_id)
                                      .Authorization(TAG_NO_AUTH_REQUIRED)
                                      .Authorization(TAG_USAGE_COUNT_LIMIT, 1)
                                      .Authorization(TAG_CERTIFICATE_SERIAL, serial_blob)
                                      .Authorization(TAG_CERTIFICATE_SUBJECT, subject_der)
                                      .SetDefaultValidity(),
                              &key_blob, &key_characteristics));

        ASSERT_GT(key_blob.size(), 0U);
        CheckBaseParams(key_characteristics);
        CheckCharacteristics(key_blob, key_characteristics);

        AuthorizationSet crypto_params = SecLevelAuthorizations(key_characteristics);

        EXPECT_TRUE(crypto_params.Contains(TAG_ALGORITHM, Algorithm::RSA));
        EXPECT_TRUE(crypto_params.Contains(TAG_KEY_SIZE, key_size))
                << "Key size " << key_size << "missing";
        EXPECT_TRUE(crypto_params.Contains(TAG_RSA_PUBLIC_EXPONENT, 65537U));

        // Check the usage count limit tag appears in the authorizations.
        AuthorizationSet auths;
        for (auto& entry : key_characteristics) {
            auths.push_back(AuthorizationSet(entry.authorizations));
        }
        EXPECT_TRUE(auths.Contains(TAG_USAGE_COUNT_LIMIT, 1U))
                << "key usage count limit " << 1U << " missing";

        // Check the usage count limit tag also appears in the attestation.
        EXPECT_TRUE(ChainSignaturesAreValid(cert_chain_));
        ASSERT_GT(cert_chain_.size(), 0);
        verify_subject_and_serial(cert_chain_[0], serial_int, subject, false);

        AuthorizationSet hw_enforced = HwEnforcedAuthorizations(key_characteristics);
        AuthorizationSet sw_enforced = SwEnforcedAuthorizations(key_characteristics);
        EXPECT_TRUE(verify_attestation_record(challenge, app_id,  //
                                              sw_enforced, hw_enforced, SecLevel(),
                                              cert_chain_[0].encodedCertificate));

        CheckedDeleteKey(&key_blob);
    }
}

/*
 * NewKeyGenerationTest.NoInvalidRsaSizes
 *
 * Verifies that keymint cannot generate any RSA key sizes that are designated as invalid.
 */
TEST_P(NewKeyGenerationTest, NoInvalidRsaSizes) {
    for (auto key_size : InvalidKeySizes(Algorithm::RSA)) {
        vector<uint8_t> key_blob;
        vector<KeyCharacteristics> key_characteristics;
        ASSERT_EQ(ErrorCode::UNSUPPORTED_KEY_SIZE,
                  GenerateKey(AuthorizationSetBuilder()
                                      .RsaSigningKey(key_size, 65537)
                                      .Digest(Digest::NONE)
                                      .Padding(PaddingMode::NONE)
                                      .SetDefaultValidity(),
                              &key_blob, &key_characteristics));
    }
}

/*
 * NewKeyGenerationTest.RsaNoDefaultSize
 *
 * Verifies that failing to specify a key size for RSA key generation returns
 * UNSUPPORTED_KEY_SIZE.
 */
TEST_P(NewKeyGenerationTest, RsaNoDefaultSize) {
    ASSERT_EQ(ErrorCode::UNSUPPORTED_KEY_SIZE,
              GenerateKey(AuthorizationSetBuilder()
                                  .Authorization(TAG_ALGORITHM, Algorithm::RSA)
                                  .Authorization(TAG_RSA_PUBLIC_EXPONENT, 3U)
                                  .SigningKey()
                                  .SetDefaultValidity()));
}

/*
 * NewKeyGenerationTest.RsaMissingParams
 *
 * Verifies that omitting optional tags works.
 */
TEST_P(NewKeyGenerationTest, RsaMissingParams) {
    for (auto key_size : ValidKeySizes(Algorithm::RSA)) {
        ASSERT_EQ(ErrorCode::OK,
                  GenerateKey(
                          AuthorizationSetBuilder().RsaKey(key_size, 65537).SetDefaultValidity()));
        CheckedDeleteKey();
    }
}

/*
 * NewKeyGenerationTest.Ecdsa
 *
 * Verifies that keymint can generate all required EC key sizes, and that the resulting keys
 * have correct characteristics.
 */
TEST_P(NewKeyGenerationTest, Ecdsa) {
    for (auto curve : ValidCurves()) {
        vector<uint8_t> key_blob;
        vector<KeyCharacteristics> key_characteristics;
        ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                     .EcdsaSigningKey(curve)
                                                     .Digest(Digest::NONE)
                                                     .SetDefaultValidity(),
                                             &key_blob, &key_characteristics));
        ASSERT_GT(key_blob.size(), 0U);
        CheckBaseParams(key_characteristics);
        CheckCharacteristics(key_blob, key_characteristics);

        AuthorizationSet crypto_params = SecLevelAuthorizations(key_characteristics);

        EXPECT_TRUE(crypto_params.Contains(TAG_ALGORITHM, Algorithm::EC));
        EXPECT_TRUE(crypto_params.Contains(TAG_EC_CURVE, curve)) << "Curve " << curve << "missing";

        CheckedDeleteKey(&key_blob);
    }
}

/*
 * NewKeyGenerationTest.EcdsaAttestation
 *
 * Verifies that for all Ecdsa key sizes, if challenge and app id is provided,
 * an attestation will be generated.
 */
TEST_P(NewKeyGenerationTest, EcdsaAttestation) {
    auto challenge = "hello";
    auto app_id = "foo";

    auto subject = "cert subj 2";
    vector<uint8_t> subject_der(make_name_from_str(subject));

    uint64_t serial_int = 0xFFFFFFFFFFFFFFFF;
    vector<uint8_t> serial_blob(build_serial_blob(serial_int));

    for (auto curve : ValidCurves()) {
        vector<uint8_t> key_blob;
        vector<KeyCharacteristics> key_characteristics;
        ASSERT_EQ(ErrorCode::OK,
                  GenerateKey(AuthorizationSetBuilder()
                                      .Authorization(TAG_NO_AUTH_REQUIRED)
                                      .EcdsaSigningKey(curve)
                                      .Digest(Digest::NONE)
                                      .AttestationChallenge(challenge)
                                      .AttestationApplicationId(app_id)
                                      .Authorization(TAG_CERTIFICATE_SERIAL, serial_blob)
                                      .Authorization(TAG_CERTIFICATE_SUBJECT, subject_der)
                                      .SetDefaultValidity(),
                              &key_blob, &key_characteristics));
        ASSERT_GT(key_blob.size(), 0U);
        CheckBaseParams(key_characteristics);
        CheckCharacteristics(key_blob, key_characteristics);

        AuthorizationSet crypto_params = SecLevelAuthorizations(key_characteristics);

        EXPECT_TRUE(crypto_params.Contains(TAG_ALGORITHM, Algorithm::EC));
        EXPECT_TRUE(crypto_params.Contains(TAG_EC_CURVE, curve)) << "Curve " << curve << "missing";

        EXPECT_TRUE(ChainSignaturesAreValid(cert_chain_));
        ASSERT_GT(cert_chain_.size(), 0);
        verify_subject_and_serial(cert_chain_[0], serial_int, subject, false);

        AuthorizationSet hw_enforced = HwEnforcedAuthorizations(key_characteristics);
        AuthorizationSet sw_enforced = SwEnforcedAuthorizations(key_characteristics);
        EXPECT_TRUE(verify_attestation_record(challenge, app_id,  //
                                              sw_enforced, hw_enforced, SecLevel(),
                                              cert_chain_[0].encodedCertificate));

        CheckedDeleteKey(&key_blob);
    }
}

/*
 * NewKeyGenerationTest.EcdsaAttestationTags
 *
 * Verifies that creation of an attested ECDSA key includes various tags in the
 * attestation extension.
 */
TEST_P(NewKeyGenerationTest, EcdsaAttestationTags) {
    auto challenge = "hello";
    auto app_id = "foo";
    auto subject = "cert subj 2";
    vector<uint8_t> subject_der(make_name_from_str(subject));
    uint64_t serial_int = 0x1010;
    vector<uint8_t> serial_blob(build_serial_blob(serial_int));
    const AuthorizationSetBuilder base_builder =
            AuthorizationSetBuilder()
                    .Authorization(TAG_NO_AUTH_REQUIRED)
                    .EcdsaSigningKey(EcCurve::P_256)
                    .Digest(Digest::NONE)
                    .AttestationChallenge(challenge)
                    .AttestationApplicationId(app_id)
                    .Authorization(TAG_CERTIFICATE_SERIAL, serial_blob)
                    .Authorization(TAG_CERTIFICATE_SUBJECT, subject_der)
                    .SetDefaultValidity();

    // Various tags that map to fields in the attestation extension ASN.1 schema.
    auto extra_tags = AuthorizationSetBuilder()
                              .Authorization(TAG_ROLLBACK_RESISTANCE)
                              .Authorization(TAG_EARLY_BOOT_ONLY)
                              .Authorization(TAG_ACTIVE_DATETIME, 1619621648000)
                              .Authorization(TAG_ORIGINATION_EXPIRE_DATETIME, 1619621648000)
                              .Authorization(TAG_USAGE_EXPIRE_DATETIME, 1619621999000)
                              .Authorization(TAG_USAGE_COUNT_LIMIT, 42)
                              .Authorization(TAG_AUTH_TIMEOUT, 100000)
                              .Authorization(TAG_ALLOW_WHILE_ON_BODY)
                              .Authorization(TAG_TRUSTED_USER_PRESENCE_REQUIRED)
                              .Authorization(TAG_TRUSTED_CONFIRMATION_REQUIRED)
                              .Authorization(TAG_UNLOCKED_DEVICE_REQUIRED)
                              .Authorization(TAG_CREATION_DATETIME, 1619621648000);
    for (const KeyParameter& tag : extra_tags) {
        SCOPED_TRACE(testing::Message() << "tag-" << tag);
        vector<uint8_t> key_blob;
        vector<KeyCharacteristics> key_characteristics;
        AuthorizationSetBuilder builder = base_builder;
        builder.push_back(tag);
        auto result = GenerateKey(builder, &key_blob, &key_characteristics);
        if (result == ErrorCode::ROLLBACK_RESISTANCE_UNAVAILABLE &&
            tag.tag == TAG_ROLLBACK_RESISTANCE) {
            continue;
        }
        if (result == ErrorCode::UNSUPPORTED_TAG && tag.tag == TAG_TRUSTED_USER_PRESENCE_REQUIRED) {
            // Tag not required to be supported by all KeyMint implementations.
            continue;
        }
        ASSERT_EQ(result, ErrorCode::OK);
        ASSERT_GT(key_blob.size(), 0U);

        EXPECT_TRUE(ChainSignaturesAreValid(cert_chain_));
        ASSERT_GT(cert_chain_.size(), 0);
        verify_subject_and_serial(cert_chain_[0], serial_int, subject, /* self_signed = */ false);

        AuthorizationSet hw_enforced = HwEnforcedAuthorizations(key_characteristics);
        AuthorizationSet sw_enforced = SwEnforcedAuthorizations(key_characteristics);
        // Some tags are optional, so don't require them to be in the enforcements.
        if (tag.tag != TAG_ATTESTATION_APPLICATION_ID && tag.tag != TAG_ALLOW_WHILE_ON_BODY) {
            EXPECT_TRUE(hw_enforced.Contains(tag.tag) || sw_enforced.Contains(tag.tag))
                    << tag << " not in hw:" << hw_enforced << " nor sw:" << sw_enforced;
        }

        // Verifying the attestation record will check for the specific tag because
        // it's included in the authorizations.
        EXPECT_TRUE(verify_attestation_record(challenge, app_id, sw_enforced, hw_enforced,
                                              SecLevel(), cert_chain_[0].encodedCertificate));

        CheckedDeleteKey(&key_blob);
    }

    // Device attestation IDs should be rejected for normal attestation requests; these fields
    // are only used for device unique attestation.
    auto invalid_tags = AuthorizationSetBuilder()
                                .Authorization(TAG_ATTESTATION_ID_BRAND, "brand")
                                .Authorization(TAG_ATTESTATION_ID_DEVICE, "device")
                                .Authorization(TAG_ATTESTATION_ID_PRODUCT, "product")
                                .Authorization(TAG_ATTESTATION_ID_SERIAL, "serial")
                                .Authorization(TAG_ATTESTATION_ID_IMEI, "imei")
                                .Authorization(TAG_ATTESTATION_ID_MEID, "meid")
                                .Authorization(TAG_ATTESTATION_ID_MANUFACTURER, "manufacturer")
                                .Authorization(TAG_ATTESTATION_ID_MODEL, "model");
    for (const KeyParameter& tag : invalid_tags) {
        SCOPED_TRACE(testing::Message() << "tag-" << tag);
        vector<uint8_t> key_blob;
        vector<KeyCharacteristics> key_characteristics;
        AuthorizationSetBuilder builder =
                AuthorizationSetBuilder()
                        .Authorization(TAG_NO_AUTH_REQUIRED)
                        .EcdsaSigningKey(EcCurve::P_256)
                        .Digest(Digest::NONE)
                        .AttestationChallenge(challenge)
                        .AttestationApplicationId(app_id)
                        .Authorization(TAG_CERTIFICATE_SERIAL, serial_blob)
                        .Authorization(TAG_CERTIFICATE_SUBJECT, subject_der)
                        .SetDefaultValidity();
        builder.push_back(tag);
        ASSERT_EQ(ErrorCode::CANNOT_ATTEST_IDS,
                  GenerateKey(builder, &key_blob, &key_characteristics));
    }
}

/*
 * NewKeyGenerationTest.EcdsaAttestationUniqueId
 *
 * Verifies that creation of an attested ECDSA key with a UNIQUE_ID included.
 */
TEST_P(NewKeyGenerationTest, EcdsaAttestationUniqueId) {
    auto get_unique_id = [this](const std::string& app_id, uint64_t datetime,
                                vector<uint8_t>* unique_id, bool reset = false) {
        auto challenge = "hello";
        auto subject = "cert subj 2";
        vector<uint8_t> subject_der(make_name_from_str(subject));
        uint64_t serial_int = 0x1010;
        vector<uint8_t> serial_blob(build_serial_blob(serial_int));
        AuthorizationSetBuilder builder =
                AuthorizationSetBuilder()
                        .Authorization(TAG_NO_AUTH_REQUIRED)
                        .Authorization(TAG_INCLUDE_UNIQUE_ID)
                        .EcdsaSigningKey(EcCurve::P_256)
                        .Digest(Digest::NONE)
                        .AttestationChallenge(challenge)
                        .Authorization(TAG_CERTIFICATE_SERIAL, serial_blob)
                        .Authorization(TAG_CERTIFICATE_SUBJECT, subject_der)
                        .AttestationApplicationId(app_id)
                        .Authorization(TAG_CREATION_DATETIME, datetime)
                        .SetDefaultValidity();
        if (reset) {
            builder.Authorization(TAG_RESET_SINCE_ID_ROTATION);
        }

        ASSERT_EQ(ErrorCode::OK, GenerateKey(builder));
        ASSERT_GT(key_blob_.size(), 0U);

        EXPECT_TRUE(ChainSignaturesAreValid(cert_chain_));
        ASSERT_GT(cert_chain_.size(), 0);
        verify_subject_and_serial(cert_chain_[0], serial_int, subject, /* self_signed = */ false);

        AuthorizationSet hw_enforced = HwEnforcedAuthorizations(key_characteristics_);
        AuthorizationSet sw_enforced = SwEnforcedAuthorizations(key_characteristics_);

        // Check that the unique ID field in the extension is non-empty.
        EXPECT_TRUE(verify_attestation_record(challenge, app_id, sw_enforced, hw_enforced,
                                              SecLevel(), cert_chain_[0].encodedCertificate,
                                              unique_id));
        EXPECT_GT(unique_id->size(), 0);
        CheckedDeleteKey();
    };

    // Generate unique ID
    auto app_id = "foo";
    uint64_t cert_date = 1619621648000;  // Wed Apr 28 14:54:08 2021 in ms since epoch
    vector<uint8_t> unique_id;
    get_unique_id(app_id, cert_date, &unique_id);

    // Generating a new key with the same parameters should give the same unique ID.
    vector<uint8_t> unique_id2;
    get_unique_id(app_id, cert_date, &unique_id2);
    EXPECT_EQ(unique_id, unique_id2);

    // Generating a new key with a slightly different date should give the same unique ID.
    uint64_t rounded_date = cert_date / 2592000000LLU;
    uint64_t min_date = rounded_date * 2592000000LLU;
    uint64_t max_date = ((rounded_date + 1) * 2592000000LLU) - 1;

    vector<uint8_t> unique_id3;
    get_unique_id(app_id, min_date, &unique_id3);
    EXPECT_EQ(unique_id, unique_id3);

    vector<uint8_t> unique_id4;
    get_unique_id(app_id, max_date, &unique_id4);
    EXPECT_EQ(unique_id, unique_id4);

    // A different attestation application ID should yield a different unique ID.
    auto app_id2 = "different_foo";
    vector<uint8_t> unique_id5;
    get_unique_id(app_id2, cert_date, &unique_id5);
    EXPECT_NE(unique_id, unique_id5);

    // A radically different date should yield a different unique ID.
    vector<uint8_t> unique_id6;
    get_unique_id(app_id, 1611621648000, &unique_id6);
    EXPECT_NE(unique_id, unique_id6);

    vector<uint8_t> unique_id7;
    get_unique_id(app_id, max_date + 1, &unique_id7);
    EXPECT_NE(unique_id, unique_id7);

    vector<uint8_t> unique_id8;
    get_unique_id(app_id, min_date - 1, &unique_id8);
    EXPECT_NE(unique_id, unique_id8);

    // Marking RESET_SINCE_ID_ROTATION should give a different unique ID.
    vector<uint8_t> unique_id9;
    get_unique_id(app_id, cert_date, &unique_id9, /* reset_id = */ true);
    EXPECT_NE(unique_id, unique_id9);
}

/*
 * NewKeyGenerationTest.EcdsaAttestationTagNoApplicationId
 *
 * Verifies that creation of an attested ECDSA key does not include APPLICATION_ID.
 */
TEST_P(NewKeyGenerationTest, EcdsaAttestationTagNoApplicationId) {
    auto challenge = "hello";
    auto attest_app_id = "foo";
    auto subject = "cert subj 2";
    vector<uint8_t> subject_der(make_name_from_str(subject));
    uint64_t serial_int = 0x1010;
    vector<uint8_t> serial_blob(build_serial_blob(serial_int));

    // Earlier versions of the attestation extension schema included a slot:
    //     applicationId  [601] EXPLICIT OCTET_STRING OPTIONAL,
    // This should never have been included, and should never be filled in.
    // Generate an attested key that include APPLICATION_ID and APPLICATION_DATA,
    // to confirm that this field never makes it into the attestation extension.
    vector<uint8_t> key_blob;
    vector<KeyCharacteristics> key_characteristics;
    auto result = GenerateKey(AuthorizationSetBuilder()
                                      .Authorization(TAG_NO_AUTH_REQUIRED)
                                      .EcdsaSigningKey(EcCurve::P_256)
                                      .Digest(Digest::NONE)
                                      .AttestationChallenge(challenge)
                                      .AttestationApplicationId(attest_app_id)
                                      .Authorization(TAG_APPLICATION_ID, "client_id")
                                      .Authorization(TAG_APPLICATION_DATA, "appdata")
                                      .Authorization(TAG_CERTIFICATE_SERIAL, serial_blob)
                                      .Authorization(TAG_CERTIFICATE_SUBJECT, subject_der)
                                      .SetDefaultValidity(),
                              &key_blob, &key_characteristics);
    ASSERT_EQ(result, ErrorCode::OK);
    ASSERT_GT(key_blob.size(), 0U);

    EXPECT_TRUE(ChainSignaturesAreValid(cert_chain_));
    ASSERT_GT(cert_chain_.size(), 0);
    verify_subject_and_serial(cert_chain_[0], serial_int, subject, /* self_signed = */ false);

    AuthorizationSet hw_enforced = HwEnforcedAuthorizations(key_characteristics);
    AuthorizationSet sw_enforced = SwEnforcedAuthorizations(key_characteristics);
    EXPECT_TRUE(verify_attestation_record(challenge, attest_app_id, sw_enforced, hw_enforced,
                                          SecLevel(), cert_chain_[0].encodedCertificate));

    // Check that the app id is not in the cert.
    string app_id = "clientid";
    std::vector<uint8_t> needle(reinterpret_cast<const uint8_t*>(app_id.data()),
                                reinterpret_cast<const uint8_t*>(app_id.data()) + app_id.size());
    ASSERT_EQ(std::search(cert_chain_[0].encodedCertificate.begin(),
                          cert_chain_[0].encodedCertificate.end(), needle.begin(), needle.end()),
              cert_chain_[0].encodedCertificate.end());

    CheckedDeleteKey(&key_blob);
}

/*
 * NewKeyGenerationTest.EcdsaSelfSignAttestation
 *
 * Verifies that if no challenge is provided to an Ecdsa key generation, then
 * the key will generate a self signed attestation.
 */
TEST_P(NewKeyGenerationTest, EcdsaSelfSignAttestation) {
    auto subject = "cert subj 2";
    vector<uint8_t> subject_der(make_name_from_str(subject));

    uint64_t serial_int = 0x123456FFF1234;
    vector<uint8_t> serial_blob(build_serial_blob(serial_int));

    for (auto curve : ValidCurves()) {
        vector<uint8_t> key_blob;
        vector<KeyCharacteristics> key_characteristics;
        ASSERT_EQ(ErrorCode::OK,
                  GenerateKey(AuthorizationSetBuilder()
                                      .EcdsaSigningKey(curve)
                                      .Digest(Digest::NONE)
                                      .Authorization(TAG_CERTIFICATE_SERIAL, serial_blob)
                                      .Authorization(TAG_CERTIFICATE_SUBJECT, subject_der)
                                      .SetDefaultValidity(),
                              &key_blob, &key_characteristics));
        ASSERT_GT(key_blob.size(), 0U);
        CheckBaseParams(key_characteristics);
        CheckCharacteristics(key_blob, key_characteristics);

        AuthorizationSet crypto_params = SecLevelAuthorizations(key_characteristics);

        EXPECT_TRUE(crypto_params.Contains(TAG_ALGORITHM, Algorithm::EC));
        EXPECT_TRUE(crypto_params.Contains(TAG_EC_CURVE, curve)) << "Curve " << curve << "missing";

        EXPECT_TRUE(ChainSignaturesAreValid(cert_chain_));
        verify_subject_and_serial(cert_chain_[0], serial_int, subject, false);
        ASSERT_EQ(cert_chain_.size(), 1);

        AuthorizationSet hw_enforced = HwEnforcedAuthorizations(key_characteristics);
        AuthorizationSet sw_enforced = SwEnforcedAuthorizations(key_characteristics);

        CheckedDeleteKey(&key_blob);
    }
}

/*
 * NewKeyGenerationTest.EcdsaAttestationRequireAppId
 *
 * Verifies that if attestation challenge is provided to Ecdsa key generation, then
 * app id must also be provided or else it will fail.
 */
TEST_P(NewKeyGenerationTest, EcdsaAttestationRequireAppId) {
    auto challenge = "hello";
    vector<uint8_t> key_blob;
    vector<KeyCharacteristics> key_characteristics;

    ASSERT_EQ(ErrorCode::ATTESTATION_APPLICATION_ID_MISSING,
              GenerateKey(AuthorizationSetBuilder()
                                  .EcdsaSigningKey(EcCurve::P_256)
                                  .Digest(Digest::NONE)
                                  .AttestationChallenge(challenge)
                                  .SetDefaultValidity(),
                          &key_blob, &key_characteristics));
}

/*
 * NewKeyGenerationTest.EcdsaIgnoreAppId
 *
 * Verifies that if no challenge is provided to the Ecdsa key generation, then
 * any appid will be ignored, and keymint will generate a self sign certificate.
 */
TEST_P(NewKeyGenerationTest, EcdsaIgnoreAppId) {
    auto app_id = "foo";

    for (auto curve : ValidCurves()) {
        vector<uint8_t> key_blob;
        vector<KeyCharacteristics> key_characteristics;
        ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                     .EcdsaSigningKey(curve)
                                                     .Digest(Digest::NONE)
                                                     .AttestationApplicationId(app_id)
                                                     .SetDefaultValidity(),
                                             &key_blob, &key_characteristics));

        ASSERT_GT(key_blob.size(), 0U);
        CheckBaseParams(key_characteristics);
        CheckCharacteristics(key_blob, key_characteristics);

        AuthorizationSet crypto_params = SecLevelAuthorizations(key_characteristics);

        EXPECT_TRUE(crypto_params.Contains(TAG_ALGORITHM, Algorithm::EC));
        EXPECT_TRUE(crypto_params.Contains(TAG_EC_CURVE, curve)) << "Curve " << curve << "missing";

        EXPECT_TRUE(ChainSignaturesAreValid(cert_chain_));
        ASSERT_EQ(cert_chain_.size(), 1);

        AuthorizationSet hw_enforced = HwEnforcedAuthorizations(key_characteristics);
        AuthorizationSet sw_enforced = SwEnforcedAuthorizations(key_characteristics);

        CheckedDeleteKey(&key_blob);
    }
}

/*
 * NewKeyGenerationTest.AttestationApplicationIDLengthProperlyEncoded
 *
 * Verifies that the Attestation Application ID software enforced tag has a proper length encoding.
 * Some implementations break strict encoding rules by encoding a length between 127 and 256 in one
 * byte. Proper DER encoding specifies that for lengths greater than 127, one byte should be used
 * to specify how many following bytes will be used to encode the length.
 */
TEST_P(NewKeyGenerationTest, AttestationApplicationIDLengthProperlyEncoded) {
    auto challenge = "hello";
    std::vector<uint32_t> app_id_lengths{143, 258};

    for (uint32_t length : app_id_lengths) {
        const string app_id(length, 'a');
        vector<uint8_t> key_blob;
        vector<KeyCharacteristics> key_characteristics;
        ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                     .Authorization(TAG_NO_AUTH_REQUIRED)
                                                     .EcdsaSigningKey(EcCurve::P_256)
                                                     .Digest(Digest::NONE)
                                                     .AttestationChallenge(challenge)
                                                     .AttestationApplicationId(app_id)
                                                     .SetDefaultValidity(),
                                             &key_blob, &key_characteristics));
        ASSERT_GT(key_blob.size(), 0U);
        CheckBaseParams(key_characteristics);
        CheckCharacteristics(key_blob, key_characteristics);

        AuthorizationSet crypto_params = SecLevelAuthorizations(key_characteristics);

        EXPECT_TRUE(crypto_params.Contains(TAG_ALGORITHM, Algorithm::EC));
        EXPECT_TRUE(crypto_params.Contains(TAG_EC_CURVE, EcCurve::P_256)) << "Curve P256 missing";

        EXPECT_TRUE(ChainSignaturesAreValid(cert_chain_));
        ASSERT_GT(cert_chain_.size(), 0);

        AuthorizationSet hw_enforced = HwEnforcedAuthorizations(key_characteristics);
        AuthorizationSet sw_enforced = SwEnforcedAuthorizations(key_characteristics);
        EXPECT_TRUE(verify_attestation_record(challenge, app_id,  //
                                              sw_enforced, hw_enforced, SecLevel(),
                                              cert_chain_[0].encodedCertificate));

        CheckedDeleteKey(&key_blob);
    }
}

/*
 * NewKeyGenerationTest.LimitedUsageEcdsa
 *
 * Verifies that KeyMint can generate all required EC key sizes with limited usage, and that the
 * resulting keys have correct characteristics.
 */
TEST_P(NewKeyGenerationTest, LimitedUsageEcdsa) {
    for (auto curve : ValidCurves()) {
        vector<uint8_t> key_blob;
        vector<KeyCharacteristics> key_characteristics;
        ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                     .EcdsaSigningKey(curve)
                                                     .Digest(Digest::NONE)
                                                     .Authorization(TAG_USAGE_COUNT_LIMIT, 1)
                                                     .SetDefaultValidity(),
                                             &key_blob, &key_characteristics));

        ASSERT_GT(key_blob.size(), 0U);
        CheckBaseParams(key_characteristics);
        CheckCharacteristics(key_blob, key_characteristics);

        AuthorizationSet crypto_params = SecLevelAuthorizations(key_characteristics);

        EXPECT_TRUE(crypto_params.Contains(TAG_ALGORITHM, Algorithm::EC));
        EXPECT_TRUE(crypto_params.Contains(TAG_EC_CURVE, curve)) << "Curve " << curve << "missing";

        // Check the usage count limit tag appears in the authorizations.
        AuthorizationSet auths;
        for (auto& entry : key_characteristics) {
            auths.push_back(AuthorizationSet(entry.authorizations));
        }
        EXPECT_TRUE(auths.Contains(TAG_USAGE_COUNT_LIMIT, 1U))
                << "key usage count limit " << 1U << " missing";

        CheckedDeleteKey(&key_blob);
    }
}

/*
 * NewKeyGenerationTest.EcdsaDefaultSize
 *
 * Verifies that failing to specify a curve for EC key generation returns
 * UNSUPPORTED_KEY_SIZE.
 */
TEST_P(NewKeyGenerationTest, EcdsaDefaultSize) {
    ASSERT_EQ(ErrorCode::UNSUPPORTED_KEY_SIZE,
              GenerateKey(AuthorizationSetBuilder()
                                  .Authorization(TAG_ALGORITHM, Algorithm::EC)
                                  .SigningKey()
                                  .Digest(Digest::NONE)
                                  .SetDefaultValidity()));
}

/*
 * NewKeyGenerationTest.EcdsaInvalidSize
 *
 * Verifies that specifying an invalid key size for EC key generation returns
 * UNSUPPORTED_KEY_SIZE.
 */
TEST_P(NewKeyGenerationTest, EcdsaInvalidSize) {
    for (auto curve : InvalidCurves()) {
        vector<uint8_t> key_blob;
        vector<KeyCharacteristics> key_characteristics;
        ASSERT_EQ(ErrorCode::UNSUPPORTED_KEY_SIZE, GenerateKey(AuthorizationSetBuilder()
                                                                       .EcdsaSigningKey(curve)
                                                                       .Digest(Digest::NONE)
                                                                       .SetDefaultValidity(),
                                                               &key_blob, &key_characteristics));
    }

    ASSERT_EQ(ErrorCode::UNSUPPORTED_KEY_SIZE,
              GenerateKey(AuthorizationSetBuilder()
                                  .Authorization(TAG_ALGORITHM, Algorithm::EC)
                                  .Authorization(TAG_KEY_SIZE, 190)
                                  .SigningKey()
                                  .Digest(Digest::NONE)
                                  .SetDefaultValidity()));
}

/*
 * NewKeyGenerationTest.EcdsaMismatchKeySize
 *
 * Verifies that specifying mismatched key size and curve for EC key generation returns
 * INVALID_ARGUMENT.
 */
TEST_P(NewKeyGenerationTest, EcdsaMismatchKeySize) {
    if (SecLevel() == SecurityLevel::STRONGBOX) return;

    auto result = GenerateKey(AuthorizationSetBuilder()
                                      .Authorization(TAG_ALGORITHM, Algorithm::EC)
                                      .Authorization(TAG_KEY_SIZE, 224)
                                      .Authorization(TAG_EC_CURVE, EcCurve::P_256)
                                      .SigningKey()
                                      .Digest(Digest::NONE)
                                      .SetDefaultValidity());
    ASSERT_TRUE(result == ErrorCode::INVALID_ARGUMENT);
}

/*
 * NewKeyGenerationTest.EcdsaAllValidCurves
 *
 * Verifies that keymint does not support any curve designated as unsupported.
 */
TEST_P(NewKeyGenerationTest, EcdsaAllValidCurves) {
    Digest digest;
    if (SecLevel() == SecurityLevel::STRONGBOX) {
        digest = Digest::SHA_2_256;
    } else {
        digest = Digest::SHA_2_512;
    }
    for (auto curve : ValidCurves()) {
        EXPECT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                     .EcdsaSigningKey(curve)
                                                     .Digest(digest)
                                                     .SetDefaultValidity()))
                << "Failed to generate key on curve: " << curve;
        CheckedDeleteKey();
    }
}

/*
 * NewKeyGenerationTest.Hmac
 *
 * Verifies that keymint supports all required digests, and that the resulting keys have correct
 * characteristics.
 */
TEST_P(NewKeyGenerationTest, Hmac) {
    for (auto digest : ValidDigests(false /* withNone */, true /* withMD5 */)) {
        vector<uint8_t> key_blob;
        vector<KeyCharacteristics> key_characteristics;
        constexpr size_t key_size = 128;
        ASSERT_EQ(ErrorCode::OK,
                  GenerateKey(
                          AuthorizationSetBuilder().HmacKey(key_size).Digest(digest).Authorization(
                                  TAG_MIN_MAC_LENGTH, 128),
                          &key_blob, &key_characteristics));

        ASSERT_GT(key_blob.size(), 0U);
        CheckBaseParams(key_characteristics);
        CheckCharacteristics(key_blob, key_characteristics);

        AuthorizationSet crypto_params = SecLevelAuthorizations(key_characteristics);
        EXPECT_TRUE(crypto_params.Contains(TAG_ALGORITHM, Algorithm::HMAC));
        EXPECT_TRUE(crypto_params.Contains(TAG_KEY_SIZE, key_size))
                << "Key size " << key_size << "missing";

        CheckedDeleteKey(&key_blob);
    }
}

/*
 * NewKeyGenerationTest.HmacNoAttestation
 *
 * Verifies that for Hmac key generation, no attestation will be generated even if challenge
 * and app id are provided.
 */
TEST_P(NewKeyGenerationTest, HmacNoAttestation) {
    auto challenge = "hello";
    auto app_id = "foo";

    for (auto digest : ValidDigests(false /* withNone */, true /* withMD5 */)) {
        vector<uint8_t> key_blob;
        vector<KeyCharacteristics> key_characteristics;
        constexpr size_t key_size = 128;
        ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                     .HmacKey(key_size)
                                                     .Digest(digest)
                                                     .AttestationChallenge(challenge)
                                                     .AttestationApplicationId(app_id)
                                                     .Authorization(TAG_MIN_MAC_LENGTH, 128),
                                             &key_blob, &key_characteristics));

        ASSERT_GT(key_blob.size(), 0U);
        ASSERT_EQ(cert_chain_.size(), 0);
        CheckBaseParams(key_characteristics);
        CheckCharacteristics(key_blob, key_characteristics);

        AuthorizationSet crypto_params = SecLevelAuthorizations(key_characteristics);
        EXPECT_TRUE(crypto_params.Contains(TAG_ALGORITHM, Algorithm::HMAC));
        EXPECT_TRUE(crypto_params.Contains(TAG_KEY_SIZE, key_size))
                << "Key size " << key_size << "missing";

        CheckedDeleteKey(&key_blob);
    }
}

/*
 * NewKeyGenerationTest.LimitedUsageHmac
 *
 * Verifies that KeyMint supports all required digests with limited usage Hmac, and that the
 * resulting keys have correct characteristics.
 */
TEST_P(NewKeyGenerationTest, LimitedUsageHmac) {
    for (auto digest : ValidDigests(false /* withNone */, true /* withMD5 */)) {
        vector<uint8_t> key_blob;
        vector<KeyCharacteristics> key_characteristics;
        constexpr size_t key_size = 128;
        ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                     .HmacKey(key_size)
                                                     .Digest(digest)
                                                     .Authorization(TAG_MIN_MAC_LENGTH, 128)
                                                     .Authorization(TAG_USAGE_COUNT_LIMIT, 1),
                                             &key_blob, &key_characteristics));

        ASSERT_GT(key_blob.size(), 0U);
        CheckBaseParams(key_characteristics);
        CheckCharacteristics(key_blob, key_characteristics);

        AuthorizationSet crypto_params = SecLevelAuthorizations(key_characteristics);
        EXPECT_TRUE(crypto_params.Contains(TAG_ALGORITHM, Algorithm::HMAC));
        EXPECT_TRUE(crypto_params.Contains(TAG_KEY_SIZE, key_size))
                << "Key size " << key_size << "missing";

        // Check the usage count limit tag appears in the authorizations.
        AuthorizationSet auths;
        for (auto& entry : key_characteristics) {
            auths.push_back(AuthorizationSet(entry.authorizations));
        }
        EXPECT_TRUE(auths.Contains(TAG_USAGE_COUNT_LIMIT, 1U))
                << "key usage count limit " << 1U << " missing";

        CheckedDeleteKey(&key_blob);
    }
}

/*
 * NewKeyGenerationTest.HmacCheckKeySizes
 *
 * Verifies that keymint supports all key sizes, and rejects all invalid key sizes.
 */
TEST_P(NewKeyGenerationTest, HmacCheckKeySizes) {
    for (size_t key_size = 0; key_size <= 512; ++key_size) {
        if (key_size < 64 || key_size % 8 != 0) {
            // To keep this test from being very slow, we only test a random fraction of
            // non-byte key sizes.  We test only ~10% of such cases. Since there are 392 of
            // them, we expect to run ~40 of them in each run.
            if (key_size % 8 == 0 || random() % 10 == 0) {
                EXPECT_EQ(ErrorCode::UNSUPPORTED_KEY_SIZE,
                          GenerateKey(AuthorizationSetBuilder()
                                              .HmacKey(key_size)
                                              .Digest(Digest::SHA_2_256)
                                              .Authorization(TAG_MIN_MAC_LENGTH, 256)))
                        << "HMAC key size " << key_size << " invalid";
            }
        } else {
            EXPECT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                         .HmacKey(key_size)
                                                         .Digest(Digest::SHA_2_256)
                                                         .Authorization(TAG_MIN_MAC_LENGTH, 256)))
                    << "Failed to generate HMAC key of size " << key_size;
            CheckedDeleteKey();
        }
    }
    if (SecLevel() == SecurityLevel::STRONGBOX) {
        // STRONGBOX devices must not support keys larger than 512 bits.
        size_t key_size = 520;
        EXPECT_EQ(ErrorCode::UNSUPPORTED_KEY_SIZE,
                  GenerateKey(AuthorizationSetBuilder()
                                      .HmacKey(key_size)
                                      .Digest(Digest::SHA_2_256)
                                      .Authorization(TAG_MIN_MAC_LENGTH, 256)))
                << "HMAC key size " << key_size << " unexpectedly valid";
    }
}

/*
 * NewKeyGenerationTest.HmacCheckMinMacLengths
 *
 * Verifies that keymint supports all required MAC lengths and rejects all invalid lengths. This
 * test is probabilistic in order to keep the runtime down, but any failure prints out the
 * specific MAC length that failed, so reproducing a failed run will be easy.
 */
TEST_P(NewKeyGenerationTest, HmacCheckMinMacLengths) {
    for (size_t min_mac_length = 0; min_mac_length <= 256; ++min_mac_length) {
        if (min_mac_length < 64 || min_mac_length % 8 != 0) {
            // To keep this test from being very long, we only test a random fraction of
            // non-byte lengths.  We test only ~10% of such cases. Since there are 172 of them,
            // we expect to run ~17 of them in each run.
            if (min_mac_length % 8 == 0 || random() % 10 == 0) {
                EXPECT_EQ(ErrorCode::UNSUPPORTED_MIN_MAC_LENGTH,
                          GenerateKey(AuthorizationSetBuilder()
                                              .HmacKey(128)
                                              .Digest(Digest::SHA_2_256)
                                              .Authorization(TAG_MIN_MAC_LENGTH, min_mac_length)))
                        << "HMAC min mac length " << min_mac_length << " invalid.";
            }
        } else {
            EXPECT_EQ(ErrorCode::OK,
                      GenerateKey(AuthorizationSetBuilder()
                                          .HmacKey(128)
                                          .Digest(Digest::SHA_2_256)
                                          .Authorization(TAG_MIN_MAC_LENGTH, min_mac_length)))
                    << "Failed to generate HMAC key with min MAC length " << min_mac_length;
            CheckedDeleteKey();
        }
    }

    // Minimum MAC length must be no more than 512 bits.
    size_t min_mac_length = 520;
    EXPECT_EQ(ErrorCode::UNSUPPORTED_MIN_MAC_LENGTH,
              GenerateKey(AuthorizationSetBuilder()
                                  .HmacKey(128)
                                  .Digest(Digest::SHA_2_256)
                                  .Authorization(TAG_MIN_MAC_LENGTH, min_mac_length)))
            << "HMAC min mac length " << min_mac_length << " invalid.";
}

/*
 * NewKeyGenerationTest.HmacMultipleDigests
 *
 * Verifies that keymint rejects HMAC key generation with multiple specified digest algorithms.
 */
TEST_P(NewKeyGenerationTest, HmacMultipleDigests) {
    if (SecLevel() == SecurityLevel::STRONGBOX) return;

    ASSERT_EQ(ErrorCode::UNSUPPORTED_DIGEST,
              GenerateKey(AuthorizationSetBuilder()
                                  .HmacKey(128)
                                  .Digest(Digest::SHA1)
                                  .Digest(Digest::SHA_2_256)
                                  .Authorization(TAG_MIN_MAC_LENGTH, 128)));
}

/*
 * NewKeyGenerationTest.HmacDigestNone
 *
 * Verifies that keymint rejects HMAC key generation with no digest or Digest::NONE
 */
TEST_P(NewKeyGenerationTest, HmacDigestNone) {
    ASSERT_EQ(ErrorCode::UNSUPPORTED_DIGEST,
              GenerateKey(AuthorizationSetBuilder().HmacKey(128).Authorization(TAG_MIN_MAC_LENGTH,
                                                                               128)));

    ASSERT_EQ(ErrorCode::UNSUPPORTED_DIGEST,
              GenerateKey(AuthorizationSetBuilder()
                                  .HmacKey(128)
                                  .Digest(Digest::NONE)
                                  .Authorization(TAG_MIN_MAC_LENGTH, 128)));
}

/*
 * NewKeyGenerationTest.AesNoAttestation
 *
 * Verifies that attestation parameters to AES keys are ignored and generateKey
 * will succeed.
 */
TEST_P(NewKeyGenerationTest, AesNoAttestation) {
    auto challenge = "hello";
    auto app_id = "foo";

    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .EcbMode()
                                                 .Padding(PaddingMode::PKCS7)
                                                 .AttestationChallenge(challenge)
                                                 .AttestationApplicationId(app_id)));

    ASSERT_EQ(cert_chain_.size(), 0);
}

/*
 * NewKeyGenerationTest.TripleDesNoAttestation
 *
 * Verifies that attesting parameters to 3DES keys are ignored and generate key
 * will be successful.  No attestation should be generated.
 */
TEST_P(NewKeyGenerationTest, TripleDesNoAttestation) {
    auto challenge = "hello";
    auto app_id = "foo";

    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .TripleDesEncryptionKey(168)
                                                 .BlockMode(BlockMode::ECB)
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .Padding(PaddingMode::NONE)
                                                 .AttestationChallenge(challenge)
                                                 .AttestationApplicationId(app_id)));
    ASSERT_EQ(cert_chain_.size(), 0);
}

INSTANTIATE_KEYMINT_AIDL_TEST(NewKeyGenerationTest);

typedef KeyMintAidlTestBase SigningOperationsTest;

/*
 * SigningOperationsTest.RsaSuccess
 *
 * Verifies that raw RSA signature operations succeed.
 */
TEST_P(SigningOperationsTest, RsaSuccess) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .RsaSigningKey(2048, 65537)
                                                 .Digest(Digest::NONE)
                                                 .Padding(PaddingMode::NONE)
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .SetDefaultValidity()));
    string message = "12345678901234567890123456789012";
    string signature = SignMessage(
            message, AuthorizationSetBuilder().Digest(Digest::NONE).Padding(PaddingMode::NONE));
    LocalVerifyMessage(message, signature,
                       AuthorizationSetBuilder().Digest(Digest::NONE).Padding(PaddingMode::NONE));
}

/*
 * SigningOperationsTest.RsaAllPaddingsAndDigests
 *
 * Verifies RSA signature/verification for all padding modes and digests.
 */
TEST_P(SigningOperationsTest, RsaAllPaddingsAndDigests) {
    auto authorizations = AuthorizationSetBuilder()
                                  .Authorization(TAG_NO_AUTH_REQUIRED)
                                  .RsaSigningKey(2048, 65537)
                                  .Digest(ValidDigests(true /* withNone */, true /* withMD5 */))
                                  .Padding(PaddingMode::NONE)
                                  .Padding(PaddingMode::RSA_PSS)
                                  .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN)
                                  .SetDefaultValidity();

    ASSERT_EQ(ErrorCode::OK, GenerateKey(authorizations));

    string message(128, 'a');
    string corrupt_message(message);
    ++corrupt_message[corrupt_message.size() / 2];

    for (auto padding :
         {PaddingMode::NONE, PaddingMode::RSA_PSS, PaddingMode::RSA_PKCS1_1_5_SIGN}) {
        for (auto digest : ValidDigests(true /* withNone */, true /* withMD5 */)) {
            if (padding == PaddingMode::NONE && digest != Digest::NONE) {
                // Digesting only makes sense with padding.
                continue;
            }

            if (padding == PaddingMode::RSA_PSS && digest == Digest::NONE) {
                // PSS requires digesting.
                continue;
            }

            string signature =
                    SignMessage(message, AuthorizationSetBuilder().Digest(digest).Padding(padding));
            LocalVerifyMessage(message, signature,
                               AuthorizationSetBuilder().Digest(digest).Padding(padding));
        }
    }
}

/*
 * SigningOperationsTest.RsaUseRequiresCorrectAppIdAppData
 *
 * Verifies that using an RSA key requires the correct app data.
 */
TEST_P(SigningOperationsTest, RsaUseRequiresCorrectAppIdAppData) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .RsaSigningKey(2048, 65537)
                                                 .Digest(Digest::NONE)
                                                 .Padding(PaddingMode::NONE)
                                                 .Authorization(TAG_APPLICATION_ID, "clientid")
                                                 .Authorization(TAG_APPLICATION_DATA, "appdata")
                                                 .SetDefaultValidity()));

    CheckAppIdCharacteristics(key_blob_, "clientid", "appdata", key_characteristics_);

    EXPECT_EQ(ErrorCode::INVALID_KEY_BLOB,
              Begin(KeyPurpose::SIGN,
                    AuthorizationSetBuilder().Digest(Digest::NONE).Padding(PaddingMode::NONE)));
    AbortIfNeeded();
    EXPECT_EQ(ErrorCode::INVALID_KEY_BLOB,
              Begin(KeyPurpose::SIGN, AuthorizationSetBuilder()
                                              .Digest(Digest::NONE)
                                              .Padding(PaddingMode::NONE)
                                              .Authorization(TAG_APPLICATION_ID, "clientid")));
    AbortIfNeeded();
    EXPECT_EQ(ErrorCode::INVALID_KEY_BLOB,
              Begin(KeyPurpose::SIGN, AuthorizationSetBuilder()
                                              .Digest(Digest::NONE)
                                              .Padding(PaddingMode::NONE)
                                              .Authorization(TAG_APPLICATION_DATA, "appdata")));
    AbortIfNeeded();
    EXPECT_EQ(ErrorCode::OK,
              Begin(KeyPurpose::SIGN, AuthorizationSetBuilder()
                                              .Digest(Digest::NONE)
                                              .Padding(PaddingMode::NONE)
                                              .Authorization(TAG_APPLICATION_DATA, "appdata")
                                              .Authorization(TAG_APPLICATION_ID, "clientid")));
    AbortIfNeeded();
}

/*
 * SigningOperationsTest.RsaPssSha256Success
 *
 * Verifies that RSA-PSS signature operations succeed.
 */
TEST_P(SigningOperationsTest, RsaPssSha256Success) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .RsaSigningKey(2048, 65537)
                                                 .Digest(Digest::SHA_2_256)
                                                 .Padding(PaddingMode::RSA_PSS)
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .SetDefaultValidity()));
    // Use large message, which won't work without digesting.
    string message(1024, 'a');
    string signature = SignMessage(
            message,
            AuthorizationSetBuilder().Digest(Digest::SHA_2_256).Padding(PaddingMode::RSA_PSS));
}

/*
 * SigningOperationsTest.RsaPaddingNoneDoesNotAllowOther
 *
 * Verifies that keymint rejects signature operations that specify a padding mode when the key
 * supports only unpadded operations.
 */
TEST_P(SigningOperationsTest, RsaPaddingNoneDoesNotAllowOther) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .RsaSigningKey(2048, 65537)
                                                 .Digest(Digest::NONE)
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .Padding(PaddingMode::NONE)
                                                 .SetDefaultValidity()));
    string message = "12345678901234567890123456789012";
    string signature;

    EXPECT_EQ(ErrorCode::INCOMPATIBLE_PADDING_MODE,
              Begin(KeyPurpose::SIGN, AuthorizationSetBuilder()
                                              .Digest(Digest::NONE)
                                              .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN)));
}

/*
 * SigningOperationsTest.NoUserConfirmation
 *
 * Verifies that keymint rejects signing operations for keys with
 * TRUSTED_CONFIRMATION_REQUIRED and no valid confirmation token
 * presented.
 */
TEST_P(SigningOperationsTest, NoUserConfirmation) {
    if (SecLevel() == SecurityLevel::STRONGBOX) return;
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .RsaSigningKey(1024, 65537)
                                                 .Digest(Digest::NONE)
                                                 .Padding(PaddingMode::NONE)
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .Authorization(TAG_TRUSTED_CONFIRMATION_REQUIRED)
                                                 .SetDefaultValidity()));

    const string message = "12345678901234567890123456789012";
    EXPECT_EQ(ErrorCode::OK,
              Begin(KeyPurpose::SIGN,
                    AuthorizationSetBuilder().Digest(Digest::NONE).Padding(PaddingMode::NONE)));
    string signature;
    EXPECT_EQ(ErrorCode::NO_USER_CONFIRMATION, Finish(message, &signature));
}

/*
 * SigningOperationsTest.RsaPkcs1Sha256Success
 *
 * Verifies that digested RSA-PKCS1 signature operations succeed.
 */
TEST_P(SigningOperationsTest, RsaPkcs1Sha256Success) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .RsaSigningKey(2048, 65537)
                                                 .Digest(Digest::SHA_2_256)
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN)
                                                 .SetDefaultValidity()));
    string message(1024, 'a');
    string signature = SignMessage(message, AuthorizationSetBuilder()
                                                    .Digest(Digest::SHA_2_256)
                                                    .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN));
}

/*
 * SigningOperationsTest.RsaPkcs1NoDigestSuccess
 *
 * Verifies that undigested RSA-PKCS1 signature operations succeed.
 */
TEST_P(SigningOperationsTest, RsaPkcs1NoDigestSuccess) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .RsaSigningKey(2048, 65537)
                                                 .Digest(Digest::NONE)
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN)
                                                 .SetDefaultValidity()));
    string message(53, 'a');
    string signature = SignMessage(message, AuthorizationSetBuilder()
                                                    .Digest(Digest::NONE)
                                                    .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN));
}

/*
 * SigningOperationsTest.RsaPkcs1NoDigestTooLarge
 *
 * Verifies that undigested RSA-PKCS1 signature operations fail with the correct error code when
 * given a too-long message.
 */
TEST_P(SigningOperationsTest, RsaPkcs1NoDigestTooLong) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .RsaSigningKey(2048, 65537)
                                                 .Digest(Digest::NONE)
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN)
                                                 .SetDefaultValidity()));
    string message(257, 'a');

    EXPECT_EQ(ErrorCode::OK,
              Begin(KeyPurpose::SIGN, AuthorizationSetBuilder()
                                              .Digest(Digest::NONE)
                                              .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN)));
    string signature;
    EXPECT_EQ(ErrorCode::INVALID_INPUT_LENGTH, Finish(message, &signature));
}

/*
 * SigningOperationsTest.RsaPssSha512TooSmallKey
 *
 * Verifies that undigested RSA-PSS signature operations fail with the correct error code when
 * used with a key that is too small for the message.
 *
 * A PSS-padded message is of length salt_size + digest_size + 16 (sizes in bits), and the
 * keymint specification requires that salt_size == digest_size, so the message will be
 * digest_size * 2 +
 * 16. Such a message can only be signed by a given key if the key is at least that size. This
 * test uses SHA512, which has a digest_size == 512, so the message size is 1040 bits, too large
 * for a 1024-bit key.
 */
TEST_P(SigningOperationsTest, RsaPssSha512TooSmallKey) {
    if (SecLevel() == SecurityLevel::STRONGBOX) return;
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .RsaSigningKey(1024, 65537)
                                                 .Digest(Digest::SHA_2_512)
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .Padding(PaddingMode::RSA_PSS)
                                                 .SetDefaultValidity()));
    EXPECT_EQ(ErrorCode::INCOMPATIBLE_DIGEST,
              Begin(KeyPurpose::SIGN, AuthorizationSetBuilder()
                                              .Digest(Digest::SHA_2_512)
                                              .Padding(PaddingMode::RSA_PSS)));
}

/*
 * SigningOperationsTest.RsaNoPaddingTooLong
 *
 * Verifies that raw RSA signature operations fail with the correct error code when
 * given a too-long message.
 */
TEST_P(SigningOperationsTest, RsaNoPaddingTooLong) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .RsaSigningKey(2048, 65537)
                                                 .Digest(Digest::NONE)
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN)
                                                 .SetDefaultValidity()));
    // One byte too long
    string message(2048 / 8 + 1, 'a');
    ASSERT_EQ(ErrorCode::OK,
              Begin(KeyPurpose::SIGN, AuthorizationSetBuilder()
                                              .Digest(Digest::NONE)
                                              .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN)));
    string result;
    ErrorCode finish_error_code = Finish(message, &result);
    EXPECT_TRUE(finish_error_code == ErrorCode::INVALID_INPUT_LENGTH ||
                finish_error_code == ErrorCode::INVALID_ARGUMENT);

    // Very large message that should exceed the transfer buffer size of any reasonable TEE.
    message = string(128 * 1024, 'a');
    ASSERT_EQ(ErrorCode::OK,
              Begin(KeyPurpose::SIGN, AuthorizationSetBuilder()
                                              .Digest(Digest::NONE)
                                              .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN)));
    finish_error_code = Finish(message, &result);
    EXPECT_TRUE(finish_error_code == ErrorCode::INVALID_INPUT_LENGTH ||
                finish_error_code == ErrorCode::INVALID_ARGUMENT);
}

/*
 * SigningOperationsTest.RsaAbort
 *
 * Verifies that operations can be aborted correctly.  Uses an RSA signing operation for the
 * test, but the behavior should be algorithm and purpose-independent.
 */
TEST_P(SigningOperationsTest, RsaAbort) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .RsaSigningKey(2048, 65537)
                                                 .Digest(Digest::NONE)
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .Padding(PaddingMode::NONE)
                                                 .SetDefaultValidity()));

    ASSERT_EQ(ErrorCode::OK,
              Begin(KeyPurpose::SIGN,
                    AuthorizationSetBuilder().Digest(Digest::NONE).Padding(PaddingMode::NONE)));
    EXPECT_EQ(ErrorCode::OK, Abort());

    // Another abort should fail
    EXPECT_EQ(ErrorCode::INVALID_OPERATION_HANDLE, Abort());

    // Set to sentinel, so TearDown() doesn't try to abort again.
    op_.reset();
}

/*
 * SigningOperationsTest.RsaNonUniqueParams
 *
 * Verifies that an operation with multiple padding modes is rejected.
 */
TEST_P(SigningOperationsTest, RsaNonUniqueParams) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .RsaSigningKey(2048, 65537)
                                                 .Digest(Digest::NONE)
                                                 .Digest(Digest::SHA1)
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .Padding(PaddingMode::NONE)
                                                 .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN)
                                                 .SetDefaultValidity()));

    ASSERT_EQ(ErrorCode::UNSUPPORTED_PADDING_MODE,
              Begin(KeyPurpose::SIGN, AuthorizationSetBuilder()
                                              .Digest(Digest::NONE)
                                              .Padding(PaddingMode::NONE)
                                              .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN)));

    auto result = Begin(KeyPurpose::SIGN, AuthorizationSetBuilder()
                                                  .Digest(Digest::NONE)
                                                  .Digest(Digest::SHA1)
                                                  .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN));
    ASSERT_TRUE(result == ErrorCode::UNSUPPORTED_DIGEST || result == ErrorCode::INVALID_ARGUMENT);

    ASSERT_EQ(ErrorCode::UNSUPPORTED_DIGEST,
              Begin(KeyPurpose::SIGN,
                    AuthorizationSetBuilder().Padding(PaddingMode::RSA_PKCS1_1_5_SIGN)));
}

/*
 * SigningOperationsTest.RsaUnsupportedPadding
 *
 * Verifies that RSA operations fail with the correct error (but key gen succeeds) when used
 * with a padding mode inappropriate for RSA.
 */
TEST_P(SigningOperationsTest, RsaUnsupportedPadding) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .RsaSigningKey(2048, 65537)
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .Digest(Digest::SHA_2_256 /* supported digest */)
                                                 .Padding(PaddingMode::PKCS7)
                                                 .SetDefaultValidity()));
    ASSERT_EQ(
            ErrorCode::UNSUPPORTED_PADDING_MODE,
            Begin(KeyPurpose::SIGN,
                  AuthorizationSetBuilder().Digest(Digest::SHA_2_256).Padding(PaddingMode::PKCS7)));
    CheckedDeleteKey();

    ASSERT_EQ(ErrorCode::OK,
              GenerateKey(
                      AuthorizationSetBuilder()
                              .RsaSigningKey(2048, 65537)
                              .Authorization(TAG_NO_AUTH_REQUIRED)
                              .Digest(Digest::SHA_2_256 /* supported digest */)
                              .Padding(PaddingMode::RSA_OAEP) /* padding mode for encryption only */
                              .SetDefaultValidity()));
    ASSERT_EQ(ErrorCode::UNSUPPORTED_PADDING_MODE,
              Begin(KeyPurpose::SIGN, AuthorizationSetBuilder()
                                              .Digest(Digest::SHA_2_256)
                                              .Padding(PaddingMode::RSA_OAEP)));
}

/*
 * SigningOperationsTest.RsaPssNoDigest
 *
 * Verifies that RSA PSS operations fail when no digest is used.  PSS requires a digest.
 */
TEST_P(SigningOperationsTest, RsaNoDigest) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .RsaSigningKey(2048, 65537)
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .Digest(Digest::NONE)
                                                 .Padding(PaddingMode::RSA_PSS)
                                                 .SetDefaultValidity()));
    ASSERT_EQ(ErrorCode::INCOMPATIBLE_DIGEST,
              Begin(KeyPurpose::SIGN,
                    AuthorizationSetBuilder().Digest(Digest::NONE).Padding(PaddingMode::RSA_PSS)));

    ASSERT_EQ(ErrorCode::UNSUPPORTED_DIGEST,
              Begin(KeyPurpose::SIGN, AuthorizationSetBuilder().Padding(PaddingMode::RSA_PSS)));
}

/*
 * SigningOperationsTest.RsaPssNoPadding
 *
 * Verifies that RSA operations fail when no padding mode is specified.  PaddingMode::NONE is
 * supported in some cases (as validated in other tests), but a mode must be specified.
 */
TEST_P(SigningOperationsTest, RsaNoPadding) {
    // Padding must be specified
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .RsaKey(2048, 65537)
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .SigningKey()
                                                 .Digest(Digest::NONE)
                                                 .SetDefaultValidity()));
    ASSERT_EQ(ErrorCode::UNSUPPORTED_PADDING_MODE,
              Begin(KeyPurpose::SIGN, AuthorizationSetBuilder().Digest(Digest::NONE)));
}

/*
 * SigningOperationsTest.RsaShortMessage
 *
 * Verifies that raw RSA signatures succeed with a message shorter than the key size.
 */
TEST_P(SigningOperationsTest, RsaTooShortMessage) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .RsaSigningKey(2048, 65537)
                                                 .Digest(Digest::NONE)
                                                 .Padding(PaddingMode::NONE)
                                                 .SetDefaultValidity()));

    // Barely shorter
    string message(2048 / 8 - 1, 'a');
    SignMessage(message, AuthorizationSetBuilder().Digest(Digest::NONE).Padding(PaddingMode::NONE));

    // Much shorter
    message = "a";
    SignMessage(message, AuthorizationSetBuilder().Digest(Digest::NONE).Padding(PaddingMode::NONE));
}

/*
 * SigningOperationsTest.RsaSignWithEncryptionKey
 *
 * Verifies that RSA encryption keys cannot be used to sign.
 */
TEST_P(SigningOperationsTest, RsaSignWithEncryptionKey) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .RsaEncryptionKey(2048, 65537)
                                                 .Digest(Digest::NONE)
                                                 .Padding(PaddingMode::NONE)
                                                 .SetDefaultValidity()));
    ASSERT_EQ(ErrorCode::INCOMPATIBLE_PURPOSE,
              Begin(KeyPurpose::SIGN,
                    AuthorizationSetBuilder().Digest(Digest::NONE).Padding(PaddingMode::NONE)));
}

/*
 * SigningOperationsTest.RsaSignTooLargeMessage
 *
 * Verifies that attempting a raw signature of a message which is the same length as the key,
 * but numerically larger than the public modulus, fails with the correct error.
 */
TEST_P(SigningOperationsTest, RsaSignTooLargeMessage) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .RsaSigningKey(2048, 65537)
                                                 .Digest(Digest::NONE)
                                                 .Padding(PaddingMode::NONE)
                                                 .SetDefaultValidity()));

    // Largest possible message will always be larger than the public modulus.
    string message(2048 / 8, static_cast<char>(0xff));
    ASSERT_EQ(ErrorCode::OK, Begin(KeyPurpose::SIGN, AuthorizationSetBuilder()
                                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                                             .Digest(Digest::NONE)
                                                             .Padding(PaddingMode::NONE)));
    string signature;
    ASSERT_EQ(ErrorCode::INVALID_ARGUMENT, Finish(message, &signature));
}

/*
 * SigningOperationsTest.EcdsaAllDigestsAndCurves
 *
 * Verifies ECDSA signature/verification for all digests and curves.
 */
TEST_P(SigningOperationsTest, EcdsaAllDigestsAndCurves) {
    auto digests = ValidDigests(true /* withNone */, false /* withMD5 */);

    string message = "1234567890";
    string corrupt_message = "2234567890";
    for (auto curve : ValidCurves()) {
        SCOPED_TRACE(testing::Message() << "Curve::" << curve);
        ErrorCode error = GenerateKey(AuthorizationSetBuilder()
                                              .Authorization(TAG_NO_AUTH_REQUIRED)
                                              .EcdsaSigningKey(curve)
                                              .Digest(digests)
                                              .SetDefaultValidity());
        EXPECT_EQ(ErrorCode::OK, error) << "Failed to generate key for EC curve " << curve;
        if (error != ErrorCode::OK) {
            continue;
        }

        for (auto digest : digests) {
            SCOPED_TRACE(testing::Message() << "Digest::" << digest);
            string signature = SignMessage(message, AuthorizationSetBuilder().Digest(digest));
            LocalVerifyMessage(message, signature, AuthorizationSetBuilder().Digest(digest));
        }

        auto rc = DeleteKey();
        ASSERT_TRUE(rc == ErrorCode::OK || rc == ErrorCode::UNIMPLEMENTED);
    }
}

/*
 * SigningOperationsTest.EcdsaAllCurves
 *
 * Verifies that ECDSA operations succeed with all possible curves.
 */
TEST_P(SigningOperationsTest, EcdsaAllCurves) {
    for (auto curve : ValidCurves()) {
        ErrorCode error = GenerateKey(AuthorizationSetBuilder()
                                              .Authorization(TAG_NO_AUTH_REQUIRED)
                                              .EcdsaSigningKey(curve)
                                              .Digest(Digest::SHA_2_256)
                                              .SetDefaultValidity());
        EXPECT_EQ(ErrorCode::OK, error) << "Failed to generate ECDSA key with curve " << curve;
        if (error != ErrorCode::OK) continue;

        string message(1024, 'a');
        SignMessage(message, AuthorizationSetBuilder().Digest(Digest::SHA_2_256));
        CheckedDeleteKey();
    }
}

/*
 * SigningOperationsTest.EcdsaNoDigestHugeData
 *
 * Verifies that ECDSA operations support very large messages, even without digesting.  This
 * should work because ECDSA actually only signs the leftmost L_n bits of the message, however
 * large it may be.  Not using digesting is a bad idea, but in some cases digesting is done by
 * the framework.
 */
TEST_P(SigningOperationsTest, EcdsaNoDigestHugeData) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .EcdsaSigningKey(EcCurve::P_256)
                                                 .Digest(Digest::NONE)
                                                 .SetDefaultValidity()));
    string message(1 * 1024, 'a');
    SignMessage(message, AuthorizationSetBuilder().Digest(Digest::NONE));
}

/*
 * SigningOperationsTest.EcUseRequiresCorrectAppIdAppData
 *
 * Verifies that using an EC key requires the correct app ID/data.
 */
TEST_P(SigningOperationsTest, EcUseRequiresCorrectAppIdAppData) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .EcdsaSigningKey(EcCurve::P_256)
                                                 .Digest(Digest::NONE)
                                                 .Authorization(TAG_APPLICATION_ID, "clientid")
                                                 .Authorization(TAG_APPLICATION_DATA, "appdata")
                                                 .SetDefaultValidity()));

    CheckAppIdCharacteristics(key_blob_, "clientid", "appdata", key_characteristics_);

    EXPECT_EQ(ErrorCode::INVALID_KEY_BLOB,
              Begin(KeyPurpose::SIGN, AuthorizationSetBuilder().Digest(Digest::NONE)));
    AbortIfNeeded();
    EXPECT_EQ(ErrorCode::INVALID_KEY_BLOB,
              Begin(KeyPurpose::SIGN, AuthorizationSetBuilder()
                                              .Digest(Digest::NONE)
                                              .Authorization(TAG_APPLICATION_ID, "clientid")));
    AbortIfNeeded();
    EXPECT_EQ(ErrorCode::INVALID_KEY_BLOB,
              Begin(KeyPurpose::SIGN, AuthorizationSetBuilder()
                                              .Digest(Digest::NONE)
                                              .Authorization(TAG_APPLICATION_DATA, "appdata")));
    AbortIfNeeded();
    EXPECT_EQ(ErrorCode::OK,
              Begin(KeyPurpose::SIGN, AuthorizationSetBuilder()
                                              .Digest(Digest::NONE)
                                              .Authorization(TAG_APPLICATION_DATA, "appdata")
                                              .Authorization(TAG_APPLICATION_ID, "clientid")));
    AbortIfNeeded();
}

/*
 * SigningOperationsTest.EcdsaIncompatibleDigest
 *
 * Verifies that using an EC key requires compatible digest.
 */
TEST_P(SigningOperationsTest, EcdsaIncompatibleDigest) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .EcdsaSigningKey(EcCurve::P_256)
                                                 .Digest(Digest::NONE)
                                                 .Digest(Digest::SHA1)
                                                 .SetDefaultValidity()));
    EXPECT_EQ(ErrorCode::INCOMPATIBLE_DIGEST,
              Begin(KeyPurpose::SIGN, AuthorizationSetBuilder().Digest(Digest::SHA_2_256)));
    AbortIfNeeded();
}

/*
 * SigningOperationsTest.AesEcbSign
 *
 * Verifies that attempts to use AES keys to sign fail in the correct way.
 */
TEST_P(SigningOperationsTest, AesEcbSign) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .SigningKey()
                                                 .AesEncryptionKey(128)
                                                 .Authorization(TAG_BLOCK_MODE, BlockMode::ECB)));

    AuthorizationSet out_params;
    EXPECT_EQ(ErrorCode::UNSUPPORTED_PURPOSE,
              Begin(KeyPurpose::SIGN, AuthorizationSet() /* in_params */, &out_params));
    EXPECT_EQ(ErrorCode::UNSUPPORTED_PURPOSE,
              Begin(KeyPurpose::VERIFY, AuthorizationSet() /* in_params */, &out_params));
}

/*
 * SigningOperationsTest.HmacAllDigests
 *
 * Verifies that HMAC works with all digests.
 */
TEST_P(SigningOperationsTest, HmacAllDigests) {
    for (auto digest : ValidDigests(false /* withNone */, false /* withMD5 */)) {
        ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                     .Authorization(TAG_NO_AUTH_REQUIRED)
                                                     .HmacKey(128)
                                                     .Digest(digest)
                                                     .Authorization(TAG_MIN_MAC_LENGTH, 160)))
                << "Failed to create HMAC key with digest " << digest;
        string message = "12345678901234567890123456789012";
        string signature = MacMessage(message, digest, 160);
        EXPECT_EQ(160U / 8U, signature.size())
                << "Failed to sign with HMAC key with digest " << digest;
        CheckedDeleteKey();
    }
}

/*
 * SigningOperationsTest.HmacSha256TooLargeMacLength
 *
 * Verifies that HMAC fails in the correct way when asked to generate a MAC larger than the
 * digest size.
 */
TEST_P(SigningOperationsTest, HmacSha256TooLargeMacLength) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .HmacKey(128)
                                                 .Digest(Digest::SHA_2_256)
                                                 .Authorization(TAG_MIN_MAC_LENGTH, 256)));
    AuthorizationSet output_params;
    EXPECT_EQ(ErrorCode::UNSUPPORTED_MAC_LENGTH, Begin(KeyPurpose::SIGN, key_blob_,
                                                       AuthorizationSetBuilder()
                                                               .Digest(Digest::SHA_2_256)
                                                               .Authorization(TAG_MAC_LENGTH, 264),
                                                       &output_params));
}

/*
 * SigningOperationsTest.HmacSha256InvalidMacLength
 *
 * Verifies that HMAC fails in the correct way when asked to generate a MAC whose length is
 * not a multiple of 8.
 */
TEST_P(SigningOperationsTest, HmacSha256InvalidMacLength) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .HmacKey(128)
                                                 .Digest(Digest::SHA_2_256)
                                                 .Authorization(TAG_MIN_MAC_LENGTH, 160)));
    AuthorizationSet output_params;
    EXPECT_EQ(ErrorCode::UNSUPPORTED_MAC_LENGTH, Begin(KeyPurpose::SIGN, key_blob_,
                                                       AuthorizationSetBuilder()
                                                               .Digest(Digest::SHA_2_256)
                                                               .Authorization(TAG_MAC_LENGTH, 161),
                                                       &output_params));
}

/*
 * SigningOperationsTest.HmacSha256TooSmallMacLength
 *
 * Verifies that HMAC fails in the correct way when asked to generate a MAC smaller than the
 * specified minimum MAC length.
 */
TEST_P(SigningOperationsTest, HmacSha256TooSmallMacLength) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .HmacKey(128)
                                                 .Digest(Digest::SHA_2_256)
                                                 .Authorization(TAG_MIN_MAC_LENGTH, 128)));
    AuthorizationSet output_params;
    EXPECT_EQ(ErrorCode::INVALID_MAC_LENGTH, Begin(KeyPurpose::SIGN, key_blob_,
                                                   AuthorizationSetBuilder()
                                                           .Digest(Digest::SHA_2_256)
                                                           .Authorization(TAG_MAC_LENGTH, 120),
                                                   &output_params));
}

/*
 * SigningOperationsTest.HmacRfc4231TestCase3
 *
 * Validates against the test vectors from RFC 4231 test case 3.
 */
TEST_P(SigningOperationsTest, HmacRfc4231TestCase3) {
    string key(20, 0xaa);
    string message(50, 0xdd);
    uint8_t sha_224_expected[] = {
            0x7f, 0xb3, 0xcb, 0x35, 0x88, 0xc6, 0xc1, 0xf6, 0xff, 0xa9, 0x69, 0x4d, 0x7d, 0x6a,
            0xd2, 0x64, 0x93, 0x65, 0xb0, 0xc1, 0xf6, 0x5d, 0x69, 0xd1, 0xec, 0x83, 0x33, 0xea,
    };
    uint8_t sha_256_expected[] = {
            0x77, 0x3e, 0xa9, 0x1e, 0x36, 0x80, 0x0e, 0x46, 0x85, 0x4d, 0xb8,
            0xeb, 0xd0, 0x91, 0x81, 0xa7, 0x29, 0x59, 0x09, 0x8b, 0x3e, 0xf8,
            0xc1, 0x22, 0xd9, 0x63, 0x55, 0x14, 0xce, 0xd5, 0x65, 0xfe,
    };
    uint8_t sha_384_expected[] = {
            0x88, 0x06, 0x26, 0x08, 0xd3, 0xe6, 0xad, 0x8a, 0x0a, 0xa2, 0xac, 0xe0,
            0x14, 0xc8, 0xa8, 0x6f, 0x0a, 0xa6, 0x35, 0xd9, 0x47, 0xac, 0x9f, 0xeb,
            0xe8, 0x3e, 0xf4, 0xe5, 0x59, 0x66, 0x14, 0x4b, 0x2a, 0x5a, 0xb3, 0x9d,
            0xc1, 0x38, 0x14, 0xb9, 0x4e, 0x3a, 0xb6, 0xe1, 0x01, 0xa3, 0x4f, 0x27,
    };
    uint8_t sha_512_expected[] = {
            0xfa, 0x73, 0xb0, 0x08, 0x9d, 0x56, 0xa2, 0x84, 0xef, 0xb0, 0xf0, 0x75, 0x6c,
            0x89, 0x0b, 0xe9, 0xb1, 0xb5, 0xdb, 0xdd, 0x8e, 0xe8, 0x1a, 0x36, 0x55, 0xf8,
            0x3e, 0x33, 0xb2, 0x27, 0x9d, 0x39, 0xbf, 0x3e, 0x84, 0x82, 0x79, 0xa7, 0x22,
            0xc8, 0x06, 0xb4, 0x85, 0xa4, 0x7e, 0x67, 0xc8, 0x07, 0xb9, 0x46, 0xa3, 0x37,
            0xbe, 0xe8, 0x94, 0x26, 0x74, 0x27, 0x88, 0x59, 0xe1, 0x32, 0x92, 0xfb,
    };

    CheckHmacTestVector(key, message, Digest::SHA_2_256, make_string(sha_256_expected));
    if (SecLevel() != SecurityLevel::STRONGBOX) {
        CheckHmacTestVector(key, message, Digest::SHA_2_224, make_string(sha_224_expected));
        CheckHmacTestVector(key, message, Digest::SHA_2_384, make_string(sha_384_expected));
        CheckHmacTestVector(key, message, Digest::SHA_2_512, make_string(sha_512_expected));
    }
}

/*
 * SigningOperationsTest.HmacRfc4231TestCase5
 *
 * Validates against the test vectors from RFC 4231 test case 5.
 */
TEST_P(SigningOperationsTest, HmacRfc4231TestCase5) {
    string key(20, 0x0c);
    string message = "Test With Truncation";

    uint8_t sha_224_expected[] = {
            0x0e, 0x2a, 0xea, 0x68, 0xa9, 0x0c, 0x8d, 0x37,
            0xc9, 0x88, 0xbc, 0xdb, 0x9f, 0xca, 0x6f, 0xa8,
    };
    uint8_t sha_256_expected[] = {
            0xa3, 0xb6, 0x16, 0x74, 0x73, 0x10, 0x0e, 0xe0,
            0x6e, 0x0c, 0x79, 0x6c, 0x29, 0x55, 0x55, 0x2b,
    };
    uint8_t sha_384_expected[] = {
            0x3a, 0xbf, 0x34, 0xc3, 0x50, 0x3b, 0x2a, 0x23,
            0xa4, 0x6e, 0xfc, 0x61, 0x9b, 0xae, 0xf8, 0x97,
    };
    uint8_t sha_512_expected[] = {
            0x41, 0x5f, 0xad, 0x62, 0x71, 0x58, 0x0a, 0x53,
            0x1d, 0x41, 0x79, 0xbc, 0x89, 0x1d, 0x87, 0xa6,
    };

    CheckHmacTestVector(key, message, Digest::SHA_2_256, make_string(sha_256_expected));
    if (SecLevel() != SecurityLevel::STRONGBOX) {
        CheckHmacTestVector(key, message, Digest::SHA_2_224, make_string(sha_224_expected));
        CheckHmacTestVector(key, message, Digest::SHA_2_384, make_string(sha_384_expected));
        CheckHmacTestVector(key, message, Digest::SHA_2_512, make_string(sha_512_expected));
    }
}

INSTANTIATE_KEYMINT_AIDL_TEST(SigningOperationsTest);

typedef KeyMintAidlTestBase VerificationOperationsTest;

/*
 * VerificationOperationsTest.HmacSigningKeyCannotVerify
 *
 * Verifies HMAC signing and verification, but that a signing key cannot be used to verify.
 */
TEST_P(VerificationOperationsTest, HmacSigningKeyCannotVerify) {
    string key_material = "HelloThisIsAKey";

    vector<uint8_t> signing_key, verification_key;
    vector<KeyCharacteristics> signing_key_chars, verification_key_chars;
    EXPECT_EQ(ErrorCode::OK,
              ImportKey(AuthorizationSetBuilder()
                                .Authorization(TAG_NO_AUTH_REQUIRED)
                                .Authorization(TAG_ALGORITHM, Algorithm::HMAC)
                                .Authorization(TAG_PURPOSE, KeyPurpose::SIGN)
                                .Digest(Digest::SHA_2_256)
                                .Authorization(TAG_MIN_MAC_LENGTH, 160),
                        KeyFormat::RAW, key_material, &signing_key, &signing_key_chars));
    EXPECT_EQ(ErrorCode::OK,
              ImportKey(AuthorizationSetBuilder()
                                .Authorization(TAG_NO_AUTH_REQUIRED)
                                .Authorization(TAG_ALGORITHM, Algorithm::HMAC)
                                .Authorization(TAG_PURPOSE, KeyPurpose::VERIFY)
                                .Digest(Digest::SHA_2_256)
                                .Authorization(TAG_MIN_MAC_LENGTH, 160),
                        KeyFormat::RAW, key_material, &verification_key, &verification_key_chars));

    string message = "This is a message.";
    string signature = SignMessage(
            signing_key, message,
            AuthorizationSetBuilder().Digest(Digest::SHA_2_256).Authorization(TAG_MAC_LENGTH, 160));

    // Signing key should not work.
    AuthorizationSet out_params;
    EXPECT_EQ(ErrorCode::INCOMPATIBLE_PURPOSE,
              Begin(KeyPurpose::VERIFY, signing_key,
                    AuthorizationSetBuilder().Digest(Digest::SHA_2_256), &out_params));

    // Verification key should work.
    VerifyMessage(verification_key, message, signature,
                  AuthorizationSetBuilder().Digest(Digest::SHA_2_256));

    CheckedDeleteKey(&signing_key);
    CheckedDeleteKey(&verification_key);
}

INSTANTIATE_KEYMINT_AIDL_TEST(VerificationOperationsTest);

typedef KeyMintAidlTestBase ExportKeyTest;

/*
 * ExportKeyTest.RsaUnsupportedKeyFormat
 *
 * Verifies that attempting to export RSA keys in PKCS#8 format fails with the correct error.
 */
// TODO(seleneh) add ExportKey to GenerateKey
// check result

class ImportKeyTest : public KeyMintAidlTestBase {
  public:
    template <TagType tag_type, Tag tag, typename ValueT>
    void CheckCryptoParam(TypedTag<tag_type, tag> ttag, ValueT expected) {
        SCOPED_TRACE("CheckCryptoParam");
        for (auto& entry : key_characteristics_) {
            if (entry.securityLevel == SecLevel()) {
                EXPECT_TRUE(contains(entry.authorizations, ttag, expected))
                        << "Tag " << tag << " with value " << expected
                        << " not found at security level" << entry.securityLevel;
            } else {
                EXPECT_FALSE(contains(entry.authorizations, ttag, expected))
                        << "Tag " << tag << " found at security level " << entry.securityLevel;
            }
        }
    }

    void CheckOrigin() {
        SCOPED_TRACE("CheckOrigin");
        // Origin isn't a crypto param, but it always lives with them.
        return CheckCryptoParam(TAG_ORIGIN, KeyOrigin::IMPORTED);
    }
};

/*
 * ImportKeyTest.RsaSuccess
 *
 * Verifies that importing and using an RSA key pair works correctly.
 */
TEST_P(ImportKeyTest, RsaSuccess) {
    uint32_t key_size;
    string key;

    if (SecLevel() == SecurityLevel::STRONGBOX) {
        key_size = 2048;
        key = rsa_2048_key;
    } else {
        key_size = 1024;
        key = rsa_key;
    }

    ASSERT_EQ(ErrorCode::OK, ImportKey(AuthorizationSetBuilder()
                                               .Authorization(TAG_NO_AUTH_REQUIRED)
                                               .RsaSigningKey(key_size, 65537)
                                               .Digest(Digest::SHA_2_256)
                                               .Padding(PaddingMode::RSA_PSS)
                                               .SetDefaultValidity(),
                                       KeyFormat::PKCS8, key));

    CheckCryptoParam(TAG_ALGORITHM, Algorithm::RSA);
    CheckCryptoParam(TAG_KEY_SIZE, key_size);
    CheckCryptoParam(TAG_RSA_PUBLIC_EXPONENT, 65537U);
    CheckCryptoParam(TAG_DIGEST, Digest::SHA_2_256);
    CheckCryptoParam(TAG_PADDING, PaddingMode::RSA_PSS);
    CheckOrigin();

    string message(1024 / 8, 'a');
    auto params = AuthorizationSetBuilder().Digest(Digest::SHA_2_256).Padding(PaddingMode::RSA_PSS);
    string signature = SignMessage(message, params);
    LocalVerifyMessage(message, signature, params);
}

/*
 * ImportKeyTest.RsaSuccessWithoutParams
 *
 * Verifies that importing and using an RSA key pair without specifying parameters
 * works correctly.
 */
TEST_P(ImportKeyTest, RsaSuccessWithoutParams) {
    uint32_t key_size;
    string key;

    if (SecLevel() == SecurityLevel::STRONGBOX) {
        key_size = 2048;
        key = rsa_2048_key;
    } else {
        key_size = 1024;
        key = rsa_key;
    }

    ASSERT_EQ(ErrorCode::OK, ImportKey(AuthorizationSetBuilder()
                                               .Authorization(TAG_NO_AUTH_REQUIRED)
                                               .SigningKey()
                                               .Authorization(TAG_ALGORITHM, Algorithm::RSA)
                                               .Digest(Digest::SHA_2_256)
                                               .Padding(PaddingMode::RSA_PSS)
                                               .SetDefaultValidity(),
                                       KeyFormat::PKCS8, key));

    // Key size and public exponent are determined from the imported key material.
    CheckCryptoParam(TAG_KEY_SIZE, key_size);
    CheckCryptoParam(TAG_RSA_PUBLIC_EXPONENT, 65537U);

    CheckCryptoParam(TAG_ALGORITHM, Algorithm::RSA);
    CheckCryptoParam(TAG_DIGEST, Digest::SHA_2_256);
    CheckCryptoParam(TAG_PADDING, PaddingMode::RSA_PSS);
    CheckOrigin();

    string message(1024 / 8, 'a');
    auto params = AuthorizationSetBuilder().Digest(Digest::SHA_2_256).Padding(PaddingMode::RSA_PSS);
    string signature = SignMessage(message, params);
    LocalVerifyMessage(message, signature, params);
}

/*
 * ImportKeyTest.RsaKeySizeMismatch
 *
 * Verifies that importing an RSA key pair with a size that doesn't match the key fails in the
 * correct way.
 */
TEST_P(ImportKeyTest, RsaKeySizeMismatch) {
    ASSERT_EQ(ErrorCode::IMPORT_PARAMETER_MISMATCH,
              ImportKey(AuthorizationSetBuilder()
                                .RsaSigningKey(2048 /* Doesn't match key */, 65537)
                                .Digest(Digest::NONE)
                                .Padding(PaddingMode::NONE)
                                .SetDefaultValidity(),
                        KeyFormat::PKCS8, rsa_key));
}

/*
 * ImportKeyTest.RsaPublicExponentMismatch
 *
 * Verifies that importing an RSA key pair with a public exponent that doesn't match the key
 * fails in the correct way.
 */
TEST_P(ImportKeyTest, RsaPublicExponentMismatch) {
    ASSERT_EQ(ErrorCode::IMPORT_PARAMETER_MISMATCH,
              ImportKey(AuthorizationSetBuilder()
                                .RsaSigningKey(1024, 3 /* Doesn't match key */)
                                .Digest(Digest::NONE)
                                .Padding(PaddingMode::NONE)
                                .SetDefaultValidity(),
                        KeyFormat::PKCS8, rsa_key));
}

/*
 * ImportKeyTest.EcdsaSuccess
 *
 * Verifies that importing and using an ECDSA P-256 key pair works correctly.
 */
TEST_P(ImportKeyTest, EcdsaSuccess) {
    ASSERT_EQ(ErrorCode::OK, ImportKey(AuthorizationSetBuilder()
                                               .Authorization(TAG_NO_AUTH_REQUIRED)
                                               .EcdsaSigningKey(EcCurve::P_256)
                                               .Digest(Digest::SHA_2_256)
                                               .SetDefaultValidity(),
                                       KeyFormat::PKCS8, ec_256_key));

    CheckCryptoParam(TAG_ALGORITHM, Algorithm::EC);
    CheckCryptoParam(TAG_DIGEST, Digest::SHA_2_256);
    CheckCryptoParam(TAG_EC_CURVE, EcCurve::P_256);

    CheckOrigin();

    string message(32, 'a');
    auto params = AuthorizationSetBuilder().Digest(Digest::SHA_2_256);
    string signature = SignMessage(message, params);
    LocalVerifyMessage(message, signature, params);
}

/*
 * ImportKeyTest.EcdsaP256RFC5915Success
 *
 * Verifies that importing and using an ECDSA P-256 key pair encoded using RFC5915 works
 * correctly.
 */
TEST_P(ImportKeyTest, EcdsaP256RFC5915Success) {
    ASSERT_EQ(ErrorCode::OK, ImportKey(AuthorizationSetBuilder()
                                               .Authorization(TAG_NO_AUTH_REQUIRED)
                                               .EcdsaSigningKey(EcCurve::P_256)
                                               .Digest(Digest::SHA_2_256)
                                               .SetDefaultValidity(),
                                       KeyFormat::PKCS8, ec_256_key_rfc5915));

    CheckCryptoParam(TAG_ALGORITHM, Algorithm::EC);
    CheckCryptoParam(TAG_DIGEST, Digest::SHA_2_256);
    CheckCryptoParam(TAG_EC_CURVE, EcCurve::P_256);

    CheckOrigin();

    string message(32, 'a');
    auto params = AuthorizationSetBuilder().Digest(Digest::SHA_2_256);
    string signature = SignMessage(message, params);
    LocalVerifyMessage(message, signature, params);
}

/*
 * ImportKeyTest.EcdsaP256SEC1Success
 *
 * Verifies that importing and using an ECDSA P-256 key pair encoded using SEC1 works correctly.
 */
TEST_P(ImportKeyTest, EcdsaP256SEC1Success) {
    ASSERT_EQ(ErrorCode::OK, ImportKey(AuthorizationSetBuilder()
                                               .Authorization(TAG_NO_AUTH_REQUIRED)
                                               .EcdsaSigningKey(EcCurve::P_256)
                                               .Digest(Digest::SHA_2_256)
                                               .SetDefaultValidity(),
                                       KeyFormat::PKCS8, ec_256_key_sec1));

    CheckCryptoParam(TAG_ALGORITHM, Algorithm::EC);
    CheckCryptoParam(TAG_DIGEST, Digest::SHA_2_256);
    CheckCryptoParam(TAG_EC_CURVE, EcCurve::P_256);

    CheckOrigin();

    string message(32, 'a');
    auto params = AuthorizationSetBuilder().Digest(Digest::SHA_2_256);
    string signature = SignMessage(message, params);
    LocalVerifyMessage(message, signature, params);
}

/*
 * ImportKeyTest.Ecdsa521Success
 *
 * Verifies that importing and using an ECDSA P-521 key pair works correctly.
 */
TEST_P(ImportKeyTest, Ecdsa521Success) {
    if (SecLevel() == SecurityLevel::STRONGBOX) return;
    ASSERT_EQ(ErrorCode::OK, ImportKey(AuthorizationSetBuilder()
                                               .Authorization(TAG_NO_AUTH_REQUIRED)
                                               .EcdsaSigningKey(EcCurve::P_521)
                                               .Digest(Digest::SHA_2_256)
                                               .SetDefaultValidity(),
                                       KeyFormat::PKCS8, ec_521_key));

    CheckCryptoParam(TAG_ALGORITHM, Algorithm::EC);
    CheckCryptoParam(TAG_DIGEST, Digest::SHA_2_256);
    CheckCryptoParam(TAG_EC_CURVE, EcCurve::P_521);
    CheckOrigin();

    string message(32, 'a');
    auto params = AuthorizationSetBuilder().Digest(Digest::SHA_2_256);
    string signature = SignMessage(message, params);
    LocalVerifyMessage(message, signature, params);
}

/*
 * ImportKeyTest.EcdsaCurveMismatch
 *
 * Verifies that importing an ECDSA key pair with a curve that doesn't match the key fails in
 * the correct way.
 */
TEST_P(ImportKeyTest, EcdsaCurveMismatch) {
    ASSERT_EQ(ErrorCode::IMPORT_PARAMETER_MISMATCH,
              ImportKey(AuthorizationSetBuilder()
                                .EcdsaSigningKey(EcCurve::P_224 /* Doesn't match key */)
                                .Digest(Digest::NONE)
                                .SetDefaultValidity(),
                        KeyFormat::PKCS8, ec_256_key));
}

/*
 * ImportKeyTest.AesSuccess
 *
 * Verifies that importing and using an AES key works.
 */
TEST_P(ImportKeyTest, AesSuccess) {
    string key = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ASSERT_EQ(ErrorCode::OK, ImportKey(AuthorizationSetBuilder()
                                               .Authorization(TAG_NO_AUTH_REQUIRED)
                                               .AesEncryptionKey(key.size() * 8)
                                               .EcbMode()
                                               .Padding(PaddingMode::PKCS7),
                                       KeyFormat::RAW, key));

    CheckCryptoParam(TAG_ALGORITHM, Algorithm::AES);
    CheckCryptoParam(TAG_KEY_SIZE, 128U);
    CheckCryptoParam(TAG_PADDING, PaddingMode::PKCS7);
    CheckCryptoParam(TAG_BLOCK_MODE, BlockMode::ECB);
    CheckOrigin();

    string message = "Hello World!";
    auto params = AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::PKCS7);
    string ciphertext = EncryptMessage(message, params);
    string plaintext = DecryptMessage(ciphertext, params);
    EXPECT_EQ(message, plaintext);
}

/*
 * ImportKeyTest.AesFailure
 *
 * Verifies that importing an invalid AES key fails.
 */
TEST_P(ImportKeyTest, AesFailure) {
    string key = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint32_t bitlen = key.size() * 8;
    for (uint32_t key_size : {bitlen - 1, bitlen + 1, bitlen - 8, bitlen + 8}) {
        // Explicit key size doesn't match that of the provided key.
        auto result = ImportKey(AuthorizationSetBuilder()
                                    .Authorization(TAG_NO_AUTH_REQUIRED)
                                    .AesEncryptionKey(key_size)
                                    .EcbMode()
                                    .Padding(PaddingMode::PKCS7),
                                KeyFormat::RAW, key);
        ASSERT_TRUE(result == ErrorCode::IMPORT_PARAMETER_MISMATCH ||
                    result == ErrorCode::UNSUPPORTED_KEY_SIZE)
                << "unexpected result: " << result;
    }

    // Explicit key size matches that of the provided key, but it's not a valid size.
    string long_key = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ASSERT_EQ(ErrorCode::UNSUPPORTED_KEY_SIZE,
              ImportKey(AuthorizationSetBuilder()
                                .Authorization(TAG_NO_AUTH_REQUIRED)
                                .AesEncryptionKey(long_key.size() * 8)
                                .EcbMode()
                                .Padding(PaddingMode::PKCS7),
                        KeyFormat::RAW, long_key));
    string short_key = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ASSERT_EQ(ErrorCode::UNSUPPORTED_KEY_SIZE,
              ImportKey(AuthorizationSetBuilder()
                                .Authorization(TAG_NO_AUTH_REQUIRED)
                                .AesEncryptionKey(short_key.size() * 8)
                                .EcbMode()
                                .Padding(PaddingMode::PKCS7),
                        KeyFormat::RAW, short_key));
}

/*
 * ImportKeyTest.TripleDesSuccess
 *
 * Verifies that importing and using a 3DES key works.
 */
TEST_P(ImportKeyTest, TripleDesSuccess) {
    string key = hex2str("a49d7564199e97cb529d2c9d97bf2f98d35edf57ba1f7358");
    ASSERT_EQ(ErrorCode::OK, ImportKey(AuthorizationSetBuilder()
                                               .Authorization(TAG_NO_AUTH_REQUIRED)
                                               .TripleDesEncryptionKey(168)
                                               .EcbMode()
                                               .Padding(PaddingMode::PKCS7),
                                       KeyFormat::RAW, key));

    CheckCryptoParam(TAG_ALGORITHM, Algorithm::TRIPLE_DES);
    CheckCryptoParam(TAG_KEY_SIZE, 168U);
    CheckCryptoParam(TAG_PADDING, PaddingMode::PKCS7);
    CheckCryptoParam(TAG_BLOCK_MODE, BlockMode::ECB);
    CheckOrigin();

    string message = "Hello World!";
    auto params = AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::PKCS7);
    string ciphertext = EncryptMessage(message, params);
    string plaintext = DecryptMessage(ciphertext, params);
    EXPECT_EQ(message, plaintext);
}

/*
 * ImportKeyTest.TripleDesFailure
 *
 * Verifies that importing an invalid 3DES key fails.
 */
TEST_P(ImportKeyTest, TripleDesFailure) {
    string key = hex2str("a49d7564199e97cb529d2c9d97bf2f98d35edf57ba1f7358");
    uint32_t bitlen = key.size() * 7;
    for (uint32_t key_size : {bitlen - 1, bitlen + 1, bitlen - 8, bitlen + 8}) {
        // Explicit key size doesn't match that of the provided key.
        auto result = ImportKey(AuthorizationSetBuilder()
                                    .Authorization(TAG_NO_AUTH_REQUIRED)
                                    .TripleDesEncryptionKey(key_size)
                                    .EcbMode()
                                    .Padding(PaddingMode::PKCS7),
                                KeyFormat::RAW, key);
        ASSERT_TRUE(result == ErrorCode::IMPORT_PARAMETER_MISMATCH ||
                    result == ErrorCode::UNSUPPORTED_KEY_SIZE)
                << "unexpected result: " << result;
    }
    // Explicit key size matches that of the provided key, but it's not a valid size.
    string long_key = hex2str("a49d7564199e97cb529d2c9d97bf2f98d35edf57ba1f735800");
    ASSERT_EQ(ErrorCode::UNSUPPORTED_KEY_SIZE,
              ImportKey(AuthorizationSetBuilder()
                                .Authorization(TAG_NO_AUTH_REQUIRED)
                                .TripleDesEncryptionKey(long_key.size() * 7)
                                .EcbMode()
                                .Padding(PaddingMode::PKCS7),
                        KeyFormat::RAW, long_key));
    string short_key = hex2str("a49d7564199e97cb529d2c9d97bf2f98d35edf57ba1f73");
    ASSERT_EQ(ErrorCode::UNSUPPORTED_KEY_SIZE,
              ImportKey(AuthorizationSetBuilder()
                                .Authorization(TAG_NO_AUTH_REQUIRED)
                                .TripleDesEncryptionKey(short_key.size() * 7)
                                .EcbMode()
                                .Padding(PaddingMode::PKCS7),
                        KeyFormat::RAW, short_key));
}

/*
 * ImportKeyTest.HmacKeySuccess
 *
 * Verifies that importing and using an HMAC key works.
 */
TEST_P(ImportKeyTest, HmacKeySuccess) {
    string key = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ASSERT_EQ(ErrorCode::OK, ImportKey(AuthorizationSetBuilder()
                                               .Authorization(TAG_NO_AUTH_REQUIRED)
                                               .HmacKey(key.size() * 8)
                                               .Digest(Digest::SHA_2_256)
                                               .Authorization(TAG_MIN_MAC_LENGTH, 256),
                                       KeyFormat::RAW, key));

    CheckCryptoParam(TAG_ALGORITHM, Algorithm::HMAC);
    CheckCryptoParam(TAG_KEY_SIZE, 128U);
    CheckCryptoParam(TAG_DIGEST, Digest::SHA_2_256);
    CheckOrigin();

    string message = "Hello World!";
    string signature = MacMessage(message, Digest::SHA_2_256, 256);
    VerifyMessage(message, signature, AuthorizationSetBuilder().Digest(Digest::SHA_2_256));
}

INSTANTIATE_KEYMINT_AIDL_TEST(ImportKeyTest);

auto wrapped_key = hex2str(
        // IKeyMintDevice.aidl
        "30820179"  // SEQUENCE length 0x179 (SecureKeyWrapper) {
        "020100"    // INTEGER length 1 value 0x00 (version)
        "04820100"  // OCTET STRING length 0x100 (encryptedTransportKey)
        "934bf94e2aa28a3f83c9f79297250262"
        "fbe3276b5a1c91159bbfa3ef8957aac8"
        "4b59b30b455a79c2973480823d8b3863"
        "c3deef4a8e243590268d80e18751a0e1"
        "30f67ce6a1ace9f79b95e097474febc9"
        "81195b1d13a69086c0863f66a7b7fdb4"
        "8792227b1ac5e2489febdf087ab54864"
        "83033a6f001ca5d1ec1e27f5c30f4cec"
        "2642074a39ae68aee552e196627a8e3d"
        "867e67a8c01b11e75f13cca0a97ab668"
        "b50cda07a8ecb7cd8e3dd7009c963653"
        "4f6f239cffe1fc8daa466f78b676c711"
        "9efb96bce4e69ca2a25d0b34ed9c3ff9"
        "99b801597d5220e307eaa5bee507fb94"
        "d1fa69f9e519b2de315bac92c36f2ea1"
        "fa1df4478c0ddedeae8c70e0233cd098"
        "040c"  // OCTET STRING length 0x0c (initializationVector)
        "d796b02c370f1fa4cc0124f1"
        "302e"    // SEQUENCE length 0x2e (KeyDescription) {
        "020103"  // INTEGER length 1 value 0x03 (keyFormat = RAW)
        "3029"    // SEQUENCE length 0x29 (AuthorizationList) {
        "a108"    // [1] context-specific constructed tag=1 length 0x08 { (purpose)
        "3106"    // SET length 0x06
        "020100"  // INTEGER length 1 value 0x00 (Encrypt)
        "020101"  // INTEGER length 1 value 0x01 (Decrypt)
        // } end SET
        // } end [1]
        "a203"    // [2] context-specific constructed tag=2 length 0x02 { (algorithm)
        "020120"  // INTEGER length 1 value 0x20 (AES)
        // } end [2]
        "a304"      // [3] context-specific constructed tag=3 length 0x04 { (keySize)
        "02020100"  // INTEGER length 2 value 0x100
        // } end [3]
        "a405"    // [4] context-specific constructed tag=4 length 0x05 { (blockMode)
        "3103"    // SET length 0x03 {
        "020101"  // INTEGER length 1 value 0x01 (ECB)
        // } end SET
        // } end [4]
        "a605"    // [6] context-specific constructed tag=6 length 0x05 { (padding)
        "3103"    // SET length 0x03 {
        "020140"  // INTEGER length 1 value 0x40 (PKCS7)
        // } end SET
        // } end [5]
        "bf837702"  // [503] context-specific constructed tag=503=0x1F7 length 0x02 {
                    // (noAuthRequired)
        "0500"      // NULL
        // } end [503]
        // } end SEQUENCE (AuthorizationList)
        // } end SEQUENCE (KeyDescription)
        "0420"  // OCTET STRING length 0x20 (encryptedKey)
        "ccd540855f833a5e1480bfd2d36faf3a"
        "eee15df5beabe2691bc82dde2a7aa910"
        "0410"  // OCTET STRING length 0x10 (tag)
        "64c9f689c60ff6223ab6e6999e0eb6e5"
        // } SEQUENCE (SecureKeyWrapper)
);

auto wrapped_key_masked = hex2str(
        // IKeyMintDevice.aidl
        "30820179"  // SEQUENCE length 0x179 (SecureKeyWrapper) {
        "020100"    // INTEGER length 1 value 0x00 (version)
        "04820100"  // OCTET STRING length 0x100 (encryptedTransportKey)
        "aad93ed5924f283b4bb5526fbe7a1412"
        "f9d9749ec30db9062b29e574a8546f33"
        "c88732452f5b8e6a391ee76c39ed1712"
        "c61d8df6213dec1cffbc17a8c6d04c7b"
        "30893d8daa9b2015213e219468215532"
        "07f8f9931c4caba23ed3bee28b36947e"
        "47f10e0a5c3dc51c988a628daad3e5e1"
        "f4005e79c2d5a96c284b4b8d7e4948f3"
        "31e5b85dd5a236f85579f3ea1d1b8484"
        "87470bdb0ab4f81a12bee42c99fe0df4"
        "bee3759453e69ad1d68a809ce06b949f"
        "7694a990429b2fe81e066ff43e56a216"
        "02db70757922a4bcc23ab89f1e35da77"
        "586775f423e519c2ea394caf48a28d0c"
        "8020f1dcf6b3a68ec246f615ae96dae9"
        "a079b1f6eb959033c1af5c125fd94168"
        "040c"  // OCTET STRING length 0x0c (initializationVector)
        "6d9721d08589581ab49204a3"
        "302e"    // SEQUENCE length 0x2e (KeyDescription) {
        "020103"  // INTEGER length 1 value 0x03 (keyFormat = RAW)
        "3029"    // SEQUENCE length 0x29 (AuthorizationList) {
        "a108"    // [1] context-specific constructed tag=1 length 0x08 { (purpose)
        "3106"    // SET length 0x06
        "020100"  // INTEGER length 1 value 0x00 (Encrypt)
        "020101"  // INTEGER length 1 value 0x01 (Decrypt)
        // } end SET
        // } end [1]
        "a203"    // [2] context-specific constructed tag=2 length 0x02 { (algorithm)
        "020120"  // INTEGER length 1 value 0x20 (AES)
        // } end [2]
        "a304"      // [3] context-specific constructed tag=3 length 0x04 { (keySize)
        "02020100"  // INTEGER length 2 value 0x100
        // } end [3]
        "a405"    // [4] context-specific constructed tag=4 length 0x05 { (blockMode
        "3103"    // SET length 0x03 {
        "020101"  // INTEGER length 1 value 0x01 (ECB)
        // } end SET
        // } end [4]
        "a605"    // [6] context-specific constructed tag=6 length 0x05 { (padding)
        "3103"    // SET length 0x03 {
        "020140"  // INTEGER length 1 value 0x40 (PKCS7)
        // } end SET
        // } end [5]
        "bf837702"  // [503] context-specific constructed tag=503=0x1F7 length 0x02 {
                    // (noAuthRequired)
        "0500"      // NULL
        // } end [503]
        // } end SEQUENCE (AuthorizationList)
        // } end SEQUENCE (KeyDescription)
        "0420"  // OCTET STRING length 0x20 (encryptedKey)
        "a61c6e247e25b3e6e69aa78eb03c2d4a"
        "c20d1f99a9a024a76f35c8e2cab9b68d"
        "0410"  // OCTET STRING length 0x10 (tag)
        "2560c70109ae67c030f00b98b512a670"
        // } SEQUENCE (SecureKeyWrapper)
);

auto wrapping_key = hex2str(
        // RFC 5208 s5
        "308204be"            // SEQUENCE length 0x4be (PrivateKeyInfo) {
        "020100"              // INTEGER length 1 value 0x00 (version)
        "300d"                // SEQUENCE length 0x0d (AlgorithmIdentifier) {
        "0609"                // OBJECT IDENTIFIER length 0x09 (algorithm)
        "2a864886f70d010101"  // 1.2.840.113549.1.1.1 (RSAES-PKCS1-v1_5 encryption scheme)
        "0500"                // NULL (parameters)
        // } SEQUENCE (AlgorithmIdentifier)
        "048204a8"  // OCTET STRING len 0x4a8 (privateKey), which contains...
        // RFC 8017 A.1.2
        "308204a4"                          // SEQUENCE len 0x4a4 (RSAPrivateKey) {
        "020100"                            // INTEGER length 1 value 0x00 (version)
        "02820101"                          // INTEGER length 0x0101 (modulus) value...
        "00aec367931d8900ce56b0067f7d70e1"  // 0x10
        "fc653f3f34d194c1fed50018fb43db93"  // 0x20
        "7b06e673a837313d56b1c725150a3fef"  // 0x30
        "86acbddc41bb759c2854eae32d35841e"  // 0x40
        "fb5c18d82bc90a1cb5c1d55adf245b02"  // 0x50
        "911f0b7cda88c421ff0ebafe7c0d23be"  // 0x60
        "312d7bd5921ffaea1347c157406fef71"  // 0x70
        "8f682643e4e5d33c6703d61c0cf7ac0b"  // 0x80
        "f4645c11f5c1374c3886427411c44979"  // 0x90
        "6792e0bef75dec858a2123c36753e02a"  // 0xa0
        "95a96d7c454b504de385a642e0dfc3e6"  // 0xb0
        "0ac3a7ee4991d0d48b0172a95f9536f0"  // 0xc0
        "2ba13cecccb92b727db5c27e5b2f5cec"  // 0xd0
        "09600b286af5cf14c42024c61ddfe71c"  // 0xe0
        "2a8d7458f185234cb00e01d282f10f8f"  // 0xf0
        "c6721d2aed3f4833cca2bd8fa62821dd"  // 0x100
        "55"                                // 0x101
        "0203010001"                        // INTEGER length 3 value 0x10001 (publicExponent)
        "02820100"                          // INTEGER length 0x100 (privateExponent) value...
        "431447b6251908112b1ee76f99f3711a"  // 0x10
        "52b6630960046c2de70de188d833f8b8"  // 0x20
        "b91e4d785caeeeaf4f0f74414e2cda40"  // 0x30
        "641f7fe24f14c67a88959bdb27766df9"  // 0x40
        "e710b630a03adc683b5d2c43080e52be"  // 0x50
        "e71e9eaeb6de297a5fea1072070d181c"  // 0x60
        "822bccff087d63c940ba8a45f670feb2"  // 0x70
        "9fb4484d1c95e6d2579ba02aae0a0090"  // 0x80
        "0c3ebf490e3d2cd7ee8d0e20c536e4dc"  // 0x90
        "5a5097272888cddd7e91f228b1c4d747"  // 0xa0
        "4c55b8fcd618c4a957bbddd5ad7407cc"  // 0xb0
        "312d8d98a5caf7e08f4a0d6b45bb41c6"  // 0xc0
        "52659d5a5ba05b663737a8696281865b"  // 0xd0
        "a20fbdd7f851e6c56e8cbe0ddbbf24dc"  // 0xe0
        "03b2d2cb4c3d540fb0af52e034a2d066"  // 0xf0
        "98b128e5f101e3b51a34f8d8b4f86181"  // 0x100
        "028181"                            // INTEGER length 0x81 (prime1) value...
        "00de392e18d682c829266cc3454e1d61"  // 0x10
        "66242f32d9a1d10577753e904ea7d08b"  // 0x20
        "ff841be5bac82a164c5970007047b8c5"  // 0x30
        "17db8f8f84e37bd5988561bdf503d4dc"  // 0x40
        "2bdb38f885434ae42c355f725c9a60f9"  // 0x50
        "1f0788e1f1a97223b524b5357fdf72e2"  // 0x60
        "f696bab7d78e32bf92ba8e1864eab122"  // 0x70
        "9e91346130748a6e3c124f9149d71c74"  // 0x80
        "35"
        "028181"                            // INTEGER length 0x81 (prime2) value...
        "00c95387c0f9d35f137b57d0d65c397c"  // 0x10
        "5e21cc251e47008ed62a542409c8b6b6"  // 0x20
        "ac7f8967b3863ca645fcce49582a9aa1"  // 0x30
        "7349db6c4a95affdae0dae612e1afac9"  // 0x40
        "9ed39a2d934c880440aed8832f984316"  // 0x50
        "3a47f27f392199dc1202f9a0f9bd0830"  // 0x60
        "8007cb1e4e7f58309366a7de25f7c3c9"  // 0x70
        "b880677c068e1be936e81288815252a8"  // 0x80
        "a1"
        "028180"                            // INTEGER length 0x80 (exponent1) value...
        "57ff8ca1895080b2cae486ef0adfd791"  // 0x10
        "fb0235c0b8b36cd6c136e52e4085f4ea"  // 0x20
        "5a063212a4f105a3764743e53281988a"  // 0x30
        "ba073f6e0027298e1c4378556e0efca0"  // 0x40
        "e14ece1af76ad0b030f27af6f0ab35fb"  // 0x50
        "73a060d8b1a0e142fa2647e93b32e36d"  // 0x60
        "8282ae0a4de50ab7afe85500a16f43a6"  // 0x70
        "4719d6e2b9439823719cd08bcd031781"  // 0x80
        "028181"                            // INTEGER length 0x81 (exponent2) value...
        "00ba73b0bb28e3f81e9bd1c568713b10"  // 0x10
        "1241acc607976c4ddccc90e65b6556ca"  // 0x20
        "31516058f92b6e09f3b160ff0e374ec4"  // 0x30
        "0d78ae4d4979fde6ac06a1a400c61dd3"  // 0x40
        "1254186af30b22c10582a8a43e34fe94"  // 0x50
        "9c5f3b9755bae7baa7b7b7a6bd03b38c"  // 0x60
        "ef55c86885fc6c1978b9cee7ef33da50"  // 0x70
        "7c9df6b9277cff1e6aaa5d57aca52846"  // 0x80
        "61"
        "028181"                            // INTEGER length 0x81 (coefficient) value...
        "00c931617c77829dfb1270502be9195c"  // 0x10
        "8f2830885f57dba869536811e6864236"  // 0x20
        "d0c4736a0008a145af36b8357a7c3d13"  // 0x30
        "9966d04c4e00934ea1aede3bb6b8ec84"  // 0x40
        "1dc95e3f579751e2bfdfe27ae778983f"  // 0x50
        "959356210723287b0affcc9f727044d4"  // 0x60
        "8c373f1babde0724fa17a4fd4da0902c"  // 0x70
        "7c9b9bf27ba61be6ad02dfddda8f4e68"  // 0x80
        "22"
        // } SEQUENCE
        // } SEQUENCE ()
);

string zero_masking_key =
        hex2str("0000000000000000000000000000000000000000000000000000000000000000");
string masking_key = hex2str("D796B02C370F1FA4CC0124F14EC8CBEBE987E825246265050F399A51FD477DFC");

class ImportWrappedKeyTest : public KeyMintAidlTestBase {};

TEST_P(ImportWrappedKeyTest, Success) {
    auto wrapping_key_desc = AuthorizationSetBuilder()
                                     .RsaEncryptionKey(2048, 65537)
                                     .Digest(Digest::SHA_2_256)
                                     .Padding(PaddingMode::RSA_OAEP)
                                     .Authorization(TAG_PURPOSE, KeyPurpose::WRAP_KEY)
                                     .SetDefaultValidity();

    ASSERT_EQ(ErrorCode::OK,
              ImportWrappedKey(wrapped_key, wrapping_key, wrapping_key_desc, zero_masking_key,
                               AuthorizationSetBuilder()
                                       .Digest(Digest::SHA_2_256)
                                       .Padding(PaddingMode::RSA_OAEP)));

    string message = "Hello World!";
    auto params = AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::PKCS7);
    string ciphertext = EncryptMessage(message, params);
    string plaintext = DecryptMessage(ciphertext, params);
    EXPECT_EQ(message, plaintext);
}

/*
 * ImportWrappedKeyTest.SuccessSidsIgnored
 *
 * Verifies that password_sid and biometric_sid are ignored on import if the authorizations don't
 * include Tag:USER_SECURE_ID.
 */
TEST_P(ImportWrappedKeyTest, SuccessSidsIgnored) {
    auto wrapping_key_desc = AuthorizationSetBuilder()
                                     .RsaEncryptionKey(2048, 65537)
                                     .Digest(Digest::SHA_2_256)
                                     .Padding(PaddingMode::RSA_OAEP)
                                     .Authorization(TAG_PURPOSE, KeyPurpose::WRAP_KEY)
                                     .SetDefaultValidity();

    int64_t password_sid = 42;
    int64_t biometric_sid = 24;
    ASSERT_EQ(ErrorCode::OK,
              ImportWrappedKey(wrapped_key, wrapping_key, wrapping_key_desc, zero_masking_key,
                               AuthorizationSetBuilder()
                                       .Digest(Digest::SHA_2_256)
                                       .Padding(PaddingMode::RSA_OAEP),
                               password_sid, biometric_sid));

    string message = "Hello World!";
    auto params = AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::PKCS7);
    string ciphertext = EncryptMessage(message, params);
    string plaintext = DecryptMessage(ciphertext, params);
    EXPECT_EQ(message, plaintext);
}

TEST_P(ImportWrappedKeyTest, SuccessMasked) {
    auto wrapping_key_desc = AuthorizationSetBuilder()
                                     .RsaEncryptionKey(2048, 65537)
                                     .Digest(Digest::SHA_2_256)
                                     .Padding(PaddingMode::RSA_OAEP)
                                     .Authorization(TAG_PURPOSE, KeyPurpose::WRAP_KEY)
                                     .SetDefaultValidity();

    ASSERT_EQ(ErrorCode::OK,
              ImportWrappedKey(wrapped_key_masked, wrapping_key, wrapping_key_desc, masking_key,
                               AuthorizationSetBuilder()
                                       .Digest(Digest::SHA_2_256)
                                       .Padding(PaddingMode::RSA_OAEP)));
}

TEST_P(ImportWrappedKeyTest, WrongMask) {
    auto wrapping_key_desc = AuthorizationSetBuilder()
                                     .RsaEncryptionKey(2048, 65537)
                                     .Digest(Digest::SHA_2_256)
                                     .Padding(PaddingMode::RSA_OAEP)
                                     .Authorization(TAG_PURPOSE, KeyPurpose::WRAP_KEY)
                                     .SetDefaultValidity();

    ASSERT_EQ(
            ErrorCode::VERIFICATION_FAILED,
            ImportWrappedKey(wrapped_key_masked, wrapping_key, wrapping_key_desc, zero_masking_key,
                             AuthorizationSetBuilder()
                                     .Digest(Digest::SHA_2_256)
                                     .Padding(PaddingMode::RSA_OAEP)));
}

TEST_P(ImportWrappedKeyTest, WrongPurpose) {
    auto wrapping_key_desc = AuthorizationSetBuilder()
                                     .RsaEncryptionKey(2048, 65537)
                                     .Digest(Digest::SHA_2_256)
                                     .Padding(PaddingMode::RSA_OAEP)
                                     .SetDefaultValidity();

    ASSERT_EQ(
            ErrorCode::INCOMPATIBLE_PURPOSE,
            ImportWrappedKey(wrapped_key_masked, wrapping_key, wrapping_key_desc, zero_masking_key,
                             AuthorizationSetBuilder()
                                     .Digest(Digest::SHA_2_256)
                                     .Padding(PaddingMode::RSA_OAEP)));
}

TEST_P(ImportWrappedKeyTest, WrongPaddingMode) {
    auto wrapping_key_desc = AuthorizationSetBuilder()
                                     .RsaEncryptionKey(2048, 65537)
                                     .Digest(Digest::SHA_2_256)
                                     .Padding(PaddingMode::RSA_PSS)
                                     .Authorization(TAG_PURPOSE, KeyPurpose::WRAP_KEY)
                                     .SetDefaultValidity();

    ASSERT_EQ(ErrorCode::INCOMPATIBLE_PADDING_MODE,
              ImportWrappedKey(wrapped_key, wrapping_key, wrapping_key_desc, zero_masking_key,
                               AuthorizationSetBuilder()
                                       .Digest(Digest::SHA_2_256)
                                       .Padding(PaddingMode::RSA_OAEP)));
}

TEST_P(ImportWrappedKeyTest, WrongDigest) {
    auto wrapping_key_desc = AuthorizationSetBuilder()
                                     .RsaEncryptionKey(2048, 65537)
                                     .Digest(Digest::SHA_2_512)
                                     .Padding(PaddingMode::RSA_OAEP)
                                     .Authorization(TAG_PURPOSE, KeyPurpose::WRAP_KEY)
                                     .SetDefaultValidity();

    ASSERT_EQ(ErrorCode::INCOMPATIBLE_DIGEST,
              ImportWrappedKey(wrapped_key, wrapping_key, wrapping_key_desc, zero_masking_key,
                               AuthorizationSetBuilder()
                                       .Digest(Digest::SHA_2_256)
                                       .Padding(PaddingMode::RSA_OAEP)));
}

INSTANTIATE_KEYMINT_AIDL_TEST(ImportWrappedKeyTest);

typedef KeyMintAidlTestBase EncryptionOperationsTest;

/*
 * EncryptionOperationsTest.RsaNoPaddingSuccess
 *
 * Verifies that raw RSA decryption works.
 */
TEST_P(EncryptionOperationsTest, RsaNoPaddingSuccess) {
    for (uint64_t exponent : {3, 65537}) {
        ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                     .Authorization(TAG_NO_AUTH_REQUIRED)
                                                     .RsaEncryptionKey(2048, exponent)
                                                     .Padding(PaddingMode::NONE)
                                                     .SetDefaultValidity()));

        string message = string(2048 / 8, 'a');
        auto params = AuthorizationSetBuilder().Padding(PaddingMode::NONE);
        string ciphertext1 = LocalRsaEncryptMessage(message, params);
        EXPECT_EQ(2048U / 8, ciphertext1.size());

        string ciphertext2 = LocalRsaEncryptMessage(message, params);
        EXPECT_EQ(2048U / 8, ciphertext2.size());

        // Unpadded RSA is deterministic
        EXPECT_EQ(ciphertext1, ciphertext2);

        CheckedDeleteKey();
    }
}

/*
 * EncryptionOperationsTest.RsaNoPaddingShortMessage
 *
 * Verifies that raw RSA decryption of short messages works.
 */
TEST_P(EncryptionOperationsTest, RsaNoPaddingShortMessage) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .RsaEncryptionKey(2048, 65537)
                                                 .Padding(PaddingMode::NONE)
                                                 .SetDefaultValidity()));

    string message = "1";
    auto params = AuthorizationSetBuilder().Padding(PaddingMode::NONE);

    string ciphertext = LocalRsaEncryptMessage(message, params);
    EXPECT_EQ(2048U / 8, ciphertext.size());

    string expected_plaintext = string(2048U / 8 - 1, 0) + message;
    string plaintext = DecryptMessage(ciphertext, params);

    EXPECT_EQ(expected_plaintext, plaintext);
}

/*
 * EncryptionOperationsTest.RsaOaepSuccess
 *
 * Verifies that RSA-OAEP decryption operations work, with all digests.
 */
TEST_P(EncryptionOperationsTest, RsaOaepSuccess) {
    auto digests = ValidDigests(false /* withNone */, true /* withMD5 */);

    size_t key_size = 2048;  // Need largish key for SHA-512 test.
    ASSERT_EQ(ErrorCode::OK,
              GenerateKey(AuthorizationSetBuilder()
                                  .Authorization(TAG_NO_AUTH_REQUIRED)
                                  .RsaEncryptionKey(key_size, 65537)
                                  .Padding(PaddingMode::RSA_OAEP)
                                  .Digest(digests)
                                  .Authorization(TAG_RSA_OAEP_MGF_DIGEST, Digest::SHA1)
                                  .SetDefaultValidity()));

    string message = "Hello";

    for (auto digest : digests) {
        SCOPED_TRACE(testing::Message() << "digest-" << digest);

        auto params = AuthorizationSetBuilder()
                              .Digest(digest)
                              .Padding(PaddingMode::RSA_OAEP)
                              .Authorization(TAG_RSA_OAEP_MGF_DIGEST, Digest::SHA1);
        string ciphertext1 = LocalRsaEncryptMessage(message, params);
        if (HasNonfatalFailure()) std::cout << "-->" << digest << std::endl;
        EXPECT_EQ(key_size / 8, ciphertext1.size());

        string ciphertext2 = LocalRsaEncryptMessage(message, params);
        EXPECT_EQ(key_size / 8, ciphertext2.size());

        // OAEP randomizes padding so every result should be different (with astronomically high
        // probability).
        EXPECT_NE(ciphertext1, ciphertext2);

        string plaintext1 = DecryptMessage(ciphertext1, params);
        EXPECT_EQ(message, plaintext1) << "RSA-OAEP failed with digest " << digest;
        string plaintext2 = DecryptMessage(ciphertext2, params);
        EXPECT_EQ(message, plaintext2) << "RSA-OAEP failed with digest " << digest;

        // Decrypting corrupted ciphertext should fail.
        size_t offset_to_corrupt = random() % ciphertext1.size();
        char corrupt_byte;
        do {
            corrupt_byte = static_cast<char>(random() % 256);
        } while (corrupt_byte == ciphertext1[offset_to_corrupt]);
        ciphertext1[offset_to_corrupt] = corrupt_byte;

        EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, params));
        string result;
        EXPECT_EQ(ErrorCode::UNKNOWN_ERROR, Finish(ciphertext1, &result));
        EXPECT_EQ(0U, result.size());
    }
}

/*
 * EncryptionOperationsTest.RsaOaepInvalidDigest
 *
 * Verifies that RSA-OAEP decryption operations fail in the correct way when asked to operate
 * without a digest.
 */
TEST_P(EncryptionOperationsTest, RsaOaepInvalidDigest) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .RsaEncryptionKey(2048, 65537)
                                                 .Padding(PaddingMode::RSA_OAEP)
                                                 .Digest(Digest::NONE)
                                                 .SetDefaultValidity()));

    auto params = AuthorizationSetBuilder().Padding(PaddingMode::RSA_OAEP).Digest(Digest::NONE);
    EXPECT_EQ(ErrorCode::INCOMPATIBLE_DIGEST, Begin(KeyPurpose::DECRYPT, params));
}

/*
 * EncryptionOperationsTest.RsaOaepInvalidPadding
 *
 * Verifies that RSA-OAEP decryption operations fail in the correct way when asked to operate
 * with a padding value that is only suitable for signing/verifying.
 */
TEST_P(EncryptionOperationsTest, RsaOaepInvalidPadding) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .RsaEncryptionKey(2048, 65537)
                                                 .Padding(PaddingMode::RSA_PSS)
                                                 .Digest(Digest::NONE)
                                                 .SetDefaultValidity()));

    auto params = AuthorizationSetBuilder().Padding(PaddingMode::RSA_PSS).Digest(Digest::NONE);
    EXPECT_EQ(ErrorCode::UNSUPPORTED_PADDING_MODE, Begin(KeyPurpose::DECRYPT, params));
}

/*
 * EncryptionOperationsTest.RsaOaepDecryptWithWrongDigest
 *
 * Verifies that RSA-OAEP decryption operations fail in the correct way when asked to decrypt
 * with a different digest than was used to encrypt.
 */
TEST_P(EncryptionOperationsTest, RsaOaepDecryptWithWrongDigest) {
    if (SecLevel() == SecurityLevel::STRONGBOX) return;

    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .RsaEncryptionKey(1024, 65537)
                                                 .Padding(PaddingMode::RSA_OAEP)
                                                 .Digest(Digest::SHA_2_224, Digest::SHA_2_256)
                                                 .SetDefaultValidity()));
    string message = "Hello World!";
    string ciphertext = LocalRsaEncryptMessage(
            message,
            AuthorizationSetBuilder().Digest(Digest::SHA_2_224).Padding(PaddingMode::RSA_OAEP));

    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, AuthorizationSetBuilder()
                                                                .Digest(Digest::SHA_2_256)
                                                                .Padding(PaddingMode::RSA_OAEP)));
    string result;
    EXPECT_EQ(ErrorCode::UNKNOWN_ERROR, Finish(ciphertext, &result));
    EXPECT_EQ(0U, result.size());
}

/*
 * EncryptionOperationsTest.RsaOaepWithMGFDigestSuccess
 *
 * Verifies that RSA-OAEP decryption operations work, with all SHA 256 digests and all type of MGF1
 * digests.
 */
TEST_P(EncryptionOperationsTest, RsaOaepWithMGFDigestSuccess) {
    auto digests = ValidDigests(false /* withNone */, true /* withMD5 */);

    size_t key_size = 2048;  // Need largish key for SHA-512 test.
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .OaepMGFDigest(digests)
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .RsaEncryptionKey(key_size, 65537)
                                                 .Padding(PaddingMode::RSA_OAEP)
                                                 .Digest(Digest::SHA_2_256)
                                                 .SetDefaultValidity()));

    string message = "Hello";

    for (auto digest : digests) {
        auto params = AuthorizationSetBuilder()
                              .Authorization(TAG_RSA_OAEP_MGF_DIGEST, digest)
                              .Digest(Digest::SHA_2_256)
                              .Padding(PaddingMode::RSA_OAEP);
        string ciphertext1 = LocalRsaEncryptMessage(message, params);
        if (HasNonfatalFailure()) std::cout << "-->" << digest << std::endl;
        EXPECT_EQ(key_size / 8, ciphertext1.size());

        string ciphertext2 = LocalRsaEncryptMessage(message, params);
        EXPECT_EQ(key_size / 8, ciphertext2.size());

        // OAEP randomizes padding so every result should be different (with astronomically high
        // probability).
        EXPECT_NE(ciphertext1, ciphertext2);

        string plaintext1 = DecryptMessage(ciphertext1, params);
        EXPECT_EQ(message, plaintext1) << "RSA-OAEP failed with digest " << digest;
        string plaintext2 = DecryptMessage(ciphertext2, params);
        EXPECT_EQ(message, plaintext2) << "RSA-OAEP failed with digest " << digest;

        // Decrypting corrupted ciphertext should fail.
        size_t offset_to_corrupt = random() % ciphertext1.size();
        char corrupt_byte;
        do {
            corrupt_byte = static_cast<char>(random() % 256);
        } while (corrupt_byte == ciphertext1[offset_to_corrupt]);
        ciphertext1[offset_to_corrupt] = corrupt_byte;

        EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, params));
        string result;
        EXPECT_EQ(ErrorCode::UNKNOWN_ERROR, Finish(ciphertext1, &result));
        EXPECT_EQ(0U, result.size());
    }
}

/*
 * EncryptionOperationsTest.RsaOaepWithMGFIncompatibleDigest
 *
 * Verifies that RSA-OAEP decryption operations fail in the correct way when asked to operate
 * with incompatible MGF digest.
 */
TEST_P(EncryptionOperationsTest, RsaOaepWithMGFIncompatibleDigest) {
    ASSERT_EQ(ErrorCode::OK,
              GenerateKey(AuthorizationSetBuilder()
                                  .Authorization(TAG_RSA_OAEP_MGF_DIGEST, Digest::SHA_2_256)
                                  .Authorization(TAG_NO_AUTH_REQUIRED)
                                  .RsaEncryptionKey(2048, 65537)
                                  .Padding(PaddingMode::RSA_OAEP)
                                  .Digest(Digest::SHA_2_256)
                                  .SetDefaultValidity()));
    string message = "Hello World!";

    auto params = AuthorizationSetBuilder()
                          .Padding(PaddingMode::RSA_OAEP)
                          .Digest(Digest::SHA_2_256)
                          .Authorization(TAG_RSA_OAEP_MGF_DIGEST, Digest::SHA_2_224);
    EXPECT_EQ(ErrorCode::INCOMPATIBLE_MGF_DIGEST, Begin(KeyPurpose::DECRYPT, params));
}

/*
 * EncryptionOperationsTest.RsaOaepWithMGFUnsupportedDigest
 *
 * Verifies that RSA-OAEP encryption operations fail in the correct way when asked to operate
 * with unsupported MGF digest.
 */
TEST_P(EncryptionOperationsTest, RsaOaepWithMGFUnsupportedDigest) {
    ASSERT_EQ(ErrorCode::OK,
              GenerateKey(AuthorizationSetBuilder()
                                  .Authorization(TAG_RSA_OAEP_MGF_DIGEST, Digest::SHA_2_256)
                                  .Authorization(TAG_NO_AUTH_REQUIRED)
                                  .RsaEncryptionKey(2048, 65537)
                                  .Padding(PaddingMode::RSA_OAEP)
                                  .Digest(Digest::SHA_2_256)
                                  .SetDefaultValidity()));
    string message = "Hello World!";

    auto params = AuthorizationSetBuilder()
                          .Padding(PaddingMode::RSA_OAEP)
                          .Digest(Digest::SHA_2_256)
                          .Authorization(TAG_RSA_OAEP_MGF_DIGEST, Digest::NONE);
    EXPECT_EQ(ErrorCode::UNSUPPORTED_MGF_DIGEST, Begin(KeyPurpose::DECRYPT, params));
}

/*
 * EncryptionOperationsTest.RsaPkcs1Success
 *
 * Verifies that RSA PKCS encryption/decrypts works.
 */
TEST_P(EncryptionOperationsTest, RsaPkcs1Success) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .RsaEncryptionKey(2048, 65537)
                                                 .Padding(PaddingMode::RSA_PKCS1_1_5_ENCRYPT)
                                                 .SetDefaultValidity()));

    string message = "Hello World!";
    auto params = AuthorizationSetBuilder().Padding(PaddingMode::RSA_PKCS1_1_5_ENCRYPT);
    string ciphertext1 = LocalRsaEncryptMessage(message, params);
    EXPECT_EQ(2048U / 8, ciphertext1.size());

    string ciphertext2 = LocalRsaEncryptMessage(message, params);
    EXPECT_EQ(2048U / 8, ciphertext2.size());

    // PKCS1 v1.5 randomizes padding so every result should be different.
    EXPECT_NE(ciphertext1, ciphertext2);

    string plaintext = DecryptMessage(ciphertext1, params);
    EXPECT_EQ(message, plaintext);

    // Decrypting corrupted ciphertext should fail.
    size_t offset_to_corrupt = random() % ciphertext1.size();
    char corrupt_byte;
    do {
        corrupt_byte = static_cast<char>(random() % 256);
    } while (corrupt_byte == ciphertext1[offset_to_corrupt]);
    ciphertext1[offset_to_corrupt] = corrupt_byte;

    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, params));
    string result;
    EXPECT_EQ(ErrorCode::UNKNOWN_ERROR, Finish(ciphertext1, &result));
    EXPECT_EQ(0U, result.size());
}

/*
 * EncryptionOperationsTest.EcdsaEncrypt
 *
 * Verifies that attempting to use ECDSA keys to encrypt fails in the correct way.
 */
TEST_P(EncryptionOperationsTest, EcdsaEncrypt) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .EcdsaSigningKey(EcCurve::P_256)
                                                 .Digest(Digest::NONE)
                                                 .SetDefaultValidity()));
    auto params = AuthorizationSetBuilder().Digest(Digest::NONE);
    ASSERT_EQ(ErrorCode::UNSUPPORTED_PURPOSE, Begin(KeyPurpose::ENCRYPT, params));
    ASSERT_EQ(ErrorCode::UNSUPPORTED_PURPOSE, Begin(KeyPurpose::DECRYPT, params));
}

/*
 * EncryptionOperationsTest.HmacEncrypt
 *
 * Verifies that attempting to use HMAC keys to encrypt fails in the correct way.
 */
TEST_P(EncryptionOperationsTest, HmacEncrypt) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .HmacKey(128)
                                                 .Digest(Digest::SHA_2_256)
                                                 .Padding(PaddingMode::NONE)
                                                 .Authorization(TAG_MIN_MAC_LENGTH, 128)));
    auto params = AuthorizationSetBuilder()
                          .Digest(Digest::SHA_2_256)
                          .Padding(PaddingMode::NONE)
                          .Authorization(TAG_MAC_LENGTH, 128);
    ASSERT_EQ(ErrorCode::UNSUPPORTED_PURPOSE, Begin(KeyPurpose::ENCRYPT, params));
    ASSERT_EQ(ErrorCode::UNSUPPORTED_PURPOSE, Begin(KeyPurpose::DECRYPT, params));
}

/*
 * EncryptionOperationsTest.AesEcbRoundTripSuccess
 *
 * Verifies that AES ECB mode works.
 */
TEST_P(EncryptionOperationsTest, AesEcbRoundTripSuccess) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .Authorization(TAG_BLOCK_MODE, BlockMode::ECB)
                                                 .Padding(PaddingMode::NONE)));

    ASSERT_GT(key_blob_.size(), 0U);
    auto params = AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::NONE);

    // Two-block message.
    string message = "12345678901234567890123456789012";
    string ciphertext1 = EncryptMessage(message, params);
    EXPECT_EQ(message.size(), ciphertext1.size());

    string ciphertext2 = EncryptMessage(string(message), params);
    EXPECT_EQ(message.size(), ciphertext2.size());

    // ECB is deterministic.
    EXPECT_EQ(ciphertext1, ciphertext2);

    string plaintext = DecryptMessage(ciphertext1, params);
    EXPECT_EQ(message, plaintext);
}

/*
 * EncryptionOperationsTest.AesEcbUnknownTag
 *
 * Verifies that AES ECB operations ignore unknown tags.
 */
TEST_P(EncryptionOperationsTest, AesEcbUnknownTag) {
    int32_t unknown_tag_value = ((7 << 28) /* TagType:BOOL */ | 150);
    Tag unknown_tag = static_cast<Tag>(unknown_tag_value);
    KeyParameter unknown_param;
    unknown_param.tag = unknown_tag;

    vector<KeyCharacteristics> key_characteristics;
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .Authorization(TAG_BLOCK_MODE, BlockMode::ECB)
                                                 .Padding(PaddingMode::NONE)
                                                 .Authorization(unknown_param),
                                         &key_blob_, &key_characteristics));
    ASSERT_GT(key_blob_.size(), 0U);

    // Unknown tags should not be returned in key characteristics.
    AuthorizationSet hw_enforced = HwEnforcedAuthorizations(key_characteristics);
    AuthorizationSet sw_enforced = SwEnforcedAuthorizations(key_characteristics);
    EXPECT_EQ(hw_enforced.find(unknown_tag), -1);
    EXPECT_EQ(sw_enforced.find(unknown_tag), -1);

    // Encrypt without mentioning the unknown parameter.
    auto params = AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::NONE);
    string message = "12345678901234567890123456789012";
    string ciphertext = EncryptMessage(message, params);
    EXPECT_EQ(message.size(), ciphertext.size());

    // Decrypt including the unknown parameter.
    auto decrypt_params = AuthorizationSetBuilder()
                                  .BlockMode(BlockMode::ECB)
                                  .Padding(PaddingMode::NONE)
                                  .Authorization(unknown_param);
    string plaintext = DecryptMessage(ciphertext, decrypt_params);
    EXPECT_EQ(message, plaintext);
}

/*
 * EncryptionOperationsTest.AesWrongMode
 *
 * Verifies that AES encryption fails in the correct way when an unauthorized mode is specified.
 */
TEST_P(EncryptionOperationsTest, AesWrongMode) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .Authorization(TAG_BLOCK_MODE, BlockMode::CBC)
                                                 .Padding(PaddingMode::NONE)));
    ASSERT_GT(key_blob_.size(), 0U);

    EXPECT_EQ(
            ErrorCode::INCOMPATIBLE_BLOCK_MODE,
            Begin(KeyPurpose::ENCRYPT,
                  AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::NONE)));
}

/*
 * EncryptionOperationsTest.AesWrongPadding
 *
 * Verifies that AES encryption fails in the correct way when an unauthorized padding is specified.
 */
TEST_P(EncryptionOperationsTest, AesWrongPadding) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .Authorization(TAG_BLOCK_MODE, BlockMode::CBC)
                                                 .Padding(PaddingMode::NONE)));
    ASSERT_GT(key_blob_.size(), 0U);

    EXPECT_EQ(
            ErrorCode::INCOMPATIBLE_PADDING_MODE,
            Begin(KeyPurpose::ENCRYPT,
                  AuthorizationSetBuilder().BlockMode(BlockMode::CBC).Padding(PaddingMode::PKCS7)));
}

/*
 * EncryptionOperationsTest.AesInvalidParams
 *
 * Verifies that AES encryption fails in the correct way when an duplicate parameters are specified.
 */
TEST_P(EncryptionOperationsTest, AesInvalidParams) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .Authorization(TAG_BLOCK_MODE, BlockMode::CBC)
                                                 .Authorization(TAG_BLOCK_MODE, BlockMode::ECB)
                                                 .Padding(PaddingMode::NONE)
                                                 .Padding(PaddingMode::PKCS7)));
    ASSERT_GT(key_blob_.size(), 0U);

    auto result = Begin(KeyPurpose::ENCRYPT, AuthorizationSetBuilder()
                                                     .BlockMode(BlockMode::CBC)
                                                     .BlockMode(BlockMode::ECB)
                                                     .Padding(PaddingMode::NONE));
    EXPECT_TRUE(result == ErrorCode::INCOMPATIBLE_BLOCK_MODE ||
                result == ErrorCode::UNSUPPORTED_BLOCK_MODE);

    result = Begin(KeyPurpose::ENCRYPT, AuthorizationSetBuilder()
                                                .BlockMode(BlockMode::ECB)
                                                .Padding(PaddingMode::NONE)
                                                .Padding(PaddingMode::PKCS7));
    EXPECT_TRUE(result == ErrorCode::INCOMPATIBLE_PADDING_MODE ||
                result == ErrorCode::UNSUPPORTED_PADDING_MODE);
}

/*
 * EncryptionOperationsTest.AesWrongPurpose
 *
 * Verifies that AES encryption fails in the correct way when an unauthorized purpose is
 * specified.
 */
TEST_P(EncryptionOperationsTest, AesWrongPurpose) {
    auto err = GenerateKey(AuthorizationSetBuilder()
                                   .Authorization(TAG_NO_AUTH_REQUIRED)
                                   .AesKey(128)
                                   .Authorization(TAG_PURPOSE, KeyPurpose::ENCRYPT)
                                   .Authorization(TAG_BLOCK_MODE, BlockMode::GCM)
                                   .Authorization(TAG_MIN_MAC_LENGTH, 128)
                                   .Padding(PaddingMode::NONE));
    ASSERT_EQ(ErrorCode::OK, err) << "Got " << err;
    ASSERT_GT(key_blob_.size(), 0U);

    err = Begin(KeyPurpose::DECRYPT, AuthorizationSetBuilder()
                                             .BlockMode(BlockMode::GCM)
                                             .Padding(PaddingMode::NONE)
                                             .Authorization(TAG_MAC_LENGTH, 128));
    EXPECT_EQ(ErrorCode::INCOMPATIBLE_PURPOSE, err) << "Got " << err;

    CheckedDeleteKey();

    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesKey(128)
                                                 .Authorization(TAG_PURPOSE, KeyPurpose::DECRYPT)
                                                 .Authorization(TAG_BLOCK_MODE, BlockMode::GCM)
                                                 .Authorization(TAG_MIN_MAC_LENGTH, 128)
                                                 .Padding(PaddingMode::NONE)));

    err = Begin(KeyPurpose::ENCRYPT, AuthorizationSetBuilder()
                                             .BlockMode(BlockMode::GCM)
                                             .Padding(PaddingMode::NONE)
                                             .Authorization(TAG_MAC_LENGTH, 128));
    EXPECT_EQ(ErrorCode::INCOMPATIBLE_PURPOSE, err) << "Got " << err;
}

/*
 * EncryptionOperationsTest.AesEcbCbcNoPaddingWrongInputSize
 *
 * Verifies that AES encryption fails in the correct way when provided an input that is not a
 * multiple of the block size and no padding is specified.
 */
TEST_P(EncryptionOperationsTest, AesEcbCbcNoPaddingWrongInputSize) {
    for (BlockMode blockMode : {BlockMode::ECB, BlockMode::CBC}) {
        ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                     .Authorization(TAG_NO_AUTH_REQUIRED)
                                                     .AesEncryptionKey(128)
                                                     .Authorization(TAG_BLOCK_MODE, blockMode)
                                                     .Padding(PaddingMode::NONE)));
        // Message is slightly shorter than two blocks.
        string message(16 * 2 - 1, 'a');

        auto params = AuthorizationSetBuilder().BlockMode(blockMode).Padding(PaddingMode::NONE);
        AuthorizationSet out_params;
        EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, params, &out_params));
        string ciphertext;
        EXPECT_EQ(ErrorCode::INVALID_INPUT_LENGTH, Finish(message, &ciphertext));
        EXPECT_EQ(0U, ciphertext.size());

        CheckedDeleteKey();
    }
}

/*
 * EncryptionOperationsTest.AesEcbPkcs7Padding
 *
 * Verifies that AES PKCS7 padding works for any message length.
 */
TEST_P(EncryptionOperationsTest, AesEcbPkcs7Padding) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .Authorization(TAG_BLOCK_MODE, BlockMode::ECB)
                                                 .Padding(PaddingMode::PKCS7)));

    auto params = AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::PKCS7);

    // Try various message lengths; all should work.
    for (size_t i = 0; i < 32; ++i) {
        string message(i, 'a');
        string ciphertext = EncryptMessage(message, params);
        EXPECT_EQ(i + 16 - (i % 16), ciphertext.size());
        string plaintext = DecryptMessage(ciphertext, params);
        EXPECT_EQ(message, plaintext);
    }
}

/*
 * EncryptionOperationsTest.AesEcbWrongPadding
 *
 * Verifies that AES enryption fails in the correct way when an unauthorized padding mode is
 * specified.
 */
TEST_P(EncryptionOperationsTest, AesEcbWrongPadding) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .Authorization(TAG_BLOCK_MODE, BlockMode::ECB)
                                                 .Padding(PaddingMode::NONE)));

    auto params = AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::PKCS7);

    // Try various message lengths; all should fail
    for (size_t i = 0; i < 32; ++i) {
        string message(i, 'a');
        EXPECT_EQ(ErrorCode::INCOMPATIBLE_PADDING_MODE, Begin(KeyPurpose::ENCRYPT, params));
    }
}

/*
 * EncryptionOperationsTest.AesEcbPkcs7PaddingCorrupted
 *
 * Verifies that AES decryption fails in the correct way when the padding is corrupted.
 */
TEST_P(EncryptionOperationsTest, AesEcbPkcs7PaddingCorrupted) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .Authorization(TAG_BLOCK_MODE, BlockMode::ECB)
                                                 .Padding(PaddingMode::PKCS7)));

    auto params = AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::PKCS7);

    string message = "a";
    string ciphertext = EncryptMessage(message, params);
    EXPECT_EQ(16U, ciphertext.size());
    EXPECT_NE(ciphertext, message);
    ++ciphertext[ciphertext.size() / 2];

    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, params));
    string plaintext;
    EXPECT_EQ(ErrorCode::INVALID_INPUT_LENGTH, Finish(message, &plaintext));
}

vector<uint8_t> CopyIv(const AuthorizationSet& set) {
    auto iv = set.GetTagValue(TAG_NONCE);
    EXPECT_TRUE(iv);
    return iv->get();
}

/*
 * EncryptionOperationsTest.AesCtrRoundTripSuccess
 *
 * Verifies that AES CTR mode works.
 */
TEST_P(EncryptionOperationsTest, AesCtrRoundTripSuccess) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .Authorization(TAG_BLOCK_MODE, BlockMode::CTR)
                                                 .Padding(PaddingMode::NONE)));

    auto params = AuthorizationSetBuilder().BlockMode(BlockMode::CTR).Padding(PaddingMode::NONE);

    string message = "123";
    AuthorizationSet out_params;
    string ciphertext1 = EncryptMessage(message, params, &out_params);
    vector<uint8_t> iv1 = CopyIv(out_params);
    EXPECT_EQ(16U, iv1.size());

    EXPECT_EQ(message.size(), ciphertext1.size());

    out_params.Clear();
    string ciphertext2 = EncryptMessage(message, params, &out_params);
    vector<uint8_t> iv2 = CopyIv(out_params);
    EXPECT_EQ(16U, iv2.size());

    // IVs should be random, so ciphertexts should differ.
    EXPECT_NE(ciphertext1, ciphertext2);

    auto params_iv1 =
            AuthorizationSetBuilder().Authorizations(params).Authorization(TAG_NONCE, iv1);
    auto params_iv2 =
            AuthorizationSetBuilder().Authorizations(params).Authorization(TAG_NONCE, iv2);

    string plaintext = DecryptMessage(ciphertext1, params_iv1);
    EXPECT_EQ(message, plaintext);
    plaintext = DecryptMessage(ciphertext2, params_iv2);
    EXPECT_EQ(message, plaintext);

    // Using the wrong IV will result in a "valid" decryption, but the data will be garbage.
    plaintext = DecryptMessage(ciphertext1, params_iv2);
    EXPECT_NE(message, plaintext);
    plaintext = DecryptMessage(ciphertext2, params_iv1);
    EXPECT_NE(message, plaintext);
}

/*
 * EncryptionOperationsTest.AesIncremental
 *
 * Verifies that AES works, all modes, when provided data in various size increments.
 */
TEST_P(EncryptionOperationsTest, AesIncremental) {
    auto block_modes = {
            BlockMode::ECB,
            BlockMode::CBC,
            BlockMode::CTR,
            BlockMode::GCM,
    };

    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .BlockMode(block_modes)
                                                 .Padding(PaddingMode::NONE)
                                                 .Authorization(TAG_MIN_MAC_LENGTH, 128)));

    for (int increment = 1; increment <= 240; ++increment) {
        for (auto block_mode : block_modes) {
            string message(240, 'a');
            auto params =
                    AuthorizationSetBuilder().BlockMode(block_mode).Padding(PaddingMode::NONE);
            if (block_mode == BlockMode::GCM) {
                params.Authorization(TAG_MAC_LENGTH, 128) /* for GCM */;
            }

            AuthorizationSet output_params;
            EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, params, &output_params));

            string ciphertext;
            string to_send;
            for (size_t i = 0; i < message.size(); i += increment) {
                EXPECT_EQ(ErrorCode::OK, Update(message.substr(i, increment), &ciphertext));
            }
            EXPECT_EQ(ErrorCode::OK, Finish(to_send, &ciphertext))
                    << "Error sending " << to_send << " with block mode " << block_mode;

            switch (block_mode) {
                case BlockMode::GCM:
                    EXPECT_EQ(message.size() + 16, ciphertext.size());
                    break;
                case BlockMode::CTR:
                    EXPECT_EQ(message.size(), ciphertext.size());
                    break;
                case BlockMode::CBC:
                case BlockMode::ECB:
                    EXPECT_EQ(message.size() + message.size() % 16, ciphertext.size());
                    break;
            }

            auto iv = output_params.GetTagValue(TAG_NONCE);
            switch (block_mode) {
                case BlockMode::CBC:
                case BlockMode::GCM:
                case BlockMode::CTR:
                    ASSERT_TRUE(iv) << "No IV for block mode " << block_mode;
                    EXPECT_EQ(block_mode == BlockMode::GCM ? 12U : 16U, iv->get().size());
                    params.push_back(TAG_NONCE, iv->get());
                    break;

                case BlockMode::ECB:
                    EXPECT_FALSE(iv) << "ECB mode should not generate IV";
                    break;
            }

            EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, params))
                    << "Decrypt begin() failed for block mode " << block_mode;

            string plaintext;
            for (size_t i = 0; i < ciphertext.size(); i += increment) {
                EXPECT_EQ(ErrorCode::OK, Update(ciphertext.substr(i, increment), &plaintext));
            }
            ErrorCode error = Finish(to_send, &plaintext);
            ASSERT_EQ(ErrorCode::OK, error) << "Decryption failed for block mode " << block_mode
                                            << " and increment " << increment;
            if (error == ErrorCode::OK) {
                ASSERT_EQ(message, plaintext) << "Decryption didn't match for block mode "
                                              << block_mode << " and increment " << increment;
            }
        }
    }
}

struct AesCtrSp80038aTestVector {
    const char* key;
    const char* nonce;
    const char* plaintext;
    const char* ciphertext;
};

// These test vectors are taken from
// http://csrc.nist.gov/publications/nistpubs/800-38a/sp800-38a.pdf, section F.5.
static const AesCtrSp80038aTestVector kAesCtrSp80038aTestVectors[] = {
        // AES-128
        {
                "2b7e151628aed2a6abf7158809cf4f3c",
                "f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff",
                "6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e51"
                "30c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710",
                "874d6191b620e3261bef6864990db6ce9806f66b7970fdff8617187bb9fffdff"
                "5ae4df3edbd5d35e5b4f09020db03eab1e031dda2fbe03d1792170a0f3009cee",
        },
        // AES-192
        {
                "8e73b0f7da0e6452c810f32b809079e562f8ead2522c6b7b",
                "f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff",
                "6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e51"
                "30c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710",
                "1abc932417521ca24f2b0459fe7e6e0b090339ec0aa6faefd5ccc2c6f4ce8e94"
                "1e36b26bd1ebc670d1bd1d665620abf74f78a7f6d29809585a97daec58c6b050",
        },
        // AES-256
        {
                "603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4",
                "f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff",
                "6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e51"
                "30c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710",
                "601ec313775789a5b7a7f504bbf3d228f443e3ca4d62b59aca84e990cacaf5c5"
                "2b0930daa23de94ce87017ba2d84988ddfc9c58db67aada613c2dd08457941a6",
        },
};

/*
 * EncryptionOperationsTest.AesCtrSp80038aTestVector
 *
 * Verifies AES CTR implementation against SP800-38A test vectors.
 */
TEST_P(EncryptionOperationsTest, AesCtrSp80038aTestVector) {
    std::vector<uint32_t> InvalidSizes = InvalidKeySizes(Algorithm::AES);
    for (size_t i = 0; i < 3; i++) {
        const AesCtrSp80038aTestVector& test(kAesCtrSp80038aTestVectors[i]);
        const string key = hex2str(test.key);
        if (std::find(InvalidSizes.begin(), InvalidSizes.end(), (key.size() * 8)) !=
            InvalidSizes.end())
            continue;
        const string nonce = hex2str(test.nonce);
        const string plaintext = hex2str(test.plaintext);
        const string ciphertext = hex2str(test.ciphertext);
        CheckAesCtrTestVector(key, nonce, plaintext, ciphertext);
    }
}

/*
 * EncryptionOperationsTest.AesCtrIncompatiblePaddingMode
 *
 * Verifies that keymint rejects use of CTR mode with PKCS7 padding in the correct way.
 */
TEST_P(EncryptionOperationsTest, AesCtrIncompatiblePaddingMode) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .Authorization(TAG_BLOCK_MODE, BlockMode::CTR)
                                                 .Padding(PaddingMode::PKCS7)));
    auto params = AuthorizationSetBuilder().BlockMode(BlockMode::CTR).Padding(PaddingMode::NONE);
    EXPECT_EQ(ErrorCode::INCOMPATIBLE_PADDING_MODE, Begin(KeyPurpose::ENCRYPT, params));
}

/*
 * EncryptionOperationsTest.AesCtrInvalidCallerNonce
 *
 * Verifies that keymint fails correctly when the user supplies an incorrect-size nonce.
 */
TEST_P(EncryptionOperationsTest, AesCtrInvalidCallerNonce) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .Authorization(TAG_BLOCK_MODE, BlockMode::CTR)
                                                 .Authorization(TAG_CALLER_NONCE)
                                                 .Padding(PaddingMode::NONE)));

    auto params = AuthorizationSetBuilder()
                          .BlockMode(BlockMode::CTR)
                          .Padding(PaddingMode::NONE)
                          .Authorization(TAG_NONCE, AidlBuf(string(1, 'a')));
    EXPECT_EQ(ErrorCode::INVALID_NONCE, Begin(KeyPurpose::ENCRYPT, params));

    params = AuthorizationSetBuilder()
                     .BlockMode(BlockMode::CTR)
                     .Padding(PaddingMode::NONE)
                     .Authorization(TAG_NONCE, AidlBuf(string(15, 'a')));
    EXPECT_EQ(ErrorCode::INVALID_NONCE, Begin(KeyPurpose::ENCRYPT, params));

    params = AuthorizationSetBuilder()
                     .BlockMode(BlockMode::CTR)
                     .Padding(PaddingMode::NONE)
                     .Authorization(TAG_NONCE, AidlBuf(string(17, 'a')));
    EXPECT_EQ(ErrorCode::INVALID_NONCE, Begin(KeyPurpose::ENCRYPT, params));
}

/*
 * EncryptionOperationsTest.AesCbcRoundTripSuccess
 *
 * Verifies that keymint fails correctly when the user supplies an incorrect-size nonce.
 */
TEST_P(EncryptionOperationsTest, AesCbcRoundTripSuccess) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .Authorization(TAG_BLOCK_MODE, BlockMode::CBC)
                                                 .Padding(PaddingMode::NONE)));
    // Two-block message.
    string message = "12345678901234567890123456789012";
    auto params = AuthorizationSetBuilder().BlockMode(BlockMode::CBC).Padding(PaddingMode::NONE);
    AuthorizationSet out_params;
    string ciphertext1 = EncryptMessage(message, params, &out_params);
    vector<uint8_t> iv1 = CopyIv(out_params);
    EXPECT_EQ(message.size(), ciphertext1.size());

    out_params.Clear();

    string ciphertext2 = EncryptMessage(message, params, &out_params);
    vector<uint8_t> iv2 = CopyIv(out_params);
    EXPECT_EQ(message.size(), ciphertext2.size());

    // IVs should be random, so ciphertexts should differ.
    EXPECT_NE(ciphertext1, ciphertext2);

    params.push_back(TAG_NONCE, iv1);
    string plaintext = DecryptMessage(ciphertext1, params);
    EXPECT_EQ(message, plaintext);
}

/*
 * EncryptionOperationsTest.AesCallerNonce
 *
 * Verifies that AES caller-provided nonces work correctly.
 */
TEST_P(EncryptionOperationsTest, AesCallerNonce) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .Authorization(TAG_BLOCK_MODE, BlockMode::CBC)
                                                 .Authorization(TAG_CALLER_NONCE)
                                                 .Padding(PaddingMode::NONE)));

    string message = "12345678901234567890123456789012";

    // Don't specify nonce, should get a random one.
    AuthorizationSetBuilder params =
            AuthorizationSetBuilder().BlockMode(BlockMode::CBC).Padding(PaddingMode::NONE);
    AuthorizationSet out_params;
    string ciphertext = EncryptMessage(message, params, &out_params);
    EXPECT_EQ(message.size(), ciphertext.size());
    EXPECT_EQ(16U, out_params.GetTagValue(TAG_NONCE)->get().size());

    params.push_back(TAG_NONCE, out_params.GetTagValue(TAG_NONCE)->get());
    string plaintext = DecryptMessage(ciphertext, params);
    EXPECT_EQ(message, plaintext);

    // Now specify a nonce, should also work.
    params = AuthorizationSetBuilder()
                     .BlockMode(BlockMode::CBC)
                     .Padding(PaddingMode::NONE)
                     .Authorization(TAG_NONCE, AidlBuf("abcdefghijklmnop"));
    out_params.Clear();
    ciphertext = EncryptMessage(message, params, &out_params);

    // Decrypt with correct nonce.
    plaintext = DecryptMessage(ciphertext, params);
    EXPECT_EQ(message, plaintext);

    // Try with wrong nonce.
    params = AuthorizationSetBuilder()
                     .BlockMode(BlockMode::CBC)
                     .Padding(PaddingMode::NONE)
                     .Authorization(TAG_NONCE, AidlBuf("aaaaaaaaaaaaaaaa"));
    plaintext = DecryptMessage(ciphertext, params);
    EXPECT_NE(message, plaintext);
}

/*
 * EncryptionOperationsTest.AesCallerNonceProhibited
 *
 * Verifies that caller-provided nonces are not permitted when not specified in the key
 * authorizations.
 */
TEST_P(EncryptionOperationsTest, AesCallerNonceProhibited) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .Authorization(TAG_BLOCK_MODE, BlockMode::CBC)
                                                 .Padding(PaddingMode::NONE)));

    string message = "12345678901234567890123456789012";

    // Don't specify nonce, should get a random one.
    AuthorizationSetBuilder params =
            AuthorizationSetBuilder().BlockMode(BlockMode::CBC).Padding(PaddingMode::NONE);
    AuthorizationSet out_params;
    string ciphertext = EncryptMessage(message, params, &out_params);
    EXPECT_EQ(message.size(), ciphertext.size());
    EXPECT_EQ(16U, out_params.GetTagValue(TAG_NONCE)->get().size());

    params.push_back(TAG_NONCE, out_params.GetTagValue(TAG_NONCE)->get());
    string plaintext = DecryptMessage(ciphertext, params);
    EXPECT_EQ(message, plaintext);

    // Now specify a nonce, should fail
    params = AuthorizationSetBuilder()
                     .BlockMode(BlockMode::CBC)
                     .Padding(PaddingMode::NONE)
                     .Authorization(TAG_NONCE, AidlBuf("abcdefghijklmnop"));
    out_params.Clear();
    EXPECT_EQ(ErrorCode::CALLER_NONCE_PROHIBITED, Begin(KeyPurpose::ENCRYPT, params, &out_params));
}

/*
 * EncryptionOperationsTest.AesGcmRoundTripSuccess
 *
 * Verifies that AES GCM mode works.
 */
TEST_P(EncryptionOperationsTest, AesGcmRoundTripSuccess) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .Authorization(TAG_BLOCK_MODE, BlockMode::GCM)
                                                 .Padding(PaddingMode::NONE)
                                                 .Authorization(TAG_MIN_MAC_LENGTH, 128)));

    string aad = "foobar";
    string message = "123456789012345678901234567890123456";

    auto begin_params = AuthorizationSetBuilder()
                                .BlockMode(BlockMode::GCM)
                                .Padding(PaddingMode::NONE)
                                .Authorization(TAG_MAC_LENGTH, 128);

    // Encrypt
    AuthorizationSet begin_out_params;
    ASSERT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, begin_params, &begin_out_params))
            << "Begin encrypt";
    string ciphertext;
    ASSERT_EQ(ErrorCode::OK, UpdateAad(aad));
    ASSERT_EQ(ErrorCode::OK, Finish(message, &ciphertext));
    ASSERT_EQ(ciphertext.length(), message.length() + 16);

    // Grab nonce
    begin_params.push_back(begin_out_params);

    // Decrypt.
    ASSERT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, begin_params)) << "Begin decrypt";
    ASSERT_EQ(ErrorCode::OK, UpdateAad(aad));
    string plaintext;
    EXPECT_EQ(ErrorCode::OK, Finish(ciphertext, &plaintext));
    EXPECT_EQ(message.length(), plaintext.length());
    EXPECT_EQ(message, plaintext);
}

/*
 * EncryptionOperationsTest.AesGcmRoundTripWithDelaySuccess
 *
 * Verifies that AES GCM mode works, even when there's a long delay
 * between operations.
 */
TEST_P(EncryptionOperationsTest, AesGcmRoundTripWithDelaySuccess) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .Authorization(TAG_BLOCK_MODE, BlockMode::GCM)
                                                 .Padding(PaddingMode::NONE)
                                                 .Authorization(TAG_MIN_MAC_LENGTH, 128)));

    string aad = "foobar";
    string message = "123456789012345678901234567890123456";

    auto begin_params = AuthorizationSetBuilder()
                                .BlockMode(BlockMode::GCM)
                                .Padding(PaddingMode::NONE)
                                .Authorization(TAG_MAC_LENGTH, 128);

    // Encrypt
    AuthorizationSet begin_out_params;
    ASSERT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, begin_params, &begin_out_params))
            << "Begin encrypt";
    string ciphertext;
    AuthorizationSet update_out_params;
    ASSERT_EQ(ErrorCode::OK, UpdateAad(aad));
    sleep(5);
    ASSERT_EQ(ErrorCode::OK, Finish(message, &ciphertext));

    ASSERT_EQ(ciphertext.length(), message.length() + 16);

    // Grab nonce
    begin_params.push_back(begin_out_params);

    // Decrypt.
    ASSERT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, begin_params)) << "Begin decrypt";
    string plaintext;
    ASSERT_EQ(ErrorCode::OK, UpdateAad(aad));
    sleep(5);
    ASSERT_EQ(ErrorCode::OK, Update(ciphertext, &plaintext));
    sleep(5);
    EXPECT_EQ(ErrorCode::OK, Finish("", &plaintext));
    EXPECT_EQ(message.length(), plaintext.length());
    EXPECT_EQ(message, plaintext);
}

/*
 * EncryptionOperationsTest.AesGcmDifferentNonces
 *
 * Verifies that encrypting the same data with different nonces produces different outputs.
 */
TEST_P(EncryptionOperationsTest, AesGcmDifferentNonces) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .Authorization(TAG_BLOCK_MODE, BlockMode::GCM)
                                                 .Padding(PaddingMode::NONE)
                                                 .Authorization(TAG_MIN_MAC_LENGTH, 128)
                                                 .Authorization(TAG_CALLER_NONCE)));

    string aad = "foobar";
    string message = "123456789012345678901234567890123456";
    string nonce1 = "000000000000";
    string nonce2 = "111111111111";
    string nonce3 = "222222222222";

    string ciphertext1 =
            EncryptMessage(message, BlockMode::GCM, PaddingMode::NONE, 128, AidlBuf(nonce1));
    string ciphertext2 =
            EncryptMessage(message, BlockMode::GCM, PaddingMode::NONE, 128, AidlBuf(nonce2));
    string ciphertext3 =
            EncryptMessage(message, BlockMode::GCM, PaddingMode::NONE, 128, AidlBuf(nonce3));

    ASSERT_NE(ciphertext1, ciphertext2);
    ASSERT_NE(ciphertext1, ciphertext3);
    ASSERT_NE(ciphertext2, ciphertext3);
}

/*
 * EncryptionOperationsTest.AesGcmDifferentAutoNonces
 *
 * Verifies that encrypting the same data with KeyMint generated nonces produces different outputs.
 */
TEST_P(EncryptionOperationsTest, AesGcmDifferentAutoNonces) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .Authorization(TAG_BLOCK_MODE, BlockMode::GCM)
                                                 .Padding(PaddingMode::NONE)
                                                 .Authorization(TAG_MIN_MAC_LENGTH, 128)));

    string aad = "foobar";
    string message = "123456789012345678901234567890123456";

    string ciphertext1 = EncryptMessage(message, BlockMode::GCM, PaddingMode::NONE, 128);
    string ciphertext2 = EncryptMessage(message, BlockMode::GCM, PaddingMode::NONE, 128);
    string ciphertext3 = EncryptMessage(message, BlockMode::GCM, PaddingMode::NONE, 128);

    ASSERT_NE(ciphertext1, ciphertext2);
    ASSERT_NE(ciphertext1, ciphertext3);
    ASSERT_NE(ciphertext2, ciphertext3);
}

/*
 * EncryptionOperationsTest.AesGcmTooShortTag
 *
 * Verifies that AES GCM mode fails correctly when a too-short tag length is specified.
 */
TEST_P(EncryptionOperationsTest, AesGcmTooShortTag) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .BlockMode(BlockMode::GCM)
                                                 .Padding(PaddingMode::NONE)
                                                 .Authorization(TAG_MIN_MAC_LENGTH, 128)));
    string message = "123456789012345678901234567890123456";
    auto params = AuthorizationSetBuilder()
                          .BlockMode(BlockMode::GCM)
                          .Padding(PaddingMode::NONE)
                          .Authorization(TAG_MAC_LENGTH, 96);

    EXPECT_EQ(ErrorCode::INVALID_MAC_LENGTH, Begin(KeyPurpose::ENCRYPT, params));
}

/*
 * EncryptionOperationsTest.AesGcmTooShortTagOnDecrypt
 *
 * Verifies that AES GCM mode fails correctly when a too-short tag is provided to decryption.
 */
TEST_P(EncryptionOperationsTest, AesGcmTooShortTagOnDecrypt) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .BlockMode(BlockMode::GCM)
                                                 .Padding(PaddingMode::NONE)
                                                 .Authorization(TAG_MIN_MAC_LENGTH, 128)));
    string aad = "foobar";
    string message = "123456789012345678901234567890123456";
    auto params = AuthorizationSetBuilder()
                          .BlockMode(BlockMode::GCM)
                          .Padding(PaddingMode::NONE)
                          .Authorization(TAG_MAC_LENGTH, 128);

    // Encrypt
    AuthorizationSet begin_out_params;
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, params, &begin_out_params));
    EXPECT_EQ(1U, begin_out_params.size());
    ASSERT_TRUE(begin_out_params.GetTagValue(TAG_NONCE));

    AuthorizationSet finish_out_params;
    string ciphertext;
    ASSERT_EQ(ErrorCode::OK, UpdateAad(aad));
    EXPECT_EQ(ErrorCode::OK, Finish(message, &ciphertext));

    params = AuthorizationSetBuilder()
                     .Authorizations(begin_out_params)
                     .BlockMode(BlockMode::GCM)
                     .Padding(PaddingMode::NONE)
                     .Authorization(TAG_MAC_LENGTH, 96);

    // Decrypt.
    EXPECT_EQ(ErrorCode::INVALID_MAC_LENGTH, Begin(KeyPurpose::DECRYPT, params));
}

/*
 * EncryptionOperationsTest.AesGcmCorruptKey
 *
 * Verifies that AES GCM mode fails correctly when the decryption key is incorrect.
 */
TEST_P(EncryptionOperationsTest, AesGcmCorruptKey) {
    const uint8_t nonce_bytes[] = {
            0xb7, 0x94, 0x37, 0xae, 0x08, 0xff, 0x35, 0x5d, 0x7d, 0x8a, 0x4d, 0x0f,
    };
    string nonce = make_string(nonce_bytes);
    const uint8_t ciphertext_bytes[] = {
            0xb3, 0xf6, 0x79, 0x9e, 0x8f, 0x93, 0x26, 0xf2, 0xdf, 0x1e, 0x80, 0xfc,
            0xd2, 0xcb, 0x16, 0xd7, 0x8c, 0x9d, 0xc7, 0xcc, 0x14, 0xbb, 0x67, 0x78,
            0x62, 0xdc, 0x6c, 0x63, 0x9b, 0x3a, 0x63, 0x38, 0xd2, 0x4b, 0x31, 0x2d,
            0x39, 0x89, 0xe5, 0x92, 0x0b, 0x5d, 0xbf, 0xc9, 0x76, 0x76, 0x5e, 0xfb,
            0xfe, 0x57, 0xbb, 0x38, 0x59, 0x40, 0xa7, 0xa4, 0x3b, 0xdf, 0x05, 0xbd,
            0xda, 0xe3, 0xc9, 0xd6, 0xa2, 0xfb, 0xbd, 0xfc, 0xc0, 0xcb, 0xa0,
    };
    string ciphertext = make_string(ciphertext_bytes);

    auto params = AuthorizationSetBuilder()
                          .BlockMode(BlockMode::GCM)
                          .Padding(PaddingMode::NONE)
                          .Authorization(TAG_MAC_LENGTH, 128)
                          .Authorization(TAG_NONCE, nonce.data(), nonce.size());

    auto import_params = AuthorizationSetBuilder()
                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                 .AesEncryptionKey(128)
                                 .BlockMode(BlockMode::GCM)
                                 .Padding(PaddingMode::NONE)
                                 .Authorization(TAG_CALLER_NONCE)
                                 .Authorization(TAG_MIN_MAC_LENGTH, 128);

    // Import correct key and decrypt
    const uint8_t key_bytes[] = {
            0xba, 0x76, 0x35, 0x4f, 0x0a, 0xed, 0x6e, 0x8d,
            0x91, 0xf4, 0x5c, 0x4f, 0xf5, 0xa0, 0x62, 0xdb,
    };
    string key = make_string(key_bytes);
    ASSERT_EQ(ErrorCode::OK, ImportKey(import_params, KeyFormat::RAW, key));
    string plaintext = DecryptMessage(ciphertext, params);
    CheckedDeleteKey();

    // Corrupt key and attempt to decrypt
    key[0] = 0;
    ASSERT_EQ(ErrorCode::OK, ImportKey(import_params, KeyFormat::RAW, key));
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, params));
    EXPECT_EQ(ErrorCode::VERIFICATION_FAILED, Finish(ciphertext, &plaintext));
    CheckedDeleteKey();
}

/*
 * EncryptionOperationsTest.AesGcmAadNoData
 *
 * Verifies that AES GCM mode works when provided additional authenticated data, but no data to
 * encrypt.
 */
TEST_P(EncryptionOperationsTest, AesGcmAadNoData) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .BlockMode(BlockMode::GCM)
                                                 .Padding(PaddingMode::NONE)
                                                 .Authorization(TAG_MIN_MAC_LENGTH, 128)));

    string aad = "1234567890123456";
    auto params = AuthorizationSetBuilder()
                          .BlockMode(BlockMode::GCM)
                          .Padding(PaddingMode::NONE)
                          .Authorization(TAG_MAC_LENGTH, 128);

    // Encrypt
    AuthorizationSet begin_out_params;
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, params, &begin_out_params));
    string ciphertext;
    AuthorizationSet finish_out_params;
    ASSERT_EQ(ErrorCode::OK, UpdateAad(aad));
    EXPECT_EQ(ErrorCode::OK, Finish(&ciphertext));
    EXPECT_TRUE(finish_out_params.empty());

    // Grab nonce
    params.push_back(begin_out_params);

    // Decrypt.
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, params));
    ASSERT_EQ(ErrorCode::OK, UpdateAad(aad));
    string plaintext;
    EXPECT_EQ(ErrorCode::OK, Finish(ciphertext, &plaintext));

    EXPECT_TRUE(finish_out_params.empty());

    EXPECT_EQ("", plaintext);
}

/*
 * EncryptionOperationsTest.AesGcmMultiPartAad
 *
 * Verifies that AES GCM mode works when provided additional authenticated data in multiple
 * chunks.
 */
TEST_P(EncryptionOperationsTest, AesGcmMultiPartAad) {
    const size_t tag_bits = 128;
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .BlockMode(BlockMode::GCM)
                                                 .Padding(PaddingMode::NONE)
                                                 .Authorization(TAG_MIN_MAC_LENGTH, 128)));

    string message = "123456789012345678901234567890123456";
    auto begin_params = AuthorizationSetBuilder()
                                .BlockMode(BlockMode::GCM)
                                .Padding(PaddingMode::NONE)
                                .Authorization(TAG_MAC_LENGTH, tag_bits);
    AuthorizationSet begin_out_params;

    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, begin_params, &begin_out_params));

    // No data, AAD only.
    EXPECT_EQ(ErrorCode::OK, UpdateAad("foo"));
    EXPECT_EQ(ErrorCode::OK, UpdateAad("foo"));
    string ciphertext;
    EXPECT_EQ(ErrorCode::OK, Update(message, &ciphertext));
    EXPECT_EQ(ErrorCode::OK, Finish(&ciphertext));

    // Expect 128-bit (16-byte) tag appended to ciphertext.
    EXPECT_EQ(message.size() + (tag_bits / 8), ciphertext.size());

    // Grab nonce.
    begin_params.push_back(begin_out_params);

    // Decrypt
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, begin_params));
    EXPECT_EQ(ErrorCode::OK, UpdateAad("foofoo"));
    string plaintext;
    EXPECT_EQ(ErrorCode::OK, Finish(ciphertext, &plaintext));
    EXPECT_EQ(message, plaintext);
}

/*
 * EncryptionOperationsTest.AesGcmAadOutOfOrder
 *
 * Verifies that AES GCM mode fails correctly when given AAD after data to encipher.
 */
TEST_P(EncryptionOperationsTest, AesGcmAadOutOfOrder) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .BlockMode(BlockMode::GCM)
                                                 .Padding(PaddingMode::NONE)
                                                 .Authorization(TAG_MIN_MAC_LENGTH, 128)));

    string message = "123456789012345678901234567890123456";
    auto begin_params = AuthorizationSetBuilder()
                                .BlockMode(BlockMode::GCM)
                                .Padding(PaddingMode::NONE)
                                .Authorization(TAG_MAC_LENGTH, 128);
    AuthorizationSet begin_out_params;

    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, begin_params, &begin_out_params));

    EXPECT_EQ(ErrorCode::OK, UpdateAad("foo"));
    string ciphertext;
    EXPECT_EQ(ErrorCode::OK, Update(message, &ciphertext));
    EXPECT_EQ(ErrorCode::INVALID_TAG, UpdateAad("foo"));

    // The failure should have already cancelled the operation.
    EXPECT_EQ(ErrorCode::INVALID_OPERATION_HANDLE, Abort());

    op_ = {};
}

/*
 * EncryptionOperationsTest.AesGcmBadAad
 *
 * Verifies that AES GCM decryption fails correctly when additional authenticated date is wrong.
 */
TEST_P(EncryptionOperationsTest, AesGcmBadAad) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .BlockMode(BlockMode::GCM)
                                                 .Padding(PaddingMode::NONE)
                                                 .Authorization(TAG_MIN_MAC_LENGTH, 128)));

    string message = "12345678901234567890123456789012";
    auto begin_params = AuthorizationSetBuilder()
                                .BlockMode(BlockMode::GCM)
                                .Padding(PaddingMode::NONE)
                                .Authorization(TAG_MAC_LENGTH, 128);

    // Encrypt
    AuthorizationSet begin_out_params;
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, begin_params, &begin_out_params));
    EXPECT_EQ(ErrorCode::OK, UpdateAad("foobar"));
    string ciphertext;
    EXPECT_EQ(ErrorCode::OK, Finish(message, &ciphertext));

    // Grab nonce
    begin_params.push_back(begin_out_params);

    // Decrypt.
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, begin_params, &begin_out_params));
    EXPECT_EQ(ErrorCode::OK, UpdateAad("barfoo"));
    string plaintext;
    EXPECT_EQ(ErrorCode::VERIFICATION_FAILED, Finish(ciphertext, &plaintext));
}

/*
 * EncryptionOperationsTest.AesGcmWrongNonce
 *
 * Verifies that AES GCM decryption fails correctly when the nonce is incorrect.
 */
TEST_P(EncryptionOperationsTest, AesGcmWrongNonce) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .BlockMode(BlockMode::GCM)
                                                 .Padding(PaddingMode::NONE)
                                                 .Authorization(TAG_MIN_MAC_LENGTH, 128)));

    string message = "12345678901234567890123456789012";
    auto begin_params = AuthorizationSetBuilder()
                                .BlockMode(BlockMode::GCM)
                                .Padding(PaddingMode::NONE)
                                .Authorization(TAG_MAC_LENGTH, 128);

    // Encrypt
    AuthorizationSet begin_out_params;
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, begin_params, &begin_out_params));
    EXPECT_EQ(ErrorCode::OK, UpdateAad("foobar"));
    string ciphertext;
    AuthorizationSet finish_out_params;
    EXPECT_EQ(ErrorCode::OK, Finish(message, &ciphertext));

    // Wrong nonce
    begin_params.push_back(TAG_NONCE, AidlBuf("123456789012"));

    // Decrypt.
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, begin_params, &begin_out_params));
    EXPECT_EQ(ErrorCode::OK, UpdateAad("foobar"));
    string plaintext;
    EXPECT_EQ(ErrorCode::VERIFICATION_FAILED, Finish(ciphertext, &plaintext));

    // With wrong nonce, should have gotten garbage plaintext (or none).
    EXPECT_NE(message, plaintext);
}

/*
 * EncryptionOperationsTest.AesGcmCorruptTag
 *
 * Verifies that AES GCM decryption fails correctly when the tag is wrong.
 */
TEST_P(EncryptionOperationsTest, AesGcmCorruptTag) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .BlockMode(BlockMode::GCM)
                                                 .Padding(PaddingMode::NONE)
                                                 .Authorization(TAG_MIN_MAC_LENGTH, 128)));

    string aad = "1234567890123456";
    string message = "123456789012345678901234567890123456";

    auto params = AuthorizationSetBuilder()
                          .BlockMode(BlockMode::GCM)
                          .Padding(PaddingMode::NONE)
                          .Authorization(TAG_MAC_LENGTH, 128);

    // Encrypt
    AuthorizationSet begin_out_params;
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, params, &begin_out_params));
    EXPECT_EQ(ErrorCode::OK, UpdateAad(aad));
    string ciphertext;
    EXPECT_EQ(ErrorCode::OK, Finish(message, &ciphertext));

    // Corrupt tag
    ++(*ciphertext.rbegin());

    // Grab nonce
    params.push_back(begin_out_params);

    // Decrypt.
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, params));
    EXPECT_EQ(ErrorCode::OK, UpdateAad(aad));
    string plaintext;
    EXPECT_EQ(ErrorCode::VERIFICATION_FAILED, Finish(ciphertext, &plaintext));
}

/*
 * EncryptionOperationsTest.TripleDesEcbRoundTripSuccess
 *
 * Verifies that 3DES is basically functional.
 */
TEST_P(EncryptionOperationsTest, TripleDesEcbRoundTripSuccess) {
    auto auths = AuthorizationSetBuilder()
                         .TripleDesEncryptionKey(168)
                         .BlockMode(BlockMode::ECB)
                         .Authorization(TAG_NO_AUTH_REQUIRED)
                         .Padding(PaddingMode::NONE);

    ASSERT_EQ(ErrorCode::OK, GenerateKey(auths));
    // Two-block message.
    string message = "1234567890123456";
    auto inParams = AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::NONE);
    string ciphertext1 = EncryptMessage(message, inParams);
    EXPECT_EQ(message.size(), ciphertext1.size());

    string ciphertext2 = EncryptMessage(string(message), inParams);
    EXPECT_EQ(message.size(), ciphertext2.size());

    // ECB is deterministic.
    EXPECT_EQ(ciphertext1, ciphertext2);

    string plaintext = DecryptMessage(ciphertext1, inParams);
    EXPECT_EQ(message, plaintext);
}

/*
 * EncryptionOperationsTest.TripleDesEcbNotAuthorized
 *
 * Verifies that CBC keys reject ECB usage.
 */
TEST_P(EncryptionOperationsTest, TripleDesEcbNotAuthorized) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .TripleDesEncryptionKey(168)
                                                 .BlockMode(BlockMode::CBC)
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .Padding(PaddingMode::NONE)));

    auto inParams = AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::NONE);
    EXPECT_EQ(ErrorCode::INCOMPATIBLE_BLOCK_MODE, Begin(KeyPurpose::ENCRYPT, inParams));
}

/*
 * EncryptionOperationsTest.TripleDesEcbPkcs7Padding
 *
 * Tests ECB mode with PKCS#7 padding, various message sizes.
 */
TEST_P(EncryptionOperationsTest, TripleDesEcbPkcs7Padding) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .TripleDesEncryptionKey(168)
                                                 .BlockMode(BlockMode::ECB)
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .Padding(PaddingMode::PKCS7)));

    for (size_t i = 0; i < 32; ++i) {
        string message(i, 'a');
        auto inParams =
                AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::PKCS7);
        string ciphertext = EncryptMessage(message, inParams);
        EXPECT_EQ(i + 8 - (i % 8), ciphertext.size());
        string plaintext = DecryptMessage(ciphertext, inParams);
        EXPECT_EQ(message, plaintext);
    }
}

/*
 * EncryptionOperationsTest.TripleDesEcbNoPaddingKeyWithPkcs7Padding
 *
 * Verifies that keys configured for no padding reject PKCS7 padding
 */
TEST_P(EncryptionOperationsTest, TripleDesEcbNoPaddingKeyWithPkcs7Padding) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .TripleDesEncryptionKey(168)
                                                 .BlockMode(BlockMode::ECB)
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .Padding(PaddingMode::NONE)));
    auto inParams = AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::PKCS7);
    EXPECT_EQ(ErrorCode::INCOMPATIBLE_PADDING_MODE, Begin(KeyPurpose::ENCRYPT, inParams));
}

/*
 * EncryptionOperationsTest.TripleDesEcbPkcs7PaddingCorrupted
 *
 * Verifies that corrupted padding is detected.
 */
TEST_P(EncryptionOperationsTest, TripleDesEcbPkcs7PaddingCorrupted) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .TripleDesEncryptionKey(168)
                                                 .BlockMode(BlockMode::ECB)
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .Padding(PaddingMode::PKCS7)));

    string message = "a";
    string ciphertext = EncryptMessage(message, BlockMode::ECB, PaddingMode::PKCS7);
    EXPECT_EQ(8U, ciphertext.size());
    EXPECT_NE(ciphertext, message);
    ++ciphertext[ciphertext.size() / 2];

    AuthorizationSetBuilder begin_params;
    begin_params.push_back(TAG_BLOCK_MODE, BlockMode::ECB);
    begin_params.push_back(TAG_PADDING, PaddingMode::PKCS7);
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, begin_params));
    string plaintext;
    EXPECT_EQ(ErrorCode::OK, Update(ciphertext, &plaintext));
    EXPECT_EQ(ErrorCode::INVALID_ARGUMENT, Finish(&plaintext));
}

struct TripleDesTestVector {
    const char* name;
    const KeyPurpose purpose;
    const BlockMode block_mode;
    const PaddingMode padding_mode;
    const char* key;
    const char* iv;
    const char* input;
    const char* output;
};

// These test vectors are from NIST CAVP, plus a few custom variants to test padding, since all
// of the NIST vectors are multiples of the block size.
static const TripleDesTestVector kTripleDesTestVectors[] = {
        {
                "TECBMMT3 Encrypt 0", KeyPurpose::ENCRYPT, BlockMode::ECB, PaddingMode::NONE,
                "a2b5bc67da13dc92cd9d344aa238544a0e1fa79ef76810cd",  // key
                "",                                                  // IV
                "329d86bdf1bc5af4",                                  // input
                "d946c2756d78633f",                                  // output
        },
        {
                "TECBMMT3 Encrypt 1", KeyPurpose::ENCRYPT, BlockMode::ECB, PaddingMode::NONE,
                "49e692290d2a5e46bace79b9648a4c5d491004c262dc9d49",  // key
                "",                                                  // IV
                "6b1540781b01ce1997adae102dbf3c5b",                  // input
                "4d0dc182d6e481ac4a3dc6ab6976ccae",                  // output
        },
        {
                "TECBMMT3 Decrypt 0", KeyPurpose::DECRYPT, BlockMode::ECB, PaddingMode::NONE,
                "52daec2ac7dc1958377392682f37860b2cc1ea2304bab0e9",  // key
                "",                                                  // IV
                "6daad94ce08acfe7",                                  // input
                "660e7d32dcc90e79",                                  // output
        },
        {
                "TECBMMT3 Decrypt 1", KeyPurpose::DECRYPT, BlockMode::ECB, PaddingMode::NONE,
                "7f8fe3d3f4a48394fb682c2919926d6ddfce8932529229ce",  // key
                "",                                                  // IV
                "e9653a0a1f05d31b9acd12d73aa9879d",                  // input
                "9b2ae9d998efe62f1b592e7e1df8ff38",                  // output
        },
        {
                "TCBCMMT3 Encrypt 0", KeyPurpose::ENCRYPT, BlockMode::CBC, PaddingMode::NONE,
                "b5cb1504802326c73df186e3e352a20de643b0d63ee30e37",  // key
                "43f791134c5647ba",                                  // IV
                "dcc153cef81d6f24",                                  // input
                "92538bd8af18d3ba",                                  // output
        },
        {
                "TCBCMMT3 Encrypt 1", KeyPurpose::ENCRYPT, BlockMode::CBC, PaddingMode::NONE,
                "a49d7564199e97cb529d2c9d97bf2f98d35edf57ba1f7358",  // key
                "c2e999cb6249023c",                                  // IV
                "c689aee38a301bb316da75db36f110b5",                  // input
                "e9afaba5ec75ea1bbe65506655bb4ecb",                  // output
        },
        {
                "TCBCMMT3 Encrypt 1 PKCS7 variant", KeyPurpose::ENCRYPT, BlockMode::CBC,
                PaddingMode::PKCS7,
                "a49d7564199e97cb529d2c9d97bf2f98d35edf57ba1f7358",  // key
                "c2e999cb6249023c",                                  // IV
                "c689aee38a301bb316da75db36f110b500",                // input
                "e9afaba5ec75ea1bbe65506655bb4ecb825aa27ec0656156",  // output
        },
        {
                "TCBCMMT3 Encrypt 1 PKCS7 decrypted", KeyPurpose::DECRYPT, BlockMode::CBC,
                PaddingMode::PKCS7,
                "a49d7564199e97cb529d2c9d97bf2f98d35edf57ba1f7358",  // key
                "c2e999cb6249023c",                                  // IV
                "e9afaba5ec75ea1bbe65506655bb4ecb825aa27ec0656156",  // input
                "c689aee38a301bb316da75db36f110b500",                // output
        },
        {
                "TCBCMMT3 Decrypt 0", KeyPurpose::DECRYPT, BlockMode::CBC, PaddingMode::NONE,
                "5eb6040d46082c7aa7d06dfd08dfeac8c18364c1548c3ba1",  // key
                "41746c7e442d3681",                                  // IV
                "c53a7b0ec40600fe",                                  // input
                "d4f00eb455de1034",                                  // output
        },
        {
                "TCBCMMT3 Decrypt 1", KeyPurpose::DECRYPT, BlockMode::CBC, PaddingMode::NONE,
                "5b1cce7c0dc1ec49130dfb4af45785ab9179e567f2c7d549",  // key
                "3982bc02c3727d45",                                  // IV
                "6006f10adef52991fcc777a1238bbb65",                  // input
                "edae09288e9e3bc05746d872b48e3b29",                  // output
        },
};

/*
 * EncryptionOperationsTest.TripleDesTestVector
 *
 * Verifies that NIST (plus a few extra) test vectors produce the correct results.
 */
TEST_P(EncryptionOperationsTest, TripleDesTestVector) {
    constexpr size_t num_tests = sizeof(kTripleDesTestVectors) / sizeof(TripleDesTestVector);
    for (auto* test = kTripleDesTestVectors; test < kTripleDesTestVectors + num_tests; ++test) {
        SCOPED_TRACE(test->name);
        CheckTripleDesTestVector(test->purpose, test->block_mode, test->padding_mode,
                                 hex2str(test->key), hex2str(test->iv), hex2str(test->input),
                                 hex2str(test->output));
    }
}

/*
 * EncryptionOperationsTest.TripleDesCbcRoundTripSuccess
 *
 * Validates CBC mode functionality.
 */
TEST_P(EncryptionOperationsTest, TripleDesCbcRoundTripSuccess) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .TripleDesEncryptionKey(168)
                                                 .BlockMode(BlockMode::CBC)
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .Padding(PaddingMode::NONE)));

    ASSERT_GT(key_blob_.size(), 0U);

    // Two-block message.
    string message = "1234567890123456";
    vector<uint8_t> iv1;
    string ciphertext1 = EncryptMessage(message, BlockMode::CBC, PaddingMode::NONE, &iv1);
    EXPECT_EQ(message.size(), ciphertext1.size());

    vector<uint8_t> iv2;
    string ciphertext2 = EncryptMessage(message, BlockMode::CBC, PaddingMode::NONE, &iv2);
    EXPECT_EQ(message.size(), ciphertext2.size());

    // IVs should be random, so ciphertexts should differ.
    EXPECT_NE(iv1, iv2);
    EXPECT_NE(ciphertext1, ciphertext2);

    string plaintext = DecryptMessage(ciphertext1, BlockMode::CBC, PaddingMode::NONE, iv1);
    EXPECT_EQ(message, plaintext);
}

/*
 * EncryptionOperationsTest.TripleDesInvalidCallerIv
 *
 * Validates that keymint fails correctly when the user supplies an incorrect-size IV.
 */
TEST_P(EncryptionOperationsTest, TripleDesInvalidCallerIv) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .TripleDesEncryptionKey(168)
                                                 .BlockMode(BlockMode::CBC)
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .Authorization(TAG_CALLER_NONCE)
                                                 .Padding(PaddingMode::NONE)));
    auto params = AuthorizationSetBuilder()
                          .BlockMode(BlockMode::CBC)
                          .Padding(PaddingMode::NONE)
                          .Authorization(TAG_NONCE, AidlBuf("abcdefg"));
    EXPECT_EQ(ErrorCode::INVALID_NONCE, Begin(KeyPurpose::ENCRYPT, params));
}

/*
 * EncryptionOperationsTest.TripleDesCallerIv
 *
 * Validates that 3DES keys can allow caller-specified IVs, and use them correctly.
 */
TEST_P(EncryptionOperationsTest, TripleDesCallerIv) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .TripleDesEncryptionKey(168)
                                                 .BlockMode(BlockMode::CBC)
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .Authorization(TAG_CALLER_NONCE)
                                                 .Padding(PaddingMode::NONE)));
    string message = "1234567890123456";
    vector<uint8_t> iv;
    // Don't specify IV, should get a random one.
    string ciphertext1 = EncryptMessage(message, BlockMode::CBC, PaddingMode::NONE, &iv);
    EXPECT_EQ(message.size(), ciphertext1.size());
    EXPECT_EQ(8U, iv.size());

    string plaintext = DecryptMessage(ciphertext1, BlockMode::CBC, PaddingMode::NONE, iv);
    EXPECT_EQ(message, plaintext);

    // Now specify an IV, should also work.
    iv = AidlBuf("abcdefgh");
    string ciphertext2 = EncryptMessage(message, BlockMode::CBC, PaddingMode::NONE, iv);

    // Decrypt with correct IV.
    plaintext = DecryptMessage(ciphertext2, BlockMode::CBC, PaddingMode::NONE, iv);
    EXPECT_EQ(message, plaintext);

    // Now try with wrong IV.
    plaintext = DecryptMessage(ciphertext2, BlockMode::CBC, PaddingMode::NONE, AidlBuf("aaaaaaaa"));
    EXPECT_NE(message, plaintext);
}

/*
 * EncryptionOperationsTest, TripleDesCallerNonceProhibited.
 *
 * Verifies that 3DES keys without TAG_CALLER_NONCE do not allow caller-specified IVs.
 */
TEST_P(EncryptionOperationsTest, TripleDesCallerNonceProhibited) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .TripleDesEncryptionKey(168)
                                                 .BlockMode(BlockMode::CBC)
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .Padding(PaddingMode::NONE)));

    string message = "12345678901234567890123456789012";
    vector<uint8_t> iv;
    // Don't specify nonce, should get a random one.
    string ciphertext1 = EncryptMessage(message, BlockMode::CBC, PaddingMode::NONE, &iv);
    EXPECT_EQ(message.size(), ciphertext1.size());
    EXPECT_EQ(8U, iv.size());

    string plaintext = DecryptMessage(ciphertext1, BlockMode::CBC, PaddingMode::NONE, iv);
    EXPECT_EQ(message, plaintext);

    // Now specify a nonce, should fail.
    auto input_params = AuthorizationSetBuilder()
                                .Authorization(TAG_NONCE, AidlBuf("abcdefgh"))
                                .BlockMode(BlockMode::CBC)
                                .Padding(PaddingMode::NONE);
    AuthorizationSet output_params;
    EXPECT_EQ(ErrorCode::CALLER_NONCE_PROHIBITED,
              Begin(KeyPurpose::ENCRYPT, input_params, &output_params));
}

/*
 * EncryptionOperationsTest.TripleDesCbcNotAuthorized
 *
 * Verifies that 3DES ECB-only keys do not allow CBC usage.
 */
TEST_P(EncryptionOperationsTest, TripleDesCbcNotAuthorized) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .TripleDesEncryptionKey(168)
                                                 .BlockMode(BlockMode::ECB)
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .Padding(PaddingMode::NONE)));
    // Two-block message.
    string message = "1234567890123456";
    auto begin_params =
            AuthorizationSetBuilder().BlockMode(BlockMode::CBC).Padding(PaddingMode::NONE);
    EXPECT_EQ(ErrorCode::INCOMPATIBLE_BLOCK_MODE, Begin(KeyPurpose::ENCRYPT, begin_params));
}

/*
 * EncryptionOperationsTest.TripleDesEcbCbcNoPaddingWrongInputSize
 *
 * Verifies that unpadded CBC operations reject inputs that are not a multiple of block size.
 */
TEST_P(EncryptionOperationsTest, TripleDesEcbCbcNoPaddingWrongInputSize) {
    for (BlockMode blockMode : {BlockMode::ECB, BlockMode::CBC}) {
        ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                     .TripleDesEncryptionKey(168)
                                                     .BlockMode(blockMode)
                                                     .Authorization(TAG_NO_AUTH_REQUIRED)
                                                     .Padding(PaddingMode::NONE)));
        // Message is slightly shorter than two blocks.
        string message = "123456789012345";

        auto begin_params =
                AuthorizationSetBuilder().BlockMode(blockMode).Padding(PaddingMode::NONE);
        AuthorizationSet output_params;
        EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, begin_params, &output_params));
        string ciphertext;
        EXPECT_EQ(ErrorCode::INVALID_INPUT_LENGTH, Finish(message, "", &ciphertext));

        CheckedDeleteKey();
    }
}

/*
 * EncryptionOperationsTest, TripleDesCbcPkcs7Padding.
 *
 * Verifies that PKCS7 padding works correctly in CBC mode.
 */
TEST_P(EncryptionOperationsTest, TripleDesCbcPkcs7Padding) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .TripleDesEncryptionKey(168)
                                                 .BlockMode(BlockMode::CBC)
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .Padding(PaddingMode::PKCS7)));

    // Try various message lengths; all should work.
    for (size_t i = 0; i < 32; ++i) {
        string message(i, 'a');
        vector<uint8_t> iv;
        string ciphertext = EncryptMessage(message, BlockMode::CBC, PaddingMode::PKCS7, &iv);
        EXPECT_EQ(i + 8 - (i % 8), ciphertext.size());
        string plaintext = DecryptMessage(ciphertext, BlockMode::CBC, PaddingMode::PKCS7, iv);
        EXPECT_EQ(message, plaintext);
    }
}

/*
 * EncryptionOperationsTest.TripleDesCbcNoPaddingKeyWithPkcs7Padding
 *
 * Verifies that a key that requires PKCS7 padding cannot be used in unpadded mode.
 */
TEST_P(EncryptionOperationsTest, TripleDesCbcNoPaddingKeyWithPkcs7Padding) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .TripleDesEncryptionKey(168)
                                                 .BlockMode(BlockMode::CBC)
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .Padding(PaddingMode::NONE)));

    // Try various message lengths; all should fail.
    for (size_t i = 0; i < 32; ++i) {
        auto begin_params =
                AuthorizationSetBuilder().BlockMode(BlockMode::CBC).Padding(PaddingMode::PKCS7);
        EXPECT_EQ(ErrorCode::INCOMPATIBLE_PADDING_MODE, Begin(KeyPurpose::ENCRYPT, begin_params));
    }
}

/*
 * EncryptionOperationsTest.TripleDesCbcPkcs7PaddingCorrupted
 *
 * Verifies that corrupted PKCS7 padding is rejected during decryption.
 */
TEST_P(EncryptionOperationsTest, TripleDesCbcPkcs7PaddingCorrupted) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .TripleDesEncryptionKey(168)
                                                 .BlockMode(BlockMode::CBC)
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .Padding(PaddingMode::PKCS7)));

    string message = "a";
    vector<uint8_t> iv;
    string ciphertext = EncryptMessage(message, BlockMode::CBC, PaddingMode::PKCS7, &iv);
    EXPECT_EQ(8U, ciphertext.size());
    EXPECT_NE(ciphertext, message);
    ++ciphertext[ciphertext.size() / 2];

    auto begin_params = AuthorizationSetBuilder()
                                .BlockMode(BlockMode::CBC)
                                .Padding(PaddingMode::PKCS7)
                                .Authorization(TAG_NONCE, iv);
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, begin_params));
    string plaintext;
    EXPECT_EQ(ErrorCode::OK, Update(ciphertext, &plaintext));
    EXPECT_EQ(ErrorCode::INVALID_ARGUMENT, Finish(&plaintext));
}

/*
 * EncryptionOperationsTest, TripleDesCbcIncrementalNoPadding.
 *
 * Verifies that 3DES CBC works with many different input sizes.
 */
TEST_P(EncryptionOperationsTest, TripleDesCbcIncrementalNoPadding) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .TripleDesEncryptionKey(168)
                                                 .BlockMode(BlockMode::CBC)
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .Padding(PaddingMode::NONE)));

    int increment = 7;
    string message(240, 'a');
    AuthorizationSet input_params =
            AuthorizationSetBuilder().BlockMode(BlockMode::CBC).Padding(PaddingMode::NONE);
    AuthorizationSet output_params;
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, input_params, &output_params));

    string ciphertext;
    for (size_t i = 0; i < message.size(); i += increment)
        EXPECT_EQ(ErrorCode::OK, Update(message.substr(i, increment), &ciphertext));
    EXPECT_EQ(ErrorCode::OK, Finish(&ciphertext));
    EXPECT_EQ(message.size(), ciphertext.size());

    // Move TAG_NONCE into input_params
    input_params = output_params;
    input_params.push_back(TAG_BLOCK_MODE, BlockMode::CBC);
    input_params.push_back(TAG_PADDING, PaddingMode::NONE);
    output_params.Clear();

    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, input_params, &output_params));
    string plaintext;
    for (size_t i = 0; i < ciphertext.size(); i += increment)
        EXPECT_EQ(ErrorCode::OK, Update(ciphertext.substr(i, increment), &plaintext));
    EXPECT_EQ(ErrorCode::OK, Finish(&plaintext));
    EXPECT_EQ(ciphertext.size(), plaintext.size());
    EXPECT_EQ(message, plaintext);
}

INSTANTIATE_KEYMINT_AIDL_TEST(EncryptionOperationsTest);

typedef KeyMintAidlTestBase MaxOperationsTest;

/*
 * MaxOperationsTest.TestLimitAes
 *
 * Verifies that the max uses per boot tag works correctly with AES keys.
 */
TEST_P(MaxOperationsTest, TestLimitAes) {
    if (SecLevel() == SecurityLevel::STRONGBOX) return;

    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .EcbMode()
                                                 .Padding(PaddingMode::NONE)
                                                 .Authorization(TAG_MAX_USES_PER_BOOT, 3)));

    string message = "1234567890123456";

    auto params = AuthorizationSetBuilder().EcbMode().Padding(PaddingMode::NONE);

    EncryptMessage(message, params);
    EncryptMessage(message, params);
    EncryptMessage(message, params);

    // Fourth time should fail.
    EXPECT_EQ(ErrorCode::KEY_MAX_OPS_EXCEEDED, Begin(KeyPurpose::ENCRYPT, params));
}

/*
 * MaxOperationsTest.TestLimitRsa
 *
 * Verifies that the max uses per boot tag works correctly with RSA keys.
 */
TEST_P(MaxOperationsTest, TestLimitRsa) {
    if (SecLevel() == SecurityLevel::STRONGBOX) return;

    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .RsaSigningKey(1024, 65537)
                                                 .NoDigestOrPadding()
                                                 .Authorization(TAG_MAX_USES_PER_BOOT, 3)
                                                 .SetDefaultValidity()));

    string message = "1234567890123456";

    auto params = AuthorizationSetBuilder().NoDigestOrPadding();

    SignMessage(message, params);
    SignMessage(message, params);
    SignMessage(message, params);

    // Fourth time should fail.
    EXPECT_EQ(ErrorCode::KEY_MAX_OPS_EXCEEDED, Begin(KeyPurpose::SIGN, params));
}

INSTANTIATE_KEYMINT_AIDL_TEST(MaxOperationsTest);

typedef KeyMintAidlTestBase UsageCountLimitTest;

/*
 * UsageCountLimitTest.TestSingleUseAes
 *
 * Verifies that the usage count limit tag = 1 works correctly with AES keys.
 */
TEST_P(UsageCountLimitTest, TestSingleUseAes) {
    if (SecLevel() == SecurityLevel::STRONGBOX) return;

    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .EcbMode()
                                                 .Padding(PaddingMode::NONE)
                                                 .Authorization(TAG_USAGE_COUNT_LIMIT, 1)));

    // Check the usage count limit tag appears in the authorizations.
    AuthorizationSet auths;
    for (auto& entry : key_characteristics_) {
        auths.push_back(AuthorizationSet(entry.authorizations));
    }
    EXPECT_TRUE(auths.Contains(TAG_USAGE_COUNT_LIMIT, 1U))
            << "key usage count limit " << 1U << " missing";

    string message = "1234567890123456";
    auto params = AuthorizationSetBuilder().EcbMode().Padding(PaddingMode::NONE);

    AuthorizationSet hardware_auths = HwEnforcedAuthorizations(key_characteristics_);
    AuthorizationSet keystore_auths =
            SecLevelAuthorizations(key_characteristics_, SecurityLevel::KEYSTORE);

    // First usage of AES key should work.
    EncryptMessage(message, params);

    if (hardware_auths.Contains(TAG_USAGE_COUNT_LIMIT, 1U)) {
        // Usage count limit tag is enforced by hardware. After using the key, the key blob
        // must be invalidated from secure storage (such as RPMB partition).
        EXPECT_EQ(ErrorCode::INVALID_KEY_BLOB, Begin(KeyPurpose::ENCRYPT, params));
    } else {
        // Usage count limit tag is enforced by keystore, keymint does nothing.
        EXPECT_TRUE(keystore_auths.Contains(TAG_USAGE_COUNT_LIMIT, 1U));
        EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, params));
    }
}

/*
 * UsageCountLimitTest.TestLimitedUseAes
 *
 * Verifies that the usage count limit tag > 1 works correctly with AES keys.
 */
TEST_P(UsageCountLimitTest, TestLimitedUseAes) {
    if (SecLevel() == SecurityLevel::STRONGBOX) return;

    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .EcbMode()
                                                 .Padding(PaddingMode::NONE)
                                                 .Authorization(TAG_USAGE_COUNT_LIMIT, 3)));

    // Check the usage count limit tag appears in the authorizations.
    AuthorizationSet auths;
    for (auto& entry : key_characteristics_) {
        auths.push_back(AuthorizationSet(entry.authorizations));
    }
    EXPECT_TRUE(auths.Contains(TAG_USAGE_COUNT_LIMIT, 3U))
            << "key usage count limit " << 3U << " missing";

    string message = "1234567890123456";
    auto params = AuthorizationSetBuilder().EcbMode().Padding(PaddingMode::NONE);

    AuthorizationSet hardware_auths = HwEnforcedAuthorizations(key_characteristics_);
    AuthorizationSet keystore_auths =
            SecLevelAuthorizations(key_characteristics_, SecurityLevel::KEYSTORE);

    EncryptMessage(message, params);
    EncryptMessage(message, params);
    EncryptMessage(message, params);

    if (hardware_auths.Contains(TAG_USAGE_COUNT_LIMIT, 3U)) {
        // Usage count limit tag is enforced by hardware. After using the key, the key blob
        // must be invalidated from secure storage (such as RPMB partition).
        EXPECT_EQ(ErrorCode::INVALID_KEY_BLOB, Begin(KeyPurpose::ENCRYPT, params));
    } else {
        // Usage count limit tag is enforced by keystore, keymint does nothing.
        EXPECT_TRUE(keystore_auths.Contains(TAG_USAGE_COUNT_LIMIT, 3U));
        EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, params));
    }
}

/*
 * UsageCountLimitTest.TestSingleUseRsa
 *
 * Verifies that the usage count limit tag = 1 works correctly with RSA keys.
 */
TEST_P(UsageCountLimitTest, TestSingleUseRsa) {
    if (SecLevel() == SecurityLevel::STRONGBOX) return;

    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .RsaSigningKey(1024, 65537)
                                                 .NoDigestOrPadding()
                                                 .Authorization(TAG_USAGE_COUNT_LIMIT, 1)
                                                 .SetDefaultValidity()));

    // Check the usage count limit tag appears in the authorizations.
    AuthorizationSet auths;
    for (auto& entry : key_characteristics_) {
        auths.push_back(AuthorizationSet(entry.authorizations));
    }
    EXPECT_TRUE(auths.Contains(TAG_USAGE_COUNT_LIMIT, 1U))
            << "key usage count limit " << 1U << " missing";

    string message = "1234567890123456";
    auto params = AuthorizationSetBuilder().NoDigestOrPadding();

    AuthorizationSet hardware_auths = HwEnforcedAuthorizations(key_characteristics_);
    AuthorizationSet keystore_auths =
            SecLevelAuthorizations(key_characteristics_, SecurityLevel::KEYSTORE);

    // First usage of RSA key should work.
    SignMessage(message, params);

    if (hardware_auths.Contains(TAG_USAGE_COUNT_LIMIT, 1U)) {
        // Usage count limit tag is enforced by hardware. After using the key, the key blob
        // must be invalidated from secure storage (such as RPMB partition).
        EXPECT_EQ(ErrorCode::INVALID_KEY_BLOB, Begin(KeyPurpose::SIGN, params));
    } else {
        // Usage count limit tag is enforced by keystore, keymint does nothing.
        EXPECT_TRUE(keystore_auths.Contains(TAG_USAGE_COUNT_LIMIT, 1U));
        EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::SIGN, params));
    }
}

/*
 * UsageCountLimitTest.TestLimitUseRsa
 *
 * Verifies that the usage count limit tag > 1 works correctly with RSA keys.
 */
TEST_P(UsageCountLimitTest, TestLimitUseRsa) {
    if (SecLevel() == SecurityLevel::STRONGBOX) return;

    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .RsaSigningKey(1024, 65537)
                                                 .NoDigestOrPadding()
                                                 .Authorization(TAG_USAGE_COUNT_LIMIT, 3)
                                                 .SetDefaultValidity()));

    // Check the usage count limit tag appears in the authorizations.
    AuthorizationSet auths;
    for (auto& entry : key_characteristics_) {
        auths.push_back(AuthorizationSet(entry.authorizations));
    }
    EXPECT_TRUE(auths.Contains(TAG_USAGE_COUNT_LIMIT, 3U))
            << "key usage count limit " << 3U << " missing";

    string message = "1234567890123456";
    auto params = AuthorizationSetBuilder().NoDigestOrPadding();

    AuthorizationSet hardware_auths = HwEnforcedAuthorizations(key_characteristics_);
    AuthorizationSet keystore_auths =
            SecLevelAuthorizations(key_characteristics_, SecurityLevel::KEYSTORE);

    SignMessage(message, params);
    SignMessage(message, params);
    SignMessage(message, params);

    if (hardware_auths.Contains(TAG_USAGE_COUNT_LIMIT, 3U)) {
        // Usage count limit tag is enforced by hardware. After using the key, the key blob
        // must be invalidated from secure storage (such as RPMB partition).
        EXPECT_EQ(ErrorCode::INVALID_KEY_BLOB, Begin(KeyPurpose::SIGN, params));
    } else {
        // Usage count limit tag is enforced by keystore, keymint does nothing.
        EXPECT_TRUE(keystore_auths.Contains(TAG_USAGE_COUNT_LIMIT, 3U));
        EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::SIGN, params));
    }
}

/*
 * UsageCountLimitTest.TestSingleUseKeyAndRollbackResistance
 *
 * Verifies that when rollback resistance is supported by the KeyMint implementation with
 * the secure hardware, the single use key with usage count limit tag = 1 must also be enforced
 * in hardware.
 */
TEST_P(UsageCountLimitTest, TestSingleUseKeyAndRollbackResistance) {
    if (SecLevel() == SecurityLevel::STRONGBOX) return;

    auto error = GenerateKey(AuthorizationSetBuilder()
                                     .RsaSigningKey(2048, 65537)
                                     .Digest(Digest::NONE)
                                     .Padding(PaddingMode::NONE)
                                     .Authorization(TAG_NO_AUTH_REQUIRED)
                                     .Authorization(TAG_ROLLBACK_RESISTANCE)
                                     .SetDefaultValidity());
    ASSERT_TRUE(error == ErrorCode::ROLLBACK_RESISTANCE_UNAVAILABLE || error == ErrorCode::OK);

    if (error == ErrorCode::OK) {
        // Rollback resistance is supported by KeyMint, verify it is enforced in hardware.
        AuthorizationSet hardwareEnforced(SecLevelAuthorizations());
        ASSERT_TRUE(hardwareEnforced.Contains(TAG_ROLLBACK_RESISTANCE));
        ASSERT_EQ(ErrorCode::OK, DeleteKey());

        // The KeyMint should also enforce single use key in hardware when it supports rollback
        // resistance.
        ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                     .Authorization(TAG_NO_AUTH_REQUIRED)
                                                     .RsaSigningKey(1024, 65537)
                                                     .NoDigestOrPadding()
                                                     .Authorization(TAG_USAGE_COUNT_LIMIT, 1)
                                                     .SetDefaultValidity()));

        // Check the usage count limit tag appears in the hardware authorizations.
        AuthorizationSet hardware_auths = HwEnforcedAuthorizations(key_characteristics_);
        EXPECT_TRUE(hardware_auths.Contains(TAG_USAGE_COUNT_LIMIT, 1U))
                << "key usage count limit " << 1U << " missing";

        string message = "1234567890123456";
        auto params = AuthorizationSetBuilder().NoDigestOrPadding();

        // First usage of RSA key should work.
        SignMessage(message, params);

        // Usage count limit tag is enforced by hardware. After using the key, the key blob
        // must be invalidated from secure storage (such as RPMB partition).
        EXPECT_EQ(ErrorCode::INVALID_KEY_BLOB, Begin(KeyPurpose::SIGN, params));
    }
}

INSTANTIATE_KEYMINT_AIDL_TEST(UsageCountLimitTest);

typedef KeyMintAidlTestBase GetHardwareInfoTest;

TEST_P(GetHardwareInfoTest, GetHardwareInfo) {
    // Retrieving hardware info should give the same result each time.
    KeyMintHardwareInfo info;
    ASSERT_TRUE(keyMint().getHardwareInfo(&info).isOk());
    KeyMintHardwareInfo info2;
    ASSERT_TRUE(keyMint().getHardwareInfo(&info2).isOk());
    EXPECT_EQ(info, info2);
}

INSTANTIATE_KEYMINT_AIDL_TEST(GetHardwareInfoTest);

typedef KeyMintAidlTestBase AddEntropyTest;

/*
 * AddEntropyTest.AddEntropy
 *
 * Verifies that the addRngEntropy method doesn't blow up.  There's no way to test that entropy
 * is actually added.
 */
TEST_P(AddEntropyTest, AddEntropy) {
    string data = "foo";
    EXPECT_TRUE(keyMint().addRngEntropy(vector<uint8_t>(data.begin(), data.end())).isOk());
}

/*
 * AddEntropyTest.AddEmptyEntropy
 *
 * Verifies that the addRngEntropy method doesn't blow up when given an empty buffer.
 */
TEST_P(AddEntropyTest, AddEmptyEntropy) {
    EXPECT_TRUE(keyMint().addRngEntropy(AidlBuf()).isOk());
}

/*
 * AddEntropyTest.AddLargeEntropy
 *
 * Verifies that the addRngEntropy method doesn't blow up when given a largish amount of data.
 */
TEST_P(AddEntropyTest, AddLargeEntropy) {
    EXPECT_TRUE(keyMint().addRngEntropy(AidlBuf(string(2 * 1024, 'a'))).isOk());
}

/*
 * AddEntropyTest.AddTooLargeEntropy
 *
 * Verifies that the addRngEntropy method rejects more than 2KiB  of data.
 */
TEST_P(AddEntropyTest, AddTooLargeEntropy) {
    ErrorCode rc = GetReturnErrorCode(keyMint().addRngEntropy(AidlBuf(string(2 * 1024 + 1, 'a'))));
    EXPECT_EQ(ErrorCode::INVALID_INPUT_LENGTH, rc);
}

INSTANTIATE_KEYMINT_AIDL_TEST(AddEntropyTest);

typedef KeyMintAidlTestBase KeyDeletionTest;

/**
 * KeyDeletionTest.DeleteKey
 *
 * This test checks that if rollback protection is implemented, DeleteKey invalidates a formerly
 * valid key blob.
 */
TEST_P(KeyDeletionTest, DeleteKey) {
    auto error = GenerateKey(AuthorizationSetBuilder()
                                     .RsaSigningKey(2048, 65537)
                                     .Digest(Digest::NONE)
                                     .Padding(PaddingMode::NONE)
                                     .Authorization(TAG_NO_AUTH_REQUIRED)
                                     .Authorization(TAG_ROLLBACK_RESISTANCE)
                                     .SetDefaultValidity());
    ASSERT_TRUE(error == ErrorCode::ROLLBACK_RESISTANCE_UNAVAILABLE || error == ErrorCode::OK);

    // Delete must work if rollback protection is implemented
    if (error == ErrorCode::OK) {
        AuthorizationSet hardwareEnforced(SecLevelAuthorizations());
        ASSERT_TRUE(hardwareEnforced.Contains(TAG_ROLLBACK_RESISTANCE));

        ASSERT_EQ(ErrorCode::OK, DeleteKey(true /* keep key blob */));

        string message = "12345678901234567890123456789012";
        AuthorizationSet begin_out_params;
        EXPECT_EQ(ErrorCode::INVALID_KEY_BLOB,
                  Begin(KeyPurpose::SIGN, key_blob_,
                        AuthorizationSetBuilder().Digest(Digest::NONE).Padding(PaddingMode::NONE),
                        &begin_out_params));
        AbortIfNeeded();
        key_blob_ = AidlBuf();
    }
}

/**
 * KeyDeletionTest.DeleteInvalidKey
 *
 * This test checks that the HAL excepts invalid key blobs..
 */
TEST_P(KeyDeletionTest, DeleteInvalidKey) {
    // Generate key just to check if rollback protection is implemented
    auto error = GenerateKey(AuthorizationSetBuilder()
                                     .RsaSigningKey(2048, 65537)
                                     .Digest(Digest::NONE)
                                     .Padding(PaddingMode::NONE)
                                     .Authorization(TAG_NO_AUTH_REQUIRED)
                                     .Authorization(TAG_ROLLBACK_RESISTANCE)
                                     .SetDefaultValidity());
    ASSERT_TRUE(error == ErrorCode::ROLLBACK_RESISTANCE_UNAVAILABLE || error == ErrorCode::OK);

    // Delete must work if rollback protection is implemented
    if (error == ErrorCode::OK) {
        AuthorizationSet enforced(SecLevelAuthorizations());
        ASSERT_TRUE(enforced.Contains(TAG_ROLLBACK_RESISTANCE));

        // Delete the key we don't care about the result at this point.
        DeleteKey();

        // Now create an invalid key blob and delete it.
        key_blob_ = AidlBuf("just some garbage data which is not a valid key blob");

        ASSERT_EQ(ErrorCode::OK, DeleteKey());
    }
}

/**
 * KeyDeletionTest.DeleteAllKeys
 *
 * This test is disarmed by default. To arm it use --arm_deleteAllKeys.
 *
 * BEWARE: This test has serious side effects. All user keys will be lost! This includes
 * FBE/FDE encryption keys, which means that the device will not even boot until after the
 * device has been wiped manually (e.g., fastboot flashall -w), and new FBE/FDE keys have
 * been provisioned. Use this test only on dedicated testing devices that have no valuable
 * credentials stored in Keystore/Keymint.
 */
TEST_P(KeyDeletionTest, DeleteAllKeys) {
    if (!arm_deleteAllKeys) return;
    auto error = GenerateKey(AuthorizationSetBuilder()
                                     .RsaSigningKey(2048, 65537)
                                     .Digest(Digest::NONE)
                                     .Padding(PaddingMode::NONE)
                                     .Authorization(TAG_NO_AUTH_REQUIRED)
                                     .Authorization(TAG_ROLLBACK_RESISTANCE));
    ASSERT_TRUE(error == ErrorCode::ROLLBACK_RESISTANCE_UNAVAILABLE || error == ErrorCode::OK);

    // Delete must work if rollback protection is implemented
    if (error == ErrorCode::OK) {
        AuthorizationSet hardwareEnforced(SecLevelAuthorizations());
        ASSERT_TRUE(hardwareEnforced.Contains(TAG_ROLLBACK_RESISTANCE));

        ASSERT_EQ(ErrorCode::OK, DeleteAllKeys());

        string message = "12345678901234567890123456789012";
        AuthorizationSet begin_out_params;

        EXPECT_EQ(ErrorCode::INVALID_KEY_BLOB,
                  Begin(KeyPurpose::SIGN, key_blob_,
                        AuthorizationSetBuilder().Digest(Digest::NONE).Padding(PaddingMode::NONE),
                        &begin_out_params));
        AbortIfNeeded();
        key_blob_ = AidlBuf();
    }
}

INSTANTIATE_KEYMINT_AIDL_TEST(KeyDeletionTest);

typedef KeyMintAidlTestBase KeyUpgradeTest;

/**
 * KeyUpgradeTest.UpgradeInvalidKey
 *
 * This test checks that the HAL excepts invalid key blobs..
 */
TEST_P(KeyUpgradeTest, UpgradeInvalidKey) {
    AidlBuf key_blob = AidlBuf("just some garbage data which is not a valid key blob");

    std::vector<uint8_t> new_blob;
    Status result = keymint_->upgradeKey(key_blob,
                                         AuthorizationSetBuilder()
                                                 .Authorization(TAG_APPLICATION_ID, "clientid")
                                                 .Authorization(TAG_APPLICATION_DATA, "appdata")
                                                 .vector_data(),
                                         &new_blob);
    ASSERT_EQ(ErrorCode::INVALID_KEY_BLOB, GetReturnErrorCode(result));
}

INSTANTIATE_KEYMINT_AIDL_TEST(KeyUpgradeTest);

using UpgradeKeyTest = KeyMintAidlTestBase;

/*
 * UpgradeKeyTest.UpgradeKey
 *
 * Verifies that calling upgrade key on an up-to-date key works (i.e. does nothing).
 */
TEST_P(UpgradeKeyTest, UpgradeKey) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .AesEncryptionKey(128)
                                                 .Padding(PaddingMode::NONE)
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)));

    auto result = UpgradeKey(key_blob_);

    // Key doesn't need upgrading.  Should get okay, but no new key blob.
    EXPECT_EQ(result, std::make_pair(ErrorCode::OK, vector<uint8_t>()));
}

INSTANTIATE_KEYMINT_AIDL_TEST(UpgradeKeyTest);

using ClearOperationsTest = KeyMintAidlTestBase;

/*
 * ClearSlotsTest.TooManyOperations
 *
 * Verifies that TOO_MANY_OPERATIONS is returned after the max number of
 * operations are started without being finished or aborted. Also verifies
 * that aborting the operations clears the operations.
 *
 */
TEST_P(ClearOperationsTest, TooManyOperations) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .RsaEncryptionKey(2048, 65537)
                                                 .Padding(PaddingMode::NONE)
                                                 .SetDefaultValidity()));

    auto params = AuthorizationSetBuilder().Padding(PaddingMode::NONE);
    constexpr size_t max_operations = 100;  // set to arbituary large number
    std::shared_ptr<IKeyMintOperation> op_handles[max_operations];
    AuthorizationSet out_params;
    ErrorCode result;
    size_t i;

    for (i = 0; i < max_operations; i++) {
        result = Begin(KeyPurpose::ENCRYPT, key_blob_, params, &out_params, op_handles[i]);
        if (ErrorCode::OK != result) {
            break;
        }
    }
    EXPECT_EQ(ErrorCode::TOO_MANY_OPERATIONS, result);
    // Try again just in case there's a weird overflow bug
    EXPECT_EQ(ErrorCode::TOO_MANY_OPERATIONS,
              Begin(KeyPurpose::ENCRYPT, key_blob_, params, &out_params));
    for (size_t j = 0; j < i; j++) {
        EXPECT_EQ(ErrorCode::OK, Abort(op_handles[j]))
                << "Aboort failed for i = " << j << std::endl;
    }
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, key_blob_, params, &out_params));
    AbortIfNeeded();
}

INSTANTIATE_KEYMINT_AIDL_TEST(ClearOperationsTest);

typedef KeyMintAidlTestBase TransportLimitTest;

/*
 * TransportLimitTest.LargeFinishInput
 *
 * Verifies that passing input data to finish succeeds as expected.
 */
TEST_P(TransportLimitTest, LargeFinishInput) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .BlockMode(BlockMode::ECB)
                                                 .Padding(PaddingMode::NONE)));

    for (int msg_size = 8 /* 256 bytes */; msg_size <= 11 /* 2 KiB */; msg_size++) {
        auto cipher_params =
                AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::NONE);

        AuthorizationSet out_params;
        EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, cipher_params, &out_params));

        string plain_message = std::string(1 << msg_size, 'x');
        string encrypted_message;
        auto rc = Finish(plain_message, &encrypted_message);

        EXPECT_EQ(ErrorCode::OK, rc);
        EXPECT_EQ(plain_message.size(), encrypted_message.size())
                << "Encrypt finish returned OK, but did not consume all of the given input";
        cipher_params.push_back(out_params);

        EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, cipher_params));

        string decrypted_message;
        rc = Finish(encrypted_message, &decrypted_message);
        EXPECT_EQ(ErrorCode::OK, rc);
        EXPECT_EQ(plain_message.size(), decrypted_message.size())
                << "Decrypt finish returned OK, did not consume all of the given input";
    }
}

INSTANTIATE_KEYMINT_AIDL_TEST(TransportLimitTest);

typedef KeyMintAidlTestBase KeyAgreementTest;

int CurveToOpenSslCurveName(EcCurve curve) {
    switch (curve) {
        case EcCurve::P_224:
            return NID_secp224r1;
        case EcCurve::P_256:
            return NID_X9_62_prime256v1;
        case EcCurve::P_384:
            return NID_secp384r1;
        case EcCurve::P_521:
            return NID_secp521r1;
    }
}

/*
 * KeyAgreementTest.Ecdh
 *
 * Verifies that ECDH works for all curves
 */
TEST_P(KeyAgreementTest, Ecdh) {
    // Because it's possible to use this API with keys on different curves, we
    // check all N^2 combinations where N is the number of supported
    // curves.
    //
    // This is not a big deal as N is 4 so we only do 16 runs. If we end up with a
    // lot more curves we can be smart about things and just pick |otherCurve| so
    // it's not |curve| and that way we end up with only 2*N runs
    //
    for (auto curve : ValidCurves()) {
        for (auto localCurve : ValidCurves()) {
            // Generate EC key locally (with access to private key material)
            auto ecKey = EC_KEY_Ptr(EC_KEY_new());
            int curveName = CurveToOpenSslCurveName(localCurve);
            auto group = EC_GROUP_Ptr(EC_GROUP_new_by_curve_name(curveName));
            ASSERT_NE(group, nullptr);
            ASSERT_EQ(EC_KEY_set_group(ecKey.get(), group.get()), 1);
            ASSERT_EQ(EC_KEY_generate_key(ecKey.get()), 1);
            auto pkey = EVP_PKEY_Ptr(EVP_PKEY_new());
            ASSERT_EQ(EVP_PKEY_set1_EC_KEY(pkey.get(), ecKey.get()), 1);

            // Get encoded form of the public part of the locally generated key...
            unsigned char* p = nullptr;
            int encodedPublicKeySize = i2d_PUBKEY(pkey.get(), &p);
            ASSERT_GT(encodedPublicKeySize, 0);
            vector<uint8_t> encodedPublicKey(
                    reinterpret_cast<const uint8_t*>(p),
                    reinterpret_cast<const uint8_t*>(p + encodedPublicKeySize));
            OPENSSL_free(p);

            // Generate EC key in KeyMint (only access to public key material)
            vector<uint8_t> challenge = {0x41, 0x42};
            EXPECT_EQ(
                    ErrorCode::OK,
                    GenerateKey(AuthorizationSetBuilder()
                                        .Authorization(TAG_NO_AUTH_REQUIRED)
                                        .Authorization(TAG_EC_CURVE, curve)
                                        .Authorization(TAG_PURPOSE, KeyPurpose::AGREE_KEY)
                                        .Authorization(TAG_ALGORITHM, Algorithm::EC)
                                        .Authorization(TAG_ATTESTATION_APPLICATION_ID, {0x61, 0x62})
                                        .Authorization(TAG_ATTESTATION_CHALLENGE, challenge)
                                        .SetDefaultValidity()))
                    << "Failed to generate key";
            ASSERT_GT(cert_chain_.size(), 0);
            X509_Ptr kmKeyCert(parse_cert_blob(cert_chain_[0].encodedCertificate));
            ASSERT_NE(kmKeyCert, nullptr);
            // Check that keyAgreement (bit 4) is set in KeyUsage
            EXPECT_TRUE((X509_get_key_usage(kmKeyCert.get()) & X509v3_KU_KEY_AGREEMENT) != 0);
            auto kmPkey = EVP_PKEY_Ptr(X509_get_pubkey(kmKeyCert.get()));
            ASSERT_NE(kmPkey, nullptr);
            if (dump_Attestations) {
                for (size_t n = 0; n < cert_chain_.size(); n++) {
                    std::cout << bin2hex(cert_chain_[n].encodedCertificate) << std::endl;
                }
            }

            // Now that we have the two keys, we ask KeyMint to perform ECDH...
            if (curve != localCurve) {
                // If the keys are using different curves KeyMint should fail with
                // ErrorCode:INVALID_ARGUMENT. Check that.
                EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::AGREE_KEY, AuthorizationSetBuilder()));
                string ZabFromKeyMintStr;
                EXPECT_EQ(ErrorCode::INVALID_ARGUMENT,
                          Finish(string(encodedPublicKey.begin(), encodedPublicKey.end()),
                                 &ZabFromKeyMintStr));

            } else {
                // Otherwise if the keys are using the same curve, it should work.
                EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::AGREE_KEY, AuthorizationSetBuilder()));
                string ZabFromKeyMintStr;
                EXPECT_EQ(ErrorCode::OK,
                          Finish(string(encodedPublicKey.begin(), encodedPublicKey.end()),
                                 &ZabFromKeyMintStr));
                vector<uint8_t> ZabFromKeyMint(ZabFromKeyMintStr.begin(), ZabFromKeyMintStr.end());

                // Perform local ECDH between the two keys so we can check if we get the same Zab..
                auto ctx = EVP_PKEY_CTX_Ptr(EVP_PKEY_CTX_new(pkey.get(), nullptr));
                ASSERT_NE(ctx, nullptr);
                ASSERT_EQ(EVP_PKEY_derive_init(ctx.get()), 1);
                ASSERT_EQ(EVP_PKEY_derive_set_peer(ctx.get(), kmPkey.get()), 1);
                size_t ZabFromTestLen = 0;
                ASSERT_EQ(EVP_PKEY_derive(ctx.get(), nullptr, &ZabFromTestLen), 1);
                vector<uint8_t> ZabFromTest;
                ZabFromTest.resize(ZabFromTestLen);
                ASSERT_EQ(EVP_PKEY_derive(ctx.get(), ZabFromTest.data(), &ZabFromTestLen), 1);

                EXPECT_EQ(ZabFromKeyMint, ZabFromTest);
            }

            CheckedDeleteKey();
        }
    }
}

INSTANTIATE_KEYMINT_AIDL_TEST(KeyAgreementTest);

using DestroyAttestationIdsTest = KeyMintAidlTestBase;

// This is a problematic test, as it can render the device under test permanently unusable.
// Re-enable and run at your own risk.
TEST_P(DestroyAttestationIdsTest, DISABLED_DestroyTest) {
    auto result = DestroyAttestationIds();
    EXPECT_TRUE(result == ErrorCode::OK || result == ErrorCode::UNIMPLEMENTED);
}

INSTANTIATE_KEYMINT_AIDL_TEST(DestroyAttestationIdsTest);

using EarlyBootKeyTest = KeyMintAidlTestBase;

/*
 * EarlyBootKeyTest.CreateEarlyBootKeys
 *
 * Verifies that creating early boot keys succeeds, even at a later stage (after boot).
 */
TEST_P(EarlyBootKeyTest, CreateEarlyBootKeys) {
    // Early boot keys can be created after early boot.
    auto [aesKeyData, hmacKeyData, rsaKeyData, ecdsaKeyData] =
            CreateTestKeys(TAG_EARLY_BOOT_ONLY, ErrorCode::OK);

    for (const auto& keyData : {aesKeyData, hmacKeyData, rsaKeyData, ecdsaKeyData}) {
        ASSERT_GT(keyData.blob.size(), 0U);
        AuthorizationSet crypto_params = SecLevelAuthorizations(keyData.characteristics);
        EXPECT_TRUE(crypto_params.Contains(TAG_EARLY_BOOT_ONLY)) << crypto_params;
    }
    CheckedDeleteKey(&aesKeyData.blob);
    CheckedDeleteKey(&hmacKeyData.blob);
    CheckedDeleteKey(&rsaKeyData.blob);
    CheckedDeleteKey(&ecdsaKeyData.blob);
}

/*
 * EarlyBootKeyTest.CreateAttestedEarlyBootKey
 *
 * Verifies that creating an early boot key with attestation succeeds.
 */
TEST_P(EarlyBootKeyTest, CreateAttestedEarlyBootKey) {
    auto [aesKeyData, hmacKeyData, rsaKeyData, ecdsaKeyData] = CreateTestKeys(
            TAG_EARLY_BOOT_ONLY, ErrorCode::OK, [](AuthorizationSetBuilder* builder) {
                builder->AttestationChallenge("challenge");
                builder->AttestationApplicationId("app_id");
            });

    for (const auto& keyData : {aesKeyData, hmacKeyData, rsaKeyData, ecdsaKeyData}) {
        ASSERT_GT(keyData.blob.size(), 0U);
        AuthorizationSet crypto_params = SecLevelAuthorizations(keyData.characteristics);
        EXPECT_TRUE(crypto_params.Contains(TAG_EARLY_BOOT_ONLY)) << crypto_params;
    }
    CheckedDeleteKey(&aesKeyData.blob);
    CheckedDeleteKey(&hmacKeyData.blob);
    CheckedDeleteKey(&rsaKeyData.blob);
    CheckedDeleteKey(&ecdsaKeyData.blob);
}

/*
 * EarlyBootKeyTest.UseEarlyBootKeyFailure
 *
 * Verifies that using early boot keys at a later stage fails.
 */
TEST_P(EarlyBootKeyTest, UseEarlyBootKeyFailure) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .Authorization(TAG_EARLY_BOOT_ONLY)
                                                 .HmacKey(128)
                                                 .Digest(Digest::SHA_2_256)
                                                 .Authorization(TAG_MIN_MAC_LENGTH, 256)));
    AuthorizationSet output_params;
    EXPECT_EQ(ErrorCode::EARLY_BOOT_ENDED, Begin(KeyPurpose::SIGN, key_blob_,
                                                 AuthorizationSetBuilder()
                                                         .Digest(Digest::SHA_2_256)
                                                         .Authorization(TAG_MAC_LENGTH, 256),
                                                 &output_params));
}

/*
 * EarlyBootKeyTest.ImportEarlyBootKeyFailure
 *
 * Verifies that importing early boot keys fails.
 */
TEST_P(EarlyBootKeyTest, ImportEarlyBootKeyFailure) {
    ASSERT_EQ(ErrorCode::EARLY_BOOT_ENDED, ImportKey(AuthorizationSetBuilder()
                                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                                             .Authorization(TAG_EARLY_BOOT_ONLY)
                                                             .EcdsaSigningKey(EcCurve::P_256)
                                                             .Digest(Digest::SHA_2_256)
                                                             .SetDefaultValidity(),
                                                     KeyFormat::PKCS8, ec_256_key));
}

// This is a more comprehensive test, but it can only be run on a machine which is still in early
// boot stage, which no proper Android device is by the time we can run VTS.  To use this,
// un-disable it and modify vold to remove the call to earlyBootEnded().  Running the test will end
// early boot, so you'll have to reboot between runs.
TEST_P(EarlyBootKeyTest, DISABLED_FullTest) {
    auto [aesKeyData, hmacKeyData, rsaKeyData, ecdsaKeyData] =
            CreateTestKeys(TAG_EARLY_BOOT_ONLY, ErrorCode::OK);
    // TAG_EARLY_BOOT_ONLY should be in hw-enforced.
    EXPECT_TRUE(HwEnforcedAuthorizations(aesKeyData.characteristics).Contains(TAG_EARLY_BOOT_ONLY));
    EXPECT_TRUE(
            HwEnforcedAuthorizations(hmacKeyData.characteristics).Contains(TAG_EARLY_BOOT_ONLY));
    EXPECT_TRUE(HwEnforcedAuthorizations(rsaKeyData.characteristics).Contains(TAG_EARLY_BOOT_ONLY));
    EXPECT_TRUE(
            HwEnforcedAuthorizations(ecdsaKeyData.characteristics).Contains(TAG_EARLY_BOOT_ONLY));

    // Should be able to use keys, since early boot has not ended
    EXPECT_EQ(ErrorCode::OK, UseAesKey(aesKeyData.blob));
    EXPECT_EQ(ErrorCode::OK, UseHmacKey(hmacKeyData.blob));
    EXPECT_EQ(ErrorCode::OK, UseRsaKey(rsaKeyData.blob));
    EXPECT_EQ(ErrorCode::OK, UseEcdsaKey(ecdsaKeyData.blob));

    // End early boot
    ErrorCode earlyBootResult = GetReturnErrorCode(keyMint().earlyBootEnded());
    EXPECT_EQ(earlyBootResult, ErrorCode::OK);

    // Should not be able to use already-created keys.
    EXPECT_EQ(ErrorCode::EARLY_BOOT_ENDED, UseAesKey(aesKeyData.blob));
    EXPECT_EQ(ErrorCode::EARLY_BOOT_ENDED, UseHmacKey(hmacKeyData.blob));
    EXPECT_EQ(ErrorCode::EARLY_BOOT_ENDED, UseRsaKey(rsaKeyData.blob));
    EXPECT_EQ(ErrorCode::EARLY_BOOT_ENDED, UseEcdsaKey(ecdsaKeyData.blob));

    CheckedDeleteKey(&aesKeyData.blob);
    CheckedDeleteKey(&hmacKeyData.blob);
    CheckedDeleteKey(&rsaKeyData.blob);
    CheckedDeleteKey(&ecdsaKeyData.blob);

    // Should not be able to create new keys
    std::tie(aesKeyData, hmacKeyData, rsaKeyData, ecdsaKeyData) =
            CreateTestKeys(TAG_EARLY_BOOT_ONLY, ErrorCode::EARLY_BOOT_ENDED);

    CheckedDeleteKey(&aesKeyData.blob);
    CheckedDeleteKey(&hmacKeyData.blob);
    CheckedDeleteKey(&rsaKeyData.blob);
    CheckedDeleteKey(&ecdsaKeyData.blob);
}

INSTANTIATE_KEYMINT_AIDL_TEST(EarlyBootKeyTest);

using UnlockedDeviceRequiredTest = KeyMintAidlTestBase;

// This may be a problematic test.  It can't be run repeatedly without unlocking the device in
// between runs... and on most test devices there are no enrolled credentials so it can't be
// unlocked at all, meaning the only way to get the test to pass again on a properly-functioning
// device is to reboot it.  For that reason, this is disabled by default.  It can be used as part of
// a manual test process, which includes unlocking between runs, which is why it's included here.
// Well, that and the fact that it's the only test we can do without also making calls into the
// Gatekeeper HAL.  We haven't written any cross-HAL tests, and don't know what all of the
// implications might be, so that may or may not be a solution.
TEST_P(UnlockedDeviceRequiredTest, DISABLED_KeysBecomeUnusable) {
    auto [aesKeyData, hmacKeyData, rsaKeyData, ecdsaKeyData] =
            CreateTestKeys(TAG_UNLOCKED_DEVICE_REQUIRED, ErrorCode::OK);

    EXPECT_EQ(ErrorCode::OK, UseAesKey(aesKeyData.blob));
    EXPECT_EQ(ErrorCode::OK, UseHmacKey(hmacKeyData.blob));
    EXPECT_EQ(ErrorCode::OK, UseRsaKey(rsaKeyData.blob));
    EXPECT_EQ(ErrorCode::OK, UseEcdsaKey(ecdsaKeyData.blob));

    ErrorCode rc = GetReturnErrorCode(
            keyMint().deviceLocked(false /* passwordOnly */, {} /* timestampToken */));
    ASSERT_EQ(ErrorCode::OK, rc);
    EXPECT_EQ(ErrorCode::DEVICE_LOCKED, UseAesKey(aesKeyData.blob));
    EXPECT_EQ(ErrorCode::DEVICE_LOCKED, UseHmacKey(hmacKeyData.blob));
    EXPECT_EQ(ErrorCode::DEVICE_LOCKED, UseRsaKey(rsaKeyData.blob));
    EXPECT_EQ(ErrorCode::DEVICE_LOCKED, UseEcdsaKey(ecdsaKeyData.blob));

    CheckedDeleteKey(&aesKeyData.blob);
    CheckedDeleteKey(&hmacKeyData.blob);
    CheckedDeleteKey(&rsaKeyData.blob);
    CheckedDeleteKey(&ecdsaKeyData.blob);
}

INSTANTIATE_KEYMINT_AIDL_TEST(UnlockedDeviceRequiredTest);

}  // namespace aidl::android::hardware::security::keymint::test

int main(int argc, char** argv) {
    std::cout << "Testing ";
    auto halInstances =
            aidl::android::hardware::security::keymint::test::KeyMintAidlTestBase::build_params();
    std::cout << "HAL instances:\n";
    for (auto& entry : halInstances) {
        std::cout << "    " << entry << '\n';
    }

    ::testing::InitGoogleTest(&argc, argv);
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            if (std::string(argv[i]) == "--arm_deleteAllKeys") {
                aidl::android::hardware::security::keymint::test::KeyMintAidlTestBase::
                        arm_deleteAllKeys = true;
            }
            if (std::string(argv[i]) == "--dump_attestations") {
                aidl::android::hardware::security::keymint::test::KeyMintAidlTestBase::
                        dump_Attestations = true;
            } else {
                std::cout << "NOT dumping attestations" << std::endl;
            }
            // TODO(drysdale): Remove this flag when available KeyMint devices comply with spec
            if (std::string(argv[i]) == "--check_patchLevels") {
                aidl::android::hardware::security::keymint::test::check_patchLevels = true;
            }
        }
    }
    return RUN_ALL_TESTS();
}
