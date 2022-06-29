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

import android.hardware.automotive.evs.BufferDesc;
import android.hardware.automotive.evs.DisplayDesc;
import android.hardware.automotive.evs.DisplayState;

/**
 * Represents a single display.
 */
@VintfStability
interface IEvsDisplay {
    /**
     * Returns the description of this display.
     *
     * @return The information of this display including id, current mode, current state,
     *         and additional vendor-specific information.
     * @throws EvsResult::UNDERLYING_SERVICE_ERROR if it fails to read a display information.
     */
    DisplayDesc getDisplayInfo();

    /**
     * This call requests the current state of the display
     *
     * The HAL implementation should report the actual current state, which might
     * transiently differ from the most recently requested state. Note, however, that
     * the logic responsible for changing display states should generally live above
     * the device layer, making it undesirable for the HAL implementation to spontaneously
     * change display states.
     *
     * @return Current DisplayState of this Display.
     */
    DisplayState getDisplayState();

    /**
     * This call returns a handle to a frame buffer associated with the display.
     *
     * @return A handle to a frame buffer.  The returned buffer may be locked and
     *         written to by software and/or GL.  This buffer must be returned via
     *         a call to returnTargetBufferForDisplay() even if the display is no
     *         longer visible.
     * @throws EvsResult::OWNERSHIP_LOST if a display is in DisplayState::DEAD.
     *        EvsResult::BUFFER_NOT_AVAILABLE if no buffer is available.
     *        EvsResult::UNDERLYING_SERVICE_ERROR for any other failures.
     */
    BufferDesc getTargetBuffer();

    /**
     * This call tells the display that the buffer is ready for display.
     *
     * The buffer is no longer valid for use by the client after this call.
     * There is no maximum time the caller may hold onto the buffer before making this
     * call. The buffer may be returned at any time and in any DisplayState, but all
     * buffers are expected to be returned before the IEvsDisplay interface is destroyed.
     *
     * @param in buffer A buffer handle to the frame that is ready for display.
     * @throws EvsResult::INVALID_ARG if a given buffer is unknown or invalid.
     *        EvsResult::OWNERSHIP_LOST if a display is in DisplayState::DEAD.
     *        EvsResult::UNDERLYING_SERVICE_ERROR for any other failures.
     */
    void returnTargetBufferForDisplay(in BufferDesc buffer);

    /**
     * Clients may set the display state to express their desired state.
     *
     * The HAL implementation must gracefully accept a request for any state while in
     * any other state, although the response may be to defer or ignore the request. The display
     * is defined to start in the NOT_VISIBLE state upon initialization. The client is
     * then expected to request the VISIBLE_ON_NEXT_FRAME state, and then begin providing
     * video. When the display is no longer required, the client is expected to request
     * the NOT_VISIBLE state after passing the last video frame.
     * Returns INVALID_ARG if the requested state is not a recognized value.
     *
     * @param in state Desired new DisplayState.
     * @throws EvsResult::INVALID_ARG if a given state is invalid.
     *        EvsResult::OWNERSHIP_LOST if a display is in DisplayState::DEAD.
     */
    void setDisplayState(in DisplayState state);
}
