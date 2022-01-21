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

import android.hardware.soundtrigger3.ISoundTriggerHwCallback;
import android.hardware.soundtrigger3.ISoundTriggerHwGlobalCallback;
import android.media.soundtrigger.ModelParameter;
import android.media.soundtrigger.ModelParameterRange;
import android.media.soundtrigger.PhraseSoundModel;
import android.media.soundtrigger.Properties;
import android.media.soundtrigger.RecognitionConfig;
import android.media.soundtrigger.SoundModel;

/**
 * SoundTrigger HAL interface. Used for hardware recognition of hotwords
 * and other sounds.
 *
 * Basic usage:
 * ============
 * ISoundTriggerHw supports the ability to have one of more  detection sessions running at a given
 * time, and listening to acoustic events. The basic flow of setting up such a session is:
 * - Load a model using loadSoundModel() or loadPhraseSoundModel(). The provided model object would
 *   indicate the (implementation-specific) detection algorithm (engine) to use, as well as any
 *   parameters applicable for this agorithm. Upon success, those methods would return a handle
 *   which will be used to reference this model in subsequent calls.
 * - Once the model had been successfully loaded, detection can begin by calling startRecognition().
 * - Recognition will continue running in the background until one of the following events occurs:
 *   - stopRecognition() has been called on this model.
 *   - A detection has occurred.
 *   - Detection was aborted, typically for resource constraints, for example, when a higher-
 *     priority use-case has been initiated.
 * - In the latter two cases, a recognition event will be sent via a the callback interface that was
 *   registered by the client upon loading. In either case, after any of these events occur, the
 *   detection becomes inactive and no more recognition callbacks are allowed.
 * - The same model maybe started again at a later time, and this process may repeat as many times
 *   as needed.
 * - Finally, an inactive model that is no longer needed may be unloaded via unloadModel().
 *
 * Important notes about the threading model:
 * ==========================================
 * Both this interface and the corresponding callback interface use a synchronous calling
 * convention. This model comes with some advantages, but also with some risks of deadlocks if the
 * implementation does not handle this correctly. Please consider the following:
 * - After stopRecognition() returns, no more recognition events for that model may be sent. This
 *   implies that any queues holding such events must be flushed before the call returns and that
 *   may imply that callback from the HAL to the client are done while stopRecognition() is blocked.
 *   This is OK, and supported by the framework.
 * - Similarly, the same relationship applies between unloadModel() and subsequent callbacks to
 *   modelUnloaded().
 * - Other than these two cases, calls into the HAL *MAY NOT* block on callbacks from the HAL, or
 *   else deadlock conditions may result, which may be handled by rebooting of the HAL process and
 *   cause service outages.
 *
 * Due to the asynchronous nature of recognition events and preemptive model unloading, the HAL must
 * correctly handle requests that would have been valid before an event has been delivered, but
 * became moot as result of the event. Namely:
 * - stopRecognition() may be called on a model that has already delivered an event and became
 *   inactive as a result. The HAL must return a successful return code in this case.
 * - Furthermore, if a model is preemptively unloaded after it triggers (typically, this would
 *   happen when it is first aborted and immediately preemptively unloaded), stopRecognition() may
 *   be called on it. The HAL must return successfully in this case.
 * - startRecognition() may be called on a model that has been preemptively unloaded. In this case,
 *   the HAL must signal a ServiceSpecificException(RESOURCE_CONTENTION) to indicate that the
 *   operation is temporarily unsuccessful.
 * - unloadSoundModel() may be called on a model that has been preemptively unloaded. The HAL must
 *   return a successful return code in this case. This also implies that model handles should
 *   generally not be reused until explicitly unloaded. To avoid the rare possibility of running out
 *   of handles, the framework may call unloadModel() on models that have been preemptively unloaded
 *   by the HAL.
 *
 * Important notes about resource constraints and concurrency
 * =========================================================
 * Up until this version, the framework would enforce concurrency constraints expressed by the
 * Properties presented by the soundtrigger instance. These include constraints on the maximum
 * amount of models that can be loaded at the same time and on running recognition while capturing
 * from the microphone.
 * This version changes the approach for how these constraints are modeled, both offering the HAL
 * implementation more flexibility and simplifying the framework's job in enforcing these
 * limitations. Note that there is no change for how the framework behaves with earlier versions,
 * everything described below only applies to this version and onward.
 * The way this is achieved is as following:
 * - The framework will no longer enforce constraints on concurrent loading of models, as expressed
 *   in the Properties.maxSoundModels field (this property is merely a hint at this point and may be
 *   deprecated in the future), or any other implicit constraints.
 * - The framework will no longer enforce constraints on concurrency of audio recording and
 *   soundtrigger operation, as expressed in the Properties.concurrentCapture field (this property
 *   is merely a hint at this point and may be deprecated in the future).
 * - The HAL implementation is free to reject loading of any model at any time by having the
 *   respective load*() method signal a ServiceSpecificException(RESOURCE_CONTENTION).
 * - The HAL implementation is free to reject starting of any model at any time by having the
 *   respective start*() method signal a ServiceSpecificException(RESOURCE_CONTENTION).
 * - The HAL implementation is free to preemptively stop a previously started model at its own
 *   discretion (for example, if a higher priority use-case which cannot coexist with detection
 *   has been requested). The HAL must notify the framework of the preemption by sending a
 *   recognition event with an `ABORTED` status. The implementation must NOT attempt to restart the
 *   recognition automatically when conditions change.
 * - The HAL implementation is free to preemptively unload a previously loaded model at its own
 *   discretion (for example, if a higher-priority model is being loaded and the two cannot
 *   coexist). When doing so, it must first abort the detection if active (as per above) and then
 *   notify the framework of the unload using the modelUnloaded() callback.
 * - When conditions change, such that a model that couldn't previously load or start or that had
 *   previously been preemptively stopped or unloaded, the HAL must notify the framework via the
 *   newly added onResourcesAvailable() callback. This callback is not a guarantee that any
 *   operation would now succeed, but merely a hint that retrying something that had previously
 *   failed, now MAY succeed. Until this callback is invoked, the client may assume that any
 *   operation that had previously failed or aborted would still fail if retried, so the
 *   implementation should not forget to deliver it.
 *   There are no guarantees regarding how the framework may respond to this event and the order in
 *   which it may choose to reload/restart its models. Typically, as result of this event the
 *   framework will make a single attempt per model to bring this model to its desired state
 *   (loaded, started).
 */
@VintfStability
interface ISoundTriggerHw {
    /**
     * Retrieve implementation properties.
     *
     * @return A Properties structure containing implementation description and capabilities.
     */
    Properties getProperties();

    /**
     * This will get called at most once per every attachment to the service.
     *
     * All events not tied to a specific model should go through this callback.
     *
     * @param callback An interface to receive global event callbacks.
     */
    void registerGlobalCallback(in ISoundTriggerHwGlobalCallback callback);

    /**
     * Load a sound model. Once loaded, recognition of this model can be started and stopped.
     *
     * @param soundModel A SoundModel structure describing the sound model to load.
     * @param callback The callback interface on which the recognitionCallback()
     *     method will be called upon completion and modelUnloaded() upon preemptive unload.
     * @return A unique handle assigned by the HAL for use by the client when controlling
     *     activity for this sound model.
     * @throws ServiceSpecificException(RESOURCE_CONTENTION) if the model cannot be loaded due
     *     to resource constraints. This is typically a temporary condition and the client may
     *     retry after the onResourcesAvailable() global callback is invoked.
     */
    int loadSoundModel(in SoundModel soundModel, in ISoundTriggerHwCallback callback);

    /**
     * Load a key phrase sound model. Once loaded, recognition of this model can be started and
     * stopped.
     *
     * @param soundModel A PhraseSoundModel structure describing the sound model to load.
     * @param callback The callback interface on which the phraseRecognitionCallback() method will
     *     be called upon completion and modelUnloaded() upon preempted unload.
     * @return A unique handle assigned by the HAL for use by the framework when controlling
     *     activity for this sound model.
     * @throws ServiceSpecificException(RESOURCE_CONTENTION) if the model cannot be loaded due
     *     to resource constraints. This is typically a temporary condition and the client may
     *     retry after the onResourcesAvailable() global callback is invoked.
     */
    int loadPhraseSoundModel(in PhraseSoundModel soundModel, in ISoundTriggerHwCallback callback);

    /**
     * Unload a sound model. A sound model may be unloaded to free up resources and make room for a
     * new one to overcome implementation limitations.
     * This call is idempotent, to avoid any race conditions.
     *
     * @param modelHandle the handle of the sound model to unload.
     */
    void unloadSoundModel(in int modelHandle);

    /**
     * Start recognition on a given model.
     * This must be called on a model that is in the stopped state.
     * The state of this model becomes active and will remain so until explicitly stopped, or a
     * recognition event had been delivered to the client.
     *
     * @param modelHandle the handle of the sound model to use for recognition
     * @param deviceHandle The handle of the audio device to be used for recognition, as declared by
     *     the audio subsystem.
     * @param ioHandle A handle assigned by the framework, which will later be used to retrieve
     *     an audio stream associated with this recognition session.
     * @param config A RecognitionConfig structure containing attributes of the recognition to
     *     perform.
     * @throws ServiceSpecificException(RESOURCE_CONTENTION) if the model cannot be started due
     *     to resource constraints. This is typically a temporary condition and the client may
     *     retry after the onResourcesAvailable() global callback is invoked.
     */
    void startRecognition(
            in int modelHandle, in int deviceHandle, in int ioHandle, in RecognitionConfig config);

    /**
     * Stop recognition on a given model.
     * This call is idempotent, to avoid any race conditions.
     *
     * @param modelHandle The handle of the sound model to use for recognition
     */
    void stopRecognition(in int modelHandle);

    /**
     * Request a recognition event to be generated.
     * The model must be in the started state and will remain started after the event is sent.
     * The model state is returned asynchronously as a RecognitionEvent via the callback that was
     * registered upon loading. That event must have a RecognitionStatus.FORCED status.
     *
     * @param modelHandle The handle of the sound model whose state is being
     *     queried.
     */
    void forceRecognitionEvent(in int modelHandle);

    /**
     * Get supported parameter attributes with respect to the provided model handle.
     * Model parameters are used to query/control model-specific detection behavior during a
     * detection session.
     * Along with determining the valid range, this API is also used to determine if a given
     * parameter ID is supported at all by the modelHandle for use with getParameter() and
     * setParameter() APIs.
     *
     * @param modelHandle The sound model handle indicating which model to query.
     * @param modelParam Parameter to set which will be validated against the ModelParameter type.
     * @return This structure indicates supported attributes of the parameter for the given model
     *      handle. If the parameter is not supported, null is returned.
     */
    @nullable ModelParameterRange queryParameter(
            in int modelHandle, in ModelParameter modelParam);

    /**
     * Get a model specific parameter.
     * If the value has not been set, a default value is returned. See ModelParameter for parameter
     * default values.
     * The caller must check if the handle supports the parameter via the queryParameter API prior
     * to calling this method.
     *
     * @param modelHandle The sound model associated with given modelParam
     * @param modelParam Parameter to set which will be validated against the ModelParameter type.
     *     Not putting ModelParameter type directly in the definition and validating internally
     *     allows for forward compatibility.
     * @return Value set to the requested parameter.
     */
    int getParameter(in int modelHandle, in ModelParameter modelParam);

    /**
     * Set a model specific parameter with the given value.
     * This parameter will keep its value for the duration the model is loaded regardless of
     * starting and stopping recognition. Once the model is unloaded, the value will be lost.
     * The caller must check if the handle supports the parameter via the queryParameter API prior
     * to calling this method.
     *
     * @param modelHandle The sound model handle indicating which model to modify parameters
     * @param modelParam Parameter to set which will be validated against the ModelParameter type.
     *     Not putting ModelParameter type directly in the definition and validating internally
     *     allows for forward compatibility.
     * @param value The value to set for the given model parameter.
     */
    void setParameter(in int modelHandle, in ModelParameter modelParam, in int value);
}
