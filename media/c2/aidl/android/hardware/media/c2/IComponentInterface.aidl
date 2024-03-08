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

import android.hardware.media.c2.IConfigurable;

/**
 * Component interface object. This object contains all of the configurations of
 * a potential or actual component. It can be created and used independently of
 * an actual Codec2 component to query supported parameters for various
 * component settings, and configurations for a potential component.
 *
 * An actual component exposes this interface via IComponent::getInterface().
 */
@VintfStability
interface IComponentInterface {
    /**
     * Returns the @ref IConfigurable instance associated to this component
     * interface.
     *
     * @return `IConfigurable` instance. This must not be null.
     */
    IConfigurable getConfigurable();
}
