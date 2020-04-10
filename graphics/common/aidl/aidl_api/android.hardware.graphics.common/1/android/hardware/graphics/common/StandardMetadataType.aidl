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
enum StandardMetadataType {
  INVALID = 0,
  BUFFER_ID = 1,
  NAME = 2,
  WIDTH = 3,
  HEIGHT = 4,
  LAYER_COUNT = 5,
  PIXEL_FORMAT_REQUESTED = 6,
  PIXEL_FORMAT_FOURCC = 7,
  PIXEL_FORMAT_MODIFIER = 8,
  USAGE = 9,
  ALLOCATION_SIZE = 10,
  PROTECTED_CONTENT = 11,
  COMPRESSION = 12,
  INTERLACED = 13,
  CHROMA_SITING = 14,
  PLANE_LAYOUTS = 15,
  CROP = 16,
  DATASPACE = 17,
  BLEND_MODE = 18,
  SMPTE2086 = 19,
  CTA861_3 = 20,
  SMPTE2094_40 = 21,
}
