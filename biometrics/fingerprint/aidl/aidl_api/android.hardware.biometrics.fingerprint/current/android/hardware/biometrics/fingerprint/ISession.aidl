///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL file. Do not edit it manually. There are
// two cases:
// 1). this is a frozen version file - do not edit this in any case.
// 2). this is a 'current' file. If you make a backwards compatible change to
//     the interface (from the latest frozen version), the build system will
//     prompt you to update this file with `m <name>-update-api`.
//
// You must not make a backward incompatible change to any AIDL file built
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
