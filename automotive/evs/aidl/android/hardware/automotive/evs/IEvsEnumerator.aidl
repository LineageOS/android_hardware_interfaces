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

package android.hardware.automotive.evs;

import android.hardware.automotive.evs.CameraDesc;
import android.hardware.automotive.evs.DisplayState;
import android.hardware.automotive.evs.IEvsCamera;
import android.hardware.automotive.evs.IEvsDisplay;
import android.hardware.automotive.evs.IEvsEnumeratorStatusCallback;
import android.hardware.automotive.evs.IEvsUltrasonicsArray;
import android.hardware.automotive.evs.Stream;
import android.hardware.automotive.evs.UltrasonicsArrayDesc;

/**
 * Provides the mechanism for EVS camera and ultrasonics array discovery
 */
@VintfStability
interface IEvsEnumerator {
    /**
     * Return the specified IEvsCamera interface as no longer in use
     *
     * When the IEvsCamera object is no longer required, it must be released.
     * NOTE: Video streaming must be cleanly stopped before making this call.
     *
     * @param in carCamera EvsCamera object to be closed.
     * @throws EvsResult::INVALID_ARG if a given camera object is invalid.
     */
    void closeCamera(in IEvsCamera carCamera);

    /**
     * Return the specified IEvsDisplay interface as no longer in use
     *
     * When the IEvsDisplay object is no longer required, it must be released.
     * NOTE: All buffers must have been returned to the display before making this call.
     *
     * @param in display EvsDisplay object to be closed.
     */
    void closeDisplay(in IEvsDisplay display);

    /**
     * Return the specified IEvsUltrasonicsArray interface as no longer in use
     *
     * When the IEvsUltrasonicsArray object is no longer required, it must be released.
     * NOTE: Data streaming must be cleanly stopped before making this call.
     *
     * @param in evsUltrasonicsArray EvsUltrasonics array object to be closed.
     */
    void closeUltrasonicsArray(in IEvsUltrasonicsArray evsUltrasonicsArray);

    /**
     * Returns a list of all EVS cameras available to the system
     *
     * @return A list of cameras availale for EVS service.
     * @throws EvsResult::PERMISSION_DENIED if the process is not permitted to enumerate
     *        camera devices.
     */
    CameraDesc[] getCameraList();

    /**
     * Returns a list of all EVS displays available to the system
     *
     * @return Identifiers of available displays.
     */
    byte[] getDisplayIdList();

    /**
     * This call requests the current state of the primary display
     *
     * If there is no open display, this returns DisplayState::NOT_OPEN. otherwise, it returns
     * the actual state of the active primary display.  This call is replicated on the
     * IEvsEnumerator interface in order to allow secondary clients to monitor the state of the EVS
     * display without acquiring exclusive ownership of the display.
     *
     * @return Current DisplayState of this Display.
     * @throws EvsResult::OWNERSHIP_LOST if current display is inactive
     *        EvsResult::PERMISSION_DENIED if the process is not permitted to do this operation.
     */
    DisplayState getDisplayState();

    /**
     * Return a list of the stream configurations a target camera device supports
     *
     * @param in description A target camera descriptor
     * @return A list of stream configurations supported by a given camera device
     */
    Stream[] getStreamList(in CameraDesc description);

    /**
     * Returns a list of all ultrasonics array available to the system.
     * Will return an empty vector if ultrasonics is not supported.
     *
     * @return A list of ultrasonics available for EVS service.
     */
    UltrasonicsArrayDesc[] getUltrasonicsArrayList();

    /**
     * Tells whether this is EvsManager or HAL implementation.
     *
     * @return False for EvsManager implementations and true for all others.
     */
    boolean isHardware();

    /**
     * Gets the IEvsCamera associated with a cameraId from a CameraDesc
     *
     * Given a camera's unique cameraId from CameraDesc, returns the
     * IEvsCamera interface associated with the specified camera. When
     * done using the camera, the caller may release it by calling closeCamera().
     *
     * @param in cameraId  A unique identifier of the camera.
     * @param in streamCfg A stream configuration the client wants to use.
     * @return EvsCamera object associated with a given cameraId.
     *         Returned object would be null if a camera device does not support a
     *         given stream configuration or is already configured differently by
     *         another client.
     * @throws EvsResult::PERMISSION_DENIED if the process is not permitted to use camera
     *        devices.
     *        EveResult::INVALID_ARG if it fails to open a camera with a given id.
     */
    IEvsCamera openCamera(in String cameraId, in Stream streamCfg);

    /**
     * Get exclusive access to IEvsDisplay for the system
     *
     * There can be more than one EVS display objects for the system and this function
     * requests access to the display identified by a given ID. If the target EVS display
     * is not available or is already in use the old instance shall be closed and give
     * the new caller exclusive access.
     * When done using the display, the caller may release it by calling closeDisplay().
     *
     * @param in id Target display identifier.
     * @return EvsDisplay object to be used.
     * @throws EvsResult::INVALID_ARG if no display with a given id exists
     */
    IEvsDisplay openDisplay(in int id);

    /**
     * Gets the IEvsUltrasonicsArray associated with a ultrasonicsArrayId from a
     * UltrasonicsDataDesc
     *
     * @param in ultrasonicsArrayId A unique identifier of the ultrasonic array.
     * @return IEvsUltrasonicsArray object associated with a given ultrasonicsArrayId.
     */
    IEvsUltrasonicsArray openUltrasonicsArray(in String ultrasonicsArrayId);

    /**
     * Registers a callback to listen to devices' status changes
     *
     * @param in callback IEvsEnumeratorStatusCallback implementation
     */
    void registerStatusCallback(in IEvsEnumeratorStatusCallback callback);

    /**
     * This call requests the current state of the display
     *
     * If the requested display is not active, this returns DisplayState::NOT_OPEN. otherwise, it
     * returns the actual state of the active display.  This call is replicated on the
     * IEvsEnumerator interface in order to allow secondary clients to monitor the state of the EVS
     * display without acquiring exclusive ownership of the display.
     *
     * @param in id ID of the requested display.
     * @return Current DisplayState of this Display.
     * @throws EvsResult::OWNERSHIP_LOST if current display is inactive
     *        EvsResult::PERMISSION_DENIED if the process is not permitted to do this operation.
     */
    DisplayState getDisplayStateById(in int id);
}
