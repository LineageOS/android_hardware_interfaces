/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.media.c2;

import android.hardware.media.c2.Buffer;
import android.hardware.media.c2.InfoBuffer;
import android.hardware.media.c2.Params;
import android.hardware.media.c2.WorkOrdinal;

/**
 * Data for an input frame or an output frame.
 *
 * This structure represents a @e frame with its metadata. A @e frame consists
 * of an ordered set of buffers, configuration changes, and info buffers along
 * with some non-configuration metadata.
 *
 * @note `FrameData` is the HIDL counterpart of `C2FrameData` in the Codec 2.0
 * standard.
 */
@VintfStability
parcelable FrameData {
    /** List of frame flags */
    /**
     * For input frames: no output frame shall be generated when processing
     * this frame, but metadata must still be processed.
     *
     * For output frames: this frame must be discarded but metadata is still
     * valid.
     */
    const int DROP_FRAME = (1 << 0);
    /**
     * This frame is the last frame of the current stream. Further frames
     * are part of a new stream.
     */
    const int END_OF_STREAM = (1 << 1);
    /**
     * This frame must be discarded with its metadata.
     *
     * This flag is only set by components, e.g. as a response to the flush
     * command.
     */
    const int DISCARD_FRAME = (1 << 2);
    /**
     * This frame is not the last frame produced for the input.
     *
     * This flag is normally set by the component - e.g. when an input frame
     * results in multiple output frames, this flag is set on all but the
     * last output frame.
     *
     * Also, when components are chained, this flag should be propagated
     * down the work chain. That is, if set on an earlier frame of a
     * work-chain, it should be propagated to all later frames in that
     * chain. Additionally, components down the chain could set this flag
     * even if not set earlier, e.g. if multiple output frames are generated
     * at that component for the input frame.
     */
    const int FLAG_INCOMPLETE = (1 << 3);
    /**
     * This frame contains only codec-specific configuration data, and no
     * actual access unit.
     */
    const int CODEC_CONFIG = (1 << 31);
    /**
     * Frame flags, as described above.
     */
    int flags;
    /**
     * @ref WorkOrdinal of the frame.
     */
    WorkOrdinal ordinal;
    /**
     * List of frame buffers.
     */
    Buffer[] buffers;
    /**
     * List of configuration updates.
     */
    Params configUpdate;
    /**
     * List of info buffers.
     */
    InfoBuffer[] infoBuffers;
}
