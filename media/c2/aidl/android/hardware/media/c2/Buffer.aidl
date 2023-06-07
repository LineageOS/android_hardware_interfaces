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

import android.hardware.media.c2.Block;
import android.hardware.media.c2.Params;

/**
 * A codec buffer, which is a collection of @ref Block objects and metadata.
 *
 * This is a part of @ref FrameData.
 */
@VintfStability
parcelable Buffer {
    /**
     * Metadata associated with the buffer.
     */
    Params info;
    /**
     * Blocks contained in the buffer.
     */
    Block[] blocks;
}
