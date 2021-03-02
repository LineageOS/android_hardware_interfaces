/*
 * Copyright 2019, The Android Open Source Project
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

#ifndef IDENTITY_SUPPORT_INCLUDE_IDENTITY_CREDENTIAL_UTILS_H_
#define IDENTITY_SUPPORT_INCLUDE_IDENTITY_CREDENTIAL_UTILS_H_

#include <cstdint>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace android {
namespace hardware {
namespace identity {
namespace support {

using ::std::optional;
using ::std::string;
using ::std::tuple;
using ::std::vector;
using ::std::pair;

// The semantic tag for a bstr which includes Encoded CBOR (RFC 7049, section 2.4)
const int kSemanticTagEncodedCbor = 24;

// ---------------------------------------------------------------------------
// Miscellaneous utilities.
// ---------------------------------------------------------------------------

// Dumps the data in |data| to stderr. The written data will be of the following
// form for the call hexdump("signature", data) where |data| is of size 71:
//
//   signature: dumping 71 bytes
//   0000  30 45 02 21 00 ac c6 12 60 56 a2 e9 ee 16 be 14  0E.!....`V......
//   0010  69 7f c4 00 95 8c e8 55 1f 22 de 34 0b 08 8a 3b  i......U.".4...;
//   0020  a0 56 54 05 07 02 20 58 77 d9 8c f9 eb 41 df fd  .VT... Xw....A..
//   0030  c1 a3 14 e0 bf b0 a2 c5 0c b6 85 8c 4a 0d f9 2b  ............J..+
//   0040  b7 8f d2 1d 9b 11 ac                             .......
//
// This should only be used for debugging.
void hexdump(const string& name, const vector<uint8_t>& data);

string encodeHex(const string& str);

string encodeHex(const vector<uint8_t>& data);

string encodeHex(const uint8_t* data, size_t dataLen);

optional<vector<uint8_t>> decodeHex(const string& hexEncoded);

// ---------------------------------------------------------------------------
// CBOR utilities.
// ---------------------------------------------------------------------------

// Returns pretty-printed CBOR for |value|.
//
// Only valid CBOR should be passed to this function.
//
// If a byte-string is larger than |maxBStrSize| its contents will not be
// printed, instead the value of the form "<bstr size=1099016
// sha1=ef549cca331f73dfae2090e6a37c04c23f84b07b>" will be printed. Pass zero
// for |maxBStrSize| to disable this.
//
// The |mapKeysToNotPrint| parameter specifies the name of map values
// to not print. This is useful for unit tests.
string cborPrettyPrint(const vector<uint8_t>& encodedCbor, size_t maxBStrSize = 32,
                       const vector<string>& mapKeysToNotPrint = {});

// ---------------------------------------------------------------------------
// Crypto functionality / abstraction.
// ---------------------------------------------------------------------------

constexpr size_t kAesGcmIvSize = 12;
constexpr size_t kAesGcmTagSize = 16;
constexpr size_t kAes128GcmKeySize = 16;

// Returns |numBytes| bytes of random data.
optional<vector<uint8_t>> getRandom(size_t numBytes);

// Calculates the SHA-256 of |data|.
vector<uint8_t> sha256(const vector<uint8_t>& data);

// Decrypts |encryptedData| using |key| and |additionalAuthenticatedData|,
// returns resulting plaintext. The format of |encryptedData| must
// be as specified in the encryptAes128Gcm() function.
optional<vector<uint8_t>> decryptAes128Gcm(const vector<uint8_t>& key,
                                           const vector<uint8_t>& encryptedData,
                                           const vector<uint8_t>& additionalAuthenticatedData);

// Encrypts |data| with |key| and |additionalAuthenticatedData| using |nonce|,
// returns the resulting (nonce || ciphertext || tag).
optional<vector<uint8_t>> encryptAes128Gcm(const vector<uint8_t>& key, const vector<uint8_t>& nonce,
                                           const vector<uint8_t>& data,
                                           const vector<uint8_t>& additionalAuthenticatedData);

// ---------------------------------------------------------------------------
// EC crypto functionality / abstraction (only supports P-256).
// ---------------------------------------------------------------------------

// Creates an 256-bit EC key using the NID_X9_62_prime256v1 curve, returns the
// DER encoded private key.  Also generates an attestation using the |challenge|
// and |applicationId|, and returns the generated certificate chain.
//
// The notBeffore field will be the current time and the notAfter will be the same
// same time as the batch certificate.
//
// The first parameter of the return value is the keyPair generated, second return in
// the pair is the attestation certificate generated.
//
optional<std::pair<vector<uint8_t>, vector<vector<uint8_t>>>> createEcKeyPairAndAttestation(
        const vector<uint8_t>& challenge, const vector<uint8_t>& applicationId,
        bool isTestCredential);

// (TODO: remove when no longer used by 3rd party.)
optional<vector<vector<uint8_t>>> createAttestationForEcPublicKey(
        const vector<uint8_t>& publicKey, const vector<uint8_t>& challenge,
        const vector<uint8_t>& applicationId);

// Creates an 256-bit EC key using the NID_X9_62_prime256v1 curve, returns the
// private key in DER format (as specified in RFC 5915).
//
optional<vector<uint8_t>> createEcKeyPair();

// For an EC key |keyPair| encoded in DER format, extracts the public key in
// uncompressed point form.
//
optional<vector<uint8_t>> ecKeyPairGetPublicKey(const vector<uint8_t>& keyPair);

// For an EC key |keyPair| encoded in DER format, extracts the private key as
// an EC uncompressed key.
//
optional<vector<uint8_t>> ecKeyPairGetPrivateKey(const vector<uint8_t>& keyPair);

// Creates a DER encoded representation from a private key (which must be uncompressed,
// e.g. 32 bytes).
//
optional<vector<uint8_t>> ecPrivateKeyToKeyPair(const vector<uint8_t>& privateKey);

// For an EC key |keyPair| encoded in DER format, creates a PKCS#12 structure
// with the key-pair (not using a password to encrypt the data). The public key
// in the created structure is included as a certificate, using the given fields
// |serialDecimal|, |issuer|, |subject|, |validityNotBefore|, and
// |validityNotAfter|.
//
optional<vector<uint8_t>> ecKeyPairGetPkcs12(const vector<uint8_t>& keyPair, const string& name,
                                             const string& serialDecimal, const string& issuer,
                                             const string& subject, time_t validityNotBefore,
                                             time_t validityNotAfter);

// Signs |data| with |key| (which must be in the format returned by
// ecKeyPairGetPrivateKey()). Signature is returned and will be in DER format.
//
optional<vector<uint8_t>> signEcDsa(const vector<uint8_t>& key, const vector<uint8_t>& data);

// Like signEcDsa() but instead of taking the data to be signed, takes a digest
// of it instead.
//
optional<vector<uint8_t>> signEcDsaDigest(const vector<uint8_t>& key,
                                          const vector<uint8_t>& dataDigest);

// Calculates the HMAC with SHA-256 for |data| using |key|. The calculated HMAC
// is returned and will be 32 bytes.
//
optional<vector<uint8_t>> hmacSha256(const vector<uint8_t>& key, const vector<uint8_t>& data);

// Checks that |signature| (in DER format) is a valid signature of |digest|,
// made with |publicKey| (which must be in the format returned by
// ecKeyPairGetPublicKey()).
//
bool checkEcDsaSignature(const vector<uint8_t>& digest, const vector<uint8_t>& signature,
                         const vector<uint8_t>& publicKey);

// Extracts the public-key from the top-most certificate in |certificateChain|
// (which should be a concatenated chain of DER-encoded X.509 certificates).
//
// The returned public key will be in the same format as returned by
// ecKeyPairGetPublicKey().
//
optional<vector<uint8_t>> certificateChainGetTopMostKey(const vector<uint8_t>& certificateChain);

// Extracts the public-key from the top-most certificate in |certificateChain|
// (which should be a concatenated chain of DER-encoded X.509 certificates).
//
// Return offset and size of the public-key
//
optional<pair<size_t, size_t>> certificateFindPublicKey(const vector<uint8_t>& x509Certificate);

// Extracts the TbsCertificate from the top-most certificate in |certificateChain|
// (which should be a concatenated chain of DER-encoded X.509 certificates).
//
// Return offset and size of the TbsCertificate
//
optional<pair<size_t, size_t>> certificateTbsCertificate(const vector<uint8_t>& x509Certificate);

// Extracts the Signature from the top-most certificate in |certificateChain|
// (which should be a concatenated chain of DER-encoded X.509 certificates).
//
// Return offset and size of the Signature
//
optional<pair<size_t, size_t>> certificateFindSignature(const vector<uint8_t>& x509Certificate);

// Extracts notBefore and notAfter from the top-most certificate in |certificateChain
// (which should be a concatenated chain of DER-encoded X.509 certificates).
//
// Returns notBefore and notAfter in that order.
//
optional<pair<time_t, time_t>> certificateGetValidity(const vector<uint8_t>& x509Certificate);

// Generates a X.509 certificate for |publicKey| (which must be in the format
// returned by ecKeyPairGetPublicKey()).
//
// The certificate is signed by |signingKey| (which must be in the format
// returned by ecKeyPairGetPrivateKey())
//
optional<vector<uint8_t>> ecPublicKeyGenerateCertificate(
        const vector<uint8_t>& publicKey, const vector<uint8_t>& signingKey,
        const string& serialDecimal, const string& issuer, const string& subject,
        time_t validityNotBefore, time_t validityNotAfter);

// Performs Elliptic-curve Diffie-Helman using |publicKey| (which must be in the
// format returned by ecKeyPairGetPublicKey()) and |privateKey| (which must be
// in the format returned by ecKeyPairGetPrivateKey()).
//
// On success, the computed shared secret is returned.
//
optional<vector<uint8_t>> ecdh(const vector<uint8_t>& publicKey, const vector<uint8_t>& privateKey);

// Key derivation function using SHA-256, conforming to RFC 5869.
//
// On success, the derived key is returned.
//
optional<vector<uint8_t>> hkdf(const vector<uint8_t>& sharedSecret, const vector<uint8_t>& salt,
                               const vector<uint8_t>& info, size_t size);

// Returns the X and Y coordinates from |publicKey| (which must be in the format
// returned by ecKeyPairGetPublicKey()).
//
// Success is indicated by the first value in the returned tuple. If successful,
// the returned coordinates will be in uncompressed form.
//
tuple<bool, vector<uint8_t>, vector<uint8_t>> ecPublicKeyGetXandY(const vector<uint8_t>& publicKey);

// Concatenates all certificates into |certificateChain| together into a
// single bytestring.
//
// This is the reverse operation of certificateChainSplit().
vector<uint8_t> certificateChainJoin(const vector<vector<uint8_t>>& certificateChain);

// Splits all the certificates in a single bytestring into individual
// certificates.
//
// Returns nothing if |certificateChain| contains invalid data.
//
// This is the reverse operation of certificateChainJoin().
optional<vector<vector<uint8_t>>> certificateChainSplit(const vector<uint8_t>& certificateChain);

// Validates that the certificate chain is valid. In particular, checks that each
// certificate in the chain is signed by the public key in the following certificate.
//
// Returns false if |certificateChain| failed validation or if each certificate
// is not signed by its successor.
//
bool certificateChainValidate(const vector<uint8_t>& certificateChain);

// Returns true if |certificate| is signed by |publicKey|.
//
bool certificateSignedByPublicKey(const vector<uint8_t>& certificate,
                                  const vector<uint8_t>& publicKey);

// Signs |data| and |detachedContent| with |key| (which must be in the format
// returned by ecKeyPairGetPrivateKey()).
//
// On success, the Signature is returned and will be in COSE_Sign1 format.
//
// If |certificateChain| is non-empty it's included in the 'x5chain'
// protected header element (as as described in'draft-ietf-cose-x509-04').
//
optional<vector<uint8_t>> coseSignEcDsa(const vector<uint8_t>& key, const vector<uint8_t>& data,
                                        const vector<uint8_t>& detachedContent,
                                        const vector<uint8_t>& certificateChain);

// Creates a COSE_Signature1 where |signatureToBeSigned| is the ECDSA signature
// of the ToBeSigned CBOR from RFC 8051 "4.4. Signing and Verification Process".
//
// The |signatureToBeSigned| is expected to be 64 bytes and contain the R value,
// then the S value.
//
// The |data| parameter will be included in the COSE_Sign1 CBOR.
//
// If |certificateChain| is non-empty it's included in the 'x5chain'
// protected header element (as as described in'draft-ietf-cose-x509-04').
//
optional<vector<uint8_t>> coseSignEcDsaWithSignature(const vector<uint8_t>& signatureToBeSigned,
                                                     const vector<uint8_t>& data,
                                                     const vector<uint8_t>& certificateChain);

// Checks that |signatureCoseSign1| (in COSE_Sign1 format) is a valid signature
// made with |public_key| (which must be in the format returned by
// ecKeyPairGetPublicKey()) where |detachedContent| is the detached content.
//
bool coseCheckEcDsaSignature(const vector<uint8_t>& signatureCoseSign1,
                             const vector<uint8_t>& detachedContent,
                             const vector<uint8_t>& publicKey);

// Converts a DER-encoded signature to the format used in 'signature' bstr in COSE_Sign1.
bool ecdsaSignatureDerToCose(const vector<uint8_t>& ecdsaDerSignature,
                             vector<uint8_t>& ecdsaCoseSignature);

// Converts from the format in in 'signature' bstr in COSE_Sign1 to DER encoding.
bool ecdsaSignatureCoseToDer(const vector<uint8_t>& ecdsaCoseSignature,
                             vector<uint8_t>& ecdsaDerSignature);

// Extracts the payload from a COSE_Sign1.
optional<vector<uint8_t>> coseSignGetPayload(const vector<uint8_t>& signatureCoseSign1);

// Extracts the signature (of the ToBeSigned CBOR) from a COSE_Sign1.
optional<vector<uint8_t>> coseSignGetSignature(const vector<uint8_t>& signatureCoseSign1);

// Extracts the signature algorithm from a COSE_Sign1.
optional<int> coseSignGetAlg(const vector<uint8_t>& signatureCoseSign1);

// Extracts the X.509 certificate chain, if present. Returns the data as a
// concatenated chain of DER-encoded X.509 certificates
//
// Returns nothing if there is no 'x5chain' element or an error occurs.
//
optional<vector<uint8_t>> coseSignGetX5Chain(const vector<uint8_t>& signatureCoseSign1);

// MACs |data| and |detachedContent| with |key| (which can be any sequence of
// bytes).
//
// If successful, the MAC is returned and will be in COSE_Mac0 format.
//
optional<vector<uint8_t>> coseMac0(const vector<uint8_t>& key, const vector<uint8_t>& data,
                                   const vector<uint8_t>& detachedContent);

// Creates a COSE_Mac0 where |digestToBeMaced| is the HMAC-SHA256
// of the ToBeMaced CBOR from RFC 8051 "6.3. How to Compute and Verify a MAC".
//
// The |digestToBeMaced| is expected to be 32 bytes.
//
// The |data| parameter will be included in the COSE_Mac0 CBOR.
//
optional<vector<uint8_t>> coseMacWithDigest(const vector<uint8_t>& digestToBeMaced,
                                            const vector<uint8_t>& data);

// ---------------------------------------------------------------------------
// Utility functions specific to IdentityCredential.
// ---------------------------------------------------------------------------

optional<vector<uint8_t>> calcMac(const vector<uint8_t>& sessionTranscriptEncoded,
                                  const string& docType,
                                  const vector<uint8_t>& deviceNameSpacesEncoded,
                                  const vector<uint8_t>& eMacKey);

optional<vector<uint8_t>> calcEMacKey(const vector<uint8_t>& privateKey,
                                      const vector<uint8_t>& publicKey,
                                      const vector<uint8_t>& sessionTranscriptBytes);

// Returns the testing AES-128 key where all bits are set to 0.
const vector<uint8_t>& getTestHardwareBoundKey();

// Splits the given bytestring into chunks. If the given vector is smaller or equal to
// |maxChunkSize| a vector with |content| as the only element is returned. Otherwise
// |content| is split into N vectors each of size |maxChunkSize| except the final element
// may be smaller than |maxChunkSize|.
vector<vector<uint8_t>> chunkVector(const vector<uint8_t>& content, size_t maxChunkSize);

}  // namespace support
}  // namespace identity
}  // namespace hardware
}  // namespace android

#endif  // IDENTITY_SUPPORT_INCLUDE_IDENTITY_CREDENTIAL_UTILS_H_
