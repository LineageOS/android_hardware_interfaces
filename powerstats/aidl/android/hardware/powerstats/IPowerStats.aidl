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

import android.hardware.powerstats.EnergyData;
import android.hardware.powerstats.PowerEntityInfo;
import android.hardware.powerstats.PowerEntityStateResidencyResult;
import android.hardware.powerstats.PowerEntityStateSpace;
import android.hardware.powerstats.RailInfo;

@VintfStability
interface IPowerStats {
    const int SUCCESS = 0;
    const int NOT_SUPPORTED = 1;
    const int INVALID_INPUT = 2;
    const int FILESYSTEM_ERROR = 3;
    const int INSUFFICIENT_RESOURCES = 4;

    /**
     * Rail level energy measurements for low frequency clients:
     * Reports accumulated energy since boot on each rail.
     *
     * @param railIndices Indices of rails for which data is required.
     *     To get data for all rails pass an empty vector. Rail name to
     *     index mapping can be queried from getRailInfo() API.
     * @return Energy values since boot for all requested rails.
     */
    EnergyData[] getEnergyData(in int[] railIndices);

    /**
     * PowerEntity information:
     * Reports information related to all supported PowerEntity(s) for which
     * data is available. A PowerEntity is defined as a platform subsystem,
     * peripheral, or power domain that impacts the total device power
     * consumption.
     *
     * @return List of information on each PowerEntity
     */
    PowerEntityInfo[] getPowerEntityInfo();

    /**
     * PowerEntity state information:
     * Reports the set of power states for which the specified
     * PowerEntity(s) provide residency data.
     *
     * @param powerEntityIds collection of IDs of PowerEntity(s) for which
     *     state information is requested. PowerEntity name to ID mapping may
     *     be queried from getPowerEntityInfo(). To get state space
     *     information for all PowerEntity(s) pass an empty vector.
     *
     * @return PowerEntity state space information for
     *     each specified PowerEntity that provides state space information.
     */
    PowerEntityStateSpace[] getPowerEntityStateInfo(in int[] powerEntityIds);

    /**
     * PowerEntity residencies for low frequency clients:
     * Reports accumulated residency data for each specified PowerEntity.
     * Each PowerEntity may reside in one of multiple states. It may also
     * transition to another state. Residency data is an accumulation of time
     * that a specified PowerEntity resided in each of its possible states,
     * the number of times that each state was entered, and a timestamp
     * corresponding to the last time that state was entered. Data is
     * accumulated starting from the last time the PowerEntity was reset.
     *
     * @param powerEntityId collection of IDs of PowerEntity(s) for which
     *     residency data is requested. PowerEntity name to ID mapping may
     *     be queried from getPowerEntityInfo(). To get state residency
     *     data for all PowerEntity(s) pass an empty vector.
     * @return state residency data for each specified
     *     PowerEntity that provides state residency data.
     */
    PowerEntityStateResidencyResult[] getPowerEntityStateResidencyData(in int[] powerEntityIds);

    /**
     * Rail information:
     * Reports information related to the rails being monitored.
     *
     * @return Information about monitored rails.
     */
    RailInfo[] getRailInfo();
}