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
@VintfStability
interface IDevice {
  android.hardware.neuralnetworks.DeviceBuffer allocate(in android.hardware.neuralnetworks.BufferDesc desc, in android.hardware.neuralnetworks.IPreparedModelParcel[] preparedModels, in android.hardware.neuralnetworks.BufferRole[] inputRoles, in android.hardware.neuralnetworks.BufferRole[] outputRoles);
  android.hardware.neuralnetworks.Capabilities getCapabilities();
  android.hardware.neuralnetworks.NumberOfCacheFiles getNumberOfCacheFilesNeeded();
  android.hardware.neuralnetworks.Extension[] getSupportedExtensions();
  boolean[] getSupportedOperations(in android.hardware.neuralnetworks.Model model);
  android.hardware.neuralnetworks.DeviceType getType();
  String getVersionString();
  void prepareModel(in android.hardware.neuralnetworks.Model model, in android.hardware.neuralnetworks.ExecutionPreference preference, in android.hardware.neuralnetworks.Priority priority, in long deadline, in ParcelFileDescriptor[] modelCache, in ParcelFileDescriptor[] dataCache, in byte[] token, in android.hardware.neuralnetworks.IPreparedModelCallback callback);
  void prepareModelFromCache(in long deadline, in ParcelFileDescriptor[] modelCache, in ParcelFileDescriptor[] dataCache, in byte[] token, in android.hardware.neuralnetworks.IPreparedModelCallback callback);
  const int BYTE_SIZE_OF_CACHE_TOKEN = 32;
  const int MAX_NUMBER_OF_CACHE_FILES = 32;
  const int EXTENSION_TYPE_HIGH_BITS_PREFIX = 15;
  const int EXTENSION_TYPE_LOW_BITS_TYPE = 16;
  const int OPERAND_TYPE_BASE_MAX = 65535;
  const int OPERATION_TYPE_BASE_MAX = 65535;
}
