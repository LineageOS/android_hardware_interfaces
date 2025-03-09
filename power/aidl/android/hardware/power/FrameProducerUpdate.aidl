
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

package android.hardware.power;

/**
 * A single update for an individual producer object.
 */
@VintfStability
parcelable FrameProducerUpdate {
    /**
     * The ID of the producer, guaranteed to be unique at a given time.
     */
    long producerId;

    /**
     * If true, this producer is no longer active and its data can be released.
     * "sessions" will be blank in this case, as there are no more associations
     * with this producer.
     */
    boolean isDead;

    /**
     * The IDs of all sessions associated with the producer, corresponding to the
     * ID in SessionConfig returned to the framework during session creation.
     *
     * If an object was created without a Session ID by using an older creation
     * method, it is not eligible to be associated to a FrameProducer.
     */
    int[] sessions;
}
