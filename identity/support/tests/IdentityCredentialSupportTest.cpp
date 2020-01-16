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

}  // namespace identity
}  // namespace hardware
}  // namespace android

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
