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

package android.hardware.power;

import android.hardware.power.Boost;
import android.hardware.power.ChannelConfig;
import android.hardware.power.CompositionData;
import android.hardware.power.CompositionUpdate;
import android.hardware.power.CpuHeadroomParams;
import android.hardware.power.CpuHeadroomResult;
import android.hardware.power.GpuHeadroomParams;
import android.hardware.power.GpuHeadroomResult;
import android.hardware.power.IPowerHintSession;
import android.hardware.power.Mode;
import android.hardware.power.SessionConfig;
import android.hardware.power.SessionTag;
import android.hardware.power.SupportInfo;

@VintfStability
interface IPower {
    /**
     * setMode() is called to enable/disable specific hint mode, which
     * may result in adjustment of power/performance parameters of the
     * cpufreq governor and other controls on device side.
     *
     * A particular platform may choose to ignore any mode hint.
     *
     * @param type Mode which is to be enable/disable.
     * @param enabled true to enable, false to disable the mode.
     */
    oneway void setMode(in Mode type, in boolean enabled);

    /**
     * isModeSupported() is called to query if the given mode hint is
     * supported by vendor.
     *
     * @return true if the hint passed is supported on this platform.
     *         If false, setting the mode will have no effect.
     * @param type Mode to be queried
     */
    boolean isModeSupported(in Mode type);

    /**
     * setBoost() indicates the device may need to boost some resources, as the
     * the load is likely to increase before the kernel governors can react.
     * Depending on the boost, it may be appropriate to raise the frequencies of
     * CPU, GPU, memory subsystem, or stop CPU from going into deep sleep state.
     * A particular platform may choose to ignore this hint.
     *
     * @param type Boost type which is to be set with a timeout.
     * @param durationMs The expected duration of the user's interaction, if
     *        known, or 0 if the expected duration is unknown.
     *        a negative value indicates canceling previous boost.
     *        A given platform can choose to boost some time based on durationMs,
     *        and may also pick an appropriate timeout for 0 case.
     */
    oneway void setBoost(in Boost type, in int durationMs);

    /**
     * isBoostSupported() is called to query if the given boost hint is
     * supported by vendor. When returns false, set the boost will have
     * no effect on the platform.
     *
     * @return true if the hint passed is supported on this platform.
     *         If false, setting the boost will have no effect.
     * @param type Boost to be queried
     */
    boolean isBoostSupported(in Boost type);

    /**
     * A Session represents a group of threads with an inter-related workload such that hints for
     * their performance should be considered as a unit. The threads in a given session should be
     * long-life and not created or destroyed dynamically.
     *
     * Each session is expected to have a periodic workload with a target duration for each
     * cycle. The cycle duration is likely greater than the target work duration to allow other
     * parts of the pipeline to run within the available budget. For example, a renderer thread may
     * work at 60hz in order to produce frames at the display's frame but have a target work
     * duration of only 6ms.
     *
     * Creates a session for the given set of threads and sets their initial target work
     * duration.
     *
     * @return  the new session if it is supported on this device, otherwise return with
     *          EX_UNSUPPORTED_OPERATION error if hint session is not supported on this device.
     * @param   tgid The TGID to be associated with this session.
     * @param   uid The UID to be associated with this session.
     * @param   threadIds The list of threads to be associated with this session.
     * @param   durationNanos The desired duration in nanoseconds for this session.
     */
    IPowerHintSession createHintSession(
            in int tgid, in int uid, in int[] threadIds, in long durationNanos);

    /**
     * Get preferred update rate (interval) information for this device. Framework must communicate
     * this rate to Apps, and also ensure the session hint sent no faster than the update rate.
     *
     * @return the preferred update rate in nanoseconds supported by device software. Return with
     *         EX_UNSUPPORTED_OPERATION if hint session is not supported.
     */
    long getHintSessionPreferredRate();

    /**
     * A version of createHintSession that returns an additional bundle of session
     * data, useful to help the session immediately communicate via an FMQ channel
     * for more efficient updates.
     *
     * @return  the new session if it is supported on this device, otherwise return
     *          with EX_UNSUPPORTED_OPERATION error if hint session is not
     *          supported on this device.
     * @param   tgid The TGID to be associated with this session.
     * @param   uid The UID to be associated with this session.
     * @param   threadIds The list of threads to be associated with this session.
     * @param   durationNanos The desired duration in nanoseconds for this session.
     * @param   config Extra session metadata to be returned to the caller.
     */
    IPowerHintSession createHintSessionWithConfig(in int tgid, in int uid, in int[] threadIds,
            in long durationNanos, in SessionTag tag, out SessionConfig config);

    /**
     * Used to get an FMQ channel, per-process. The channel should be unique to
     * that process, and should return the same ChannelConfig if called multiple
     * times from that same process.
     *
     * @return  the channel config if hint sessions are supported on this device,
     *          otherwise return with EX_UNSUPPORTED_OPERATION.
     * @param   tgid The TGID to be associated with this channel.
     * @param   uid The UID to be associated with this channel.
     */
    ChannelConfig getSessionChannel(in int tgid, in int uid);

    /**
     * Used to close a channel once it is no longer needed by a process, or that
     * process dies.
     *
     * @param   tgid The TGID to be associated with this channel.
     * @param   uid The UID to be associated with this channel.
     */
    oneway void closeSessionChannel(in int tgid, in int uid);

    /**
     * Called to get detailed information on the support status of various PowerHAL
     * features, such as hint sessions and specific boosts.
     *
     * @return  a SupportInfo giving detailed support information.
     */
    SupportInfo getSupportInfo();

    /**
     * Provides an estimate of available CPU headroom the device based on past history.
     * <p>
     * @param params params to customize the CPU headroom calculation
     * @throws EX_UNSUPPORTED_OPERATION if the API is unsupported or the request params can't be
     *         served.
     * @throws EX_SECURITY if the TIDs passed in do not belong to the same process.
     * @throws EX_ILLEGAL_STATE if the TIDs passed in do not have the same core affinity setting.
     */
    CpuHeadroomResult getCpuHeadroom(in CpuHeadroomParams params);

    /**
     * Provides an estimate of available GPU headroom the device based on past history.
     * <p>
     * @param params params to customize the GPU headroom calculation
     * @throws EX_UNSUPPORTED_OPERATION if the API is unsupported or the request params can't be
     *         served.
     */
    GpuHeadroomResult getGpuHeadroom(in GpuHeadroomParams params);

    /**
     * Sent to PowerHAL when there are surface-attached sessions being composed,
     * providing FPS and frame timing data that can be used to supplement
     * and validate timing sent via reportActual. This call can be batched,
     * especially in the case of a steady state or low-intensity workload.
     *
     * @param   data The aggregated composition data object.
     */
    oneway void sendCompositionData(in CompositionData[] data);

    /**
     * Sent to inform the HAL about important updates outside of the normal
     * reporting cycle, such as lifecycle updates for displays or FrameProducers.
     *
     * @param   update The aggregated composition update object.
     */
    oneway void sendCompositionUpdate(in CompositionUpdate update);
}
