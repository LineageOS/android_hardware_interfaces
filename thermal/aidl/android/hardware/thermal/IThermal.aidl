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

package android.hardware.thermal;

import android.hardware.thermal.CoolingDevice;
import android.hardware.thermal.CoolingType;
import android.hardware.thermal.ICoolingDeviceChangedCallback;
import android.hardware.thermal.IThermalChangedCallback;
import android.hardware.thermal.Temperature;
import android.hardware.thermal.TemperatureThreshold;
import android.hardware.thermal.TemperatureType;

/* @hide */
@VintfStability
interface IThermal {
    /**
     * Retrieves the cooling devices information.
     *
     * @return devices If succeed, it's filled with the
     *    current cooling device information. The order of built-in cooling
     *    devices in the list must be kept the same regardless the number
     *    of calls to this method even if they go offline, if these devices
     *    exist on boot. The method always returns and never removes from
     *    the list such cooling devices.
     *
     * @throws EX_ILLEGAL_STATE If the Thermal HAL is not initialized successfully. And the
     *         getMessage() must be populated with human-readable error message.
     */
    CoolingDevice[] getCoolingDevices();

    /**
     * Retrieves the cooling devices information of a given CoolingType.
     *
     * @param type the CoolingDevice such as CPU/GPU.
     *
     * @return devices If succeed, it's filled with the current
     *    cooling device information. The order of built-in cooling
     *    devices in the list must be kept the same regardless of the number
     *    of calls to this method even if they go offline, if these devices
     *    exist on boot. The method always returns and never removes from
     *    the list such cooling devices.
     *
     * @throws EX_ILLEGAL_STATE If the Thermal HAL is not initialized successfully. And the
     *         getMessage() must be populated with human-readable error message.
     */
    CoolingDevice[] getCoolingDevicesWithType(in CoolingType type);

    /**
     * Retrieves temperatures in Celsius.
     *
     * @return temperatures If succeed, it's filled with the
     *    current temperatures. The order of temperatures of built-in
     *    devices (such as CPUs, GPUs and etc.) in the list must be kept
     *    the same regardless the number of calls to this method even if
     *    they go offline, if these devices exist on boot. The method
     *    always returns and never removes such temperatures.
     *
     * @throws EX_ILLEGAL_STATE If the Thermal HAL is not initialized successfully. And the
     *         getMessage() must be populated with human-readable error message.
     */
    Temperature[] getTemperatures();

    /**
     * Retrieves temperatures in Celsius with a given TemperatureType.
     *
     * @param type the TemperatureType such as battery or skin.
     *
     * @return temperatures If succeed, it's filled with the
     *    current temperatures. The order of temperatures of built-in
     *    devices (such as CPUs, GPUs and etc.) in the list must be kept
     *    the same regardless of the number of calls to this method even if
     *    they go offline, if these devices exist on boot. The method
     *    always returns and never removes such temperatures.
     *
     * @throws EX_ILLEGAL_STATE If the Thermal HAL is not initialized successfully. And the
     *         getMessage() must be populated with human-readable error message.
     */
    Temperature[] getTemperaturesWithType(in TemperatureType type);

    /**
     * Retrieves static temperature thresholds in Celsius.
     *
     * @return temperatureThresholds If succeed, it's filled with the
     *    temperatures thresholds. The order of temperatures of built-in
     *    devices (such as CPUs, GPUs and etc.) in the list must be kept
     *    the same regardless of the number of calls to this method even if
     *    they go offline, if these devices exist on boot. The method
     *    always returns and never removes such temperatures. The thresholds
     *    are returned as static values and must not change across calls. The actual
     *    throttling state is determined in device thermal mitigation policy/algorithm
     *    which might not be simple thresholds so these values Thermal HAL provided
     *    may not be accurate to determine the throttling status. To get accurate
     *    throttling status, use getTemperatures or registerThermalChangedCallback
     *    and listen to the callback.
     *
     * @throws EX_ILLEGAL_STATE If the Thermal HAL is not initialized successfully. And the
     *         getMessage() must be populated with human-readable error message.
     */
    TemperatureThreshold[] getTemperatureThresholds();

    /**
     * Retrieves static temperature thresholds in Celsius of a given temperature
     * type.
     *
     * @param type the TemperatureType such as battery or skin.
     *
     * @return temperatureThresholds If succeed, it's filled with the
     *    temperatures thresholds. The order of temperatures of built-in
     *    devices (such as CPUs, GPUs and etc.) in the list must be kept
     *    the same regardless of the number of calls to this method even if
     *    they go offline, if these devices exist on boot. The method
     *    always returns and never removes such temperatures. The thresholds
     *    are returned as static values and must not change across calls. The actual
     *    throttling state is determined in device thermal mitigation policy/algorithm
     *    which might not be simple thresholds so these values Thermal HAL provided
     *    may not be accurate to determine the throttling status. To get accurate
     *    throttling status, use getTemperatures or registerThermalChangedCallback
     *    and listen to the callback.
     *
     * @throws EX_ILLEGAL_STATE If the Thermal HAL is not initialized successfully. And the
     *         getMessage() must be populated with human-readable error message.
     */
    TemperatureThreshold[] getTemperatureThresholdsWithType(in TemperatureType type);

    /**
     * Register an IThermalChangedCallback, used by the Thermal HAL to receive
     * thermal events when thermal mitigation status changed.
     * Multiple registrations with different IThermalChangedCallback must be allowed.
     * Multiple registrations with same IThermalChangedCallback is not allowed, client
     * should unregister the given IThermalChangedCallback first.
     *
     * @param callback the IThermalChangedCallback to use for receiving
     *    thermal events. if nullptr callback is given, the status code will be
     *    STATUS_BAD_VALUE and the operation will fail.
     *
     * @throws EX_ILLEGAL_ARGUMENT If the callback is given nullptr or already registered. And the
     *         getMessage() must be populated with human-readable error message.
     * @throws EX_ILLEGAL_STATE If the Thermal HAL is not initialized successfully. And the
     *         getMessage() must be populated with human-readable error message.
     */
    void registerThermalChangedCallback(in IThermalChangedCallback callback);

    /**
     * Register an IThermalChangedCallback for a given TemperatureType, used by
     * the Thermal HAL to receive thermal events when thermal mitigation status
     * changed.
     * Multiple registrations with different IThermalChangedCallback must be allowed.
     * Multiple registrations with same IThermalChangedCallback is not allowed, client
     * should unregister the given IThermalChangedCallback first.
     *
     * @param callback the IThermalChangedCallback to use for receiving
     *    thermal events. if nullptr callback is given, the status code will be
     *    STATUS_BAD_VALUE and the operation will fail.
     * @param type the type to be filtered.
     *
     * @throws EX_ILLEGAL_ARGUMENT If the callback is given nullptr or already registered. And the
     *         getMessage() must be populated with human-readable error message.
     * @throws EX_ILLEGAL_STATE If the Thermal HAL is not initialized successfully. And the
     *         getMessage() must be populated with human-readable error message.
     */
    void registerThermalChangedCallbackWithType(
            in IThermalChangedCallback callback, in TemperatureType type);

    /**
     * Unregister an IThermalChangedCallback, used by the Thermal HAL
     * to receive thermal events when thermal mitigation status changed.
     *
     * @param callback the IThermalChangedCallback to use for receiving
     *    thermal events. if nullptr callback is given, the status code will be
     *    STATUS_BAD_VALUE and the operation will fail.
     *
     * @throws EX_ILLEGAL_ARGUMENT If the callback is given nullptr or not previously registered.
     *         And the getMessage() must be populated with human-readable error message.
     * @throws EX_ILLEGAL_STATE If the Thermal HAL is not initialized successfully. And the
     *         getMessage() must be populated with human-readable error message.
     */
    void unregisterThermalChangedCallback(in IThermalChangedCallback callback);

    /**
     * Register an ICoolingDeviceChangedCallback for a given CoolingType, used by
     * the Thermal HAL to receive CDEV events when cooling device status
     * changed.
     * Multiple registrations with different ICoolingDeviceChangedCallback must be allowed.
     * Multiple registrations with same ICoolingDeviceChangedCallback is not allowed, client
     * should unregister the given ICoolingDeviceChangedCallback first.
     *
     * @param callback the ICoolingChangedCallback to use for receiving
     *    cooling device events. If nullptr callback is given, the status code will be
     *    STATUS_BAD_VALUE and the operation will fail.
     * @param type the type to be filtered.
     *
     * @throws EX_ILLEGAL_ARGUMENT If the callback is given nullptr or already registered. And the
     *         getMessage() must be populated with human-readable error message.
     * @throws EX_ILLEGAL_STATE If the Thermal HAL is not initialized successfully. And the
     *         getMessage() must be populated with human-readable error message.
     */
    void registerCoolingDeviceChangedCallbackWithType(
            in ICoolingDeviceChangedCallback callback, in CoolingType type);

    /**
     * Unregister an ICoolingDeviceChangedCallback, used by the Thermal HAL
     * to receive CDEV events when cooling device status changed.
     *
     * @param callback the ICoolingDeviceChangedCallback to use for receiving
     *    cooling device events. if nullptr callback is given, the status code will be
     *    STATUS_BAD_VALUE and the operation will fail.
     *
     * @throws EX_ILLEGAL_ARGUMENT If the callback is given nullptr or not previously registered.
     *         And the getMessage() must be populated with human-readable error message.
     * @throws EX_ILLEGAL_STATE If the Thermal HAL is not initialized successfully. And the
     *         getMessage() must be populated with human-readable error message.
     */
    void unregisterCoolingDeviceChangedCallback(in ICoolingDeviceChangedCallback callback);
}
