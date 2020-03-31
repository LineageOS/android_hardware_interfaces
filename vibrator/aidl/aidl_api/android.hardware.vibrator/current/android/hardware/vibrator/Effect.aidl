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

package android.hardware.vibrator;
@Backing(type="int") @VintfStability
enum Effect {
  CLICK = 0,
  DOUBLE_CLICK = 1,
  TICK = 2,
  THUD = 3,
  POP = 4,
  HEAVY_CLICK = 5,
  RINGTONE_1 = 6,
  RINGTONE_2 = 7,
  RINGTONE_3 = 8,
  RINGTONE_4 = 9,
  RINGTONE_5 = 10,
  RINGTONE_6 = 11,
  RINGTONE_7 = 12,
  RINGTONE_8 = 13,
  RINGTONE_9 = 14,
  RINGTONE_10 = 15,
  RINGTONE_11 = 16,
  RINGTONE_12 = 17,
  RINGTONE_13 = 18,
  RINGTONE_14 = 19,
  RINGTONE_15 = 20,
  TEXTURE_TICK = 21,
}
