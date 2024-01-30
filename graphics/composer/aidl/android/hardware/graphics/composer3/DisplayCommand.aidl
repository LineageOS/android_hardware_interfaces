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

import android.hardware.graphics.composer3.Buffer;
import android.hardware.graphics.composer3.ClientTarget;
import android.hardware.graphics.composer3.ClockMonotonicTimestamp;
import android.hardware.graphics.composer3.DisplayBrightness;
import android.hardware.graphics.composer3.LayerCommand;

@VintfStability
parcelable DisplayCommand {
    /**
     * The display which this commands refers to.
     */
    long display;

    /**
     * Sets layer commands for this display.
     * @see LayerCommand.
     */
    LayerCommand[] layers;

    /**
     * Sets a color transform which will be applied after composition.
     *
     * If the device is not capable of either using the matrix to
     * apply the desired color transform, it must force all layers to client
     * composition during VALIDATE_DISPLAY.
     *
     * If Capability.SKIP_CLIENT_COLOR_TRANSFORM is present, then
     * the client must never apply the color transform during client
     * composition, even if all layers are being composed by the client.
     *
     * The matrix provided is an affine color transformation of the following
     * form:
     *
     * |r.r r.g r.b 0|
     * |g.r g.g g.b 0|
     * |b.r b.g b.b 0|
     * |Tr  Tg  Tb  1|
     *
     * This matrix must be provided in row-major form:
     *
     * {r.r, r.g, r.b, 0, g.r, ...}.
     *
     * Given a matrix of this form and an input color [R_in, G_in, B_in], the
     * output color [R_out, G_out, B_out] will be:
     *
     * R_out = R_in * r.r + G_in * g.r + B_in * b.r + Tr
     * G_out = R_in * r.g + G_in * g.g + B_in * b.g + Tg
     * B_out = R_in * r.b + G_in * g.b + B_in * b.b + Tb
     *
     */
    @nullable float[] colorTransformMatrix;

    /**
     * Sets the desired brightness of the display.
     *
     * Ideally, the brightness of the display will take effect within this frame so that it can be
     * aligned with color transforms. Some display architectures may take multiple frames to apply
     * the display brightness, for example when internally switching the display between multiple
     * power modes to achieve higher luminance. In those cases, the underlying display panel's real
     * brightness may not be applied atomically; however, layer dimming when mixing HDR and SDR
     * content must be synchronized to ensure that there is no user-perceptable flicker.
     *
     * The display luminance must be updated by this command even if there is not pending validate
     * or present command.
     */
    @nullable DisplayBrightness brightness;

    /**
     * Sets the buffer handle which will receive the output of client
     * composition.  Layers marked as Composition.CLIENT must be composited
     * into this buffer prior to the call to PRESENT_DISPLAY, and layers not
     * marked as Composition.CLIENT must be composited with this buffer by
     * the device.
     *
     * The buffer handle provided may be empty if no layers are being
     * composited by the client. This must not result in an error (unless an
     * invalid display handle is also provided).
     *
     * Also provides a file descriptor referring to an acquire sync fence
     * object, which must be signaled when it is safe to read from the client
     * target buffer.  If it is already safe to read from this buffer, an
     * empty handle may be passed instead.
     *
     * For more about dataspaces, see SET_LAYER_DATASPACE.
     *
     * The damage parameter describes a surface damage region as defined in
     * the description of SET_LAYER_SURFACE_DAMAGE.
     *
     * Will be called before PRESENT_DISPLAY if any of the layers are marked
     * as Composition.CLIENT. If no layers are so marked, then it is not
     * necessary to call this function. It is not necessary to call
     * validateDisplay after changing the target through this function.
     */
    @nullable ClientTarget clientTarget;

    /**
     * Sets the output buffer for a virtual display. That is, the buffer to
     * which the composition result will be written.
     *
     * Also provides a file descriptor referring to a release sync fence
     * object, which must be signaled when it is safe to write to the output
     * buffer. If it is already safe to write to the output buffer, an empty
     * handle may be passed instead.
     *
     * Must be called at least once before PRESENT_DISPLAY, but does not have
     * any interaction with layer state or display validation.
     */
    @nullable Buffer virtualDisplayOutputBuffer;

    /**
     * Sets the expected present time to present the current content on screen.
     * The implementation should try to present the display as close as possible
     * to the given expectedPresentTime. If expectedPresentTime is 0, the
     * implementation should present the display as soon as possible.
     */
    @nullable ClockMonotonicTimestamp expectedPresentTime;

    /**
     * Instructs the device to inspect all of the layer state and determine if
     * there are any composition type changes necessary before presenting the
     * display. Permitted changes are described in the definition of
     * Composition above.
     */
    boolean validateDisplay;

    /**
     * Accepts the changes required by the device from the previous
     * validateDisplay call (which may be queried using
     * getChangedCompositionTypes) and revalidates the display. This function
     * is equivalent to requesting the changed types from
     * getChangedCompositionTypes, setting those types on the corresponding
     * layers, and then calling validateDisplay again.
     *
     * After this call it must be valid to present this display. Calling this
     * after validateDisplay returns 0 changes must succeed with NONE, but
     * must have no other effect.
     */
    boolean acceptDisplayChanges;

    /**
     * Presents the current display contents on the screen (or in the case of
     * virtual displays, into the output buffer).
     *
     * Prior to calling this function, the display must be successfully
     * validated with validateDisplay. Note that setLayerBuffer and
     * setLayerSurfaceDamage specifically do not count as layer state, so if
     * there are no other changes to the layer state (or to the buffer's
     * properties as described in setLayerBuffer), then it is safe to call
     * this function without first validating the display.
     */
    boolean presentDisplay;

    /**
     * Presents the current display contents on the screen (or in the case of
     * virtual displays, into the output buffer) if validate can be skipped,
     * or perform a VALIDATE_DISPLAY action instead.
     */
    boolean presentOrValidateDisplay;

    /**
     * If a value greater than 0 is set, it provides a hint about the next frame(s)
     * cadence. This parameter represents the time in nanoseconds of when to expect the
     * next frames to arrive. For example. frameIntervalNs=33333333 indicates that the
     * cadence of the next frames is 30Hz.
     *
     * The implementation should take the necessary steps to present the next frames as
     * close as possible to the cadence.
     */
    int frameIntervalNs;
}
