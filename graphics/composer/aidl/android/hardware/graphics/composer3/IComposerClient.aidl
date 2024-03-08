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

package android.hardware.graphics.composer3;

import android.hardware.graphics.common.DisplayDecorationSupport;
import android.hardware.graphics.common.Hdr;
import android.hardware.graphics.common.HdrConversionCapability;
import android.hardware.graphics.common.HdrConversionStrategy;
import android.hardware.graphics.common.Transform;
import android.hardware.graphics.composer3.ClientTargetProperty;
import android.hardware.graphics.composer3.ClockMonotonicTimestamp;
import android.hardware.graphics.composer3.ColorMode;
import android.hardware.graphics.composer3.CommandResultPayload;
import android.hardware.graphics.composer3.ContentType;
import android.hardware.graphics.composer3.DisplayAttribute;
import android.hardware.graphics.composer3.DisplayCapability;
import android.hardware.graphics.composer3.DisplayCommand;
import android.hardware.graphics.composer3.DisplayConfiguration;
import android.hardware.graphics.composer3.DisplayConnectionType;
import android.hardware.graphics.composer3.DisplayContentSample;
import android.hardware.graphics.composer3.DisplayContentSamplingAttributes;
import android.hardware.graphics.composer3.DisplayIdentification;
import android.hardware.graphics.composer3.FormatColorComponent;
import android.hardware.graphics.composer3.HdrCapabilities;
import android.hardware.graphics.composer3.IComposerCallback;
import android.hardware.graphics.composer3.OverlayProperties;
import android.hardware.graphics.composer3.PerFrameMetadataKey;
import android.hardware.graphics.composer3.PowerMode;
import android.hardware.graphics.composer3.ReadbackBufferAttributes;
import android.hardware.graphics.composer3.RenderIntent;
import android.hardware.graphics.composer3.VirtualDisplay;
import android.hardware.graphics.composer3.VsyncPeriodChangeConstraints;
import android.hardware.graphics.composer3.VsyncPeriodChangeTimeline;

@VintfStability
interface IComposerClient {
    /**
     * Invalid Config Exception
     */
    const int EX_BAD_CONFIG = 1;

    /**
     * Invalid Display Exception
     */
    const int EX_BAD_DISPLAY = 2;

    /**
     * Invalid Layer Exception
     */
    const int EX_BAD_LAYER = 3;

    /**
     * Invalid width, height, etc.
     */
    const int EX_BAD_PARAMETER = 4;

    /**
     * Reserved for historical reasons
     */
    const int EX_RESERVED = 5;
    /**
     * Temporary failure due to resource contention Exception
     */
    const int EX_NO_RESOURCES = 6;

    /**
     * validateDisplay has not been called Exception
     */
    const int EX_NOT_VALIDATED = 7;

    /**
     * Seamless cannot be required for configurations that don't share a
     * config group Exception
     */
    const int EX_UNSUPPORTED = 8;

    const int EX_SEAMLESS_NOT_ALLOWED = 9;
    /**
     * Seamless requirements cannot be met Exception
     */
    const int EX_SEAMLESS_NOT_POSSIBLE = 10;

    /**
     * Integer.MAX_VALUE is reserved for the invalid configuration.
     * This should not be returned as a valid configuration.
     */
    const int INVALID_CONFIGURATION = 0x7fffffff;

    /**
     * Creates a new layer on the given display.
     *
     * @param display is the display on which to create the layer.
     * @param bufferSlotCount is the number of buffer slot to be reserved.
     *
     * @return is the handle of the new layer.
     *
     * @exception EX_BAD_DISPLAY when an invalid display handle was passed in.
     * @exception EX_NO_RESOURCES when the device was unable to create a layer this
     *                      time.
     */
    long createLayer(long display, int bufferSlotCount);

    /**
     * Creates a new virtual display with the given width and height. The
     * format passed into this function is the default format requested by the
     * consumer of the virtual display output buffers.
     *
     * The display must be assumed to be on from the time the first frame is
     * presented until the display is destroyed.
     *
     * @param width is the width in pixels.
     * @param height is the height in pixels.
     * @param formatHint is the default output buffer format selected by
     *        the consumer.
     * @param outputBufferSlotCount is the number of output buffer slots to be
     *        reserved.
     *
     * @return is the newly-created virtual display.
     *
     * @exception EX_UNSUPPORTED when the width or height is too large for the
     *                     device to be able to create a virtual display.
     * @exception EX_NO_RESOURCES when the device is unable to create a new virtual
     *                      display at this time.
     */
    VirtualDisplay createVirtualDisplay(int width, int height,
            android.hardware.graphics.common.PixelFormat formatHint, int outputBufferSlotCount);

    /**
     * Destroys the given layer.
     *
     * @param display is the display on which the layer was created.
     * @param layer is the layer to destroy.
     *
     * @exception EX_BAD_DISPLAY when an invalid display handle was passed in.
     * @exception EX_BAD_LAYER when an invalid layer handle was passed in.
     */
    void destroyLayer(long display, long layer);

    /**
     * Destroys a virtual display. After this call all resources consumed by
     * this display may be freed by the device and any operations performed on
     * this display must fail.
     *
     * @param display is the virtual display to destroy.
     *
     * @exception EX_BAD_DISPLAY when an invalid display handle was passed in.
     * @exception EX_BAD_PARAMETER when the display handle which was passed in does
     *                       not refer to a virtual display.
     */
    void destroyVirtualDisplay(long display);

    /**
     * Executes commands.
     *
     * @param commands are the commands to be processed.
     *
     * @return are the command statuses.
     */
    CommandResultPayload[] executeCommands(in DisplayCommand[] commands);

    /**
     * Retrieves which display configuration is currently active.
     *
     * If no display configuration is currently active, this function raise
     * the exception EX_BAD_CONFIG. It is the responsibility of the client to call
     * setActiveConfig with a valid configuration before attempting to present
     * anything on the display.
     *
     * @param display is the display to which the active config is queried.
     *
     * @return is the currently active display configuration.
     *
     * @exception EX_BAD_DISPLAY when an invalid display handle was passed in.
     * @exception EX_BAD_CONFIG when no configuration is currently active.
     */
    int getActiveConfig(long display);

    /**
     * Returns the color modes supported on this display.
     *
     * All devices must support at least ColorMode.NATIVE.
     *
     * @param display is the display to query.
     *
     * @return is an array of color modes.
     *
     * @exception EX_BAD_DISPLAY when an invalid display handle was passed in.
     */
    ColorMode[] getColorModes(long display);

    /**
     * By default, layer dataspaces are mapped to the current color mode
     * colorimetrically with a few exceptions.
     *
     * When the layer dataspace is a legacy dataspace (see
     * common@1.1::Dataspace) and the display render intent is
     * RenderIntent.ENHANCE, the pixel values can go through an
     * implementation-defined saturation transform before being mapped to the
     * current color mode colorimetrically.
     *
     * Colors that are out of the gamut of the current color mode are
     * hard-clipped.
     *
     *
     * Returns the saturation matrix of the specified legacy dataspace.
     *
     * The saturation matrix can be used to approximate the legacy dataspace
     * saturation transform. It is to be applied on linear pixel values like
     * this:
     *
     *   (in GLSL)
     *   linearSrgb = saturationMatrix * linearSrgb;
     *
     * @param dataspace must be Dataspace::SRGB_LINEAR.
     *
     * @return is the 4x4 column-major matrix used to approximate the
     *         legacy dataspace saturation operation. The last row must be
     *         [0.0, 0.0, 0.0, 1.0].
     *
     * @exception EX_BAD_PARAMETER when an invalid dataspace was passed in.
     */
    float[] getDataspaceSaturationMatrix(android.hardware.graphics.common.Dataspace dataspace);

    /**
     * @deprecated use getDisplayConfigurations instead.
     *
     * Returns a display attribute value for a particular display
     * configuration.
     *
     * For legacy support getDisplayAttribute should return valid values for any requested
     * DisplayAttribute, and for all of the configs obtained either through getDisplayConfigs
     * or getDisplayConfigurations.
     *
     * @see getDisplayConfigurations
     * @see getDisplayConfigs
     *
     * @param display is the display to query.
     * @param config is the display configuration for which to return
     *        attribute values.
     *
     * @return is the value of the attribute.
     *
     * @exception EX_BAD_DISPLAY when an invalid display handle was passed in.
     * @exception EX_BAD_CONFIG when config does not name a valid configuration for
     *                    this display.
     * @exception EX_BAD_PARAMETER when attribute is unrecognized.
     * @exception EX_UNSUPPORTED when attribute cannot be queried for the config.
     */
    int getDisplayAttribute(long display, int config, DisplayAttribute attribute);

    /**
     * Provides a list of supported capabilities (as described in the
     * definition of DisplayCapability above). This list must not change after
     * initialization.
     *
     * @return is a list of supported capabilities.
     *
     * @exception EX_BAD_DISPLAY when an invalid display handle was passed in.
     */
    DisplayCapability[] getDisplayCapabilities(long display);

    /**
     * @deprecated use getDisplayConfigurations instead.
     * For legacy support getDisplayConfigs should return at least one valid config.
     * All the configs returned from the getDisplayConfigs should also be returned
     * from getDisplayConfigurations.
     *
     * @see getDisplayConfigurations
     */
    int[] getDisplayConfigs(long display);

    /**
     * Returns whether the given physical display is internal or external.
     *
     * @return is the connection type of the display.
     *
     * @exception EX_BAD_DISPLAY when the given display is invalid or virtual.
     */
    DisplayConnectionType getDisplayConnectionType(long display);

    /**
     * Returns the port and data that describe a physical display. The port is
     * a unique number that identifies a physical connector (e.g. eDP, HDMI)
     * for display output. The data blob is parsed to determine its format,
     * typically EDID 1.3 as specified in VESA E-EDID Standard Release A
     * Revision 1.
     *
     * @param display is the display to query.
     * @exception EX_BAD_DISPLAY when an invalid display handle was passed in.
     * @exception EX_UNSUPPORTED when identification data is unavailable.
     * @return the connector to which the display is connected and the EDID 1.3
     *         blob identifying the display.
     */
    DisplayIdentification getDisplayIdentificationData(long display);

    /**
     * Returns a human-readable version of the display's name.
     *
     * @return is the name of the display.
     *
     * @exception EX_BAD_DISPLAY when an invalid display handle was passed in.
     */
    String getDisplayName(long display);

    /**
     * Retrieves which vsync period the display is currently using.
     *
     * If no display configuration is currently active, this function must
     * return BAD_CONFIG. If the vsync period is about to change due to a
     * setActiveConfigWithConstraints call, this function must return the current vsync period
     * until the change takes place.
     *
     * @param display is the display for which the vsync period is queried.
     *
     * @return is the current vsync period of the display.
     *
     * @exception EX_BAD_DISPLAY when an invalid display handle was passed in.
     * @exception EX_BAD_CONFIG when no configuration is currently active.
     */
    int getDisplayVsyncPeriod(long display);

    /**
     * Collects the results of display content color sampling for display.
     *
     * Collection of data can occur whether the sampling is in ENABLE or
     * DISABLE state.
     *
     * @param  display     is the display to which the sampling is collected.
     * @param  maxFrames   is the maximum number of frames that should be represented in the sample.
     *                     The sample represents the most-recently posted frames.
     *                     If maxFrames is 0, all frames are to be represented by the sample.
     * @param  timestamp   is the timestamp after which any frames were posted that should be
     *                     included in the sample. Timestamp is CLOCK_MONOTONIC.
     *                     If timestamp is 0, do not filter from the sample by time.
     *
     * @return is the sample.
     *
     * @exception EX_BAD_DISPLAY   when an invalid display was passed in, or
     * @exception EX_UNSUPPORTED   when there is no efficient way to sample, or
     * @exception EX_BAD_PARAMETER when the component is not supported by the hardware.
     */
    DisplayContentSample getDisplayedContentSample(long display, long maxFrames, long timestamp);

    /**
     * Query for what types of color sampling the hardware supports.
     *
     * @param  display        is the display where the samples are collected.
     *
     * @return are the sampling attributes
     *
     * @exception EX_BAD_DISPLAY when an invalid display was passed in, or
     * @exception EX_UNSUPPORTED when there is no efficient way to sample.
     */
    DisplayContentSamplingAttributes getDisplayedContentSamplingAttributes(long display);

    /**
     * Queries the physical orientation of a display. Orientation 'Transform::NONE'
     * represents a display that doesn't require any transformation on layers
     * to be presented at their natural orientation.
     *
     * @param display is the display where the physical orientation is queried.
     *
     * @return is one of the below values:
     *         Transform::NONE
     *         Transform::ROT_90
     *         Transform::ROT_180
     *         Transform::ROT_270
     *
     * @exception EX_BAD_DISPLAY when an invalid display was passed in.
     */
    Transform getDisplayPhysicalOrientation(long display);

    /**
     * Returns the high dynamic range (HDR) capabilities of the given display,
     * which are invariant with regard to the active configuration.
     *
     * Displays which are not HDR-capable must return no types.
     *
     * @param display is the display to query.
     *
     * @return are the HDR capabilities
     *
     * @exception EX_BAD_DISPLAY when an invalid display handle was passed in.
     */
    HdrCapabilities getHdrCapabilities(long display);

    /**
     * Returns the maximum number of virtual displays supported by this device
     * (which may be 0). The client must not attempt to create more than this
     * many virtual displays on this device. This number must not change for
     * the lifetime of the device.
     *
     * @return is the maximum number of virtual displays supported.
     */
    int getMaxVirtualDisplayCount();

    /**
     * Returns the PerFrameMetadataKeys that are supported by this device.
     *
     * @param display is the display on which to create the layer.
     * @return is the vector of PerFrameMetadataKey keys that are
     *        supported by this device.
     * @exception EX_UNSUPPORTED if not supported on underlying HAL
     */
    PerFrameMetadataKey[] getPerFrameMetadataKeys(long display);

    /**
     * Returns the format which should be used when allocating a buffer for use by
     * device readback as well as the dataspace in which its contents must be
     * interpreted.
     *
     * The width and height of this buffer must be those of the currently-active
     * display configuration, and the usage flags must consist of the following:
     *   BufferUsage.CPU_READ | BufferUsage.GPU_TEXTURE |
     *   BufferUsage.COMPOSER_OUTPUT
     *
     * The format and dataspace provided must be sufficient such that if a
     * correctly-configured buffer is passed into setReadbackBuffer, filled by
     * the device, and then displayed by the client as a full-screen buffer, the
     * output of the display remains the same (subject to the note about protected
     * content in the description of setReadbackBuffer).
     *
     * If the active configuration or color mode of this display has changed
     * since a previous call to this function, it must be called again prior to
     * setting a readback buffer such that the returned format and dataspace will
     * be updated accordingly.
     *
     * Parameters:
     * @param display - the display on which to create the layer.
     *
     * @return is the readback buffer attributes.
     *
     * @exception EX_BAD_DISPLAY when an invalid display handle was passed in.
     * @exception EX_UNSUPPORTED if not supported on underlying HAL
     *
     * See also:
     *   setReadbackBuffer
     *   getReadbackBufferFence
     */
    ReadbackBufferAttributes getReadbackBufferAttributes(long display);

    /**
     * Returns an acquire sync fence file descriptor which must signal when the
     * buffer provided to setReadbackBuffer has been filled by the device and is
     * safe for the client to read.
     *
     * If it is already safe to read from this buffer, -1 may be returned instead.
     * The client takes ownership of this file descriptor and is responsible for
     * closing it when it is no longer needed.
     *
     * This function must be called immediately after the composition cycle being
     * captured into the readback buffer. The complete ordering of a readback buffer
     * capture is as follows:
     *
     *   getReadbackBufferAttributes
     *   // Readback buffer is allocated
     *   // Many frames may pass
     *
     *   setReadbackBuffer
     *   validateDisplay
     *   presentDisplay
     *   getReadbackBufferFence
     *   // Implicitly wait on the acquire fence before accessing the buffer
     *
     * Parameters:
     * @param display - the display on which to create the layer.
     *
     * @return is a sync fence file descriptor as described above; pointer
     *       must be non-NULL
     *
     * @exception EX_BAD_DISPLAY - an invalid display handle was passed in
     * @exception EX_NO_RESOURCES - the readback operation was successful, but
     *                        resulted in a different validate result than would
     *                        have occurred without readback
     * @exception EX_UNSUPPORTED - the readback operation was unsuccessful because of
     *                       resource constraints, the presence of protected
     *                       content, or other reasons; -1 must be returned for
     *                       acquireFence
     *
     * See also:
     *   getReadbackBufferAttributes
     *   setReadbackBuffer
     */
    @nullable ParcelFileDescriptor getReadbackBufferFence(long display);

    /**
     * Returns the render intents supported by the specified display and color
     * mode.
     *
     * For SDR color modes, RenderIntent.COLORIMETRIC must be supported. For
     * HDR color modes, RenderIntent.TONE_MAP_COLORIMETRIC must be supported.
     *
     * @param display is the display to query.
     * @param mode is the color mode to query.
     *
     * @return is an array of render intents.
     *
     * @exception EX_BAD_DISPLAY when an invalid display handle was passed in.
     * @exception EX_BAD_PARAMETER when an invalid color mode was passed in.
     */
    RenderIntent[] getRenderIntents(long display, ColorMode mode);

    /**
     * Provides a list of all the content types supported by this display (any of
     * ContentType.{GRAPHICS, PHOTO, CINEMA, GAME}). This list must not change after
     * initialization.
     *
     * Content types are introduced in HDMI 1.4 and supporting them is optional. The
     * ContentType.NONE is always supported and will not be returned by this method..
     *
     * @return out is a list of supported content types.
     *
     * @exception EX_BAD_DISPLAY when an invalid display handle was passed in.
     */
    ContentType[] getSupportedContentTypes(long display);

    /**
     * Report whether and how this display supports Composition.DISPLAY_DECORATION.
     *
     * @return A description of how the display supports DISPLAY_DECORATION, or null
     * if it is unsupported.
     *
     * @exception EX_BAD_DISPLAY when an invalid display handle was passed in.
     */
    @nullable DisplayDecorationSupport getDisplayDecorationSupport(long display);

    /**
     * Provides a IComposerCallback object for the device to call.
     *
     * This function must be called only once.
     *
     * @param callback is the IComposerCallback object.
     */
    void registerCallback(in IComposerCallback callback);

    /**
     * Sets the active configuration for this display. Upon returning, the
     * given display configuration must be active and remain so until either
     * this function is called again or the display is disconnected.
     *
     * @param display is the display to which the active config is set.
     * @param config is the new display configuration.
     *
     * @exception EX_BAD_DISPLAY when an invalid display handle was passed in.
     * @exception EX_BAD_CONFIG when the configuration handle passed in is not valid
     *                    for this display.
     */
    void setActiveConfig(long display, int config);

    /**
     * Sets the active configuration and the refresh rate for this display.
     * If the new config shares the same config group as the current config,
     * only the vsync period shall change.
     * Upon returning, the given display configuration, except vsync period, must be active and
     * remain so until either this function is called again or the display is disconnected.
     * When the display starts to refresh at the new vsync period, onVsync_2_4 callback must be
     * called with the new vsync period.
     *
     * @param display is the display for which the active config is set.
     * @param config is the new display configuration.
     * @param vsyncPeriodChangeConstraints are the constraints required for changing vsync period.
     *
     * @exception EX_BAD_DISPLAY when an invalid display handle was passed in.
     * @exception EX_BAD_CONFIG when the configuration handle passed in is not valid
     *                    for this display.
     * @exception EX_SEAMLESS_NOT_ALLOWED when seamlessRequired was true but config provided doesn't
     *                              share the same config group as the current config.
     * @exception EX_SEAMLESS_NOT_POSSIBLE when seamlessRequired was true but the display cannot
     * achieve the vsync period change without a noticeable visual artifact. When the conditions
     * change and it may be possible to change the vsync period seamlessly, onSeamlessPossible
     * callback must be called to indicate that caller should retry.
     *
     * @return is the timeline for the vsync period change.
     */
    VsyncPeriodChangeTimeline setActiveConfigWithConstraints(
            long display, int config, in VsyncPeriodChangeConstraints vsyncPeriodChangeConstraints);

    /**
     * Sets the display config in which the device boots.
     *
     * If the device is unable to boot in this config for any reason (example HDMI display changed),
     * the implementation should try to find a config which matches the resolution and refresh-rate
     * of this config. If no such config exists, the implementation's preferred display config
     * should be used.
     *
     * @param display is the display for which the boot config is set.
     * @param config is the new boot config for the display.
     *
     * @exception EX_BAD_DISPLAY when an invalid display handle was passed in.
     * @exception EX_BAD_CONFIG when an invalid config id was passed in.
     * @exception EX_UNSUPPORTED when not supported by the underlying HAL
     *
     * @see getDisplayConfigs
     * @see clearBootDisplayConfig
     * @see getPreferredBootDisplayConfig
     */
    void setBootDisplayConfig(long display, int config);

    /**
     * Clears the boot display config.
     *
     * The device should boot in the implementation's preferred display config.
     *
     * @param display is the display for which the cached boot config is cleared.
     *
     * @exception EX_BAD_DISPLAY when an invalid display handle was passed in.
     * @exception EX_UNSUPPORTED when not supported by the underlying HAL
     *
     * @see getDisplayConfigs
     * @see setBootDisplayConfig
     * @see getPreferredBootDisplayConfig
     */
    void clearBootDisplayConfig(long display);

    /**
     * Returns the implementation's preferred display config.
     *
     * This is the display config used by the implementation at boot time, if the boot display
     * config has not been requested yet, or if it has been previously cleared.
     *
     * @param display is the display to which the preferred config is queried.
     * @return the implementation's preferred display config.
     *
     * @exception EX_BAD_DISPLAY when an invalid display handle was passed in.
     * @exception EX_UNSUPPORTED when not supported by the underlying HAL
     *
     * @see getDisplayConfigs
     * @see setBootDisplayConfig
     * @see clearBootDisplayConfig
     */
    int getPreferredBootDisplayConfig(long display);

    /**
     * Requests the display to enable/disable its low latency mode.
     *
     * If the display is connected via HDMI 2.1, then Auto Low Latency Mode should be triggered. If
     * the display is internally connected and a custom low latency mode is available, that should
     * be triggered.
     *
     * This function should only be called if the display reports support for
     * DisplayCapability.AUTO_LOW_LATENCY_MODE from getDisplayCapabilities_2_4.
     *
     * @exception EX_BAD_DISPLAY when an invalid display handle was passed in.
     * @exception EX_UNSUPPORTED when AUTO_LOW_LATENCY_MODE is not supported by the composer
     *                           implementation or the given display
     */
    void setAutoLowLatencyMode(long display, boolean on);

    /**
     * Set the number of client target slots to be reserved.
     *
     * @param display is the display to which the slots are reserved.
     * @param clientTargetSlotCount is the slot count for client targets.
     * @exception EX_BAD_DISPLAY when an invalid display handle was passed in.
     * @exception EX_NO_RESOURCES when unable to reserve the slots.
     */
    void setClientTargetSlotCount(long display, int clientTargetSlotCount);

    /**
     * Sets the color mode and render intent of the given display.
     *
     * The color mode and render intent change must take effect on next
     * presentDisplay.
     *
     * All devices must support at least ColorMode.NATIVE and
     * RenderIntent.COLORIMETRIC, and displays are assumed to be in this mode
     * upon hotplug.
     *
     * @param display is the display to which the color mode is set.
     * @param mode is the color mode to set to.
     * @param intent is the render intent to set to.
     *
     * @exception EX_BAD_DISPLAY when an invalid display handle was passed in.
     * @exception EX_BAD_PARAMETER when mode or intent is invalid
     * @exception EX_UNSUPPORTED when mode or intent is not supported on this
     *         display.
     */
    void setColorMode(long display, ColorMode mode, RenderIntent intent);

    /**
     * Instructs the connected display that the content being shown is of the given type - one of
     * GRAPHICS, PHOTO, CINEMA, GAME.
     *
     * Content types are introduced in HDMI 1.4 and supporting them is optional. If they are
     * supported, this signal should switch the display to a mode that is optimal for the given
     * type of content. See HDMI 1.4 specification for more information.
     *
     * If the display is internally connected (not through HDMI), and such modes are available,
     * this method should trigger them.
     *
     * This function can be called for a content type even if no support for it is
     * reported from getSupportedContentTypes.
     *
     * @exception EX_BAD_DISPLAY when an invalid display handle was passed in.
     * @exception EX_UNSUPPORTED when the given content type is not supported by the composer
     *         implementation or the given display
     */
    void setContentType(long display, ContentType type);

    /**
     * Enables or disables the collection of color content statistics
     * on this display.
     *
     * Sampling occurs on the contents of the final composition on this display
     * (i.e., the contents presented on screen). Samples should be collected after all
     * color transforms have been applied.
     *
     * Sampling support is optional, and is set to DISABLE by default.
     * On each call to ENABLE, all collected statistics must be reset.
     *
     * Sample data can be queried via getDisplayedContentSample().
     *
     * @param display        is the display to which the sampling mode is set.
     * @param enabled        indicates whether to enable or disable sampling.
     * @param componentMask  The mask of which components should be sampled. If zero, all supported
     *                       components are to be enabled.
     * @param maxFrames      is the maximum number of frames that should be stored before discard.
     *                       The sample represents the most-recently posted frames.
     * @exception EX_BAD_DISPLAY when an invalid display handle was passed in,
     * @exception EX_BAD_PARAMETER when enabled was an invalid value, or
     * @exception EX_NO_RESOURCES when the requested ringbuffer size via maxFrames was
     *                                 not available.
     * @exception EX_UNSUPPORTED when there is no efficient way to sample.
     */
    void setDisplayedContentSamplingEnabled(
            long display, boolean enable, FormatColorComponent componentMask, long maxFrames);

    /**
     * Sets the power mode of the given display. The transition must be
     * complete when this function returns. It is valid to call this function
     * multiple times with the same power mode.
     *
     * All displays must support PowerMode.ON and PowerMode.OFF.  Whether a
     * display supports PowerMode.DOZE or PowerMode.DOZE_SUSPEND may be
     * queried using getDozeSupport.
     *
     * @param display is the display to which the power mode is set.
     * @param mode is the new power mode.
     * @exception EX_BAD_DISPLAY when an invalid display handle was passed in.
     * @exception EX_BAD_PARAMETER when mode was not a valid power mode.
     * @exception EX_UNSUPPORTED when mode is not supported on this display.
     */
    void setPowerMode(long display, PowerMode mode);

    /**
     * Sets the readback buffer to be filled with the contents of the next
     * composition performed for this display (i.e., the contents present at the
     * time of the next validateDisplay/presentDisplay cycle).
     *
     * This buffer must have been allocated as described in
     * getReadbackBufferAttributes and is in the dataspace provided by the same.
     *
     * Also provides a file descriptor referring to a release sync fence
     * object, which must be signaled when it is safe to write to the readback
     * buffer. If it is already safe to write to the readback buffer, null may be passed instead.
     *
     * If there is hardware protected content on the display at the time of the next
     * composition, the area of the readback buffer covered by such content must be
     * completely black. Any areas of the buffer not covered by such content may
     * optionally be black as well.
     *
     *
     * This function must not be called between any call to validateDisplay and a
     * subsequent call to presentDisplay.
     *
     * Parameters:
     * @param display - the display on which to create the layer.
     * @param buffer - the new readback buffer
     * @param releaseFence - a sync fence file descriptor as described above or null if it is
     *                       already safe to write to the readback buffer.
     *
     * @exception EX_BAD_DISPLAY - an invalid display handle was passed in
     * @exception EX_BAD_PARAMETER - the new readback buffer handle was invalid
     *
     * See also:
     *   getReadbackBufferAttributes
     *   getReadbackBufferFence
     */
    void setReadbackBuffer(long display, in android.hardware.common.NativeHandle buffer,
            in @nullable ParcelFileDescriptor releaseFence);

    /**
     * Enables or disables the vsync signal for the given display. Virtual
     * displays never generate vsync callbacks, and any attempt to enable
     * vsync for a virtual display though this function must succeed and have
     * no other effect.
     *
     * @param display is the display to which the vsync mode is set.
     * @param enabled indicates whether to enable or disable vsync
     * @exception EX_BAD_DISPLAY when an invalid display handle was passed in.
     * @exception EX_BAD_PARAMETER when enabled was an invalid value.
     */
    void setVsyncEnabled(long display, boolean enabled);

    /**
     * Enables or disables the idle timer on this display.
     *
     * Idle timer is used to allow the display to go into a panel idle mode after some
     * idle period.
     *
     * This function should only be called if the display reports support for
     * DisplayCapability.DISPLAY_IDLE_TIMER from getDisplayCapabilities.
     *
     * @param display is the display to which the idle timer is set.
     * @param timeoutMs is the minimum requirements of idle period in milliseconds. Panel
     *                should not go into the idle state within the minimum requirement after
     *                idle for a while. 0 means disabled, panel should not go into idle state.
     *
     * @exception EX_BAD_DISPLAY when an invalid display handle was passed in.
     * @exception EX_BAD_PARAMETER when timeout is a negative number.
     * @exception EX_UNSUPPORTED when idle is not supported on this display.
     *
     */
    void setIdleTimerEnabled(long display, int timeoutMs);

    /**
     * Hardware overlays is a technique to composite different buffers directly to the screen
     * while bypassing GPU composition.
     *
     * This function returns what the device's overlays support.
     *
     * @exception EX_UNSUPPORTED when not supported by the underlying HAL
     *
     * @return the overlay properties of the device.
     */
    OverlayProperties getOverlaySupport();

    /**
     * Returns the array of HDR conversion capability. Each HdrConversionCapability depicts that
     * HDR conversion is possible from sourceType to outputType. This doesn't change after
     * initialization.
     *
     * @exception EX_UNSUPPORTED when not supported by the underlying HAL
     *
     * @see setHdrConversionStrategy
     */
    HdrConversionCapability[] getHdrConversionCapabilities();

    /**
     * Sets the of HDR conversion strategy.
     *
     * @return the chosen HDR type in case HdrConversionStrategy has autoAllowedHdrTypes set. In
     * other cases, return HDR type INVALID.
     * @exception EX_UNSUPPORTED when not supported by the underlying HAL
     *
     * @see getHdrConversionCapabilities
     */
    Hdr setHdrConversionStrategy(in HdrConversionStrategy conversionStrategy);

    /*
     * Sets either the callback for the refresh rate change is enabled or disabled
     * for the provided display.
     *
     * @see IComposerCallback.onRefreshRateChangedDebug
     *
     * @param display is the display on which the callback is enabled on.
     * @param enabled true when refresh rate callback is enabled,
     *        false when refresh rate callback is disabled.
     */
    void setRefreshRateChangedCallbackDebugEnabled(long display, boolean enabled);

    /**
     * Returns all of the valid display configurations.
     * getDisplayConfigurations is the superset of getDisplayConfigs and
     * getDisplayConfigs should return at least one config.
     *
     * @param display is the display for which the configurations are requested.
     * @param maxFrameIntervalNs refers to the largest frameInterval to be set for
     * VrrConfig.frameIntervalPowerHints in nanoseconds
     *
     * @see getDisplayConfigs
     */
    DisplayConfiguration[] getDisplayConfigurations(long display, int maxFrameIntervalNs);

    /**
     * Provides an early hint for a frame that is likely to be presented.
     * This is used for the implementation to take the necessary steps to ensure that
     * the next frame(s) could be presented as close as possible to the expectedPresentTime and
     * according to the frameIntervalNs cadence.
     * See DisplayCommand.expectedPresentTime and DisplayCommand.frameIntervalNs.
     *
     * The framework will call this function based on the parameters specified in
     * DisplayConfiguration.VrrConfig:
     * - notifyExpectedPresentTimeoutNs specifies the idle time from the previous present command
     * where the framework must send the early hint for the next frame.
     * - notifyExpectedPresentHeadsUpNs specifies minimal time that framework must send
     * the early hint before the next frame.
     *
     * The framework can omit calling this API when the next present command matches
     * the cadence of the previous present command frameIntervalNs.
     *
     * If DisplayConfiguration.notifyExpectedPresentConfig is null, this function will never be
     * called.
     *
     * @param display is the display for which the notifyExpectedPresent is called.
     * @param expectedPresentTime is the expectedPresentTime that will be provided in the next
     * present command
     * @param frameIntervalNs is a hint about the cadence of the next frames in nanoseconds.
     */
    oneway void notifyExpectedPresent(
            long display, in ClockMonotonicTimestamp expectedPresentTime, int frameIntervalNs);
}
