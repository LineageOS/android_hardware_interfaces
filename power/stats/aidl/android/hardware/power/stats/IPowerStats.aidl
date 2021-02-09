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

package android.hardware.power.stats;

import android.hardware.power.stats.Channel;
import android.hardware.power.stats.EnergyConsumer;
import android.hardware.power.stats.EnergyConsumerResult;
import android.hardware.power.stats.EnergyMeasurement;
import android.hardware.power.stats.PowerEntity;
import android.hardware.power.stats.StateResidencyResult;

@VintfStability
interface IPowerStats {
    /**
     * Return information related to all supported PowerEntity(s) for which state residency data
     * is available.
     *
     * A PowerEntity is defined as a platform subsystem, peripheral, or power domain that impacts
     * the total device power consumption.
     *
     * @return List of information on each PowerEntity
     */
    PowerEntity[] getPowerEntityInfo();

    /**
     * Reports the accumulated state residency for each requested PowerEntity.
     *
     * Each PowerEntity may reside in one of multiple states. It may also
     * transition from one state to another. StateResidency is defined as
     * an accumulation of time that a PowerEntity resided in each
     * of its possible states, the number of times that each state was
     * entered, and a timestamp corresponding to the last time that state
     * was entered.
     *
     * Data is accumulated starting at device boot.
     *
     * @param powerEntityIds List of IDs of PowerEntities for which data is requested.
     *     Passing an empty list will return state residency for all available PowerEntitys.
     *     ID of each PowerEntity is contained in PowerEntityInfo.
     *
     * @return StateResidency since boot for each requested PowerEntity
     *
     * Returns the following service-specific exceptions in order of highest priority:
     *  - STATUS_BAD_VALUE if an invalid powerEntityId is provided
     *  - STATUS_FAILED_TRANSACTION if any StateResidencyResult fails to be returned
     */
    StateResidencyResult[] getStateResidency(in int[] powerEntityIds);

    /**
     * Return the list EnergyConsumers for which energy consumption data is available.
     *
     * An EnergyConsumer is a device subsystem or peripheral that consumes energy. Energy
     * consumption data may be used by framework for the purpose of power attribution.
     *
     * @return List of EnergyConsumers that are available.
     */
    EnergyConsumer[] getEnergyConsumerInfo();

    /**
     * Reports the energy consumed since boot by each requested EnergyConsumer.
     *
     * @param energyConsumerIds List of IDs of EnergyConsumers for which data is requested.
     *     Passing an empty list will return state residency for all available EnergyConsumers.
     *
     * @return Energy consumed since boot for each requested EnergyConsumer
     *
     * Returns the following service-specific exceptions in order of highest priority:
     *  - STATUS_BAD_VALUE if an invalid energyConsumerId is provided
     *  - STATUS_FAILED_TRANSACTION if any EnergyConsumerResult fails to be returned
     */
    EnergyConsumerResult[] getEnergyConsumed(in int[] energyConsumerIds);

    /**
     * Return information related to all channels monitored by Energy Meters.
     *
     * An Energy Meter is a device that monitors energy and may support monitoring multiple
     * channels simultaneously. A channel may correspond a bus, sense resistor, or power rail.
     *
     * @return Channels monitored by Energy Meters.
     */
    Channel[] getEnergyMeterInfo();

    /**
     * Reports accumulated energy for each specified channel.
     *
     * @param channelIds IDs of channels for which data is requested.
     *     Passing an empty list will return energy measurements for all available channels.
     *     ID of each channel is contained in ChannelInfo.
     *
     * @return Energy measured since boot for each requested channel
     *
     * Returns the following service-specific exceptions in order of highest priority:
     *  - STATUS_BAD_VALUE if an invalid channelId is provided
     *  - STATUS_FAILED_TRANSACTION if any EnergyMeasurement fails to be returned
     */
    EnergyMeasurement[] readEnergyMeter(in int[] channelIds);
}
