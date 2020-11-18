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

package android.hardware.biometrics.fingerprint;
@VintfStability
interface ISession {
  void generateChallenge(in int cookie, in int timeoutSec);
  void revokeChallenge(in int cookie, in long challenge);
  android.hardware.biometrics.common.ICancellationSignal enroll(in int cookie, in android.hardware.keymaster.HardwareAuthToken hat);
  android.hardware.biometrics.common.ICancellationSignal authenticate(in int cookie, in long operationId);
  android.hardware.biometrics.common.ICancellationSignal detectInteraction(in int cookie);
  void enumerateEnrollments(in int cookie);
  void removeEnrollments(in int cookie, in int[] enrollmentIds);
  void getAuthenticatorId(in int cookie);
  void invalidateAuthenticatorId(in int cookie);
  void resetLockout(in int cookie, in android.hardware.keymaster.HardwareAuthToken hat);
  void onPointerDown(in int pointerId, in int x, in int y, in float minor, in float major);
  void onPointerUp(in int pointerId);
  void onUiReady();
}
