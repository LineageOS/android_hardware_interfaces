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

package android.hardware.health;

/*
 * Disk statistics since boot.
 *
 * See {@code struct disk_stats} in {@code storaged} for interpretations of these fields.
 *
 * All integers in this struct must be interpreted as unsigned.
 */
@VintfStability
parcelable DiskStats {
    /**
     * Number of reads processed.
     *
     * Value must be interpreted as unsigned.
     */
    long reads;
    /**
     * number of read I/Os merged with in-queue I/Os.
     *
     * Value must be interpreted as unsigned.
     */
    long readMerges;
    /**
     * number of sectors read.
     *
     * Value must be interpreted as unsigned.
     */
    long readSectors;
    /**
     * total wait time for read requests.
     *
     * Value must be interpreted as unsigned.
     */
    long readTicks;
    /**
     * number of writes processed.
     *
     * Value must be interpreted as unsigned.
     */
    long writes;
    /**
     * number of writes merged with in-queue I/Os.
     *
     * Value must be interpreted as unsigned.
     */
    long writeMerges;
    /**
     * number of sectors written.
     *
     * Value must be interpreted as unsigned.
     */
    long writeSectors;
    /**
     * total wait time for write requests.
     *
     * Value must be interpreted as unsigned.
     */
    long writeTicks;
    /**
     * number of I/Os currently in flight.
     *
     * Value must be interpreted as unsigned.
     */
    long ioInFlight;
    /**
     * total time this block device has been active.
     *
     * Value must be interpreted as unsigned.
     */
    long ioTicks;
    /**
     * total wait time for all requests.
     *
     * Value must be interpreted as unsigned.
     */
    long ioInQueue;
}
