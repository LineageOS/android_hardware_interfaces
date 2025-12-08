/*
 * Copyright (C) 2025 The Android Open Source Project
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

package android.hardware.bluetooth.gatt;

import android.hardware.bluetooth.gatt.GattCapabilities;
import android.hardware.bluetooth.gatt.GattCharacteristic;
import android.hardware.bluetooth.gatt.IBluetoothGattCallback;
import android.hardware.bluetooth.gatt.Uuid;
import android.hardware.contexthub.EndpointId;

/**
 * The interface for host stack to register callback, to get capabilities, to offload
 * GATT service, and to unoffload GATT service.
 */
@VintfStability
interface IBluetoothGatt {
    /**
     * API to initialize the GATT HAL and to register a callback for receiving asynchronous
     * events.
     *
     * This method is the entry point for interacting with the GATT hardware abstraction layer.
     * Subsequent calls to this method with a different callback object must replace the previously
     * registered one.
     *
     * @param callback An instance of the |IBluetoothGattCallback| AIDL interface object
     */
    void init(in IBluetoothGattCallback callback);

    /**
     * API to retrieve the supported GATT offload capabilities.
     *
     * This method allows to query the underlying low power processor and HAL implementation
     * to determine which GATT offload features are supported on this device.
     *
     * @return A {@link GattCapabilities} object detailing the specific GATT offload features
     * supported by the low power processor.
     */
    GattCapabilities getGattCapabilities();

    /**
     * Represent GATT role. A device can act as either a {@link SERVER} or a {@link CLIENT} on
     * the GATT offload session.
     */
    @VintfStability
    @Backing(type="int")
    enum Role {
        SERVER,
        CLIENT,
    }

    /**
     * API to offload the GATT service to the endpoint for either GATT client or server.
     *
     * This message allows the GATT app to delegate the handling of a subset of the characteristics
     of a GATT service to the endpoint.
     *
     * When used for a GATT client role, this operation must be performed after a service
     * discovery from remote GATT server. This is crucial because all subsequent GATT operations
     depend on the attribute handles of the characteristics. These handles are dynamically assigned
     by the GATT server and are obtained exclusively during the service discovery process.
     *
     * @param sessionId Identifier assigned to the offload session by the host stack. Used to
              uniquely identify the offload session in other callbacks and method
              invocations.
     * @param aclConnectionHandle Handle of the ACL connection over which the GATT service is
              offloaded.
     * @param attMtu Maximum transmission unit for ATT protocol negotiated for this connection.
     * @param role GATT role (SERVER or CLIENT) for which this offload session is being established.
     * @param serviceUuid UUID of the GATT service which {@code characteristics} are associated with
     * @param endpointId Unique identifier for an endpoint at the offload path
     */
    void registerService(in int sessionId, in int aclConnectionHandle, in int attMtu, in Role role,
            in Uuid serviceUuid, in GattCharacteristic[] characteristics, in EndpointId endpointId);

    /**
     * API to unregister a previously offloaded GATT offload session or signal its closure.
     *
     * This API can be invoked under several circumstances, including when the host
     * application explicitly requests to unregister GATT offload session, when the
     * underlying channel disconnects, or when the GATT client or server is unregistered by
     * the Host application.
     *
     * @param sessionId The unique identifier for the GATT session that was previously
     *        assigned when the service was offloaded
     */
    void unregisterService(in int sessionId);

    /**
     * Requests the offload stack to clear the database of offloaded characteristics
     * for the selected ACL Connection.
     *
     * This method is invoked when the host Bluetooth stack detects that the remote GATT
     * server's database has changed and is no longer synchronized with the local copy.
     *
     * The offload stack must ensure that no pending ATT procedure exists for the selected
     * ACL connect before reporting completion through
     * IBluetoothGattCallback.clearServicesComplete().
     *
     * This API will be called when the host stack becomes aware of a change in the remote
     * database through DATABASE_OUT_OF_SYNC errors or Service Change notifications.
     *
     * @param aclConnectionHandle  Handle of the selected ACL connection
     */
    void clearServices(in int aclConnectionHandle);
}
