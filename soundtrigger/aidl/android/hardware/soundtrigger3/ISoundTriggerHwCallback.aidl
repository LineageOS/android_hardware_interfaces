/*
 * Copyright 2021 The Android Open Source Project
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

package android.hardware.soundtrigger3;

import android.media.soundtrigger.PhraseRecognitionEvent;
import android.media.soundtrigger.RecognitionEvent;

/**
 * SoundTrigger HAL per-model Callback interface.
 */
@VintfStability
interface ISoundTriggerHwCallback {
    /**
     * Callback method called by the HAL when a model has been unloaded at the HAL implementation's
     * discretion.
     * This event may only be delivered when the model state is 'stopped'.
     * This event is NOT sent as part of an unload sequence initiated by the client.
     *
     * @param model The model handle.
     */
    void modelUnloaded(in int model);

    /**
     * Callback method called by the HAL when the sound recognition triggers for a key phrase sound
     * model.
     * This event may only be delivered when the model state is 'started'.
     * Unless the status of the event is RecognitionStatus.FORCED, this event indicates that the
     * state of this model has become 'stopped'.
     *
     * @param event A RecognitionEvent structure containing detailed results of the recognition
     *     triggered
     */
    void phraseRecognitionCallback(in int model, in PhraseRecognitionEvent event);

    /**
     * Callback method called by the HAL when the sound recognition triggers.
     * This event may only be delivered when the model state is 'started'.
     * Unless the status of the event is RecognitionStatus.FORCED, this event indicates that the
     * state of this model has become 'stopped'.
     *
     * @param event A RecognitionEvent structure containing detailed results of the recognition
     *     triggered
     */
    void recognitionCallback(in int model, in RecognitionEvent event);
}
