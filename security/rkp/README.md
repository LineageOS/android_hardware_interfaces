# Remote Provisioning HAL

## Objective

Design a HAL to support over-the-air provisioning of certificates for asymmetric
keys. The HAL must interact effectively with Keystore (and other daemons) and
protect device privacy and security.

Note that this API was originally designed for KeyMint, with the intention that
it should be usable for other HALs that require certificate provisioning.
Throughout this document we'll refer to the Keystore and KeyMint (formerly
called Keymaster) components, but only for concreteness and convenience; those
labels could be replaced with the names of any system and secure area
components, respectively, that need certificates provisioned.

## Key design decisions

### General approach

To more securely and reliably get keys and certificates to Android devices, we
need to create a system where no party outside of the device's secure components
is responsible for managing private keys. The strategy we've chosen is to
deliver certificates over the air, using an asymmetric key pair created
on-device in the factory as a root of trust to create an authenticated, secure
channel. In this document we refer to this device-unique asymmetric key pair as
Device Key (DK), its public half DK\_pub, its private half DK\_priv and a Device
Key Certificate containing DK\_pub is denoted DKC.

In order for the provisioning service to use DK (or a key authenticated by DK),
it must know whether a given DK\_pub is known and trusted. To prove trust, we
ask device OEMs to use one of two mechanisms:

1.  (Preferred, recommended) The device OEM extracts DK\_pub from each device it
    manufactures and uploads the public keys to a backend server.

1.  The device OEM signs the DK\_pub to produce DKC and stores it on the device.
    This has the advantage that they don't need to upload a DK\_pub for every
    device immediately, but the disadvantage that they have to manage their
    private signing keys, which means they have to have HSMs, configure and
    secure them correctly, etc. Some backend providers may also require that the
    OEM passes a factory security audit, and additionally promises to upload the
    keys eventually as well.

Note that in the full elaboration of this plan, DK\_pub is not the key used to
establish a secure channel. Instead, DK\_pub is just the first public key in a
chain of public keys which ends with the KeyMint public key, KM\_pub. All keys
in the chain are device-unique and are joined in a certificate chain called the
_Boot Certificate Chain_ (BCC), because in phases 2 and 3 of the remote
provisioning project it is a chain of certificates corresponding to boot phases.
We speak of the BCC even for phase 1, though in phase 1 it contains only a
single self-signed DKC. This is described in more depth in the Phases section
below.

The BCC is authenticated by DK\_pub. To authenticate DK\_pub, we may have
additional DKCs, from the SoC vendor, the device OEM, or both. Those are not
part of the BCC but included as optional fields in the certificate request
structure.

The format of the the DK and BCC is specified within [Open Profile for DICE]
(https://pigweed.googlesource.com/open-dice/+/HEAD/docs/specification.md).  To
map phrases within this document to their equivalent terminology in the DICE
specification, read the terms as follows: the DK corresponds to the UDS-derived
key pair, DKC corresponds to the UDS certificate, and the BCC entries between
DK\_pub and KM\_pub correspond to a chain of CDI certificates.

Note: In addition to allowing 32 byte hash values for fields in the BCC payload,
this spec additionally constrains some of the choices allowed in open-DICE.
Specifically, these include which entries are required and which are optional in
the BCC payload, and which algorithms are acceptable for use.

### Phases

RKP will be deployed in three phases, in terms of managing the root of trust
binding between the device and the backend. To briefly describe them:

* Phase 1: In phase 1 there is only one entry in the BCC; DK_pub and KM_pub are
  the same key and the certificate is self-signed.
* Phase 2: This is identical to phase 1, except it leverages the hardware root
  of trust process described by DICE. Instead of trust being rooted in the TEE,
  it is now rooted in the ROM by key material blown into fuses which are only
  accessible to the ROM code.
* Phase 3: This is identical to Phase 2, except the SoC vendor also does the
  public key extraction or certification in their facilities, along with the OEM
  doing it in the factory. This tightens up the "supply chain" and aims to make
  key upload management more secure.

### Privacy considerations

Because DK and the DKCs are unique, immutable, unspoofable hardware-bound
identifiers for the device, we must limit access to them to the absolute minimum
possible. We do this in two ways:

1.  We require KeyMint (which knows the BCC and either knows or at least has the
ability to use KM\_priv) to refuse to ever divulge the BCC or additional
signatures in plaintext. Instead, KeyMint requires the caller to provide an
_Endpoint Encryption Key_ (EEK), with which it will encrypt the data before
returning it. When provisioning production keys, the EEK must be signed by an
approved authority whose public key is embedded in KeyMint. When certifying test
keys, KeyMint will accept any EEK without checking the signature, but will
encrypt and return a test BCC, rather than the real one.  The result is that
only an entity in possession of an Trusted EEK (TEEK) private key can discover
the plaintext of the production BCC.
1.  Having thus limited access to the public keys to the trusted party only, we
need to prevent the entity from abusing this unique device identifier.  The
approach and mechanisms for doing that are beyond the scope of this document
(they must be addressed in the server design), but generally involve taking care
to ensure that we do not create any links between user IDs, IP addresses or
issued certificates and the device pubkey.

Although the details of the mechanisms for preventing the entity from abusing
the BCC are, as stated, beyond the scope of this document, there is a subtle
design decision here made specifically to enable abuse prevention. Specifically
the `CertificateRequest` message sent to the server is (in
[CDDL](https://tools.ietf.org/html/rfc8610)):

```
cddl
CertificateRequest = [
    DeviceInfo,
    challenge : bstr,
    ProtectedData,
    MacedKeysToSign
]
```

The public keys to be attested by the server are in `MacedKeysToSign`, which is
a COSE\_Mac0 structure, MACed with a key that is found in `ProtectedData`. The
MAC key is signed by DK\_pub.

This structure allows the backend component that has access to EEK\_priv to
decrypt `ProtectedData`, validate that the request is from an authorized device,
check that the request is fresh and verify and extract the MAC key. That backend
component never sees any data related to the keys to be signed, but can provide
the MAC key to another backend component that can verify `MacedKeysToSign` and
proceed to generate the certificates.

In this way, we can partition the provisioning server into one component that
knows the device identity, as represented by DK\_pub, but never sees the keys to
be certified or certificates generated, and another component that sees the keys
to be certified and certificates generated but does not know the device
identity.

### Key and cryptographic message formatting

For simplicity of generation and parsing, compactness of wire representation,
and flexibility and standardization, we've settled on using the CBOR Object
Signing and Encryption (COSE) standard, defined in [RFC
8152](https://tools.ietf.org/html/rfc8152). COSE provides compact and reasonably
simple, yet easily-extensible, wire formats for:

*   Keys,
*   MACed messages,
*   Signed messages, and
*   Encrypted messages

COSE enables easy layering of these message formats, such as using a COSE\_Sign
structure to contain a COSE\_Key with a public key in it. We call this a
"certificate".

Due to the complexity of the standard, we'll spell out the COSE structures
completely in this document and in the HAL and other documentation, so that
although implementors will need to understand CBOR and the CBOR Data Definition
Language ([CDDL, defined in RFC 8610](https://tools.ietf.org/html/rfc8610)),
they shouldn't need to understand COSE.

Note, however, that the certificate chains returned from the provisioning server
are standard X.509 certificates.

### Algorithm choices

This document uses:

*   ECDSA P-256 for attestation signing keys;
*   Remote provisioning protocol signing keys:
  *  Ed25519 / P-256 / P-384
*   ECDH keys:
  *  X25519 / P-256
*   AES-GCM for all encryption;
*   SHA-256 / SHA-384 / SHA-512 for message digesting;
*   HMAC with a supported message digest for all MACing; and
*   HKDF with a supported message digest for all key derivation.

We believe that Curve25519 offers the best tradeoff in terms of security,
efficiency and global trustworthiness, and that it is now sufficiently
widely-used and widely-implemented to make it a practical choice.

However, since hardware such as Secure Elements (SE) do not currently offer
support for curve 25519, we are allowing implementations to instead make use of
ECDSA and ECDH.

The CDDL in the rest of the document will use the '/' operator to show areas
where either curve 25519, P-256 or P-384 may be used. Since there is no easy way
to bind choices across different CDDL groups, it is important that the
implementor stays consistent in which type is chosen. E.g. taking ES256 as the
choice for algorithm implies the implementor should also choose the P256 public
key group further down in the COSE structure.

### Testability

It's critical that the remote provisioning implementation be testable, to
minimize the probability that broken devices are sold to end users. To support
testing, the remote provisioning HAL methods take a `testMode` argument. Keys
created in test mode are tagged to indicate this. The provisioning server will
check for the test mode tag and issue test certificates that do not chain back
to a trusted public key. In test mode, any EEK will be accepted, enabling
testing tools to use EEKs for which they have the private key so they can
validate the content of certificate requests. The BCC included in the
`CertificateRequest` must contain freshly-generated keys, not the real BCC keys.

Keystore (or similar) will need to be able to handle both testMode keys and
production keys and keep them distinct, generating test certificate requests
when asked with a test EEK and production certificate requests when asked with a
production EEK. Likewise, the interface used to instruct Keystore to create keys
will need to be able to specify whether test or production keys are desired.

## Design

### Certificate provisioning flow

TODO(jbires): Replace this with a `.png` containing a sequence diagram.  The
provisioning flow looks something like this:

Provisioner -> Keystore: Prepare N keys
Keystore -> KeyMint: generateKeyPair
KeyMint -> KeyMint: Generate  key pair
KeyMint --> Keystore: key\_blob,pubkey
Keystore -> Keystore: Store key\_blob,pubkey
Provisioner -> Server: Get TEEK
Server --> Provisioner: TEEK
Provisioner -> Keystore: genCertReq(N, TEEK)
Keystore -> KeyMint: genCertReq(pubkeys, TEEK)
KeyMint -> KeyMint: Sign pubkeys & encrypt BCC
KeyMint --> Keystore: signature, encrypted BCC
Keystore -> Keystore: Construct cert\_request
Keystore --> Provisioner: cert\_request
Provisioner --> Server: cert\_request
Server -> Server: Validate cert\_request
Server -> Server: Generate certificates
Server --> Provisioner: certificates
Provisioner -> Keystore: certificates
Keystore -> Keystore: Store certificates

The actors in the above diagram are:

*   **Server** is the backend certificate provisioning server. It has access to
    the uploaded device public keys and is responsible for providing encryption
    keys, decrypting and validating requests, and generating certificates in
    response to requests.
*   **Provisioner** is an application that is responsible for communicating with
    the server and all of the system components that require key certificates
    from the server. It also implements the policy that defines how many key
    pairs each client should keep in their pool.
*   **Keystore** is the [Android keystore
    daemon](https://developer.android.com/training/articles/keystore) (or, more
    generally, whatever system component manages communications with a
    particular secure aread component).
*   **KeyMint** is the secure area component that manages cryptographic keys and
    performs attestations (or perhaps some other secure area component).

### `BCC`

The _Boot Certificate Chain_ (BCC) is the chain of certificates that contains
DK\_pub as well as other often device-unique certificates. The BCC is
represented as a COSE\_Key containing DK\_pub followed by an array of
COSE\_Sign1 "certificates" containing public keys and optional additional
information, ordered from root to leaf, with each certificate signing the next.
The first certificate in the array is signed by DK\_pub, the last certificate
has the KeyMint (or whatever) signing key's public key, KM\_pub. In phase 1
there is only one entry; DK\_pub and KM\_pub are the same key and the
certificate is self-signed.

Each COSE\_Sign1 certificate is a CBOR Web Token (CWT) as described in [RFC
8392](https://tools.ietf.org/html/rfc8392) with additional fields as described
in the Open Profile for DICE. Of these additional fields, only the
_subjectPublicKey_ and _keyUsage_ fields are expected to be present for the
KM\_pub entry (that is, the last entry) in a BCC, but all fields required by the
Open Profile for DICE are expected for other entries (each of which corresponds
to a particular firmware component or boot stage). The CWT fields _iss_ and
_sub_ identify the issuer and subject of the certificate and are consistent
along the BCC entries; the issuer of a given entry matches the subject of the
previous entry.

The BCC is designed to be constructed using the Open Profile for DICE. In this
case the DK key pair is derived from the UDS as described by that profile and
all BCC entries before the leaf are CBOR CDI certificates chained from DK\_pub.
The KM key pair is not part of the derived DICE chain. It is generated (not
derived) by the KeyMint module, certified by the last key in the DICE chain, and
added as the leaf BCC entry. The key usage field in this leaf certificate must
indicate the key is not used to sign certificates. If a UDS certificate is
available on the device it should appear in the certificate request as the leaf
of a DKCertChain in AdditionalDKSignatures (see
[CertificateRequest](#certificaterequest)).

#### Mode

The Open Profile for DICE specifies four possible modes with the most important
mode being `normal`. A certificate must only set the mode to `normal` when all
of the following conditions are met when loading and verifying the software
component that is being described by the certificate:

*   verified boot with anti-rollback protection is enabled
*   only the verified boot authorities for production images are enabled
*   debug ports, fuses or other debug facilities are disabled
*   device booted software from the normal primary source e.g. internal flash

If any of these conditions are not met then it is recommended to explicitly
acknowledge this fact by using the `debug` mode. The mode should never be `not
configured`.

#### Configuration descriptor

The Open Profile for DICE allows for an arbitrary configuration descriptor. For
BCC entries, this configuration descriptor is a CBOR map with the following
optional fields. If no fields are relevant, an empty map should be encoded.
Additional implementation-specific fields may be added using key values not in
the range \[-70000, -70999\] (these are reserved for future additions here).

```
| Name              | Key    | Value type | Meaning                           |
| ----------------- | ------ | ---------- | ----------------------------------|
| Component name    | -70002 | tstr       | Name of firmware component / boot |
:                   :        :            : stage                             :
| Component version | -70003 | int / tstr | Version of firmware component /   |
:                   :        :            : boot stage                        :
| Resettable        | -70004 | null       | If present, key changes on factory|
:                   :        :            : reset                             :
| Security version  | -70005 | uint       | Machine-comparable, monotonically |
:                   :        :            : increasing version of the firmware:
:                   :        :            : component / boot stage where a    :
:                   :        :            : greater value indicates a newer   :
:                   :        :            : version                           :
```

Please see
[ProtectedData.aidl](https://cs.android.com/android/platform/superproject/+/master:hardware/interfaces/security/rkp/aidl/android/hardware/security/keymint/ProtectedData.aidl)
for a full CDDL definition of the BCC.

### `CertificateRequest`

The full CBOR message that will be sent to the server to request certificates
is:

```cddl
CertificateRequest = [
    DeviceInfo,
    challenge : bstr,       // Provided by the server
    ProtectedData,          // See ProtectedData.aidl
    MacedKeysToSign         // See IRemotelyProvisionedComponent.aidl
]

DeviceInfo = [
    VerifiedDeviceInfo,     // See DeviceInfo.aidl
    UnverifiedDeviceInfo
]

// Unverified info is anything provided by the HLOS. Subject to change out of
// step with the HAL.
UnverifiedDeviceInfo = {
    ? "fingerprint" : tstr,
}

```

It will be the responsibility of Keystore and the Provisioner to construct the
`CertificateRequest`. The HAL provides a method to generate the elements that
need to be constructed on the secure side, which are the tag field of
`MacedKeysToSign`, `VerifiedDeviceInfo`, and the ciphertext field of
`ProtectedData`.

### HAL

The remote provisioning HAL provides a simple interface that can be implemented
by multiple secure components that require remote provisioning. It would be
slightly simpler to extend the KeyMint API, but that approach would only serve
the needs of KeyMint, this is more general.

NOTE the data structures defined in this HAL may look a little bloated and
complex. This is because the COSE data structures are fully spelled-out; we
could make it much more compact by not re-specifying the standardized elements
and instead just referencing the standard, but it seems better to fully specify
them. If the apparent complexity seems daunting, consider what the same would
look like if traditional ASN.1 DER-based structures from X.509 and related
standards were used and also fully elaborated.

Please see the related HAL documentation directly in the source code at the
following links:

*   [IRemotelyProvisionedComponent
    HAL](https://cs.android.com/android/platform/superproject/+/master:hardware/interfaces/security/rkp/aidl/android/hardware/security/keymint/IRemotelyProvisionedComponent.aidl)
*   [ProtectedData](https://cs.android.com/android/platform/superproject/+/master:hardware/interfaces/security/rkp/aidl/android/hardware/security/keymint/ProtectedData.aidl)
*   [MacedPublicKey](https://cs.android.com/android/platform/superproject/+/master:hardware/interfaces/security/rkp/aidl/android/hardware/security/keymint/MacedPublicKey.aidl)
*   [RpcHardwareInfo](https://cs.android.com/android/platform/superproject/+/master:hardware/interfaces/security/rkp/aidl/android/hardware/security/keymint/RpcHardwareInfo.aidl)
*   [DeviceInfo](https://cs.android.com/android/platform/superproject/+/master:hardware/interfaces/security/rkp/aidl/android/hardware/security/keymint/DeviceInfo.aidl)

