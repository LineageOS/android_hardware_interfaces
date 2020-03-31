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
@Backing(type="long") @VintfStability
enum BufferUsage {
  CPU_READ_MASK = 15,
  CPU_READ_NEVER = 0,
  CPU_READ_RARELY = 2,
  CPU_READ_OFTEN = 3,
  CPU_WRITE_MASK = 240,
  CPU_WRITE_NEVER = 0,
  CPU_WRITE_RARELY = 32,
  CPU_WRITE_OFTEN = 48,
  GPU_TEXTURE = 256,
  GPU_RENDER_TARGET = 512,
  COMPOSER_OVERLAY = 2048,
  COMPOSER_CLIENT_TARGET = 4096,
  PROTECTED = 16384,
  COMPOSER_CURSOR = 32768,
  VIDEO_ENCODER = 65536,
  CAMERA_OUTPUT = 131072,
  CAMERA_INPUT = 262144,
  RENDERSCRIPT = 1048576,
  VIDEO_DECODER = 4194304,
  SENSOR_DIRECT_DATA = 8388608,
  GPU_CUBE_MAP = 33554432,
  GPU_MIPMAP_COMPLETE = 67108864,
  HW_IMAGE_ENCODER = 134217728,
  GPU_DATA_BUFFER = 16777216,
  VENDOR_MASK = -268435456,
  VENDOR_MASK_HI = -281474976710656,
}
