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

package android.hardware.broadcastradio;

import android.hardware.broadcastradio.AmFmRegionConfig;
import android.hardware.broadcastradio.AnnouncementType;
import android.hardware.broadcastradio.ConfigFlag;
import android.hardware.broadcastradio.DabTableEntry;
import android.hardware.broadcastradio.IAnnouncementListener;
import android.hardware.broadcastradio.ICloseHandle;
import android.hardware.broadcastradio.ITunerCallback;
import android.hardware.broadcastradio.ProgramFilter;
import android.hardware.broadcastradio.ProgramSelector;
import android.hardware.broadcastradio.Properties;
import android.hardware.broadcastradio.VendorKeyValue;

/**
 * Represents a hardware broadcast radio module. A single module may contain
 * multiple hardware tuners (i.e. with an additional background tuner), but the
 * layers above the HAL see them as a single logical unit.
 */
@VintfStability
interface IBroadcastRadio {
    /**
     * Invalid identifier for {@link IBroadcastRadio#getImage}.
     */
    const int INVALID_IMAGE = 0;

    /**
     * If the antenna is disconnected from the beginning, the
     * {@link ITunerCallback#onAntennaStateChange} callback must be
     * called within this time.
     */
    const int ANTENNA_STATE_CHANGE_TIMEOUT_MS = 100;

    /**
     * All chunks of a signal program list update must be transmitted
     * within this time.
     */
    const int LIST_COMPLETE_TIMEOUT_MS = 300000;

    /**
     * All tune, seek and step operations must be completed within
     * this time.
     */
    const int TUNER_TIMEOUT_MS = 30000;

    /**
     * Returns module properties: a description of a module and its
     * capabilities. This method must not fail.
     *
     * @return Module description.
     */
    Properties getProperties();

    /**
     * Fetches current or possible AM/FM region configuration.
     *
     * If the tuner doesn't support AM/FM, a service-specific error
     * {@link Result#NOT_SUPPORTED} will be returned.
     *
     * @param full If {@code true}, returns full hardware capabilities.
     *             If {@code false}, returns current regional configuration.
     * @return config Hardware capabilities (full={@code true}) or current configuration
     *                (full={@code false}).
     */
    AmFmRegionConfig getAmFmRegionConfig(in boolean full);

    /**
     * Fetches current DAB region configuration.
     *
     * If tuner doesn't support DAB, a service-specific error
     * {@link Result#NOT_SUPPORTED} wiil be returned.
     *
     * @return config Current configuration.
     */
    DabTableEntry[] getDabRegionConfig();

    /**
     * Sets callback interface.
     *
     * It is expected that there will only ever be a single callback set.
     * If called when a callback is already set, the existing one should be
     * replaced with the new callback.
     *
     * If the callback to be set is null, a service-specific error
     * {@link Result#INVALID_ARGUMENTS} will be returned; if the callback
     * is not set successfully, a service-specific error
     * {@link Result#NOT_SUPPORTED} should be returned.
     *
     * @param callback The callback interface used for BroadcastRadio HAL.
     */
    void setTunerCallback(in ITunerCallback callback);

    /**
     * Unsets callback interface.
     *
     * The existing callback is set to null.
     */
    void unsetTunerCallback();

    /**
     * Tunes to a specified program.
     *
     * Automatically cancels pending tune(), seek() or step().
     * The method should first check whether tune can be processed by the status
     * of tuner and inputs, schedule tune task, and then return status
     * immediately. If a non-null callback is not set, a service-specific
     * error {@link Result#INVALID_STATE} will be returned; if the program
     * selector doesn't contain any supported identifier, a service-specific error
     * {@link Result#NOT_SUPPORTED} will be returned; if the program selector
     * contains identifiers in invalid format (i.e. out of range), a
     * service-specific error {@link Result#INVALID_ARGUMENTS} will be returned;
     * otherwise, OK will be returned as status. Tune task should be processed
     * asynchronously after the method returns status. If the method returns OK,
     * {@link ITunerCallback#tuneFailed} or
     * {@link ITunerCallback#currentProgramInfoChanged} callback must be called
     * after the tune task completes.
     *
     * @param program Program to tune to.
     */
    void tune(in ProgramSelector program);

    /**
     * Seeks the next valid program on the "air".
     *
     * Advance to the next detected program and stay there.
     *
     * Automatically cancels pending tune(), seek() or step().
     * The method should first check whether seek can be processed by the status
     * of tuner and inputs, schedule seek task, and then return status
     * immediately. If a non-null callback is not set, a service-specific
     * error {@link Result#INVALID_STATE} will be returned; otherwise, OK will
     * be returned as status. Seek task should be processed asynchronously
     * after the method returns status. If the method returns OK,
     * {@link ITunerCallback#tuneFailed} or
     * {@link ITunerCallback#currentProgramInfoChanged} callback must be called
     * after the seek task completes.
     *
     * The skipSubChannel parameter is used to skip digital radio subchannels:
     *  - HD Radio SPS;
     *  - DAB secondary service.
     *
     * As an implementation detail, the HAL has the option to perform an actual
     * seek or select the next program from the list retrieved in the
     * background.
     *
     * @param directionUp {@code true} to change towards higher numeric values
     *                    (frequency, channel number), {@code false} towards
     *                    lower.
     * @param skipSubChannel Don't tune to subchannels.
     */
    void seek(in boolean directionUp, in boolean skipSubChannel);

    /**
     * Steps to the adjacent channel, which may not be occupied by any program.
     *
     * Automatically cancels pending tune(), seek() or step().
     * The method should first check whether step can be processed by the status
     * of tuner and inputs, schedule step task, and then return status
     * immediately. If a non-null callback is not set, service-specific
     * error {@link Result#INVALID_STATE} will be returned; if tuning to an
     * unoccupied channel is not supported (i.e. for satellite radio), a
     * service-specific error {@link Result#NOT_SUPPORTED} will be returned;
     * otherwise, OK should be returned as status. Step task should be
     * processed asynchronously after the method returns status. If the
     * method returns OK, {@link ITunerCallback#tuneFailed} or
     * {@link currentProgramInfoChanged} callback must be called after the
     * step task completes.
     *
     * @param directionUp {@code true} to change towards higher numeric values
     *                    (frequency, channel number), {@code false} towards lower.
     */
    void step(in boolean directionUp);

    /**
     * Cancels pending tune(), seek() or step().
     *
     * If there is no such operation running, the call can be ignored.
     * If cancel is called after the HAL completes an operation (tune, seek, and step)
     * and before the callback completions, the cancel can be ignored and the callback
     * should complete.
     */
    void cancel();

    /**
     * Applies a filter to the program list and starts sending program list
     * update over {@link ITunerCallback#onProgramListUpdated} callback.
     *
     * There may be only one updates stream active at the moment. Calling this
     * method again must result in cancelling the pending update request.
     *
     * This call clears the program list on the client side, the HAL must send
     * the whole list again.
     *
     * If the program list scanning hardware (i.e. background tuner) is
     * unavailable at the moment, the call must succeed and start updates
     * when it becomes available.
     *
     * If the program list scanning is not supported by the hardware, a
     * service-specific error {@link Result#NOT_SUPPORTED} will be returned.
     *
     * @param filter Filter to apply on the fetched program list.
     */
    void startProgramListUpdates(in ProgramFilter filter);

    /**
     * Stops sending program list updates.
     *
     * If stopProgramListUpdates is called after the HAL completes a program list update
     * and before the onCurrentProgramInfoChanged callback completions,
     * stopProgramListUpdates can be ignored and the callback should complete.
     */
    void stopProgramListUpdates();

    /**
     * Fetches the current setting of a given config flag.
     *
     * The success/failure result must be consistent with setConfigFlag.
     *
     * If the flag is not applicable, a service-specific error
     * {@link Result#INVALID_STATE} will be returned. If the flag is not
     * supported at all, a service-specific error {@link Result#NOT_SUPPORTED}
     * will be returned.
     *
     * @return the current value of the flag, if succeed.
     */
    boolean isConfigFlagSet(in ConfigFlag flag);

    /**
     * Sets the config flag.
     *
     * The success/failure result must be consistent with isConfigFlagSet.
     *
     * If the flag is not applicable, a service-specific error
     * {@link Result#INVALID_STATE} will be returned. If the flag is not
     * supported at all, a service-specific error {@link Result#NOT_SUPPORTED}
     * will be returned.
     *
     * @param flag Flag to set.
     * @param value The new value of a given flag.
     */
    void setConfigFlag(in ConfigFlag flag, in boolean value);

    /**
     * Generic method for setting vendor-specific parameter values.
     * The framework does not interpret the parameters, they are passed
     * in an opaque manner between a vendor application and HAL.
     *
     * Framework does not make any assumptions on the keys or values, other than
     * ones stated in VendorKeyValue documentation (a requirement of key
     * prefixes).
     *
     * For each pair in the result array, the key must be one of the keys
     * contained in the input (possibly with wildcards expanded), and the value
     * must be a vendor-specific result status (i.e. the string "OK" or an error
     * code). The implementation may choose to return an empty array, or only
     * return a status for a subset of the provided inputs, at its discretion.
     *
     * Application and HAL must not use keys with unknown prefix. In particular,
     * it must not place a key-value pair in results array for unknown key from
     * parameters array - instead, an unknown key should simply be ignored.
     * In other words, results array may contain a subset of parameter keys
     * (however, the framework doesn't enforce a strict subset - the only
     * formal requirement is vendor domain prefix for keys).
     *
     * @param parameters Vendor-specific key-value pairs.
     * @return Operation completion status for parameters being set.
     */
    VendorKeyValue[] setParameters(in VendorKeyValue[] parameters);

    /**
     * Generic method for retrieving vendor-specific parameter values.
     * The framework does not interpret the parameters, they are passed
     * in an opaque manner between a vendor application and HAL.
     *
     * Framework does not cache set/get requests, so it's allowed for
     * getParameter to return a different value than previous setParameter call.
     *
     * The syntax and semantics of keys are up to the vendor (as long as prefix
     * rules are obeyed). For instance, vendors may include some form of
     * wildcard support. In such case, result array may be of different size
     * than requested keys array. However, wildcards are not recognized by
     * framework and they are passed as-is to the HAL implementation.
     *
     * Unknown keys must be ignored and not placed into results array.
     *
     * @param keys Parameter keys to fetch.
     * @return Vendor-specific key-value pairs.
     */
    VendorKeyValue[] getParameters(in String[] keys);

    /**
     * Fetches image from radio module cache.
     *
     * This is out-of-band transport mechanism for images carried with metadata.
     * The metadata array only passes the identifier, so the client may cache
     * images or even not fetch them.
     *
     * The identifier may be any arbitrary number (i.e. sha256 prefix) selected
     * by the vendor. It must be stable so the application may cache it.
     *
     * The data must be a valid PNG, JPEG, GIF or BMP file, and must be less
     * than 1MB, due to hard limit on binder transaction buffer.
     *
     * Image data with an invalid format must be handled gracefully in the same
     * way as a missing image.
     *
     * The image identifier may become invalid after some time from passing it
     * with metadata struct (due to resource cleanup at the HAL implementation).
     * However, it must remain valid for a currently tuned program at least
     * until onCurrentProgramInfoChanged is called.
     *
     * @param id Identifier of an image (value of {@link IBroadcastRadio#INVALID_IMAGE}
     *           is reserved and must be treated as invalid image).
     * @return A binary blob with image data
     *         or a zero-length array if identifier doesn't exist.
     */
    byte[] getImage(in int id);

    /**
     * Registers announcement listener.
     *
     * If there is at least one observer registered, HAL implementation must
     * notify about announcements.
     *
     * If the observer dies, the HAL implementation must unregister observer
     * automatically.
     *
     * If the tuner doesn't support announcements, a service-specific error
     * {@link Result#NOT_SUPPORTED} will be returned.
     *
     * @param listener The listener interface.
     * @param enabled The list of announcement types to watch for.
     * @return a handle to unregister observer.
     */
    ICloseHandle registerAnnouncementListener(
            in IAnnouncementListener listener, in AnnouncementType[] enabled);
}
