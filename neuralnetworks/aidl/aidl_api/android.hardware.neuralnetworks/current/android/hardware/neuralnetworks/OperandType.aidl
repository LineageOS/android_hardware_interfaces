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

package android.hardware.neuralnetworks;
@Backing(type="int") @VintfStability
enum OperandType {
  FLOAT32 = 0,
  INT32 = 1,
  UINT32 = 2,
  TENSOR_FLOAT32 = 3,
  TENSOR_INT32 = 4,
  TENSOR_QUANT8_ASYMM = 5,
  BOOL = 6,
  TENSOR_QUANT16_SYMM = 7,
  TENSOR_FLOAT16 = 8,
  TENSOR_BOOL8 = 9,
  FLOAT16 = 10,
  TENSOR_QUANT8_SYMM_PER_CHANNEL = 11,
  TENSOR_QUANT16_ASYMM = 12,
  TENSOR_QUANT8_SYMM = 13,
  TENSOR_QUANT8_ASYMM_SIGNED = 14,
  SUBGRAPH = 15,
}
