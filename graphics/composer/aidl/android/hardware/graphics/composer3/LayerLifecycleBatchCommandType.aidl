/**
 * Copyright (c) 2023, The Android Open Source Project
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

/**
 * Possible batch command types for a given layer.
 */
@VintfStability
@Backing(type="int")
enum LayerLifecycleBatchCommandType {
    /**
     * Layer attributes are being modified for already created layer.
     */
    MODIFY = 0,
    /**
     * This indicates that the current LayerCommand should also create the layer,
     * before processing the other attributes in the LayerCommand.
     */
    CREATE = 1,
    /**
     * This indicates that the current LayerCommand should also destroyes the layer,
     * after processing the other attributes in the LayerCommand.
     */
    DESTROY = 2,
}
