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

import android.hardware.common.NativeHandle;
import android.hardware.graphics.common.FRect;
import android.hardware.graphics.common.Point;
import android.hardware.graphics.common.Rect;
import android.hardware.graphics.composer3.Buffer;
import android.hardware.graphics.composer3.Color;
import android.hardware.graphics.composer3.LayerBrightness;
import android.hardware.graphics.composer3.LayerLifecycleBatchCommandType;
import android.hardware.graphics.composer3.ParcelableBlendMode;
import android.hardware.graphics.composer3.ParcelableComposition;
import android.hardware.graphics.composer3.ParcelableDataspace;
import android.hardware.graphics.composer3.ParcelableTransform;
import android.hardware.graphics.composer3.PerFrameMetadata;
import android.hardware.graphics.composer3.PerFrameMetadataBlob;
import android.hardware.graphics.composer3.PlaneAlpha;
import android.hardware.graphics.composer3.ZOrder;

@VintfStability
parcelable LayerCommand {
    /**
     * The layer which this commands refers to.
     * @see IComposer.createLayer
     */
    long layer;

    /**
     * Sets the position of a cursor layer.
     *
     * The position of a cursor layer can be updated without a validate/present display
     * sequence if that layer was marked as Composition.CURSOR and validation previously succeeded
     * (i.e., the device didn't request a composition).
     */
    @nullable Point cursorPosition;

    /**
     * Sets the buffer handle to be displayed for this layer. If the buffer
     * properties set at allocation time (width, height, format, and usage)
     * have not changed since the previous frame, it is not necessary to call
     * validateDisplay before calling presentDisplay unless new state needs to
     * be validated in the interim.
     *
     * Also provides a file descriptor referring to an acquire sync fence
     * object, which must be signaled when it is safe to read from the given
     * buffer. If it is already safe to read from the buffer, an empty handle
     * may be passed instead.
     *
     * This function must return NONE and have no other effect if called for a
     * layer with a composition type of Composition.SOLID_COLOR (because it
     * has no buffer) or Composition.SIDEBAND or Composition.CLIENT (because
     * synchronization and buffer updates for these layers are handled
     * elsewhere).
     */
    @nullable Buffer buffer;

    /**
     * Provides the region of the source buffer which has been modified since
     * the last frame. This region does not need to be validated before
     * calling presentDisplay.
     *
     * Once set through this function, the damage region remains the same
     * until a subsequent call to this function.
     *
     * If damage is non-empty, then it may be assumed that any portion of the
     * source buffer not covered by one of the rects has not been modified
     * this frame. If damage is empty, then the whole source buffer must be
     * treated as if it has been modified.
     *
     * If the layer's contents are not modified relative to the prior frame,
     * damage must contain exactly one empty rect([0, 0, 0, 0]).
     *
     * The damage rects are relative to the pre-transformed buffer, and their
     * origin is the top-left corner. They must not exceed the dimensions of
     * the latched buffer.
     */
    @nullable Rect[] damage;

    /**
     * Sets the blend mode of the given layer.
     */
    @nullable ParcelableBlendMode blendMode;

    /**
     * Sets the color of the given layer. If the composition type of the layer
     * is not Composition.SOLID_COLOR, this call must succeed and have no
     * other effect.
     */
    @nullable Color color;

    /**
     * Sets the desired composition type of the given layer. During
     * validateDisplay, the device may request changes to the composition
     * types of any of the layers as described in the definition of
     * Composition above.
     */
    @nullable ParcelableComposition composition;

    /**
     * Sets the dataspace of the layer.
     *
     * The dataspace provides more information about how to interpret the buffer
     * or solid color, such as the encoding standard and color transform.
     *
     * See the values of ParcelableDataspace for more information.
     */
    @nullable ParcelableDataspace dataspace;

    /**
     * Sets the display frame (the portion of the display covered by a layer)
     * of the given layer. This frame must not exceed the display dimensions.
     */
    @nullable Rect displayFrame;

    /**
     * Sets an alpha value (a floating point value in the range [0.0, 1.0])
     * which will be applied to the whole layer. It can be conceptualized as a
     * preprocessing step which applies the following function:
     *   if (blendMode == BlendMode.PREMULTIPLIED)
     *       out.rgb = in.rgb * planeAlpha
     *   out.a = in.a * planeAlpha
     *
     * If the device does not support this operation on a layer which is
     * marked Composition.DEVICE, it must request a composition type change
     * to Composition.CLIENT upon the next validateDisplay call.
     *
     */
    @nullable PlaneAlpha planeAlpha;

    /**
     * Sets the sideband stream for this layer. If the composition type of the
     * given layer is not Composition.SIDEBAND, this call must succeed and
     * have no other effect.
     */
    @nullable NativeHandle sidebandStream;

    /**
     * Sets the source crop (the portion of the source buffer which will fill
     * the display frame) of the given layer. This crop rectangle must not
     * exceed the dimensions of the latched buffer.
     *
     * If the device is not capable of supporting a true float source crop
     * (i.e., it will truncate or round the floats to integers), it must set
     * this layer to Composition.CLIENT when crop is non-integral for the
     * most accurate rendering.
     *
     * If the device cannot support float source crops, but still wants to
     * handle the layer, it must use the following code (or similar) to
     * convert to an integer crop:
     *   intCrop.left = (int) ceilf(crop.left);
     *   intCrop.top = (int) ceilf(crop.top);
     *   intCrop.right = (int) floorf(crop.right);
     *   intCrop.bottom = (int) floorf(crop.bottom);
     */
    @nullable FRect sourceCrop;

    /**
     * Sets the transform (rotation/flip) of the given layer.
     */
    @nullable ParcelableTransform transform;

    /**
     * Specifies the portion of the layer that is visible, including portions
     * under translucent areas of other layers. The region is in screen space,
     * and must not exceed the dimensions of the screen.
     */
    @nullable Rect[] visibleRegion;

    /**
     * Sets the desired Z order (height) of the given layer. A layer with a
     * greater Z value occludes a layer with a lesser Z value.
     */
    @nullable ZOrder z;

    /**
     * Sets a matrix for color transform which will be applied on this layer
     * before composition.
     *
     * If the device is not capable of apply the matrix on this layer, it must force
     * this layer to client composition during VALIDATE_DISPLAY.
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
     * Given a matrix of this form and an input color [R_in, G_in, B_in],
     * the input color must first be converted to linear space
     * [R_linear, G_linear, B_linear], then the output linear color
     * [R_out_linear, G_out_linear, B_out_linear] will be:
     *
     * R_out_linear = R_linear * r.r + G_linear * g.r + B_linear * b.r + Tr
     * G_out_linear = R_linear * r.g + G_linear * g.g + B_linear * b.g + Tg
     * B_out_linear = R_linear * r.b + G_linear * g.b + B_linear * b.b + Tb
     *
     * [R_out_linear, G_out_linear, B_out_linear] must then be converted to
     * gamma space: [R_out, G_out, B_out] before blending.
     */
    @nullable float[] colorTransform;

    /**
     * Sets the desired brightness for the layer. This is intended to be used for instance when
     * presenting an SDR layer alongside HDR content. The HDR content will be presented at the
     * display brightness in nits, and accordingly SDR content shall be dimmed according to the
     * provided brightness ratio.
     */
    @nullable LayerBrightness brightness;

    /**
     * Sets the PerFrameMetadata for the display. This metadata must be used
     * by the implementation to better tone map content to that display.
     *
     * This is a command that may be called every frame.
     */
    @nullable PerFrameMetadata[] perFrameMetadata;

    /**
     * This command sends metadata that may be used for tone-mapping the
     * associated layer.  The metadata structure follows a {key, blob}
     * format (see the PerFrameMetadataBlob struct).  All keys must be
     * returned by a prior call to getPerFrameMetadataKeys and must
     * be part of the list of keys associated with blob-type metadata
     * (see PerFrameMetadataKey).
     *
     * This command may be called every frame.
     */
    @nullable PerFrameMetadataBlob[] perFrameMetadataBlob;

    /**
     * Specifies a region of the layer that is transparent and may be skipped
     * by the DPU, e.g. using a blocking region, in order to save power. This
     * is only a hint, so the composition of the layer must look the same
     * whether or not this region is skipped.
     *
     * The region is in screen space and must not exceed the dimensions of
     * the screen.
     */
    @nullable Rect[] blockingRegion;

    /**
     * Specifies which buffer slots should be cleared of buffer references
     * because these buffers will no longer be used and the memory should
     * be freed.
     */
    @nullable int[] bufferSlotsToClear;

    /**
     * Specifies if this layer command is on type modify, create or destroy.
     * This command is replacing the older IComposerClient.createLayer and destroyLayer
     * and making it more efficient with reduced aidls to the HAL.
     * The HAL will report the errors by setting CommandResultPayload::CommandError.
     */
    LayerLifecycleBatchCommandType layerLifecycleBatchCommandType;

    /**
     * Specifies the number of buffer slot to be reserved.
     */
    int newBufferSlotCount;
}
