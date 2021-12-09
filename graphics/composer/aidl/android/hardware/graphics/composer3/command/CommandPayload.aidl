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

package android.hardware.graphics.composer3.command;

import android.hardware.graphics.composer3.command.DisplayCommand;
import android.hardware.graphics.composer3.command.LayerCommand;

/**
 * Type of commands that can be used in IComposerClient.executeCommands.
 * Note that this is a union and each command can only have one type.
 */
@VintfStability
union CommandPayload {
    DisplayCommand displayCommand;
    LayerCommand layerCommand;
}
