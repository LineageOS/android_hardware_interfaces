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
@VintfStability
interface IVibrator {
  int getCapabilities();
  void off();
  void on(in int timeoutMs, in android.hardware.vibrator.IVibratorCallback callback);
  int perform(in android.hardware.vibrator.Effect effect, in android.hardware.vibrator.EffectStrength strength, in android.hardware.vibrator.IVibratorCallback callback);
  android.hardware.vibrator.Effect[] getSupportedEffects();
  void setAmplitude(in float amplitude);
  void setExternalControl(in boolean enabled);
  int getCompositionDelayMax();
  int getCompositionSizeMax();
  android.hardware.vibrator.CompositePrimitive[] getSupportedPrimitives();
  int getPrimitiveDuration(android.hardware.vibrator.CompositePrimitive primitive);
  void compose(in android.hardware.vibrator.CompositeEffect[] composite, in android.hardware.vibrator.IVibratorCallback callback);
  android.hardware.vibrator.Effect[] getSupportedAlwaysOnEffects();
  void alwaysOnEnable(in int id, in android.hardware.vibrator.Effect effect, in android.hardware.vibrator.EffectStrength strength);
  void alwaysOnDisable(in int id);
  const int CAP_ON_CALLBACK = 1;
  const int CAP_PERFORM_CALLBACK = 2;
  const int CAP_AMPLITUDE_CONTROL = 4;
  const int CAP_EXTERNAL_CONTROL = 8;
  const int CAP_EXTERNAL_AMPLITUDE_CONTROL = 16;
  const int CAP_COMPOSE_EFFECTS = 32;
  const int CAP_ALWAYS_ON_CONTROL = 64;
}
