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

package android.hardware.broadcastradio;

import android.hardware.broadcastradio.Metadata;
import android.hardware.broadcastradio.ProgramIdentifier;
import android.hardware.broadcastradio.ProgramSelector;
import android.hardware.broadcastradio.VendorKeyValue;

/**
 * Program (channel, station) information.
 *
 * Carries both user-visible information (like station name) and technical
 * details (tuning selector).
 */
@VintfStability
@JavaDerive(equals=true, toString=true)
parcelable ProgramInfo {
    /**
     * Set when the program is currently playing live stream.
     * This may result in a slightly altered reception parameters,
     * usually targeted at reduced latency.
     */
    const int FLAG_LIVE = 1 << 0;

    /**
     * Radio stream is not playing, ie. due to bad reception conditions or
     * buffering. In this state volume knob MAY be disabled to prevent user
     * increasing volume too much.
     */
    const int FLAG_MUTED = 1 << 1;

    /**
     * Station broadcasts traffic information regularly,
     * but not necessarily right now.
     */
    const int FLAG_TRAFFIC_PROGRAM = 1 << 2;

    /**
     * Station is broadcasting traffic information at the very moment.
     */
    const int FLAG_TRAFFIC_ANNOUNCEMENT = 1 << 3;

    /**
     * Station can be tuned to (not playing static).
     *
     * It's the same condition that would stop a seek operation
     * (i.e. {@link IBroadcastRadio#seek}).
     *
     * By definition, this flag must be set for all items on the program list.
     */
    const int FLAG_TUNABLE = 1 << 4;

    /**
     * Audio stream is MONO if this bit is not set.
     */
    const int FLAG_STEREO = 1 << 5;

    /**
     * A signal has been acquired if this bit is set.
     */

    const int FLAG_SIGNAL_ACQUISITION = 1 << 6;
    /**
     * An HD Station Information Service (SIS) information is available if this
     * bit is set.
     */

    const int FLAG_HD_SIS_ACQUISITION = 1 << 7;

    /**
     * An HD digital audio is available if this bit is set.
     */
    const int FLAG_HD_AUDIO_ACQUISITION = 1 << 8;

    /**
     * An identifier used to point at the program (primarily to tune to it).
     *
     * This field is required - its type field must not be set to
     * {@link IdentifierType#INVALID}.
     */
    ProgramSelector selector;

    /**
     * Identifier currently used for program selection.
     *
     * It allows to determine which technology is currently used for reception.
     *
     * Some program selectors contain tuning information for different radio
     * technologies (i.e. FM RDS and DAB). For example, user may tune using
     * a ProgramSelector with RDS_PI primary identifier, but the tuner hardware
     * may choose to use DAB technology to make actual tuning. This identifier
     * must reflect that.
     *
     * This field is required for currently tuned program only.
     * For all other items on the program list, its type field must be
     * initialized to {@link IdentifierType#INVALID}.
     *
     * Only primary identifiers for a given radio technology are valid:
     *  - AMFM_FREQUENCY_KHZ for analog AM/FM;
     *  - RDS_PI for FM RDS;
     *  - HD_STATION_ID_EXT;
     *  - DAB_SID_EXT;
     *  - DRMO_SERVICE_ID;
     *  - SXM_SERVICE_ID;
     *  - VENDOR_*;
     *  - more might come in next minor versions of this HAL.
     */
    ProgramIdentifier logicallyTunedTo;

    /**
     * Identifier currently used by hardware to physically tune to a channel.
     *
     * Some radio technologies broadcast the same program on multiple channels,
     * i.e. with RDS AF the same program may be broadcasted on multiple
     * alternative frequencies; the same DAB program may be broadcast on
     * multiple ensembles. This identifier points to the channel to which the
     * radio hardware is physically tuned to.
     *
     * This field is required for currently tuned program only.
     * For all other items on the program list, its type field must be
     * initialized to {@link IdentifierType#INVALID}.
     *
     * Only physical identifiers are valid:
     *  - AMFM_FREQUENCY_KHZ;
     *  - DAB_FREQUENCY_KHZ;
     *  - DRMO_FREQUENCY_KHZ;
     *  - SXM_CHANNEL;
     *  - VENDOR_*;
     *  - more might come in next minor versions of this HAL.
     */
    ProgramIdentifier physicallyTunedTo;

    /**
     * Primary identifiers of related contents.
     *
     * Some radio technologies provide pointers to other programs that carry
     * related content (i.e. DAB soft-links). This field is a list of pointers
     * to other programs on the program list.
     *
     * This is not a list of programs that carry the same content (i.e.
     * DAB hard-links, RDS AF). Switching to programs from this list usually
     * require user action.
     *
     * Please note, that these identifiers do not have to exist on the program
     * list - i.e. DAB tuner may provide information on FM RDS alternatives
     * despite not supporting FM RDS. If the system has multiple tuners, another
     * one may have it on its list.
     *
     * This field is optional.
     */
    @nullable ProgramIdentifier[] relatedContent;

    /**
     * Program flags.
     *
     * It can be a combination of {@link #FLAG_LIVE}, {@link #FLAG_MUTED},
     * {@link #FLAG_TRAFFIC_PROGRAM}, {@link #FLAG_TRAFFIC_ANNOUNCEMENT},
     * {@link #FLAG_TUNABLE}, {@link #FLAG_STEREO}, {@link #FLAG_SIGNAL_ACQUISITION},
     * {@link #FLAG_HD_SIS_ACQUISITION}, and {@link #FLAG_HD_AUDIO_ACQUISITION}.
     */
    int infoFlags;

    /**
     * Signal quality measured in 0% to 100% range to be shown in the UI.
     */
    int signalQuality;

    /**
     * Program metadata (station name, PTY, song title).
     */
    Metadata[] metadata;

    /**
     * Vendor-specific information.
     *
     * It may be used for extra features, not supported by the platform,
     * for example: paid-service=true; bitrate=320kbps.
     */
    VendorKeyValue[] vendorInfo;
}
