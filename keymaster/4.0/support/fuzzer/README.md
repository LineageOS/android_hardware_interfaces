# Fuzzers for libkeymaster4support

## Plugin Design Considerations
The fuzzer plugins for libkeymaster4support are designed based on the understanding of the
source code and try to achieve the following:

##### Maximize code coverage
The configuration parameters are not hardcoded, but instead selected based on
incoming data. This ensures more code paths are reached by the fuzzers.

libkeymaster4support supports the following parameters:
1. Security Level (parameter name: `securityLevel`)
2. Ec Curve (parameter name: `ecCurve`)
3. Padding Mode (parameter name: `paddingMode`)
4. Digest (parameter name: `digest`)
5. Tag (parameter name: `tag`)

| Parameter| Valid Values| Configured Value|
|------------- |-------------| ----- |
| `securityLevel` | 0.`SecurityLevel::SOFTWARE` 1.`SecurityLevel::TRUSTED_ENVIRONMENT` 2.`SecurityLevel::STRONGBOX`| Value obtained from FuzzedDataProvider|
| `ecCurve` | 0.`EcCurve::P_224` 1.`EcCurve::P_256` 2.`EcCurve::P_384` 3. `EcCurve::P_521`| Value obtained from FuzzedDataProvider|
| `paddingMode` | 0.`PaddingMode::NONE` 1.`PaddingMode::RSA_OAEP` 2.`PaddingMode::RSA_PSS` 3. `PaddingMode::RSA_PKCS1_1_5_ENCRYPT` 4.`PaddingMode::RSA_PKCS1_1_5_SIGN` 5.`PaddingMode::PKCS7`| Value obtained from FuzzedDataProvider|
| `digest` | 1. `Digest::NONE` 2.`Digest::MD5` 3.`Digest::SHA1` 4.`Digest::SHA_2_224` 5.`Digest::SHA_2_256` 6.`Digest::SHA_2_384`  7.`Digest::SHA_2_512`| Value obtained from FuzzedDataProvider|
| `tag` | 1. `Tag::INVALID` 2.`Tag::PURPOSE` 3.`Tag::ALGORITHM` 4.`Tag::KEY_SIZE` 5.`Tag::BLOCK_MODE` 6.`Tag::DIGEST` 7.`Tag::PADDING` 8.`Tag::CALLER_NONCE` 9.`Tag::MIN_MAC_LENGTH` 10.`Tag::EC_CURVE` 11.`Tag::RSA_PUBLIC_EXPONENT` 12.`Tag::INCLUDE_UNIQUE_ID` 13. `Tag::BLOB_USAGE_REQUIREMENTS` 14.`Tag::BOOTLOADER_ONLY` 15.`Tag::ROLLBACK_RESISTANCE` 16.`Tag::HARDWARE_TYPE` 17.`Tag::ACTIVE_DATETIME` 18. `Tag::ORIGINATION_EXPIRE_DATETIME` 19.`Tag::USAGE_EXPIRE_DATETIME` 20.`Tag::MIN_SECONDS_BETWEEN_OPS` 21.`Tag::MAX_USES_PER_BOOT` 22.`Tag::USER_ID` 23.` Tag::USER_SECURE_ID` 24.`Tag::NO_AUTH_REQUIRED` 25.`Tag::USER_AUTH_TYPE` 26.`Tag::AUTH_TIMEOUT` 27.`Tag::ALLOW_WHILE_ON_BODY` 28.`Tag::TRUSTED_USER_PRESENCE_REQUIRED` 29.`Tag::TRUSTED_CONFIRMATION_REQUIRED` 30.`Tag::UNLOCKED_DEVICE_REQUIRED` 31.`Tag::APPLICATION_ID` 32.`Tag::APPLICATION_DATA` 33.`Tag::CREATION_DATETIME` 34.`Tag::ORIGIN` 35.`Tag::ROOT_OF_TRUST` 36.`Tag::OS_VERSION` 37.`Tag::OS_PATCHLEVEL` 38.`Tag::UNIQUE_ID` 39.`Tag::ATTESTATION_CHALLENGE` 40.`Tag::ATTESTATION_APPLICATION_ID` 41.`Tag::ATTESTATION_ID_BRAND` 42.`Tag::ATTESTATION_ID_DEVICE` 43.`Tag::ATTESTATION_ID_PRODUCT` 44.`Tag::ATTESTATION_ID_SERIAL` 45.`Tag::ATTESTATION_ID_IMEI` 46.`Tag::ATTESTATION_ID_MEID` 47.`Tag::ATTESTATION_ID_MANUFACTURER` 48.`Tag::ATTESTATION_ID_MODEL` 49.`Tag::VENDOR_PATCHLEVEL` 50.`Tag::BOOT_PATCHLEVEL` 51.`Tag::ASSOCIATED_DATA` 52.`Tag::NONCE` 53.`Tag::MAC_LENGTH` 54.`Tag::RESET_SINCE_ID_ROTATION` 55.`Tag::CONFIRMATION_TOKEN`| Value obtained from FuzzedDataProvider|

This also ensures that the plugins are always deterministic for any given input.

##### Maximize utilization of input data
The plugins feed the entire input data to the module.
This ensures that the plugins tolerate any kind of input (empty, huge,
malformed, etc) and dont `exit()` on any input and thereby increasing the
chance of identifying vulnerabilities.

## Build

This describes steps to build keymaster4_attestation_fuzzer, keymaster4_authSet_fuzzer and keymaster4_utils_fuzzer binaries

### Android

#### Steps to build
Build the fuzzer
```
  $ mm -j$(nproc) keymaster4_attestation_fuzzer
  $ mm -j$(nproc) keymaster4_authSet_fuzzer
  $ mm -j$(nproc) keymaster4_utils_fuzzer
```
#### Steps to run
To run on device
```
  $ adb sync data
  $ adb shell /data/fuzz/${TARGET_ARCH}/keymaster4_attestation_fuzzer/keymaster4_attestation_fuzzer
  $ adb shell /data/fuzz/${TARGET_ARCH}/keymaster4_authSet_fuzzer/keymaster4_authSet_fuzzer
  $ adb shell /data/fuzz/${TARGET_ARCH}/keymaster4_utils_fuzzer/keymaster4_utils_fuzzer
```

## References:
 * http://llvm.org/docs/LibFuzzer.html
 * https://github.com/google/oss-fuzz
