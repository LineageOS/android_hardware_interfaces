/*
 * Copyright (C) 2024 The Android Open Source Project
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

import android.hardware.audio.effect.VendorExtension;
import android.media.audio.eraser.Capability;
import android.media.audio.eraser.Configuration;
import android.media.audio.eraser.Mode;

/**
 * The Audio Eraser Effect is an audio effect that combines multiple capabilities to manipulate and
 * enhance audio streams.
 *
 * The Audio Eraser Effect integrates three primary components:
 *
 * Sound Separator: Detects and splits the input audio into multiple sound sources.
 * Sound Classifier: Classifies each separated sound source into predefined categories based on the
 * AudioSet ontology.
 * Remixer: Adjusts the gain factor (volume) of each classified sound source according to specified
 * configurations, then recombines them into a single output audio stream.
 *
 * The Audio Eraser Effect operates in different modes, each leveraging a subset of these
 * components to achieve specific functionalities as defined in `android.media.audio.eraser.Mode`.
 *
 * Flow Diagrams for each operation mode as below.
 *
 * ERASER:
 *                                                  +-----------------+
 *                                              +-->| Sound Classifier|---+
 *                                              |   +-----------------+   |
 *                                              |                         |
 *+----------------+       +----------------+   |   +-----------------+   |   +----------------+
 *|   Input Audio  |------>| Sound Separator|---+-->| Sound Classifier|---+-->|    Remixer     |
 *+----------------+       +----------------+   |   +-----------------+   |   +--------+-------+
 *                                              |                         |            |
 *                                              |   +-----------------+   |            |
 *                                              +-->| Sound Classifier|---+            |
 *                                                  +-----------------+                |
 *                                                            |                        v
 *                                                            v               +----------------+
 *                                               {Classification Metadata}    |  Output Audio  |
 *                                                                            +----------------+
 *
 * CLASSIFIER:
 *+----------------+       +-----------------+       +-----------------+
 *|   Input Audio  |------>| Sound Classifier|------>| Original Audio  |
 *+----------------+       +-----------------+       +-----------------+
 *                                  |
 *                                  v
 *                       {Classification Metadata}
 *
 */
@VintfStability
union Eraser {
    /**
     * Parameter Id with union tag to identify the parameters for getParameter().
     */
    @VintfStability
    union Id {
        VendorExtension vendorExtensionTag;
        Eraser.Tag commonTag;
    }

    /**
     * Vendor extension parameters which can be customized.
     */
    VendorExtension vendor;

    /**
     * Eraser capability, defines supported input/output data formats, available work modes, and
     * the specific capabilities of the sound classifier and separator
     */
    Capability capability;

    /**
     * Eraser configuration, contains the list of configurations for the eraser effect.
     */
    Configuration configuration;
}
