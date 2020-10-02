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

package android.hardware.powerstats;

import android.hardware.powerstats.ChannelInfo;
import android.hardware.powerstats.EnergyConsumerId;
import android.hardware.powerstats.EnergyConsumerResult;
import android.hardware.powerstats.EnergyMeasurement;
import android.hardware.powerstats.PowerEntityInfo;
import android.hardware.powerstats.StateResidencyResult;

@VintfStability
interface IPowerStats {
    /**
     * Reports information related to all supported PowerEntity(s) for which
     * data is available. A PowerEntity is defined as a platform subsystem,
     * peripheral, or power domain that impacts the total device power
     * consumption.
     *
     * @return List of information on each PowerEntity
     */
    PowerEntityInfo[] getPowerEntityInfo();

    /**
     * Reports accumulated residency data for each specified PowerEntity.
     * Each PowerEntity may reside in one of multiple states. It may also
     * transition from one state to another. StateResidency is defined as
     * an accumulation of time that a PowerEntity resided in each
     * of its possible states, the number of times that each state was
     * entered, and a timestamp corresponding to the last time that state
     * was entered. Data is accumulated starting from the last time the
     * PowerEntity was reset.
     *
     * @param powerEntityIds IDs of PowerEntities for which data is required.
     *     To get data for all PowerEntities pass an empty vector. PowerEntity name to
     *     ID mapping can be queried from getPowerEntityInfo() API.
     * @return StateResidency since boot for all requested PowerEntity(s).
     */
    StateResidencyResult[] getPowerEntityStateResidency(in int[] powerEntityIds);

    /**
     * Reports a list of IDs corresponding to all enabled EnergyConsumers.
     *
     * @return list of EnergyConsumersIds that are available.
     */
    EnergyConsumerId[] getEnergyConsumerInfo();

    /**
     * Returns any available energy consumption results.
     *
     * @param energyConsumerIds IDs of EnergyConsumers for which data is requested.
     *     To get data for all EnergyConsumers pass an empty list.
     * @return List of EnergyConsumerResults reporting energy consumed since boot for each requested
     *     EnergyConsumerId.
     *
     * Returns the following service-specific exceptions:
     *     STATUS_FAILED_TRANSACTION if any of the requested energy results is unavailable
     *     STATUS_BAD_VALUE if an invalid EnergyConsumer Id is provided
     */
    EnergyConsumerResult[] getEnergyConsumed(in EnergyConsumerId[] energyConsumerIds);

    /**
     * Reports channels monitored by Energy Meters.
     * Each channel has a name, which may correspond to the name of a power rail on the device,
     * and an Id which is used to relate EnergyMeasurements returned by readEnergyMeters() with a
     * given ChannelInfo.
     *
     * @return Information about channels monitored by Energy Meters.
     */
    ChannelInfo[] getEnergyMeterInfo();

    /**
     * Reports accumulated energy since boot for each specified channel.
     *
     * @param channelIds IDs of channels for which data is requested.
     *     To get data for all channels pass an empty list. Channel name to
     *     ID mapping can be queried from getEnergyMeterInfo() API.
     * @return Energy measured since boot for all requested channels.
     *
     * Returns the following service-specific exceptions:
     *     STATUS_FAILED_TRANSACTION if any of the requested energy measurements are unavailable
     *     STATUS_BAD_VALUE if an invalid channelId is provided
     */
    EnergyMeasurement[] readEnergyMeters(in int[] channelIds);
}