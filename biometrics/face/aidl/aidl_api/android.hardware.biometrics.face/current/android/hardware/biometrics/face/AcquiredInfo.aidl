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

package android.hardware.biometrics.face;
@Backing(type="byte") @VintfStability
enum AcquiredInfo {
  GOOD = 0,
  INSUFFICIENT = 1,
  TOO_BRIGHT = 2,
  TOO_DARK = 3,
  TOO_CLOSE = 4,
  TOO_FAR = 5,
  FACE_TOO_HIGH = 6,
  FACE_TOO_LOW = 7,
  FACE_TOO_RIGHT = 8,
  FACE_TOO_LEFT = 9,
  POOR_GAZE = 10,
  NOT_DETECTED = 11,
  TOO_MUCH_MOTION = 12,
  RECALIBRATE = 13,
  TOO_DIFFERENT = 14,
  TOO_SIMILAR = 15,
  PAN_TOO_EXTREME = 16,
  TILT_TOO_EXTREME = 17,
  ROLL_TOO_EXTREME = 18,
  FACE_OBSCURED = 19,
  START = 20,
  SENSOR_DIRTY = 21,
  VENDOR = 22,
}
