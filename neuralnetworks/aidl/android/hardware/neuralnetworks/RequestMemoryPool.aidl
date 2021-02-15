/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.hardware.neuralnetworks;

import android.hardware.neuralnetworks.Memory;

/**
 * A memory pool.
 */
@VintfStability
union RequestMemoryPool {
    /**
     * Specifies a client-managed shared memory pool.
     */
    Memory pool;
    /**
     * Specifies a driver-managed buffer. It is the token returned from IDevice::allocate, and is
     * specific to the IDevice object.
     */
    int token;
}
