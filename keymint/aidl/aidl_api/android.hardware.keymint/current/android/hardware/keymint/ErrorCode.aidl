///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL interface (or parcelable). Do not try to
// edit this file. It looks like you are doing that because you have modified
// an AIDL interface in a backward-incompatible way, e.g., deleting a function
// from an interface or a field from a parcelable and it broke the build. That
// breakage is intended.
//
// You must not make a backward incompatible changes to the AIDL files built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package android.hardware.keymint;
@Backing(type="int") @VintfStability
enum ErrorCode {
  OK = 0,
  ROOT_OF_TRUST_ALREADY_SET = -1,
  UNSUPPORTED_PURPOSE = -2,
  INCOMPATIBLE_PURPOSE = -3,
  UNSUPPORTED_ALGORITHM = -4,
  INCOMPATIBLE_ALGORITHM = -5,
  UNSUPPORTED_KEY_SIZE = -6,
  UNSUPPORTED_BLOCK_MODE = -7,
  INCOMPATIBLE_BLOCK_MODE = -8,
  UNSUPPORTED_MAC_LENGTH = -9,
  UNSUPPORTED_PADDING_MODE = -10,
  INCOMPATIBLE_PADDING_MODE = -11,
  UNSUPPORTED_DIGEST = -12,
  INCOMPATIBLE_DIGEST = -13,
  INVALID_EXPIRATION_TIME = -14,
  INVALID_USER_ID = -15,
  INVALID_AUTHORIZATION_TIMEOUT = -16,
  UNSUPPORTED_KEY_FORMAT = -17,
  INCOMPATIBLE_KEY_FORMAT = -18,
  UNSUPPORTED_KEY_ENCRYPTION_ALGORITHM = -19,
  UNSUPPORTED_KEY_VERIFICATION_ALGORITHM = -20,
  INVALID_INPUT_LENGTH = -21,
  KEY_EXPORT_OPTIONS_INVALID = -22,
  DELEGATION_NOT_ALLOWED = -23,
  KEY_NOT_YET_VALID = -24,
  KEY_EXPIRED = -25,
  KEY_USER_NOT_AUTHENTICATED = -26,
  OUTPUT_PARAMETER_NULL = -27,
  INVALID_OPERATION_HANDLE = -28,
  INSUFFICIENT_BUFFER_SPACE = -29,
  VERIFICATION_FAILED = -30,
  TOO_MANY_OPERATIONS = -31,
  UNEXPECTED_NULL_POINTER = -32,
  INVALID_KEY_BLOB = -33,
  IMPORTED_KEY_NOT_ENCRYPTED = -34,
  IMPORTED_KEY_DECRYPTION_FAILED = -35,
  IMPORTED_KEY_NOT_SIGNED = -36,
  IMPORTED_KEY_VERIFICATION_FAILED = -37,
  INVALID_ARGUMENT = -38,
  UNSUPPORTED_TAG = -39,
  INVALID_TAG = -40,
  MEMORY_ALLOCATION_FAILED = -41,
  IMPORT_PARAMETER_MISMATCH = -44,
  SECURE_HW_ACCESS_DENIED = -45,
  OPERATION_CANCELLED = -46,
  CONCURRENT_ACCESS_CONFLICT = -47,
  SECURE_HW_BUSY = -48,
  SECURE_HW_COMMUNICATION_FAILED = -49,
  UNSUPPORTED_EC_FIELD = -50,
  MISSING_NONCE = -51,
  INVALID_NONCE = -52,
  MISSING_MAC_LENGTH = -53,
  KEY_RATE_LIMIT_EXCEEDED = -54,
  CALLER_NONCE_PROHIBITED = -55,
  KEY_MAX_OPS_EXCEEDED = -56,
  INVALID_MAC_LENGTH = -57,
  MISSING_MIN_MAC_LENGTH = -58,
  UNSUPPORTED_MIN_MAC_LENGTH = -59,
  UNSUPPORTED_KDF = -60,
  UNSUPPORTED_EC_CURVE = -61,
  KEY_REQUIRES_UPGRADE = -62,
  ATTESTATION_CHALLENGE_MISSING = -63,
  KEYMINT_NOT_CONFIGURED = -64,
  ATTESTATION_APPLICATION_ID_MISSING = -65,
  CANNOT_ATTEST_IDS = -66,
  ROLLBACK_RESISTANCE_UNAVAILABLE = -67,
  HARDWARE_TYPE_UNAVAILABLE = -68,
  PROOF_OF_PRESENCE_REQUIRED = -69,
  CONCURRENT_PROOF_OF_PRESENCE_REQUESTED = -70,
  NO_USER_CONFIRMATION = -71,
  DEVICE_LOCKED = -72,
  EARLY_BOOT_ENDED = -73,
  ATTESTATION_KEYS_NOT_PROVISIONED = -74,
  ATTESTATION_IDS_NOT_PROVISIONED = -75,
  INVALID_OPERATION = -76,
  STORAGE_KEY_UNSUPPORTED = -77,
  UNIMPLEMENTED = -100,
  VERSION_MISMATCH = -101,
  UNKNOWN_ERROR = -1000,
}
