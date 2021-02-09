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
interface IPreparedModel {
  android.hardware.neuralnetworks.ExecutionResult executeSynchronously(in android.hardware.neuralnetworks.Request request, in boolean measureTiming, in long deadline, in long loopTimeoutDuration);
  android.hardware.neuralnetworks.IFencedExecutionCallback executeFenced(in android.hardware.neuralnetworks.Request request, in ParcelFileDescriptor[] waitFor, in boolean measureTiming, in long deadline, in long loopTimeoutDuration, in long duration, out @nullable ParcelFileDescriptor syncFence);
  const long DEFAULT_LOOP_TIMEOUT_DURATION_NS = 2000000000;
  const long MAXIMUM_LOOP_TIMEOUT_DURATION_NS = 15000000000;
}
