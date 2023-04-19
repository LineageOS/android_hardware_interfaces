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

import android.hardware.media.c2.BaseBlock;
import android.hardware.media.c2.Work;

/**
 * List of `Work` objects.
 *
 * `WorkBundle` is used in IComponent::queue(), IComponent::flush() and
 * IComponentListener::onWorkDone(). A `WorkBundle` object consists of a list of
 * `Work` objects and a list of `BaseBlock` objects. Bundling multiple `Work`
 * objects together provides two benefits:
 *   1. Batching of `Work` objects can reduce the number of IPC calls.
 *   2. If multiple `Work` objects contain `Block`s that refer to the same
 *      `BaseBlock`, the number of `BaseBlock`s that is sent between processes
 *      is also reduced.
 *
 * @note `WorkBundle` is the AIDL counterpart of the vector of `C2Work` in the
 * Codec 2.0 standard. The presence of #baseBlocks helps with minimizing the
 * data transferred over an IPC.
 */
@VintfStability
parcelable WorkBundle {
    /**
     * A list of Work items.
     */
    Work[] works;
    /**
     * A list of blocks indexed by elements of #works.
     */
    BaseBlock[] baseBlocks;
}
