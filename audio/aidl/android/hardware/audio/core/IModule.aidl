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

package android.hardware.audio.core;

import android.hardware.audio.common.SinkMetadata;
import android.hardware.audio.common.SourceMetadata;
import android.hardware.audio.core.AudioPatch;
import android.hardware.audio.core.AudioRoute;
import android.hardware.audio.core.IStreamIn;
import android.hardware.audio.core.IStreamOut;
import android.media.audio.common.AudioOffloadInfo;
import android.media.audio.common.AudioPort;
import android.media.audio.common.AudioPortConfig;

/**
 * Each instance of IModule corresponds to a separate audio module. The system
 * (the term "system" as used here applies to the entire device running Android)
 * may have multiple modules due to the physical architecture, for example, it
 * can have multiple DSPs or other audio I/O units which are not interconnected
 * in hardware directly. Usually there is at least one audio module which is
 * responsible for the "main" (or "built-in") audio functionality of the
 * system. Even if the system lacks any physical audio I/O capabilities, there
 * will be a "null" audio module.
 *
 * On a typical mobile phone there is usually a main DSP module which handles
 * most of the phone's audio I/O via the built-in speakers and microphones. USB
 * audio can exist as a separate module. Some audio modules can be implemented
 * purely in software, for example, the remote submix module.
 */
@VintfStability
interface IModule {
    /**
     * Return all audio patches of this module.
     *
     * Returns a list of audio patches, that is, established connections between
     * audio port configurations.
     *
     * @return The list of audio patches.
     */
    AudioPatch[] getAudioPatches();

    /**
     * Return the current state of the audio port.
     *
     * Using the port ID provided on input, returns the current state of the
     * audio port. For device port representing a connection to some external
     * device, e.g. over HDMI or USB, currently supported audio profiles and
     * extra audio descriptors may change.
     *
     * For all other audio ports it must be the same configuration as returned
     * for this port ID by 'getAudioPorts'.
     *
     * @return The current state of an audio port.
     * @param portId The ID of the audio port.
     * @throws EX_ILLEGAL_ARGUMENT If the port can not be found by the ID.
     */
    AudioPort getAudioPort(int portId);

    /**
     * Return all active audio port configurations of this module.
     *
     * Returns a list of active configurations that are currently set for mix
     * ports and device ports. Each returned configuration must have an unique
     * ID within this module ('AudioPortConfig.id' field), which can coincide
     * with an ID of an audio port, if the port only supports a single active
     * configuration. Each returned configuration must also have a reference to
     * an existing port ('AudioPortConfig.portId' field). All optional
     * (nullable) fields of the configurations must be initialized by the HAL
     * module.
     *
     * @return The list of active audio port configurations.
     */
    AudioPortConfig[] getAudioPortConfigs();

    /**
     * Return all audio ports provided by this module.
     *
     * Returns a list of all mix ports and device ports provided by this
     * module. Each returned port must have a unique ID within this module
     * ('AudioPort.id' field). The returned list must not change during
     * the lifetime of the IModule instance. For audio ports with dynamic
     * profiles (changing depending on external devices being connected
     * to the system) an empty list of profiles must be returned. The list
     * of currently supported audio profiles is obtained from 'getAudioPort'
     * method.
     *
     * @return The list of audio ports.
     */
    AudioPort[] getAudioPorts();

    /**
     * Return all audio routes of this module.
     *
     * Returns a list of audio routes, that is, allowed connections between
     * audio ports. The returned list must not change during the lifetime of the
     * IModule instance.
     *
     * @return The list of audio routes.
     */
    AudioRoute[] getAudioRoutes();

    /**
     * Open an input stream using an existing audio mix port configuration.
     *
     * The audio port configuration ID must be obtained by calling
     * 'setAudioPortConfig' method. Existence of an audio patch involving this
     * port configuration is not required for successful opening of a stream.
     *
     * Only one stream is allowed per audio port configuration. HAL module can
     * also set a limit on how many output streams can be opened for a particular
     * mix port by using its 'AudioPortMixExt.maxOpenStreamCount' field.
     *
     * @return An opened input stream.
     * @param portConfigId The ID of the audio mix port config.
     * @param sinkMetadata Description of the audio that will be recorded.
     * @throws EX_ILLEGAL_ARGUMENT In the following cases:
     *                             - If the port config can not be found by the ID.
     *                             - If the port config is not of an input mix port.
     * @throws EX_ILLEGAL_STATE In the following cases:
     *                          - If the port config already has a stream opened on it.
     *                          - If the limit on the open stream count for the port has
     *                            been reached.
     */
    IStreamIn openInputStream(int portConfigId, in SinkMetadata sinkMetadata);

    /**
     * Open an output stream using an existing audio mix port configuration.
     *
     * The audio port configuration ID must be obtained by calling
     * 'setAudioPortConfig' method. Existence of an audio patch involving this
     * port configuration is not required for successful opening of a stream.
     *
     * If the port configuration has 'COMPRESS_OFFLOAD' output flag set,
     * the framework must provide additional information about the encoded
     * audio stream in 'offloadInfo' argument.
     *
     * Only one stream is allowed per audio port configuration. HAL module can
     * also set a limit on how many output streams can be opened for a particular
     * mix port by using its 'AudioPortMixExt.maxOpenStreamCount' field.
     * Only one stream can be opened on the audio port with 'PRIMARY' output
     * flag. This rule can not be overridden with 'maxOpenStreamCount' field.
     *
     * @return An opened output stream.
     * @param portConfigId The ID of the audio mix port config.
     * @param sourceMetadata Description of the audio that will be played.
     * @param offloadInfo Additional information for offloaded playback.
     * @throws EX_ILLEGAL_ARGUMENT In the following cases:
     *                             - If the port config can not be found by the ID.
     *                             - If the port config is not of an output mix port.
     * @throws EX_ILLEGAL_STATE In the following cases:
     *                          - If the port config already has a stream opened on it.
     *                          - If the limit on the open stream count for the port has
     *                            been reached.
     *                          - If another opened stream already exists for the 'PRIMARY'
     *                            output port.
     */
    IStreamOut openOutputStream(int portConfigId, in SourceMetadata sourceMetadata,
            in @nullable AudioOffloadInfo offloadInfo);

    /**
     * Set an audio patch.
     *
     * This method creates new or updates an existing audio patch. If the
     * requested audio patch does not have a specified id, then a new patch is
     * created and an ID is allocated for it by the HAL module. Otherwise an
     * attempt to update an existing patch is made. It is recommended that
     * updating of an existing audio patch should be performed by the HAL module
     * in a way that does not interrupt active audio streams involving audio
     * port configurations of the patch. If the HAL module is unable to avoid
     * interruption when updating a certain patch, it is permitted to allocate a
     * new patch ID for the result. The returned audio patch contains all the
     * information about the new or updated audio patch.
     *
     * Audio port configurations specified in the patch must be obtained by
     * calling 'setAudioPortConfig' method. There must be an audio route which
     * allows connection between the audio ports whose configurations are used.
     * An audio patch may be created before or after an audio steam is created
     * for this configuration.
     *
     * @return Resulting audio patch.
     * @param requested Requested audio patch.
     * @throws EX_ILLEGAL_ARGUMENT In the following cases:
     *                             - If the patch is invalid (see AudioPatch).
     *                             - If a port config can not be found from the specified IDs.
     *                             - If there are no routes satisfying the patch.
     *                             - If an existing patch can not be found by the ID.
     * @throws EX_ILLEGAL_STATE In the following cases:
     *                          - If application of the patch can only use a route with an
     *                            exclusive use the sink port, and it is already patched.
     * @throws EX_UNSUPPORTED_OPERATION If the patch can not be established because
     *                                  the HAL module does not support this otherwise valid
     *                                  patch configuration. For example, if it's a patch
     *                                  between multiple sources and sinks, and the HAL module
     *                                  does not support this.
     */
    AudioPatch setAudioPatch(in AudioPatch requested);

    /**
     * Set the active configuration of an audio port.
     *
     * This method is used to create or update an active configuration for a mix
     * port or a device port. The port is specified using the
     * 'AudioPortConfig.portId' field. If the requested audio port
     * configuration does not have a specified id in the 'AudioPortConfig.id'
     * field, then a new configuration is created and an ID is allocated for it
     * by the HAL module. Otherwise an attempt to update an existing port
     * configuration is made. The HAL module returns the resulting audio port
     * configuration. Depending on the port and on the capabilities of the HAL
     * module, it can either update an existing port configuration (same port
     * configuration ID remains), or create a new one. The resulting port
     * configuration ID is returned in the 'id' field of the 'suggested'
     * argument.
     *
     * If the specified port configuration can not be set, this method must
     * return 'false' and provide its own suggestion in the output
     * parameter. The framework can then set the suggested configuration on a
     * subsequent retry call to this method.
     *
     * @return Whether the requested configuration has been applied.
     * @param requested Requested audio port configuration.
     * @param suggested Same as requested configuration, if it was applied.
     *                  Suggested audio port configuration if the requested
     *                  configuration can't be applied.
     * @throws EX_ILLEGAL_ARGUMENT In the following cases:
     *                             - If neither port config ID, nor port ID are specified.
     *                             - If an existing port config can not be found by the ID.
     *                             - If the port can not be found by the port ID.
     *                             - If it is not possible to generate a suggested port
     *                               configuration, for example, if the port only has dynamic
     *                               profiles and they are currently empty.
     */
    boolean setAudioPortConfig(in AudioPortConfig requested, out AudioPortConfig suggested);

    /**
     * Reset the audio patch.
     *
     * Resets previously created audio patch using its ID ('AudioPatch.id').  It
     * is allowed to reset a patch which uses audio port configurations having
     * associated streams. In this case the mix port becomes disconnected from
     * the hardware, but the stream does not close.
     *
     * @param patchId The ID of the audio patch.
     * @throws EX_ILLEGAL_ARGUMENT If an existing patch can not be found by the ID.
     */
    void resetAudioPatch(int patchId);

    /**
     * Reset the audio port configuration.
     *
     * Resets the specified audio port configuration, discarding all changes
     * previously done by the framework. That means, if a call to this method is
     * a success, the effect of all previous calls to 'setAudioPortConfig' which
     * used or initially have generated the provided 'portConfigId', since the
     * module start, or since the last call to this method, has been canceled.
     *
     * Audio port configurations of mix ports with streams opened on them can
     * not be reset. Also can not be reset port configurations currently used by
     * any patches.
     *
     * @param portConfigId The ID of the audio port config.
     * @throws EX_ILLEGAL_ARGUMENT If the port config can not be found by the ID.
     * @throws EX_ILLEGAL_STATE In the following cases:
     *                          - If the port config has a stream opened on it;
     *                          - If the port config is used by a patch.
     */
    void resetAudioPortConfig(int portConfigId);
}
