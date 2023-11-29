/*
 * Copyright (C) 2023 The Android Open Source Project
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

package android.hardware.automotive.vehicle;

/**
 * Used to enumerate the current state of driver drowsiness and attention monitoring.
 *
 * This enum could be extended in future releases to include additional feature states.
 */
@VintfStability
@Backing(type="int")
enum DriverDrowsinessAttentionState {
    /**
     * This state is used as an alternative for any DriverDrowsinessAttentionState value that is not
     * defined in the platform. Ideally, implementations of
     * VehicleProperty#DRIVER_DROWSINESS_ATTENTION_STATE should not use this state. The framework
     * can use this field to remain backwards compatible if DriverDrowsinessAttentionState is
     * extended to include additional states.
     */
    OTHER = 0,
    /**
     * Karolinska Sleepiness Scale Rating 1 described as extermely alert.
     */
    KSS_RATING_1_EXTREMELY_ALERT = 1,
    /**
     * Karolinska Sleepiness Scale Rating 2 described as very alert.
     */
    KSS_RATING_2_VERY_ALERT = 2,
    /**
     * Karolinska Sleepiness Scale Rating 3 described as alert.
     */
    KSS_RATING_3_ALERT = 3,
    /**
     * Karolinska Sleepiness Scale Rating 4 described as rather alert.
     */
    KSS_RATING_4_RATHER_ALERT = 4,
    /**
     * Karolinska Sleepiness Scale Rating 5 described as neither alert nor sleepy.
     */
    KSS_RATING_5_NEITHER_ALERT_NOR_SLEEPY = 5,
    /**
     * Karolinska Sleepiness Scale Rating 6 described as some signs of sleepiness.
     */
    KSS_RATING_6_SOME_SLEEPINESS = 6,
    /**
     * Karolinska Sleepiness Scale Rating 7 described as sleepy with no effort to
     * keep awake.
     */
    KSS_RATING_7_SLEEPY_NO_EFFORT = 7,
    /**
     * Karolinska Sleepiness Scale Rating 8 described as sleepy with some effort to
     * keep awake.
     */
    KSS_RATING_8_SLEEPY_SOME_EFFORT = 8,
    /**
     * Karolinska Sleepiness Scale Rating 9 described as very sleepy, with great
     * effort to keep away, and fighthing sleep.
     */
    KSS_RATING_9_VERY_SLEEPY = 9,
}
