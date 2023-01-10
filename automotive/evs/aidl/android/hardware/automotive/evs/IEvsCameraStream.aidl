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
import android.hardware.automotive.evs.EvsEventDesc;
import android.hardware.graphics.common.HardwareBuffer;

/**
 * Implemented on client side to receive asynchronous streaming event deliveries.
 */
@VintfStability
oneway interface IEvsCameraStream {
    /**
     * Receives calls from the HAL each time video frames is ready for inspection.
     * Buffer handles received by this method must be returned via calls to
     * IEvsCamera::doneWithFrame(). When the video stream is stopped via a call
     * to IEvsCamera::stopVideoStream(), this callback may continue to happen for
     * some time as the pipeline drains. Each frame must still be returned.
     * When the last frame in the stream has been delivered, STREAM_STOPPED
     * event must be delivered.  No further frame deliveries may happen
     * thereafter.
     *
     * A camera device will deliver the same number of frames as number of
     * backing physical camera devices; it means, a physical camera device
     * sends always a single frame and a logical camera device sends multiple
     * frames as many as number of backing physical camera devices.
     *
     * @param in buffer Buffer descriptors of delivered image frames.
     */
    void deliverFrame(in BufferDesc[] buffer);

    /**
     * Receives calls from the HAL each time an event happens.
     *
     * @param in event EVS event with possible event information.  If ths HIDL
     *                 recipients are expected to exist, the size of the event
     *                 payload must not exceed 16 bytes; otherwise, a notification
     *                 will not reach them.
     */
    void notify(in EvsEventDesc event);
}
