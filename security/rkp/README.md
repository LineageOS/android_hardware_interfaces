# Remote Provisioning HAL

## Objective

Design a HAL to support over-the-air provisioning of certificates for asymmetric
keys. The HAL must interact effectively with Keystore (and other services) and
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
deliver certificates over the air, using an asymmetric key pair derived from a
unique device secret (UDS) as a root of trust for authenticated requests from
the secure components. We refer to the public half of this asymmetric key pair
as UDS\_pub.

In order for the provisioning service to trust UDS\_pub we ask device OEMs to
use one of two mechanisms:

1.  (Preferred, recommended) The device OEM extracts the UDS\_pub from each
    device they manufacture and uploads the public keys to a backend server.

1.  The device OEM signs the UDS\_pub and stores the certificates on the device
    rather than uploading a UDS\_pub for every device immediately. However,
    there are many disadvantages and costs associated with this option as the
    OEM will need to pass a security audit of their factory's physical security,
    CA and HSM configuration, and incident response processes before the OEM's
    public key is registered with the provisioning server.

Note that in the full elaboration of this plan, UDS\_pub is not the key used to
sign certificate requests. Instead, UDS\_pub is just the first public key in a
chain of public keys that end the KeyMint public key. All keys in the chain are
transitively derived from the UDS and joined in a certificate chain following
the specification of the [Android Profile for DICE](android-profile-for-dice).

[android-profile-for-dice]: https://pigweed.googlesource.com/open-dice/+/refs/heads/main/docs/android.md

### Phases

RKP will be deployed with phased management of the root of trust
binding between the device and the backend. To briefly describe them:

* Degenerate DICE (Phase 1): A TEE root of trust key pair is used to sign
  certificate requests; a single self-signed certificate signifies this phase.
* DICE (Phase 2): A hardware root of trust key pair is only accessible to ROM
  or ROM extension code; the boot process follows the [Android Profile for
  DICE](android-profile-for-dice).
* SoC vendor certified DICE (Phase 3): This is identical to Phase 2, except the
  SoC vendor also does the UDS\_pub extraction or certification in their
  facilities, along with the OEM doing it in the factory. This tightens up the
  "supply chain" and aims to make key upload management more secure.

### Privacy considerations

Because the UDS, CDIs and derived values are unique, immutable, unspoofable
hardware-bound identifiers for the device, we must limit access to them. We
require that the values are never exposed in public APIs and are only available
to the minimum set of system components that require access to them to function
correctly.

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

## Design

### Certificate provisioning flow

TODO(jbires): Replace this with a `.png` containing a sequence diagram.  The
provisioning flow looks something like this:

rkpd -> KeyMint: generateKeyPair
KeyMint -> KeyMint: Generate key pair
KeyMint --> rkpd: key\_blob,pubkey
rkpd -> rkpd: Store key\_blob,pubkey
rkpd -> Server: Get challenge
Server --> rkpd: challenge
rkpd -> KeyMint: genCertReq(pubkeys, challenge)
KeyMint -> KeyMint: Sign CSR
KeyMint --> rkpd: signed CSR
rkpd --> Server: CSR
Server -> Server: Validate CSR
Server -> Server: Generate certificates
Server --> rkpd: certificates
rkpd -> rkpd: Store certificates

The actors in the above diagram are:

*   **Server** is the backend certificate provisioning server. It has access to
    the uploaded device public keys and is responsible for providing encryption
    keys, decrypting and validating requests, and generating certificates in
    response to requests.
*   **rkpd** is, optionally, a modular system component that is responsible for
    communicating with the server and all of the system components that require
    key certificates from the server. It also implements the policy that defines
    how many key pairs each client should keep in their pool. When a system
    ships with rkpd as a modular component, it may be updated independently from
    the rest of the system.
*   **Keystore** is the [Android keystore
    daemon](https://developer.android.com/training/articles/keystore) (or, more
    generally, whatever system component manages communications with a
    particular secure aread component).
*   **KeyMint** is the secure area component that manages cryptographic keys and
    performs attestations (or perhaps some other secure area component).

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

### Support for Android Virtualization Framework

The Android Virtualization Framwork (AVF) relies on RKP to provision keys for VMs. A
privileged vm, the RKP VM, is reponsible for generating and managing the keys for client
VMs that run virtualized workloads. See the following for more background information on the
RKP VM:
*    [rkp-vm](https://android.googlesource.com/platform/packages/modules/Virtualization/+/main/service_vm/README.md#rkp-vm-remote-key-provisioning-virtual-machine)
*    [rkp-service](https://source.android.com/docs/core/ota/modular-system/remote-key-provisioning#stack-architecture)

It is important to distinquish the RKP VM from other components, such as KeyMint. An
[RKP VM marker](https://pigweed.googlesource.com/open-dice/+/HEAD/docs/android.md#configuration-descriptor)
(key `-70006) is used for this purpose. The existence or absence of this marker is used to
identify the type of component decribed by a given DICE chain.

The following describes which certificate types may be request based on the RKP VM marker:
1. "rkp-vm": If a DICE chain has zero or more certificates without the RKP VM
   marker followed by one or more certificates with the marker, then that chain
   describes an RKP VM. If there are further certificates without the RKP VM
   marker, then the chain does not describe an RKP VM.

   Implementations must include the first RPK VM marker as early as possible
   after the point of divergence between TEE and non-TEE components in the DICE
   chain, prior to loading the Android Bootloader (ABL).
2. "widevine" or "keymint": If there are no certificates with the RKP VM
   marker then it describes a TEE component.
3. None: Any component described by a DICE chain that does not match the above
   two categories.