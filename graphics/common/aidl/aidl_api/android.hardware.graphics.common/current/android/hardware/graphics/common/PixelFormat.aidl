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

package android.hardware.graphics.common;
@Backing(type="int") @VintfStability
enum PixelFormat {
  UNSPECIFIED = 0,
  RGBA_8888 = 1,
  RGBX_8888 = 2,
  RGB_888 = 3,
  RGB_565 = 4,
  BGRA_8888 = 5,
  YCBCR_422_SP = 16,
  YCRCB_420_SP = 17,
  YCBCR_422_I = 20,
  RGBA_FP16 = 22,
  RAW16 = 32,
  BLOB = 33,
  IMPLEMENTATION_DEFINED = 34,
  YCBCR_420_888 = 35,
  RAW_OPAQUE = 36,
  RAW10 = 37,
  RAW12 = 38,
  RGBA_1010102 = 43,
  Y8 = 538982489,
  Y16 = 540422489,
  YV12 = 842094169,
  DEPTH_16 = 48,
  DEPTH_24 = 49,
  DEPTH_24_STENCIL_8 = 50,
  DEPTH_32F = 51,
  DEPTH_32F_STENCIL_8 = 52,
  STENCIL_8 = 53,
  YCBCR_P010 = 54,
  HSV_888 = 55,
}
