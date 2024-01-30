/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.biometrics.face;
/**
 * @hide
 */
@VintfStability
@Backing(type="byte")
enum AcquiredInfo {
    /**
     * Placeholder value used for default initialization of AcquiredInfo. This
     * value means AcquiredInfo wasn't explicitly initialized and must be
     * discarded by the recipient.
     */
    UNKNOWN,

    /**
     * The acquired face data was good, no further user interaction is necessary.
     */
    GOOD,

    /**
     * The acquired face data was too noisy or did not have sufficient detail.
     * This is a catch-all for all acquisition errors not captured by the other
     * constants.
     */
    INSUFFICIENT,

    /**
     * Because there was too much ambient light, the captured face data was too
     * bright. It's reasonable to return this after multiple
     * AcquiredInfo.INSUFFICIENT.
     *
     * The user is expected to take action to retry the operation in better
     * lighting conditions when this is returned.
     */
    TOO_BRIGHT,

    /**
     * Because there was not enough illumination, the captured face data was too
     * dark. It's reasonable to return this after multiple
     * AcquiredInfo.INSUFFICIENT.
     *
     * The user is expected to take action to retry the operation in better
     * lighting conditions when this is returned.
     */
    TOO_DARK,

    /**
     * The detected face is too close to the sensor, and the image cannot be
     * processed.
     *
     * The user is expected to be informed to move further from the sensor when
     * this is returned.
     */
    TOO_CLOSE,

    /**
     * The detected face is too small, as the user might be too far away from
     * the sensor.
     *
     * The user is expected to be informed to move closer to the sensor when
     * this is returned.
     */
    TOO_FAR,

    /**
     * Only the upper part of the face was detected. The sensor's field of view
     * is too high.
     *
     * The user should be informed to move up with respect to the sensor when
     * this is returned.
     */
    FACE_TOO_HIGH,

    /**
     * Only the lower part of the face was detected. The sensor's field of view
     * is too low.
     *
     * The user should be informed to move down with respect to the sensor when
     * this is returned.
     */
    FACE_TOO_LOW,

    /**
     * Only the right part of the face was detected. The sensor's field of view
     * is too far right.
     *
     * The user should be informed to move to the right with respect to the
     * sensor when this is returned.
     */
    FACE_TOO_RIGHT,

    /**
     * Only the left part of the face was detected. The sensor's field of view
     * is too far left.
     *
     * The user should be informed to move to the left with respect to the
     * sensor when this is returned.
     */
    FACE_TOO_LEFT,

    /**
     * The user's eyes have strayed away from the sensor. If this message is
     * sent, the user should be informed to look at the device. If the user
     * can't be found in the frame, one of the other acquisition messages
     * must be sent, e.g. NOT_DETECTED.
     */
    POOR_GAZE,

    /**
     * No face was detected within the sensor's field of view.
     *
     * The user should be informed to point the sensor to a face when this is
     * returned.
     */
    NOT_DETECTED,

    /**
     * Too much motion was detected.
     *
     * The user should be informed to keep their face steady relative to the
     * sensor.
     */
    TOO_MUCH_MOTION,

    /**
     * The sensor needs to be re-calibrated. This is an unexpected condition,
     * and must only be sent if a serious, uncorrectable, and unrecoverable
     * calibration issue is detected which requires user intervention, e.g.
     * re-enrolling. The expected response to this message is to direct the
     * user to re-enroll.
     */
    RECALIBRATE,

    /**
     * The face is too different from a previous acquisition. This condition
     * only applies to enrollment. This can happen if the user passes the
     * device to someone else in the middle of enrollment.
     */
    TOO_DIFFERENT,

    /**
     * The face is too similar to a previous acquisition. This condition only
     * applies to enrollment. The user should change their pose.
     */
    TOO_SIMILAR,

    /**
     * The magnitude of the pan angle of the user’s face with respect to the sensor’s
     * capture plane is too high.
     *
     * The pan angle is defined as the angle swept out by the user’s face turning
     * their neck left and right. The pan angle would be zero if the user faced the
     * camera directly.
     *
     * The user should be informed to look more directly at the camera.
     */
    PAN_TOO_EXTREME,

    /**
     * The magnitude of the tilt angle of the user’s face with respect to the sensor’s
     * capture plane is too high.
     *
     * The tilt angle is defined as the angle swept out by the user’s face looking up
     * and down. The tilt angle would be zero if the user faced the camera directly.
     *
     * The user should be informed to look more directly at the camera.
     */
    TILT_TOO_EXTREME,

    /**
     * The magnitude of the roll angle of the user’s face with respect to the sensor’s
     * capture plane is too high.
     *
     * The roll angle is defined as the angle swept out by the user’s face tilting their head
     * towards their shoulders to the left and right. The roll angle would be zero if the user's
     * head is vertically aligned with the camera.
     *
     * The user should be informed to look more directly at the camera.
     */
    ROLL_TOO_EXTREME,

    /**
     * The user’s face has been obscured by some object.
     *
     * The user should be informed to remove any objects from the line of sight from
     * the sensor to the user’s face.
     */
    FACE_OBSCURED,

    /**
     * This message represents the earliest message sent at the beginning of the authentication
     * pipeline. It is expected to be used to measure latency. For example, in a camera-based
     * authentication system it's expected to be sent prior to camera initialization. The framework
     * will measure latency based on the time between the last START message and the onAuthenticated
     * callback.
     */
    START,

    /**
     * The sensor is dirty. The user should be informed to clean the sensor.
     */
    SENSOR_DIRTY,

    /**
     * Vendor-specific acquisition message. See ISessionCallback#onAcquired vendorCode
     * documentation.
     */
    VENDOR,

    /**
     * The first frame from the camera has been received.
     */
    FIRST_FRAME_RECEIVED,

    /**
     * Dark glasses detected. This can be useful for providing relevant feedback to the user and
     * enabling an alternative authentication logic if the implementation supports it.
     */
    DARK_GLASSES_DETECTED,

    /**
     * A face mask or face covering detected. This can be useful for providing relevant feedback to
     * the user and enabling an alternative authentication logic if the implementation supports it.
     */
    MOUTH_COVERING_DETECTED,
}
