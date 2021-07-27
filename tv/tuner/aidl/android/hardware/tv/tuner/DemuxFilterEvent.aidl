/*
 * Copyright 2021 The Android Open Source Project
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

package android.hardware.tv.tuner;

import android.hardware.tv.tuner.DemuxFilterDownloadEvent;
import android.hardware.tv.tuner.DemuxFilterIpPayloadEvent;
import android.hardware.tv.tuner.DemuxFilterMediaEvent;
import android.hardware.tv.tuner.DemuxFilterMmtpRecordEvent;
import android.hardware.tv.tuner.DemuxFilterMonitorEvent;
import android.hardware.tv.tuner.DemuxFilterPesEvent;
import android.hardware.tv.tuner.DemuxFilterSectionEvent;
import android.hardware.tv.tuner.DemuxFilterTemiEvent;
import android.hardware.tv.tuner.DemuxFilterTsRecordEvent;

/**
 * Filter Event.
 * @hide
 */
@VintfStability
union DemuxFilterEvent {
    DemuxFilterSectionEvent section;

    DemuxFilterMediaEvent media;

    DemuxFilterPesEvent pes;

    DemuxFilterTsRecordEvent tsRecord;

    DemuxFilterMmtpRecordEvent mmtpRecord;

    DemuxFilterDownloadEvent download;

    DemuxFilterIpPayloadEvent ipPayload;

    DemuxFilterTemiEvent temi;

    DemuxFilterMonitorEvent monitorEvent;

    /**
     * An unique ID to mark the start point of receiving the valid filter events after
     * reconfiguring the filter. It must be sent at least once in the first event after the
     * filter is restarted. 0 is reserved for the newly opened filter's first start, which is
     * optional for HAL to send.
     *
     * When sending starId, DemuxFilterEvent.events should only contain one startId event.
     */
    int startId;
}
