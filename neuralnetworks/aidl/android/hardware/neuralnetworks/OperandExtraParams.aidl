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

import android.hardware.neuralnetworks.SymmPerChannelQuantParams;

/**
 * Parameters specific to a particular operand type.
 */
@VintfStability
union OperandExtraParams {
    /**
     * Symmetric per-channel quantization parameters.
     *
     * Only applicable to operands of type TENSOR_QUANT8_SYMM_PER_CHANNEL.
     */
    SymmPerChannelQuantParams channelQuant;
    /**
     * Extension operand parameters.
     *
     * The framework treats this as an opaque data blob.
     * The format is up to individual extensions.
     */
    byte[] extension;
}
