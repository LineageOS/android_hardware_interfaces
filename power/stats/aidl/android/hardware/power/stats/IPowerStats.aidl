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
     * @return List of information on each PowerEntity for which state residency can be requested.
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
     * @return StateResidencyResults since boot for each requested and available PowerEntity. Note
     * that StateResidencyResult for a given PowerEntity may not always be available. Clients shall
     * not rely on StateResidencyResult always being returned for every request.
     *
     * Returns the following exception codes:
     *  - EX_ILLEGAL_ARGUMENT if an invalid powerEntityId is provided
     */
    StateResidencyResult[] getStateResidency(in int[] powerEntityIds);

    /**
     * Return the list EnergyConsumers for which energy consumption data is available.
     *
     * An EnergyConsumer is a device subsystem or peripheral that consumes energy. Energy
     * consumption data may be used by framework for the purpose of power attribution.
     *
     * @return List of EnergyConsumers for which energy consumption can be requested.
     */
    EnergyConsumer[] getEnergyConsumerInfo();

    /**
     * Reports the energy consumed since boot by each requested EnergyConsumer.
     *
     * @param energyConsumerIds List of IDs of EnergyConsumers for which data is requested.
     *     Passing an empty list will return results for all available EnergyConsumers.
     *
     * @return Energy consumed since boot for each requested and available EnergyConsumer. Note
     * that EnergyConsumerResult for a given EnergyConsumer may not always be available. Clients
     * shall not rely on EnergyConsumerResult always being returned for every request.
     *
     * Returns the following exception codes:
     *  - EX_ILLEGAL_ARGUMENT if an invalid energyConsumerId is provided
     */
    EnergyConsumerResult[] getEnergyConsumed(in int[] energyConsumerIds);

    /**
     * Return information related to all Channels monitored by Energy Meters.
     *
     * An Energy Meter is a device that monitors energy and may support monitoring multiple
     * channels simultaneously. A channel may correspond a bus, sense resistor, or power rail.
     *
     * @return All Channels for which energy measurements can be requested.
     */
    Channel[] getEnergyMeterInfo();

    /**
     * Reports accumulated energy for each specified Channel.
     *
     * @param channelIds IDs of channels for which data is requested.
     *     Passing an empty list will return energy measurements for all available channels.
     *     ID of each channel is contained in ChannelInfo.
     *
     * @return Energy measured since boot for each requested and available Channel. Note
     * that EnergyMeasurement for a given Channel may not always be available. Clients
     * shall not rely on EnergyMeasurement always being returned for every request.
     *
     * Returns the following exception codes:
     *  - EX_ILLEGAL_ARGUMENT if an invalid channelId is provided
     */
    EnergyMeasurement[] readEnergyMeter(in int[] channelIds);
}
