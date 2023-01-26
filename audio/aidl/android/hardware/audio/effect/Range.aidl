/*
 * Copyright (C) 2023 The Android Open Source Project
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

/**
 * Define the range (min and max) of a certain field, identified by tag.
 * Can be used by effect capabilities to define supported value ranges.
 */
@VintfStability
parcelable Range {
    /**
     * The union tag name which the range is defined for.
     * For example: if used in AutomaticGainControlV1.Capability, value of Range.tag could be
     * targetLevelDbFs or compressionGainDb.
     */
    int tag;

    @VintfStability
    parcelable Int {
        int min;
        int max;
    }

    @VintfStability
    parcelable Float {
        float min;
        float max;
    }

    @VintfStability
    parcelable Long {
        long min;
        long max;
    }

    @VintfStability
    parcelable Byte {
        byte min;
        byte max;
    }

    @VintfStability
    union Types {
        Int rangeInt;
        Float rangeFloat;
        Long rangeLong;
        Byte rangeByte;
    }

    Types types;
}
