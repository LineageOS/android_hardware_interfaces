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

package android.hardware.audio.effect;

import android.hardware.audio.effect.Descriptor;

/**
 * Effect interfaces definitions to configure and control the effect instance.
 */
@VintfStability
interface IEffect {
    /**
     * Open an effect instance, effect should not start processing data before receive START
     * command. All necessary information should be allocated and instance should transfer to IDLE
     * state after open() call has been handled successfully.
     * After open, the effect instance should be able to handle all IEffect interface calls.
     *
     * @throws a EX_UNSUPPORTED_OPERATION if device capability/resource is not enough or system
     *         failure happens.
     * @note Open an already-opened effect instance should do nothing and should not throw an error.
     */
    void open();

    /**
     * Called by the client to close the effect instance, processing thread should be destroyed and
     * consume no CPU after close.
     *
     * It is recommended to close the effect on the client side as soon as it becomes unused, it's
     * client responsibility to make sure all parameter/buffer is correct if client wants to reopen
     * a closed instance.
     *
     * Effect instance close interface should always succeed unless:
     * 1. The effect instance is not in a proper state to be closed, for example it's still in
     * processing state.
     * 2. There is system/hardware related failure when close.
     *
     * @throws EX_ILLEGAL_STATE if the effect instance is not in a proper state to be closed.
     * @throws EX_UNSUPPORTED_OPERATION if the effect instance failed to close for any other reason.
     * @note Close an already-closed effect should do nothing and should not throw an error.
     */
    void close();

    /**
     * Return the @c Descriptor of this effect instance.
     *
     * Must be available for the effect instance at anytime and should always succeed.
     *
     * @return Descriptor The @c Descriptor of this effect instance.
     */
    Descriptor getDescriptor();
}
