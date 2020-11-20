/*
 * Copyright (c) 2019, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iomanip>
#include <iostream>
#include <sstream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <android/hardware/identity/support/IdentityCredentialSupport.h>

#include <cppbor.h>
#include <cppbor_parse.h>

using std::optional;
using std::string;
using std::vector;

namespace android {
namespace hardware {
namespace identity {

TEST(IdentityCredentialSupport, encodeHex) {
    EXPECT_EQ("", support::encodeHex(vector<uint8_t>({})));
    EXPECT_EQ("01", support::encodeHex(vector<uint8_t>({1})));
    EXPECT_EQ("000102030405060708090a0b0c0d0e0f10",
              support::encodeHex(
                      vector<uint8_t>({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16})));
    EXPECT_EQ("0102ffe060", support::encodeHex(vector<uint8_t>({1, 2, 255, 224, 96})));
}

TEST(IdentityCredentialSupport, decodeHex) {
    EXPECT_EQ(vector<uint8_t>({}), support::decodeHex(""));
    EXPECT_EQ(vector<uint8_t>({1}), support::decodeHex("01"));

    EXPECT_EQ(vector<uint8_t>({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}),
              support::decodeHex("000102030405060708090a0b0c0d0e0f10"));

    EXPECT_FALSE(support::decodeHex("0g"));
    EXPECT_FALSE(support::decodeHex("0"));
    EXPECT_FALSE(support::decodeHex("012"));
}

TEST(IdentityCredentialSupport, CborPrettyPrint) {
    EXPECT_EQ("'Some text'", support::cborPrettyPrint(cppbor::Tstr("Some text").encode()));

    EXPECT_EQ("''", support::cborPrettyPrint(cppbor::Tstr("").encode()));

    EXPECT_EQ("{0x01, 0x00, 0x02, 0xf0, 0xff, 0x40}",
              support::cborPrettyPrint(
                      cppbor::Bstr(vector<uint8_t>({1, 0, 2, 240, 255, 64})).encode()));

    EXPECT_EQ("{}", support::cborPrettyPrint(cppbor::Bstr(vector<uint8_t>()).encode()));

    EXPECT_EQ("true", support::cborPrettyPrint(cppbor::Bool(true).encode()));

    EXPECT_EQ("false", support::cborPrettyPrint(cppbor::Bool(false).encode()));

    EXPECT_EQ("42", support::cborPrettyPrint(cppbor::Uint(42).encode()));

    EXPECT_EQ("9223372036854775807",  // 0x7fff ffff ffff ffff
              support::cborPrettyPrint(cppbor::Uint(std::numeric_limits<int64_t>::max()).encode()));

    EXPECT_EQ("-42", support::cborPrettyPrint(cppbor::Nint(-42).encode()));

    EXPECT_EQ("-9223372036854775808",  // -0x8000 0000 0000 0000
              support::cborPrettyPrint(cppbor::Nint(std::numeric_limits<int64_t>::min()).encode()));
}

TEST(IdentityCredentialSupport, CborPrettyPrintCompound) {
    cppbor::Array array = cppbor::Array("foo", "bar", "baz");
    EXPECT_EQ("['foo', 'bar', 'baz', ]", support::cborPrettyPrint(array.encode()));

    cppbor::Map map = cppbor::Map().add("foo", 42).add("bar", 43).add("baz", 44);
    EXPECT_EQ(
            "{\n"
            "  'foo' : 42,\n"
            "  'bar' : 43,\n"
            "  'baz' : 44,\n"
            "}",
            support::cborPrettyPrint(map.encode()));

    cppbor::Array array2 = cppbor::Array(cppbor::Tstr("Some text"), cppbor::Nint(-42));
    EXPECT_EQ("['Some text', -42, ]", support::cborPrettyPrint(array2.encode()));

    cppbor::Map map2 = cppbor::Map().add(42, "foo").add(43, "bar").add(44, "baz");
    EXPECT_EQ(
            "{\n"
            "  42 : 'foo',\n"
            "  43 : 'bar',\n"
            "  44 : 'baz',\n"
            "}",
            support::cborPrettyPrint(map2.encode()));

    cppbor::Array deeplyNestedArrays =
            cppbor::Array(cppbor::Array(cppbor::Array("a", "b", "c")),
                          cppbor::Array(cppbor::Array("d", "e", cppbor::Array("f", "g"))));
    EXPECT_EQ(
            "[\n"
            "  ['a', 'b', 'c', ],\n"
            "  [\n    'd',\n"
            "    'e',\n"
            "    ['f', 'g', ],\n"
            "  ],\n"
            "]",
            support::cborPrettyPrint(deeplyNestedArrays.encode()));

    EXPECT_EQ(
            "[\n"
            "  {0x0a, 0x0b},\n"
            "  'foo',\n"
            "  42,\n"
            "  ['foo', 'bar', 'baz', ],\n"
            "  {\n"
            "    'foo' : 42,\n"
            "    'bar' : 43,\n"
            "    'baz' : 44,\n"
            "  },\n"
            "  {\n"
            "    'deep1' : ['Some text', -42, ],\n"
            "    'deep2' : {\n"
            "      42 : 'foo',\n"
            "      43 : 'bar',\n"
            "      44 : 'baz',\n"
            "    },\n"
            "  },\n"
            "]",
            support::cborPrettyPrint(cppbor::Array(cppbor::Bstr(vector<uint8_t>{10, 11}),
                                                   cppbor::Tstr("foo"), cppbor::Uint(42),
                                                   std::move(array), std::move(map),
                                                   (cppbor::Map()
                                                            .add("deep1", std::move(array2))
                                                            .add("deep2", std::move(map2))))
                                             .encode()));
}

TEST(IdentityCredentialSupport, Signatures) {
    vector<uint8_t> data = {1, 2, 3};

    optional<vector<uint8_t>> keyPair = support::createEcKeyPair();
    ASSERT_TRUE(keyPair);
    optional<vector<uint8_t>> privKey = support::ecKeyPairGetPrivateKey(keyPair.value());
    ASSERT_TRUE(privKey);
    optional<vector<uint8_t>> pubKey = support::ecKeyPairGetPublicKey(keyPair.value());
    ASSERT_TRUE(pubKey);

    optional<vector<uint8_t>> signature = support::signEcDsa(privKey.value(), data);
    ASSERT_TRUE(
            support::checkEcDsaSignature(support::sha256(data), signature.value(), pubKey.value()));

    // Manipulate the signature, check that verification fails.
    vector<uint8_t> modifiedSignature = signature.value();
    modifiedSignature[0] ^= 0xff;
    ASSERT_FALSE(
            support::checkEcDsaSignature(support::sha256(data), modifiedSignature, pubKey.value()));

    // Manipulate the data being checked, check that verification fails.
    vector<uint8_t> modifiedDigest = support::sha256(data);
    modifiedDigest[0] ^= 0xff;
    ASSERT_FALSE(support::checkEcDsaSignature(modifiedDigest, signature.value(), pubKey.value()));
}

string replaceLine(const string& str, ssize_t lineNumber, const string& replacement) {
    vector<string> lines;
    std::istringstream f(str);
    string s;
    while (std::getline(f, s, '\n')) {
        lines.push_back(s);
    }

    size_t numLines = lines.size();
    if (lineNumber < 0) {
        lineNumber = numLines - (-lineNumber);
    }

    string ret;
    size_t n = 0;
    for (const string& line : lines) {
        if (n == lineNumber) {
            ret += replacement + "\n";
        } else {
            ret += line + "\n";
        }
        n++;
    }
    return ret;
}

TEST(IdentityCredentialSupport, CoseSignatures) {
    optional<vector<uint8_t>> keyPair = support::createEcKeyPair();
    ASSERT_TRUE(keyPair);
    optional<vector<uint8_t>> privKey = support::ecKeyPairGetPrivateKey(keyPair.value());
    ASSERT_TRUE(privKey);
    optional<vector<uint8_t>> pubKey = support::ecKeyPairGetPublicKey(keyPair.value());
    ASSERT_TRUE(pubKey);

    vector<uint8_t> data = {1, 2, 3};
    optional<vector<uint8_t>> coseSign1 = support::coseSignEcDsa(
            privKey.value(), data, {} /* detachedContent */, {} /* x5chain */);
    ASSERT_TRUE(support::coseCheckEcDsaSignature(coseSign1.value(), {} /* detachedContent */,
                                                 pubKey.value()));

    optional<vector<uint8_t>> payload = support::coseSignGetPayload(coseSign1.value());
    ASSERT_TRUE(payload);
    ASSERT_EQ(data, payload.value());

    // Finally, check that |coseSign1| are the bytes of a valid COSE_Sign1 message
    string out = support::cborPrettyPrint(coseSign1.value());
    out = replaceLine(out, -2, "  [] // Signature Removed");
    EXPECT_EQ(
            "[\n"
            "  {0xa1, 0x01, 0x26},\n"  // Bytes of {1:-7} 1 is 'alg' label and -7 is "ECDSA 256"
            "  {},\n"
            "  {0x01, 0x02, 0x03},\n"
            "  [] // Signature Removed\n"
            "]\n",
            out);
}

TEST(IdentityCredentialSupport, CoseSignaturesAdditionalData) {
    optional<vector<uint8_t>> keyPair = support::createEcKeyPair();
    ASSERT_TRUE(keyPair);
    optional<vector<uint8_t>> privKey = support::ecKeyPairGetPrivateKey(keyPair.value());
    ASSERT_TRUE(privKey);
    optional<vector<uint8_t>> pubKey = support::ecKeyPairGetPublicKey(keyPair.value());
    ASSERT_TRUE(pubKey);

    vector<uint8_t> detachedContent = {1, 2, 3};
    optional<vector<uint8_t>> coseSign1 = support::coseSignEcDsa(privKey.value(), {} /* data */,
                                                                 detachedContent, {} /* x5chain */);
    ASSERT_TRUE(
            support::coseCheckEcDsaSignature(coseSign1.value(), detachedContent, pubKey.value()));

    optional<vector<uint8_t>> payload = support::coseSignGetPayload(coseSign1.value());
    ASSERT_TRUE(payload);
    ASSERT_EQ(0, payload.value().size());

    // Finally, check that |coseSign1| are the bytes of a valid COSE_Sign1 message
    string out = support::cborPrettyPrint(coseSign1.value());
    out = replaceLine(out, -2, "  [] // Signature Removed");
    EXPECT_EQ(
            "[\n"
            "  {0xa1, 0x01, 0x26},\n"  // Bytes of {1:-7} 1 is 'alg' label and -7 is "ECDSA 256"
            "  {},\n"
            "  null,\n"
            "  [] // Signature Removed\n"
            "]\n",
            out);
}

vector<uint8_t> generateCertChain(size_t numCerts) {
    vector<vector<uint8_t>> certs;

    for (size_t n = 0; n < numCerts; n++) {
        optional<vector<uint8_t>> keyPair = support::createEcKeyPair();
        optional<vector<uint8_t>> privKey = support::ecKeyPairGetPrivateKey(keyPair.value());
        optional<vector<uint8_t>> pubKey = support::ecKeyPairGetPublicKey(keyPair.value());

        optional<vector<uint8_t>> cert = support::ecPublicKeyGenerateCertificate(
                pubKey.value(), privKey.value(), "0001", "someIssuer", "someSubject", 0, 0);
        certs.push_back(cert.value());
    }
    return support::certificateChainJoin(certs);
}

TEST(IdentityCredentialSupport, CoseSignaturesX5ChainWithSingleCert) {
    optional<vector<uint8_t>> keyPair = support::createEcKeyPair();
    ASSERT_TRUE(keyPair);
    optional<vector<uint8_t>> privKey = support::ecKeyPairGetPrivateKey(keyPair.value());
    ASSERT_TRUE(privKey);
    optional<vector<uint8_t>> pubKey = support::ecKeyPairGetPublicKey(keyPair.value());
    ASSERT_TRUE(pubKey);

    vector<uint8_t> certChain = generateCertChain(1);
    optional<vector<vector<uint8_t>>> splitCerts = support::certificateChainSplit(certChain);
    ASSERT_EQ(1, splitCerts.value().size());

    vector<uint8_t> detachedContent = {1, 2, 3};
    optional<vector<uint8_t>> coseSign1 =
            support::coseSignEcDsa(privKey.value(), {} /* data */, detachedContent, certChain);
    ASSERT_TRUE(
            support::coseCheckEcDsaSignature(coseSign1.value(), detachedContent, pubKey.value()));

    optional<vector<uint8_t>> payload = support::coseSignGetPayload(coseSign1.value());
    ASSERT_TRUE(payload);
    ASSERT_EQ(0, payload.value().size());

    optional<vector<uint8_t>> certsRecovered = support::coseSignGetX5Chain(coseSign1.value());
    EXPECT_EQ(certsRecovered.value(), certChain);
}

TEST(IdentityCredentialSupport, CoseSignaturesX5ChainWithMultipleCerts) {
    optional<vector<uint8_t>> keyPair = support::createEcKeyPair();
    ASSERT_TRUE(keyPair);
    optional<vector<uint8_t>> privKey = support::ecKeyPairGetPrivateKey(keyPair.value());
    ASSERT_TRUE(privKey);
    optional<vector<uint8_t>> pubKey = support::ecKeyPairGetPublicKey(keyPair.value());
    ASSERT_TRUE(pubKey);

    vector<uint8_t> certChain = generateCertChain(5);
    optional<vector<vector<uint8_t>>> splitCerts = support::certificateChainSplit(certChain);
    ASSERT_EQ(5, splitCerts.value().size());

    vector<uint8_t> detachedContent = {1, 2, 3};
    optional<vector<uint8_t>> coseSign1 =
            support::coseSignEcDsa(privKey.value(), {} /* data */, detachedContent, certChain);
    ASSERT_TRUE(
            support::coseCheckEcDsaSignature(coseSign1.value(), detachedContent, pubKey.value()));

    optional<vector<uint8_t>> payload = support::coseSignGetPayload(coseSign1.value());
    ASSERT_TRUE(payload);
    ASSERT_EQ(0, payload.value().size());

    optional<vector<uint8_t>> certsRecovered = support::coseSignGetX5Chain(coseSign1.value());
    EXPECT_EQ(certsRecovered.value(), certChain);
}

TEST(IdentityCredentialSupport, CertificateChain) {
    optional<vector<uint8_t>> keyPair = support::createEcKeyPair();
    ASSERT_TRUE(keyPair);
    optional<vector<uint8_t>> privKey = support::ecKeyPairGetPrivateKey(keyPair.value());
    ASSERT_TRUE(privKey);
    optional<vector<uint8_t>> pubKey = support::ecKeyPairGetPublicKey(keyPair.value());
    ASSERT_TRUE(pubKey);

    optional<vector<uint8_t>> cert = support::ecPublicKeyGenerateCertificate(
            pubKey.value(), privKey.value(), "0001", "someIssuer", "someSubject", 0, 0);

    optional<vector<uint8_t>> extractedPubKey =
            support::certificateChainGetTopMostKey(cert.value());
    ASSERT_TRUE(extractedPubKey);
    ASSERT_EQ(pubKey.value(), extractedPubKey.value());

    // We expect to the chain returned by ecPublicKeyGenerateCertificate() to only have a
    // single element
    optional<vector<vector<uint8_t>>> splitCerts = support::certificateChainSplit(cert.value());
    ASSERT_EQ(1, splitCerts.value().size());
    ASSERT_EQ(splitCerts.value()[0], cert.value());

    optional<vector<uint8_t>> otherKeyPair = support::createEcKeyPair();
    ASSERT_TRUE(otherKeyPair);
    optional<vector<uint8_t>> otherPrivKey = support::ecKeyPairGetPrivateKey(keyPair.value());
    ASSERT_TRUE(otherPrivKey);
    optional<vector<uint8_t>> otherPubKey = support::ecKeyPairGetPublicKey(keyPair.value());
    ASSERT_TRUE(otherPubKey);
    optional<vector<uint8_t>> otherCert = support::ecPublicKeyGenerateCertificate(
            otherPubKey.value(), privKey.value(), "0001", "someIssuer", "someSubject", 0, 0);

    // Now both cert and otherCert are two distinct certificates. Let's make a
    // chain and check that certificateChainSplit() works as expected.
    ASSERT_NE(cert.value(), otherCert.value());
    const vector<vector<uint8_t>> certs2 = {cert.value(), otherCert.value()};
    vector<uint8_t> certs2combined = support::certificateChainJoin(certs2);
    ASSERT_EQ(certs2combined.size(), cert.value().size() + otherCert.value().size());
    optional<vector<vector<uint8_t>>> splitCerts2 = support::certificateChainSplit(certs2combined);
    ASSERT_EQ(certs2, splitCerts2.value());
}

vector<uint8_t> strToVec(const string& str) {
    vector<uint8_t> ret;
    size_t size = str.size();
    ret.resize(size);
    memcpy(ret.data(), str.data(), size);
    return ret;
}

// Test vector from https://en.wikipedia.org/wiki/HMAC
TEST(IdentityCredentialSupport, hmacSha256) {
    vector<uint8_t> key = strToVec("key");
    vector<uint8_t> data = strToVec("The quick brown fox jumps over the lazy dog");

    vector<uint8_t> expected =
            support::decodeHex("f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8")
                    .value();

    optional<vector<uint8_t>> hmac = support::hmacSha256(key, data);
    ASSERT_TRUE(hmac);
    ASSERT_EQ(expected, hmac.value());
}

// See also CoseMac0 test in UtilUnitTest.java inside cts/tests/tests/identity/
TEST(IdentityCredentialSupport, CoseMac0) {
    vector<uint8_t> key;
    key.resize(32);
    vector<uint8_t> data = {0x10, 0x11, 0x12, 0x13};
    vector<uint8_t> detachedContent = {};

    optional<vector<uint8_t>> mac = support::coseMac0(key, data, detachedContent);
    ASSERT_TRUE(mac);

    EXPECT_EQ(
            "[\n"
            "  {0xa1, 0x01, 0x05},\n"
            "  {},\n"
            "  {0x10, 0x11, 0x12, 0x13},\n"
            "  {0x6c, 0xec, 0xb5, 0x6a, 0xc9, 0x5c, 0xae, 0x3b, 0x41, 0x13, 0xde, 0xa4, 0xd8, "
            "0x86, 0x5c, 0x28, 0x2c, 0xd5, 0xa5, 0x13, 0xff, 0x3b, 0xd1, 0xde, 0x70, 0x5e, 0xbb, "
            "0xe2, 0x2d, 0x42, 0xbe, 0x53},\n"
            "]",
            support::cborPrettyPrint(mac.value()));
}

TEST(IdentityCredentialSupport, CoseMac0DetachedContent) {
    vector<uint8_t> key;
    key.resize(32);
    vector<uint8_t> data = {};
    vector<uint8_t> detachedContent = {0x10, 0x11, 0x12, 0x13};

    optional<vector<uint8_t>> mac = support::coseMac0(key, data, detachedContent);
    ASSERT_TRUE(mac);

    // Same HMAC as in CoseMac0 test, only difference is that payload is null.
    EXPECT_EQ(
            "[\n"
            "  {0xa1, 0x01, 0x05},\n"
            "  {},\n"
            "  null,\n"
            "  {0x6c, 0xec, 0xb5, 0x6a, 0xc9, 0x5c, 0xae, 0x3b, 0x41, 0x13, 0xde, 0xa4, 0xd8, "
            "0x86, 0x5c, 0x28, 0x2c, 0xd5, 0xa5, 0x13, 0xff, 0x3b, 0xd1, 0xde, 0x70, 0x5e, 0xbb, "
            "0xe2, 0x2d, 0x42, 0xbe, 0x53},\n"
            "]",
            support::cborPrettyPrint(mac.value()));
}

// Generates a private key in DER format for a small value of 'd'.
//
// Used for test vectors.
//
vector<uint8_t> p256PrivateKeyFromD(uint8_t d) {
    vector<uint8_t> privateUncompressed;
    privateUncompressed.resize(32);
    privateUncompressed[31] = d;
    optional<vector<uint8_t>> privateKey = support::ecPrivateKeyToKeyPair(privateUncompressed);
    return privateKey.value();
}

std::pair<vector<uint8_t>, vector<uint8_t>> p256PrivateKeyGetXandY(
        const vector<uint8_t> privateKey) {
    optional<vector<uint8_t>> publicUncompressed = support::ecKeyPairGetPublicKey(privateKey);
    vector<uint8_t> x = vector<uint8_t>(publicUncompressed.value().begin() + 1,
                                        publicUncompressed.value().begin() + 33);
    vector<uint8_t> y = vector<uint8_t>(publicUncompressed.value().begin() + 33,
                                        publicUncompressed.value().begin() + 65);
    return std::make_pair(x, y);
}

const cppbor::Item* findValueForTstr(const cppbor::Map* map, const string& keyValue) {
    // TODO: Need cast until libcppbor's Map::get() is marked as const
    auto [item, found] = ((cppbor::Map*)map)->get(keyValue);
    if (!found) {
        return nullptr;
    }
    return item.get();
}

const cppbor::Array* findArrayValueForTstr(const cppbor::Map* map, const string& keyValue) {
    const cppbor::Item* item = findValueForTstr(map, keyValue);
    if (item == nullptr) {
        return nullptr;
    }
    return item->asArray();
}

const cppbor::Map* findMapValueForTstr(const cppbor::Map* map, const string& keyValue) {
    const cppbor::Item* item = findValueForTstr(map, keyValue);
    if (item == nullptr) {
        return nullptr;
    }
    return item->asMap();
}

const cppbor::Semantic* findSemanticValueForTstr(const cppbor::Map* map, const string& keyValue) {
    const cppbor::Item* item = findValueForTstr(map, keyValue);
    if (item == nullptr) {
        return nullptr;
    }
    return item->asSemantic();
}

const std::string findStringValueForTstr(const cppbor::Map* map, const string& keyValue) {
    const cppbor::Item* item = findValueForTstr(map, keyValue);
    if (item == nullptr) {
        return nullptr;
    }
    const cppbor::Tstr* tstr = item->asTstr();
    if (tstr == nullptr) {
        return "";
    }
    return tstr->value();
}

TEST(IdentityCredentialSupport, testVectors_18013_5) {
    // This is a test against known vectors for ISO 18013-5.
    //
    // The objective of this test is to verify that support::calcEMacKey() and
    // support::calcMac() agree with the given test vectors.
    //

    // We're given static device key:
    //
    //     x: 28412803729898893058558238221310261427084375743576167377786533380249859400145
    //     y: 65403602826180996396520286939226973026599920614829401631985882360676038096704
    //     d: 11
    //
    vector<uint8_t> deviceKey = p256PrivateKeyFromD(11);
    auto [deviceKeyX, deviceKeyY] = p256PrivateKeyGetXandY(deviceKey);
    EXPECT_EQ(support::encodeHex(deviceKeyX),
              "3ed113b7883b4c590638379db0c21cda16742ed0255048bf433391d374bc21d1");
    EXPECT_EQ(support::encodeHex(deviceKeyY),
              "9099209accc4c8a224c843afa4f4c68a090d04da5e9889dae2f8eefce82a3740");

    // We're given Ephemeral reader key:
    //
    //   x: 59535862115950685744176693329402396749019581632805653266809849538337418304154
    //   y: 53776829996815113213100700404832701936765102413212294632483274374518863708344
    //   d: 20
    //
    vector<uint8_t> ephemeralReaderKey = p256PrivateKeyFromD(20);
    auto [ephemeralReaderKeyX, ephemeralReaderKeyY] = p256PrivateKeyGetXandY(ephemeralReaderKey);
    EXPECT_EQ(support::encodeHex(ephemeralReaderKeyX),
              "83a01a9378395bab9bcd6a0ad03cc56d56e6b19250465a94a234dc4c6b28da9a");
    EXPECT_EQ(support::encodeHex(ephemeralReaderKeyY),
              "76e49b6de2f73234ae6a5eb9d612b75c9f2202bb6923f54ff8240aaa86f640b8");
    vector<uint8_t> ephemeralReaderKeyPublic =
            support::ecKeyPairGetPublicKey(ephemeralReaderKey).value();

    // We're given SessionEstablishment.
    //
    //   SessionEstablishment = {
    //     "eReaderKey" : EReaderKeyBytes,
    //     "data" : bstr ; Encrypted mdoc request
    //   }
    //
    // Fish out EReaderKey from this.
    //
    // Note that the test vector below is incorrect insofar that it uses
    // "eReaderKeyBytes" instead of just "eReaderKey". This will be corrected in
    // the future.
    //
    optional<vector<uint8_t>> sessionEstablishmentEncoded = support::decodeHex(
            "a26f655265616465724b65794279746573d818584ba40102200121582083a01a9378395bab9bcd6a0ad03c"
            "c56d56e6b19250465a94a234dc4c6b28da9a22582076e49b6de2f73234ae6a5eb9d612b75c9f2202bb6923"
            "f54ff8240aaa86f640b864646174615902d945b31040c57491acb6d46a71f6c1f67a0b837df1bda9089fd0"
            "3d0b1fdac3eeb2874a4ef6f90c97d03397186ba00a91102faae7e992e15f761d5662c3c37e3c6c2cfd2ebc"
            "0bf59dbb8795e377bd7dd353230a41ba2d82294b45871a39b42ca531f26b52f46e356fbaf5075c8fd5b8b0"
            "8a0df4a1d2e1bdd2e5d69169c1efbb51e393e608d833d325bebfbccb2e15ec08f94b264582fa7b93f7cebc"
            "aa69f4f0cac2744d4fe35b04df26b2ae69273eed33024949080c1c95a6ef046beede959e9494297dd770af"
            "4ac6fdd56783aa012555c213dc05cf0f41d1c95119720fcfe1621027f80e2ddd56ea3c1fc596f7b2579333"
            "5a887ec788092b4a69d23b6219e27d0249b50b3fdcb95b5227007689362e0416b3bae3dae7cb56b4394666"
            "4e3a3f60dce8d0b678fcd754bebf87bd2b0278dd782d952488a46f2874e34c2dd97bb74084a62b850e9719"
            "252cd1dca7dbf1858193f6cf093cb3735312bbe1138cf29d8f350e285923f8ef07065299926720b42264e8"
            "fd5d4b133e72f47c4e999ea689c353f8b41e50a59838e1a0d09eca4a557f77a9c389a0591ad1639119ce86"
            "edc3320130480ee5101effae6066e8c85aac9ead2ae83e49c1e508aab02f753decbb522ea2200d62fd5d26"
            "094bd35100bffaa1cdc6af9f7e9cfe7b63da6b5671cd5ac2cf5da450c72addc64cde441f3b7f7fdaf930ad"
            "1e13388e8a7308d8ca4607e59e082db431a232e7e12cb692baeb4b2127e110ff24cea322ffdbc2e4d9c4c6"
            "bed27753137d07897c8613627a799a560cf1a2d1edb3de029442862940a5ed7785eea8b6ace93aa6af0792"
            "fd82877f62d07b757d0179ecbb7347004ecc9c0690d41f75f188cb17ffd2cec2ad8c9675466bb33b737a2a"
            "e7592b2dcb8132aced2e572266f3f5413a5f9d6d4339a1e4662622af2e7e157a4ea3bfd5c4247e2ec91d8c"
            "5c3c17427d5edfae673d0e0f782a8d40fa805fd8bc82ae3cb21a65cdad863e02309f6b01d1753fa884b778"
            "f6e019a2004d8964deeb11f1fd478fcb");
    ASSERT_TRUE(sessionEstablishmentEncoded);
    auto [sessionEstablishmentItem, _se, _se2] = cppbor::parse(sessionEstablishmentEncoded.value());
    const cppbor::Map* sessionEstablishment = sessionEstablishmentItem->asMap();
    ASSERT_NE(sessionEstablishment, nullptr);
    const cppbor::Semantic* eReaderKeyBytes =
            findSemanticValueForTstr(sessionEstablishment, "eReaderKeyBytes");
    ASSERT_NE(eReaderKeyBytes, nullptr);
    ASSERT_EQ(eReaderKeyBytes->value(), 24);
    const cppbor::Bstr* eReaderKeyBstr = eReaderKeyBytes->child()->asBstr();
    ASSERT_NE(eReaderKeyBstr, nullptr);
    vector<uint8_t> eReaderKeyEncoded = eReaderKeyBstr->value();
    // TODO: verify this agrees with ephemeralReaderKeyX and ephemeralReaderKeyY

    // We're given DeviceEngagement.
    //
    vector<uint8_t> deviceEngagementEncoded =
            support::decodeHex(
                    "a20063312e30018201d818584ba401022001215820cef66d6b2a3a993e591214d1ea223fb545ca"
                    "6c471c48306e4c36069404c5723f225820878662a229aaae906e123cdd9d3b4c10590ded29fe75"
                    "1eeeca34bbaa44af0773")
                    .value();

    // Now calculate SessionTranscriptBytes. It is defined as
    //
    //   SessionTranscript = [
    //      DeviceEngagementBytes,
    //      EReaderKeyBytes,
    //      Handover
    //   ]
    //
    //   SessionTranscriptBytes = #6.24(bstr .cbor SessionTranscript)
    //
    cppbor::Array sessionTranscript;
    sessionTranscript.add(cppbor::Semantic(24, deviceEngagementEncoded));
    sessionTranscript.add(cppbor::Semantic(24, eReaderKeyEncoded));
    sessionTranscript.add(cppbor::Null());
    vector<uint8_t> sessionTranscriptEncoded = sessionTranscript.encode();
    vector<uint8_t> sessionTranscriptBytes =
            cppbor::Semantic(24, sessionTranscriptEncoded).encode();

    // The expected EMacKey is 4c1ebb8aacc633465390fa44edfdb49cb57f2e079aaa771d812584699c0b97e2
    //
    // Verify that support::calcEMacKey() gets the same result.
    //
    optional<vector<uint8_t>> eMacKey =
            support::calcEMacKey(support::ecKeyPairGetPrivateKey(deviceKey).value(),  // private key
                                 ephemeralReaderKeyPublic,                            // public key
                                 sessionTranscriptBytes);  // sessionTranscriptBytes
    ASSERT_TRUE(eMacKey);
    ASSERT_EQ(support::encodeHex(eMacKey.value()),
              "4c1ebb8aacc633465390fa44edfdb49cb57f2e079aaa771d812584699c0b97e2");

    // Also do it the other way around
    //
    optional<vector<uint8_t>> eMacKey2 = support::calcEMacKey(
            support::ecKeyPairGetPrivateKey(ephemeralReaderKey).value(),  // private key
            support::ecKeyPairGetPublicKey(deviceKey).value(),            // public key
            sessionTranscriptBytes);                                      // sessionTranscriptBytes
    ASSERT_TRUE(eMacKey2);
    ASSERT_EQ(support::encodeHex(eMacKey2.value()),
              "4c1ebb8aacc633465390fa44edfdb49cb57f2e079aaa771d812584699c0b97e2");

    // We're given DeviceResponse
    //
    vector<uint8_t> deviceResponseEncoded =
            support::decodeHex(
                    "a36776657273696f6e63312e3069646f63756d656e747381a367646f6354797065756f72672e69"
                    "736f2e31383031332e352e312e6d444c6c6973737565725369676e6564a26a6e616d6553706163"
                    "6573a2716f72672e69736f2e31383031332e352e3181d8185863a4686469676573744944016672"
                    "616e646f6d58208798645b20ea200e19ffabac92624bee6aec63aceedecfb1b80077d22bfc20e9"
                    "71656c656d656e744964656e7469666965726b66616d696c795f6e616d656c656c656d656e7456"
                    "616c756563446f656b636f6d2e6578616d706c6581d8185864a468646967657374494401667261"
                    "6e646f6d5820218ecf13521b53f4b96abaebe56417afec0e4c91fc8fb26086cd1e5cdc1a94ff71"
                    "656c656d656e744964656e7469666965726f616e6f746865725f656c656d656e746c656c656d65"
                    "6e7456616c75650a6a697373756572417574688443a10126a118215901d2308201ce30820174a0"
                    "0302010202141f7d44f4f107c5ee3f566049cf5d72de294b0d23300a06082a8648ce3d04030230"
                    "233114301206035504030c0b75746f7069612069616361310b3009060355040613025553301e17"
                    "0d3230313030313030303030305a170d3231313030313030303030305a30213112301006035504"
                    "030c0975746f706961206473310b30090603550406130255533059301306072a8648ce3d020106"
                    "082a8648ce3d03010703420004301d9e502dc7e05da85da026a7ae9aa0fac9db7d52a95b3e3e3f"
                    "9aa0a1b45b8b6551b6f6b3061223e0d23c026b017d72298d9ae46887ca61d58db6aea17ee267a3"
                    "8187308184301e0603551d120417301581136578616d706c65406578616d706c652e636f6d301c"
                    "0603551d1f041530133011a00fa00d820b6578616d706c652e636f6d301d0603551d0e04160414"
                    "7bef4db59a1ffb07592bfc57f4743b8a73aea792300e0603551d0f0101ff040403020780301506"
                    "03551d250101ff040b3009060728818c5d050102300a06082a8648ce3d04030203480030450220"
                    "21d52fb1fbda80e5bfda1e8dfb1bc7bf0acb7261d5c9ff54425af76eb21571c602210082bf301f"
                    "89e0a2cb9ca9c9050352de80b47956764f7a3e07bf6a8cd87528a3b55901d2d8185901cda66776"
                    "657273696f6e63312e306f646967657374416c676f726974686d675348412d3235366c76616c75"
                    "6544696765737473a2716f72672e69736f2e31383031332e352e31a20058203b22af1126771f02"
                    "f0ea0d546d4ee3c5b51637381154f5211b79daf5f9facaa8015820f2cba0ce3cde5df901a3da75"
                    "13a4d7f7225fdfe5a306544529bf3dbcce655ca06b636f6d2e6578616d706c65a200582072636d"
                    "ddc282424a63499f4b3927aaa3b74da7b9c0134178bf735e949e4a761e01582006322d3cbe6603"
                    "876bdacc5b6679b51b0fc53d029c244fd5ea719d9028459c916d6465766963654b6579496e666f"
                    "a1696465766963654b6579a4010220012158203ed113b7883b4c590638379db0c21cda16742ed0"
                    "255048bf433391d374bc21d12258209099209accc4c8a224c843afa4f4c68a090d04da5e9889da"
                    "e2f8eefce82a374067646f6354797065756f72672e69736f2e31383031332e352e312e6d444c6c"
                    "76616c6964697479496e666fa3667369676e6564c074323032302d31302d30315431333a33303a"
                    "30325a6976616c696446726f6dc074323032302d31302d30315431333a33303a30325a6a76616c"
                    "6964556e74696cc074323032312d31302d30315431333a33303a30325a5840273ec1b59817d571"
                    "b5a2c5c0ab0ea213d42acb18547fd7097afcc888a22ecbb863c6461ce0e240880895b4aaa84308"
                    "784571c7be7aa3a2e7e3a2ea1a145ed1966c6465766963655369676e6564a26a6e616d65537061"
                    "636573d81841a06a64657669636541757468a1696465766963654d61638443a10105a0f6582009"
                    "da7c964ac004ec36ec64edd0c1abf50c03433c215c3ddb144768abcdf20a60667374617475730"
                    "0")
                    .value();
    auto [deviceResponseItem, _, _2] = cppbor::parse(deviceResponseEncoded);
    const cppbor::Map* deviceResponse = deviceResponseItem->asMap();
    ASSERT_NE(deviceResponse, nullptr);
    const cppbor::Array* documents = findArrayValueForTstr(deviceResponse, "documents");
    ASSERT_NE(documents, nullptr);
    ASSERT_EQ(documents->size(), 1);
    const cppbor::Map* document = ((*documents)[0])->asMap();
    ASSERT_NE(document, nullptr);

    // Get docType
    string docType = findStringValueForTstr(document, "docType");
    ASSERT_EQ(docType, "org.iso.18013.5.1.mDL");

    // Drill down...
    const cppbor::Map* deviceSigned = findMapValueForTstr(document, "deviceSigned");
    ASSERT_NE(deviceSigned, nullptr);

    // Dig out the encoded form of DeviceNameSpaces
    //
    const cppbor::Semantic* deviceNameSpacesBytes =
            findSemanticValueForTstr(deviceSigned, "nameSpaces");
    ASSERT_NE(deviceNameSpacesBytes, nullptr);
    ASSERT_EQ(deviceNameSpacesBytes->value(), 24);
    const cppbor::Bstr* deviceNameSpacesBstr = deviceNameSpacesBytes->child()->asBstr();
    ASSERT_NE(deviceNameSpacesBstr, nullptr);
    vector<uint8_t> deviceNameSpacesEncoded = deviceNameSpacesBstr->value();

    // (For this version of 18013-5, DeviceNameSpaces is always supposed to be empty, check that.)
    EXPECT_EQ(deviceNameSpacesEncoded, cppbor::Map().encode());

    const cppbor::Map* deviceAuth = findMapValueForTstr(deviceSigned, "deviceAuth");
    ASSERT_NE(deviceAuth, nullptr);
    // deviceMac is is the COSE_Mac0.. dig out the encoded form to check that
    // support::calcMac() gives exactly the same bytes.
    //
    const cppbor::Array* deviceMac = findArrayValueForTstr(deviceAuth, "deviceMac");
    ASSERT_NE(deviceMac, nullptr);
    vector<uint8_t> deviceMacEncoded = deviceMac->encode();

    // Now we calculate what it should be..
    optional<vector<uint8_t>> calculatedMac =
            support::calcMac(sessionTranscriptEncoded,  // SessionTranscript
                             docType,                   // DocType
                             deviceNameSpacesEncoded,   // DeviceNamespaces
                             eMacKey.value());          // EMacKey
    ASSERT_TRUE(calculatedMac);

    // ... and hopefully it's the same!
    ASSERT_EQ(calculatedMac.value().size(), deviceMacEncoded.size());
    EXPECT_TRUE(memcmp(calculatedMac.value().data(), deviceMacEncoded.data(),
                       deviceMacEncoded.size()) == 0);
}

}  // namespace identity
}  // namespace hardware
}  // namespace android

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
