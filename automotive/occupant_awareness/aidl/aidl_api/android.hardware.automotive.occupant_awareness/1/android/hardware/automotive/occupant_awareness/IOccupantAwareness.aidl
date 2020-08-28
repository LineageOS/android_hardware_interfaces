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

package android.hardware.automotive.occupant_awareness;
@VintfStability
interface IOccupantAwareness {
  android.hardware.automotive.occupant_awareness.OccupantAwarenessStatus startDetection();
  android.hardware.automotive.occupant_awareness.OccupantAwarenessStatus stopDetection();
  int getCapabilityForRole(in android.hardware.automotive.occupant_awareness.Role occupantRole);
  android.hardware.automotive.occupant_awareness.OccupantAwarenessStatus getState(in android.hardware.automotive.occupant_awareness.Role occupantRole, in int detectionCapability);
  void setCallback(in android.hardware.automotive.occupant_awareness.IOccupantAwarenessClientCallback callback);
  void getLatestDetection(out android.hardware.automotive.occupant_awareness.OccupantDetections detections);
  const int CAP_NONE = 0;
  const int CAP_PRESENCE_DETECTION = 1;
  const int CAP_GAZE_DETECTION = 2;
  const int CAP_DRIVER_MONITORING_DETECTION = 4;
}
