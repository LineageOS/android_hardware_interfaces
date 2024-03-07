/**
 * Copyright (c) 2021, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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

package android.hardware.graphics.composer3;
@VintfStability
interface IComposerClient {
  long createLayer(long display, int bufferSlotCount);
  android.hardware.graphics.composer3.VirtualDisplay createVirtualDisplay(int width, int height, android.hardware.graphics.common.PixelFormat formatHint, int outputBufferSlotCount);
  void destroyLayer(long display, long layer);
  void destroyVirtualDisplay(long display);
  android.hardware.graphics.composer3.CommandResultPayload[] executeCommands(in android.hardware.graphics.composer3.DisplayCommand[] commands);
  int getActiveConfig(long display);
  android.hardware.graphics.composer3.ColorMode[] getColorModes(long display);
  float[] getDataspaceSaturationMatrix(android.hardware.graphics.common.Dataspace dataspace);
  /**
   * @deprecated use getDisplayConfigurations instead. Returns a display attribute value for a particular display configuration. For legacy support getDisplayAttribute should return valid values for any requested DisplayAttribute, and for all of the configs obtained either through getDisplayConfigs or getDisplayConfigurations.
   */
  int getDisplayAttribute(long display, int config, android.hardware.graphics.composer3.DisplayAttribute attribute);
  android.hardware.graphics.composer3.DisplayCapability[] getDisplayCapabilities(long display);
  /**
   * @deprecated use getDisplayConfigurations instead. For legacy support getDisplayConfigs should return at least one valid config. All the configs returned from the getDisplayConfigs should also be returned from getDisplayConfigurations.
   */
  int[] getDisplayConfigs(long display);
  android.hardware.graphics.composer3.DisplayConnectionType getDisplayConnectionType(long display);
  android.hardware.graphics.composer3.DisplayIdentification getDisplayIdentificationData(long display);
  String getDisplayName(long display);
  int getDisplayVsyncPeriod(long display);
  android.hardware.graphics.composer3.DisplayContentSample getDisplayedContentSample(long display, long maxFrames, long timestamp);
  android.hardware.graphics.composer3.DisplayContentSamplingAttributes getDisplayedContentSamplingAttributes(long display);
  android.hardware.graphics.common.Transform getDisplayPhysicalOrientation(long display);
  android.hardware.graphics.composer3.HdrCapabilities getHdrCapabilities(long display);
  int getMaxVirtualDisplayCount();
  android.hardware.graphics.composer3.PerFrameMetadataKey[] getPerFrameMetadataKeys(long display);
  android.hardware.graphics.composer3.ReadbackBufferAttributes getReadbackBufferAttributes(long display);
  @nullable ParcelFileDescriptor getReadbackBufferFence(long display);
  android.hardware.graphics.composer3.RenderIntent[] getRenderIntents(long display, android.hardware.graphics.composer3.ColorMode mode);
  android.hardware.graphics.composer3.ContentType[] getSupportedContentTypes(long display);
  @nullable android.hardware.graphics.common.DisplayDecorationSupport getDisplayDecorationSupport(long display);
  void registerCallback(in android.hardware.graphics.composer3.IComposerCallback callback);
  void setActiveConfig(long display, int config);
  android.hardware.graphics.composer3.VsyncPeriodChangeTimeline setActiveConfigWithConstraints(long display, int config, in android.hardware.graphics.composer3.VsyncPeriodChangeConstraints vsyncPeriodChangeConstraints);
  void setBootDisplayConfig(long display, int config);
  void clearBootDisplayConfig(long display);
  int getPreferredBootDisplayConfig(long display);
  void setAutoLowLatencyMode(long display, boolean on);
  void setClientTargetSlotCount(long display, int clientTargetSlotCount);
  void setColorMode(long display, android.hardware.graphics.composer3.ColorMode mode, android.hardware.graphics.composer3.RenderIntent intent);
  void setContentType(long display, android.hardware.graphics.composer3.ContentType type);
  void setDisplayedContentSamplingEnabled(long display, boolean enable, android.hardware.graphics.composer3.FormatColorComponent componentMask, long maxFrames);
  void setPowerMode(long display, android.hardware.graphics.composer3.PowerMode mode);
  void setReadbackBuffer(long display, in android.hardware.common.NativeHandle buffer, in @nullable ParcelFileDescriptor releaseFence);
  void setVsyncEnabled(long display, boolean enabled);
  void setIdleTimerEnabled(long display, int timeoutMs);
  android.hardware.graphics.composer3.OverlayProperties getOverlaySupport();
  android.hardware.graphics.common.HdrConversionCapability[] getHdrConversionCapabilities();
  android.hardware.graphics.common.Hdr setHdrConversionStrategy(in android.hardware.graphics.common.HdrConversionStrategy conversionStrategy);
  void setRefreshRateChangedCallbackDebugEnabled(long display, boolean enabled);
  android.hardware.graphics.composer3.DisplayConfiguration[] getDisplayConfigurations(long display, int maxFrameIntervalNs);
  oneway void notifyExpectedPresent(long display, in android.hardware.graphics.composer3.ClockMonotonicTimestamp expectedPresentTime, int frameIntervalNs);
  const int EX_BAD_CONFIG = 1;
  const int EX_BAD_DISPLAY = 2;
  const int EX_BAD_LAYER = 3;
  const int EX_BAD_PARAMETER = 4;
  const int EX_RESERVED = 5;
  const int EX_NO_RESOURCES = 6;
  const int EX_NOT_VALIDATED = 7;
  const int EX_UNSUPPORTED = 8;
  const int EX_SEAMLESS_NOT_ALLOWED = 9;
  const int EX_SEAMLESS_NOT_POSSIBLE = 10;
  const int INVALID_CONFIGURATION = 0x7fffffff;
}
