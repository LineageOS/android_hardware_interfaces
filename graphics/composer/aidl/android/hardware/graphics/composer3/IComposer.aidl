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

import android.hardware.graphics.composer3.Capability;
import android.hardware.graphics.composer3.IComposerClient;

@VintfStability
interface IComposer {
    /**
     * Temporary failure due to resource contention Exception
     */
    const int EX_NO_RESOURCES = 6;

    /**
     * Creates a client of the composer.
     *
     * @return is the newly created client.
     *
     * @exception EX_NO_RESOURCES when the client could not be created.
     */
    IComposerClient createClient();

    /**
     * Provides a list of supported capabilities (as described in the
     * definition of Capability above). This list must not change after
     * initialization.
     *
     * @return is a list of supported capabilities.
     */
    Capability[] getCapabilities();
}
