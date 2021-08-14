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

package android.hardware.gnss;

import android.hardware.gnss.CorrelationVector;
import android.hardware.gnss.GnssMultipathIndicator;
import android.hardware.gnss.GnssSignalType;
import android.hardware.gnss.SatellitePvt;

/**
 * Represents a GNSS Measurement, it contains raw and computed information.
 *
 * All signal measurement information (e.g. svTime, pseudorangeRate, multipathIndicator) reported in
 * this struct must be based on GNSS signal measurements only. You must not synthesize measurements
 * by calculating or reporting expected measurements based on known or estimated position, velocity,
 * or time.
 */
@VintfStability
parcelable GnssMeasurement {
    /** Bit mask indicating a valid 'snr' is stored in the GnssMeasurement. */
    const int HAS_SNR = 1 << 0;
    /** Bit mask indicating a valid 'carrier frequency' is stored in the GnssMeasurement. */
    const int HAS_CARRIER_FREQUENCY = 1 << 9;
    /** Bit mask indicating a valid 'carrier cycles' is stored in the GnssMeasurement. */
    const int HAS_CARRIER_CYCLES = 1 << 10;
    /** Bit mask indicating a valid 'carrier phase' is stored in the GnssMeasurement. */
    const int HAS_CARRIER_PHASE = 1 << 11;
    /** Bit mask indicating a valid 'carrier phase uncertainty' is stored in the GnssMeasurement. */
    const int HAS_CARRIER_PHASE_UNCERTAINTY = 1 << 12;
    /** Bit mask indicating a valid automatic gain control is stored in the GnssMeasurement. */
    const int HAS_AUTOMATIC_GAIN_CONTROL = 1 << 13;
    /** Bit mask indicating a valid full inter-signal bias is stored in the GnssMeasurement. */
    const int HAS_FULL_ISB = 1 << 16;
    /**
     * Bit mask indicating a valid full inter-signal bias uncertainty is stored in the
     * GnssMeasurement.
     */
    const int HAS_FULL_ISB_UNCERTAINTY = 1 << 17;
    /**
     * Bit mask indicating a valid satellite inter-signal bias is stored in the GnssMeasurement.
     */
    const int HAS_SATELLITE_ISB = 1 << 18;
    /**
     * Bit mask indicating a valid satellite inter-signal bias uncertainty is stored in the
     * GnssMeasurement.
     */
    const int HAS_SATELLITE_ISB_UNCERTAINTY = 1 << 19;
    /**
     * Bit mask indicating a valid satellite PVT is stored in the GnssMeasurement.
     */
    const int HAS_SATELLITE_PVT = 1 << 20;
    /**
     * Bit mask indicating valid correlation vectors are stored in the GnssMeasurement.
     */
    const int HAS_CORRELATION_VECTOR = 1 << 21;

    /**
     * A bitfield of flags indicating the validity of the fields in this GnssMeasurement. The bit
     * masks are defined in the constants with prefix HAS_*
     *
     * Fields for which there is no corresponding flag must be filled in with a valid value.  For
     * convenience, these are marked as mandatory.
     *
     * Others fields may have invalid information in them, if not marked as valid by the
     * corresponding bit in flags.
     */
    int flags;

    /**
     * Satellite vehicle ID number, as defined in GnssSvInfo::svid
     *
     * This value is mandatory.
     */
    int svid;

    /**
     * Defines the constellation of the given SV.
     *
     * This value is mandatory.
     */
    GnssSignalType signalType;

    /**
     * Time offset at which the measurement was taken in nanoseconds.
     * The reference receiver's time is specified by GnssData::clock::timeNs.
     *
     * The sign of timeOffsetNs is given by the following equation:
     *      measurement time = GnssClock::timeNs + timeOffsetNs
     *
     * It provides an individual time-stamp for the measurement, and allows
     * sub-nanosecond accuracy. It may be zero if all measurements are
     * aligned to a common time.
     *
     * This value is mandatory.
     */
    double timeOffsetNs;

    /**
     * Flags indicating the GNSS measurement state.
     *
     * The expected behavior here is for GNSS HAL to set all the flags that apply. For example, if
     * the state for a satellite is only C/A code locked and bit synchronized, and there is still
     * millisecond ambiguity, the state must be set as:
     *
     * STATE_CODE_LOCK | STATE_BIT_SYNC |  STATE_MSEC_AMBIGUOUS
     *
     * If GNSS is still searching for a satellite, the corresponding state must be set to
     * STATE_UNKNOWN(0).
     *
     * The received satellite time is relative to the beginning of the system week for all
     * constellations except for Glonass where it is relative to the beginning of the Glonass system
     * day.
     *
     * The table below indicates the valid range of the received GNSS satellite time.  These ranges
     * depend on the constellation and code being tracked and the state of the tracking algorithms
     * given by the getState method. If the state flag is set, then the valid measurement range is
     * zero to the value in the table. The state flag with the widest range indicates the range of
     * the received GNSS satellite time value.
     *
     * +---------------------------+--------------------+-----+-----------+--------------------+------+
     * |                           |GPS/QZSS            |GLNS |BDS        |GAL                 |SBAS  |
     * +---------------------------+------+------+------+-----+------+----+------+------+------+------+
     * |State Flag	               |L1    |L5I   |L5Q   |L1OF |B1I   |B1I |E1B   |E1C   |E5AQ  |L1    |
     * |                           |C/A   |      |      |     |(D1)  |(D2)|      |      |      |C/A   |
     * |---------------------------+------+------+------+-----+------+----+------+------+------+------+
     * |STATE_UNKNOWN              |0     |0     |0     |0    |0     |0   |0     |0     |0     |0     |
     * |---------------------------+------+------+------+-----+------+----+------+------+------+------+
     * |STATE_CODE_LOCK            |1ms   |1 ms  |1 ms  |1 ms |1 ms  |1 ms|-     |-     |1 ms  |1 ms  |
     * |---------------------------+------+------+------+-----+------+----+------+------+------+------+
     * |STATE_SYMBOL_SYNC          |20ms  |10 ms |1 ms  |10 ms|20 ms |2 ms|4 ms  |4 ms  |1 ms  |2 ms  |
     * |                           |(opt.)|      |(opt.)|     |(opt.)|    |(opt.)|(opt.)|(opt.)|      |
     * |---------------------------+------+------+------+-----+------+----+------+------+------+------+
     * |STATE_BIT_SYNC             |20 ms |20 ms |1 ms  |20 ms|20 ms |-   |8 ms  |-     |1 ms  |4 ms  |
     * |                           |      |      |(opt.)|     |      |    |      |      |(opt.)|      |
     * |---------------------------+------+------+------+-----+------+----+------+------+------+------+
     * |STATE_SUBFRAME_SYNC        |6s    |6s    |-     |2 s  |6 s   |-   |-     |-     |100 ms|-     |
     * |---------------------------+------+------+------+-----+------+----+------+------+------+------+
     * |STATE_TOW_DECODED          |1 week|-     |-     |1 day|1 week|-   |1 week|-     |-     |1 week|
     * |---------------------------+------+------+------+-----+------+----+------+------+------+------+
     * |STATE_TOW_KNOWN            |1 week|-     |-     |1 day|1 week|-   |1 week|-     |-     |1 week|
     * |---------------------------+------+------+------+-----+------+----+------+------+------+------+
     * |STATE_GLO_STRING_SYNC      |-     |-     |-     |2 s  |-     |-   |-     |-     |-     |-     |
     * |---------------------------+------+------+------+-----+------+----+------+------+------+------+
     * |STATE_GLO_TOD_DECODED      |-     |-     |-     |1 day|-     |-   |-     |-     |-     |-     |
     * |---------------------------+------+------+------+-----+------+----+------+------+------+------+
     * |STATE_GLO_TOD_KNOWN        |-     |-     |-     |1 day|-     |-   |-     |-     |-     |-     |
     * |---------------------------+------+------+------+-----+------+----+------+------+------+------+
     * |STATE_BDS_D2_BIT_SYNC      |-     |-     |-     |-    |-     |2 ms|-     |-     |-     |-     |
     * |---------------------------+------+------+------+-----+------+----+------+------+------+------+
     * |STATE_BDS_D2_SUBFRAME_SYNC |-     |-     |-     |-    |-     |600 |-     |-     |-     |-     |
     * |                           |      |      |      |     |      |ms  |      |      |      |      |
     * |---------------------------+------+------+------+-----+------+----+------+------+------+------+
     * |STATE_GAL_E1BC_CODE_LOCK   |-     |-     |-     |-    |-     |-   |4 ms  |4 ms  |-     |-     |
     * |---------------------------+------+------+------+-----+------+----+------+------+------+------+
     * |STATE_GAL_E1C_2ND_CODE_LOCK|-     |-     |-     |-    |-     |-   |-     |100 ms|-     |-     |
     * |---------------------------+------+------+------+-----+------+----+------+------+------+------+
     * |STATE_2ND_CODE_LOCK        |-     |10 ms |20 ms |-    |-     |-	  |-     |100 ms|100 ms|-     |
     * |                           |      |(opt.)|      |     |      |    |      |(opt.)|      |      |
     * |---------------------------+------+------+------+-----+------+----+------+------+------+------+
     * |STATE_GAL_E1B_PAGE_SYNC    |-     |-     |-     |-    |-     |-   |2 s   |-     |-     |-     |
     * |---------------------------+------+------+------+-----+------+----+------+------+------+------+
     * |STATE_SBAS_SYNC            |-     |-     |-     |-    |-     |-   |-     |-     |-     |1s    |
     * +---------------------------+------+------+------+-----+------+----+------+------+------+------+
     *
     * Note: TOW Known refers to the case where TOW is possibly not decoded over the air but has
     * been determined from other sources. If TOW decoded is set then TOW Known must also be set.
     *
     * Note well: if there is any ambiguity in integer millisecond, STATE_MSEC_AMBIGUOUS must be
     * set accordingly, in the 'state' field.  This value must be populated if 'state' !=
     * STATE_UNKNOWN.
     *
     * Note on optional flags:
     *   - For L1 C/A and B1I, STATE_SYMBOL_SYNC is optional since the symbol length is the
     *     same as the bit length.
     *   - For L5Q and E5aQ, STATE_BIT_SYNC and STATE_SYMBOL_SYNC are optional since they are
     *     implied by STATE_CODE_LOCK.
     *   - STATE_2ND_CODE_LOCK for L5I is optional since it is implied by STATE_SYMBOL_SYNC.
     *   - STATE_2ND_CODE_LOCK for E1C is optional since it is implied by
     *     STATE_GAL_E1C_2ND_CODE_LOCK.
     *   - For E1B and E1C, STATE_SYMBOL_SYNC is optional, because it is implied by
     *     STATE_GAL_E1BC_CODE_LOCK.
     */
    const int STATE_UNKNOWN = 0;
    const int STATE_CODE_LOCK = 1 << 0;
    const int STATE_BIT_SYNC = 1 << 1;
    const int STATE_SUBFRAME_SYNC = 1 << 2;
    const int STATE_TOW_DECODED = 1 << 3;
    const int STATE_MSEC_AMBIGUOUS = 1 << 4;
    const int STATE_SYMBOL_SYNC = 1 << 5;
    const int STATE_GLO_STRING_SYNC = 1 << 6;
    const int STATE_GLO_TOD_DECODED = 1 << 7;
    const int STATE_BDS_D2_BIT_SYNC = 1 << 8;
    const int STATE_BDS_D2_SUBFRAME_SYNC = 1 << 9;
    const int STATE_GAL_E1BC_CODE_LOCK = 1 << 10;
    const int STATE_GAL_E1C_2ND_CODE_LOCK = 1 << 11;
    const int STATE_GAL_E1B_PAGE_SYNC = 1 << 12;
    const int STATE_SBAS_SYNC = 1 << 13;
    const int STATE_TOW_KNOWN = 1 << 14;
    const int STATE_GLO_TOD_KNOWN = 1 << 15;
    const int STATE_2ND_CODE_LOCK = 1 << 16;

    /**
     * A bitfield of flags indicating the GnssMeasurementState per satellite sync state. It
     * represents the current sync state for the associated satellite.
     *
     * Based on the sync state, the 'received GNSS tow' field must be interpreted accordingly.
     *
     * The bit masks are defined in the constants with prefix STATE_.
     *
     * This value is mandatory.
     */
    int state;

    /**
     * The received GNSS Time-of-Week at the measurement time, in nanoseconds.
     * For GNSS & QZSS, this is the received GNSS Time-of-Week at the
     * measurement time, in nanoseconds. The value is relative to the
     * beginning of the current GNSS week.
     *
     * Given the highest sync state that can be achieved, per each satellite,
     * valid range for this field can be:
     * Searching       : [ 0       ] : STATE_UNKNOWN
     * C/A code lock   : [ 0 1ms   ] : STATE_CODE_LOCK set
     * Bit sync        : [ 0 20ms  ] : STATE_BIT_SYNC set
     * Subframe sync   : [ 0  6s   ] : STATE_SUBFRAME_SYNC set
     * TOW decoded     : [ 0 1week ] : STATE_TOW_DECODED set
     * TOW Known       : [ 0 1week ] : STATE_TOW_KNOWN set
     *
     * Note: TOW Known refers to the case where TOW is possibly not decoded
     * over the air but has been determined from other sources. If TOW
     * decoded is set then TOW Known must also be set.
     *
     * Note: If there is any ambiguity in integer millisecond,
     * STATE_MSEC_AMBIGUOUS must be set accordingly, in the
     * 'state' field.
     *
     * This value must be populated if 'state' != STATE_UNKNOWN.
     *
     * For Glonass, this is the received Glonass time of day, at the
     * measurement time in nanoseconds.
     *
     * Given the highest sync state that can be achieved, per each satellite,
     * valid range for this field can be:
     * Searching           : [ 0       ] : STATE_UNKNOWN set
     * C/A code lock       : [ 0   1ms ] : STATE_CODE_LOCK set
     * Symbol sync         : [ 0  10ms ] : STATE_SYMBOL_SYNC set
     * Bit sync            : [ 0  20ms ] : STATE_BIT_SYNC set
     * String sync         : [ 0    2s ] : STATE_GLO_STRING_SYNC set
     * Time of day decoded : [ 0  1day ] : STATE_GLO_TOD_DECODED set
     * Time of day known   : [ 0  1day ] : STATE_GLO_TOD_KNOWN set
     *
     * Note: Time of day known refers to the case where it is possibly not
     * decoded over the air but has been determined from other sources. If
     * Time of day decoded is set then Time of day known must also be set.
     *
     * For Beidou, this is the received Beidou time of week,
     * at the measurement time in nanoseconds.
     *
     * Given the highest sync state that can be achieved, per each satellite,
     * valid range for this field can be:
     * Searching            : [ 0       ] : STATE_UNKNOWN set.
     * C/A code lock        : [ 0   1ms ] : STATE_CODE_LOCK set.
     * Bit sync (D2)        : [ 0   2ms ] : STATE_BDS_D2_BIT_SYNC set.
     * Bit sync (D1)        : [ 0  20ms ] : STATE_BIT_SYNC set.
     * Subframe (D2)        : [ 0  0.6s ] : STATE_BDS_D2_SUBFRAME_SYNC set.
     * Subframe (D1)        : [ 0    6s ] : STATE_SUBFRAME_SYNC set.
     * Time of week decoded : [ 0 1week ] : STATE_TOW_DECODED set.
     * Time of week known   : [ 0 1week ] : STATE_TOW_KNOWN set
     *
     * Note: TOW Known refers to the case where TOW is possibly not decoded
     * over the air but has been determined from other sources. If TOW
     * decoded is set then TOW Known must also be set.
     *
     * For Galileo, this is the received Galileo time of week,
     * at the measurement time in nanoseconds.
     *
     * E1BC code lock       : [ 0  4ms ] : STATE_GAL_E1BC_CODE_LOCK set.
     * E1C 2nd code lock    : [ 0 100ms] : STATE_GAL_E1C_2ND_CODE_LOCK set.
     * E1B page             : [ 0   2s ] : STATE_GAL_E1B_PAGE_SYNC set.
     * Time of week decoded : [ 0 1week] : STATE_TOW_DECODED is set.
     * Time of week known   : [ 0 1week] : STATE_TOW_KNOWN set
     *
     * Note: TOW Known refers to the case where TOW is possibly not decoded
     * over the air but has been determined from other sources. If TOW
     * decoded is set then TOW Known must also be set.
     *
     * For SBAS, this is received SBAS time, at the measurement time in
     * nanoseconds.
     *
     * Given the highest sync state that can be achieved, per each satellite,
     * valid range for this field can be:
     * Searching    : [ 0     ] : STATE_UNKNOWN
     * C/A code lock: [ 0 1ms ] : STATE_CODE_LOCK is set
     * Symbol sync  : [ 0 2ms ] : STATE_SYMBOL_SYNC is set
     * Message      : [ 0  1s ] : STATE_SBAS_SYNC is set
     */
    long receivedSvTimeInNs;

    /**
     * 1-Sigma uncertainty of the Received GNSS Time-of-Week in nanoseconds.
     *
     * This value must be populated if 'state' != STATE_UNKNOWN.
     */
    long receivedSvTimeUncertaintyInNs;

    /**
     * Carrier-to-noise density in dB-Hz, typically in the range [0, 63].
     * It contains the measured C/N0 value for the signal at the antenna port.
     *
     * If a signal has separate components (e.g. Pilot and Data channels) and
     * the receiver only processes one of the components, then the reported
     * antennaCN0DbHz reflects only the component that is processed.
     *
     * This value is mandatory.
     */
    double antennaCN0DbHz;

    /**
     * Baseband Carrier-to-noise density in dB-Hz, typically in the range [0, 63]. It contains the
     * measured C/N0 value for the signal measured at the baseband.
     *
     * This is typically a few dB weaker than the value estimated for C/N0 at the antenna port,
     * which is reported in cN0DbHz.
     *
     * If a signal has separate components (e.g. Pilot and Data channels) and the receiver only
     * processes one of the components, then the reported basebandCN0DbHz reflects only the
     * component that is processed.
     *
     * This value is mandatory.
     */
    double basebandCN0DbHz;

    /**
     * Pseudorange rate at the timestamp in m/s. The correction of a given
     * Pseudorange Rate value includes corrections for receiver and satellite
     * clock frequency errors. Ensure that this field is independent (see
     * comment at top of GnssMeasurement struct.)
     *
     * It is mandatory to provide the 'uncorrected' 'pseudorange rate', and
     * provide GnssClock's 'drift' field as well. When providing the
     * uncorrected pseudorange rate, do not apply the corrections described above.)
     *
     * The value includes the 'pseudorange rate uncertainty' in it.
     * A positive 'uncorrected' value indicates that the SV is moving away from
     * the receiver.
     *
     * The sign of the 'uncorrected' 'pseudorange rate' and its relation to the
     * sign of 'doppler shift' is given by the equation:
     *      pseudorange rate = -k * doppler shift   (where k is a constant)
     *
     * This must be the most accurate pseudorange rate available, based on
     * fresh signal measurements from this channel.
     *
     * It is mandatory that this value be provided at typical carrier phase PRR
     * quality (few cm/sec per second of uncertainty, or better) - when signals
     * are sufficiently strong & stable, e.g. signals from a GNSS simulator at >=
     * 35 dB-Hz.
     */
    double pseudorangeRateMps;

    /**
     * 1-Sigma uncertainty of the pseudorangeRateMps.
     * The uncertainty is represented as an absolute (single sided) value.
     *
     * This value is mandatory.
     */
    double pseudorangeRateUncertaintyMps;

    /**
     * Flags indicating the Accumulated Delta Range's states.
     *
     * See the table below for a detailed interpretation of each state.
     *
     * +---------------------+-------------------+-----------------------------+
     * | ADR_STATE           | Time of relevance | Interpretation              |
     * +---------------------+-------------------+-----------------------------+
     * | UNKNOWN             | ADR(t)            | No valid carrier phase      |
     * |                     |                   | information is available    |
     * |                     |                   | at time t.                  |
     * +---------------------+-------------------+-----------------------------+
     * | VALID               | ADR(t)            | Valid carrier phase         |
     * |                     |                   | information is available    |
     * |                     |                   | at time t. This indicates   |
     * |                     |                   | that this measurement can   |
     * |                     |                   | be used as a reference for  |
     * |                     |                   | future measurements.        |
     * |                     |                   | However, to compare it to   |
     * |                     |                   | previous measurements to    |
     * |                     |                   | compute delta range,        |
     * |                     |                   | other bits should be        |
     * |                     |                   | checked. Specifically, it   |
     * |                     |                   | can be used for delta range |
     * |                     |                   | computation if it is valid  |
     * |                     |                   | and has no reset or cycle   |
     * |                     |                   | slip at this epoch i.e.     |
     * |                     |                   | if VALID_BIT == 1 &&        |
     * |                     |                   | CYCLE_SLIP_BIT == 0 &&      |
     * |                     |                   | RESET_BIT == 0.             |
     * +---------------------+-------------------+-----------------------------+
     * | RESET               | ADR(t) - ADR(t-1) | Carrier phase accumulation  |
     * |                     |                   | has been restarted between  |
     * |                     |                   | current time t and previous |
     * |                     |                   | time t-1. This indicates    |
     * |                     |                   | that this measurement can   |
     * |                     |                   | be used as a reference for  |
     * |                     |                   | future measurements, but it |
     * |                     |                   | should not be compared to   |
     * |                     |                   | previous measurements to    |
     * |                     |                   | compute delta range.        |
     * +---------------------+-------------------+-----------------------------+
     * | CYCLE_SLIP          | ADR(t) - ADR(t-1) | Cycle slip(s) have been     |
     * |                     |                   | detected between the        |
     * |                     |                   | current time t and previous |
     * |                     |                   | time t-1. This indicates    |
     * |                     |                   | that this measurement can   |
     * |                     |                   | be used as a reference for  |
     * |                     |                   | future measurements.        |
     * |                     |                   | Clients can use a           |
     * |                     |                   | measurement with a cycle    |
     * |                     |                   | slip to compute delta range |
     * |                     |                   | against previous            |
     * |                     |                   | measurements at their own   |
     * |                     |                   | risk.                       |
     * +---------------------+-------------------+-----------------------------+
     * | HALF_CYCLE_RESOLVED | ADR(t)            | Half cycle ambiguity is     |
     * |                     |                   | resolved at time t.         |
     * |                     |                   |                             |
     * |                     |                   | For signals that have       |
     * |                     |                   | databits, the carrier phase |
     * |                     |                   | tracking loops typically    |
     * |                     |                   | use a costas loop           |
     * |                     |                   | discriminator. This type of |
     * |                     |                   | tracking loop introduces a  |
     * |                     |                   | half-cycle ambiguity that   |
     * |                     |                   | is resolved by searching    |
     * |                     |                   | through the received data   |
     * |                     |                   | for known patterns of       |
     * |                     |                   | databits (e.g. GPS uses the |
     * |                     |                   | TLM word) which then        |
     * |                     |                   | determines the polarity of  |
     * |                     |                   | the incoming data and       |
     * |                     |                   | resolves the half-cycle     |
     * |                     |                   | ambiguity.                  |
     * |                     |                   |                             |
     * |                     |                   | Before the half-cycle       |
     * |                     |                   | ambiguity has been resolved |
     * |                     |                   | it is possible that the     |
     * |                     |                   | ADR_STATE_VALID flag is     |
     * |                     |                   | set, but the ADR_STATE_     |
     * |                     |                   | HALF_CYCLE_RESOLVED flag is |
     * |                     |                   | not set.                    |
     * +---------------------+-------------------+-----------------------------+
     */
    const int ADR_STATE_UNKNOWN = 0;
    const int ADR_STATE_VALID = 1 << 0;
    const int ADR_STATE_RESET = 1 << 1;
    const int ADR_STATE_CYCLE_SLIP = 1 << 2;
    const int ADR_STATE_HALF_CYCLE_RESOLVED = 1 << 3;

    /**
     * A bitfield of flags indicating the accumulated delta range's state. It indicates whether ADR
     * is reset or there is a cycle slip(indicating loss of lock).
     *
     * The bit masks are defined in constants with prefix ADR_STATE_.
     *
     * This value is mandatory.
     */
    int accumulatedDeltaRangeState;

    /**
     * Accumulated delta range since the last channel reset in meters.
     * A positive value indicates that the SV is moving away from the receiver.
     *
     * The sign of the 'accumulated delta range' and its relation to the sign of
     * 'carrier phase' is given by the equation:
     * accumulated delta range = -k * carrier phase (where k is a constant)
     *
     * This value must be populated if 'accumulated delta range state' !=
     * ADR_STATE_UNKNOWN.
     * However, it is expected that the data is only accurate when:
     *      'accumulated delta range state' == ADR_STATE_VALID.
     *
     * The alignment of the phase measurement will not be  adjusted by the receiver so the in-phase
     * and quadrature phase components will have a quarter cycle offset as they do when transmitted
     * from the satellites. If the measurement is from a combination of the in-phase and quadrature
     * phase components, then the alignment of the phase measurement will be aligned to the in-phase
     * component.
     */
    double accumulatedDeltaRangeM;

    /**
     * 1-Sigma uncertainty of the accumulated delta range in meters.
     * This value must be populated if 'accumulated delta range state' !=
     * ADR_STATE_UNKNOWN.
     */
    double accumulatedDeltaRangeUncertaintyM;

    /**
     * The number of full carrier cycles between the satellite and the
     * receiver. The reference frequency is given by the field
     * 'carrierFrequencyHz'. Indications of possible cycle slips and
     * resets in the accumulation of this value can be inferred from the
     * accumulatedDeltaRangeState flags.
     *
     * If the data is available, gnssMeasurementFlags must contain
     * HAS_CARRIER_CYCLES.
     */
    long carrierCycles;

    /**
     * The RF phase detected by the receiver, in the range [0.0, 1.0].
     * This is usually the fractional part of the complete carrier phase
     * measurement.
     *
     * The reference frequency is given by the field 'carrierFrequencyHz'.
     * The value contains the 'carrier-phase uncertainty' in it.
     *
     * If the data is available, gnssMeasurementFlags must contain
     * HAS_CARRIER_PHASE.
     */
    double carrierPhase;

    /**
     * 1-Sigma uncertainty of the carrier-phase.
     * If the data is available, gnssMeasurementFlags must contain
     * HAS_CARRIER_PHASE_UNCERTAINTY.
     */
    double carrierPhaseUncertainty;

    /**
     * An enumeration that indicates the 'multipath' state of the event.
     *
     * The multipath Indicator is intended to report the presence of overlapping
     * signals that manifest as distorted correlation peaks.
     *
     * - if there is a distorted correlation peak shape, report that multipath
     *   is MULTIPATH_INDICATOR_PRESENT.
     * - if there is no distorted correlation peak shape, report
     *   MULTIPATH_INDICATOR_NOT_PRESENT
     * - if signals are too weak to discern this information, report
     *   MULTIPATH_INDICATOR_UNKNOWN
     *
     * Example: when doing the standardized overlapping Multipath Performance
     * test (3GPP TS 34.171) the Multipath indicator must report
     * MULTIPATH_INDICATOR_PRESENT for those signals that are tracked, and
     * contain multipath, and MULTIPATH_INDICATOR_NOT_PRESENT for those
     * signals that are tracked and do not contain multipath.
     */
    GnssMultipathIndicator multipathIndicator = GnssMultipathIndicator.UNKNOWN;

    /**
     * Signal-to-noise ratio at correlator output in dB.
     * If the data is available, GnssMeasurementFlags must contain HAS_SNR.
     * This is the power ratio of the "correlation peak height above the
     * observed noise floor" to "the noise RMS".
     */
    double snrDb;

    /**
     * Automatic gain control (AGC) level. AGC acts as a variable gain amplifier adjusting the power
     * of the incoming signal. The AGC level may be used to indicate potential interference. Higher
     * gain (and/or lower input power) must be output as a positive number. Hence in cases of strong
     * jamming, in the band of this signal, this value must go more negative. This value must be
     * consistent given the same level of the incoming signal power.
     *
     * Note: Different hardware designs (e.g. antenna, pre-amplification, or other RF HW components)
     * may also affect the typical output of this value on any given hardware design in an open sky
     * test - the important aspect of this output is that changes in this value are indicative of
     * changes on input signal power in the frequency band for this measurement.
     */
    double agcLevelDb;

    /**
     * The full inter-signal bias (ISB) in nanoseconds.
     *
     * This value is the sum of the estimated receiver-side and the space-segment-side inter-system
     * bias, inter-frequency bias and inter-code bias, including
     *
     * - Receiver inter-constellation bias (with respect to the constellation in
     *   GnssClock.referenceSignalTypeForIsb)
     * - Receiver inter-frequency bias (with respect to the carrier frequency in
     *   GnssClock.referenceSignalTypeForIsb)
     * - Receiver inter-code bias (with respect to the code type in
     *   GnssClock.referenceSignalTypeForIsb)
     * - Master clock bias (e.g., GPS-GAL Time Offset (GGTO), GPS-UTC Time Offset (TauGps), BDS-GLO
     *   Time Offset (BGTO)) (with respect to the constellation in
     *   GnssClock.referenceSignalTypeForIsb)
     * - Group delay (e.g., Total Group Delay (TGD))
     * - Satellite inter-frequency bias (GLO only) (with respect to the carrier frequency in
     *   GnssClock.referenceSignalTypeForIsb)
     * - Satellite inter-code bias (e.g., Differential Code Bias (DCB)) (with respect to the code
     *   type in GnssClock.referenceSignalTypeForIsb)
     *
     * If a component of the above is already compensated in the provided
     * GnssMeasurement.receivedSvTimeInNs, then it must not be included in the reported full ISB.
     *
     * The value does not include the inter-frequency Ionospheric bias.
     *
     * The full ISB of GnssClock.referenceSignalTypeForIsb is defined to be 0.0 nanoseconds.
     */
    double fullInterSignalBiasNs;

    /**
     * 1-sigma uncertainty associated with the full inter-signal bias in nanoseconds.
     */
    double fullInterSignalBiasUncertaintyNs;

    /**
     * The satellite inter-signal bias in nanoseconds.
     *
     * This value is the sum of the space-segment-side inter-system bias, inter-frequency bias
     * and inter-code bias, including
     *
     * - Master clock bias (e.g., GPS-GAL Time Offset (GGTO), GPS-UTC Time Offset (TauGps), BDS-GLO
     *   Time Offset (BGTO)) (with respect to the constellation in
     *   GnssClock.referenceSignalTypeForIsb)
     * - Group delay (e.g., Total Group Delay (TGD))
     * - Satellite inter-frequency bias (GLO only) (with respect to the carrier frequency in
     *   GnssClock.referenceSignalTypeForIsb)
     * - Satellite inter-code bias (e.g., Differential Code Bias (DCB)) (with respect to the code
     *   type in GnssClock.referenceSignalTypeForIsb)
     *
     * The satellite ISB of GnssClock.referenceSignalTypeForIsb is defined to be 0.0 nanoseconds.
     */
    double satelliteInterSignalBiasNs;

    /**
     * 1-sigma uncertainty associated with the satellite inter-signal bias in nanoseconds.
     */
    double satelliteInterSignalBiasUncertaintyNs;

    /**
     * The GNSS satellite position, velocity and time information at the same signal transmission
     * time receivedSvTimeInNs.
     *
     * The position and velocity must be in ECEF coordinates.
     *
     * If the data is available, gnssMeasurementFlags must contain HAS_SATELLITE_PVT.
     *
     * If SatellitePvt is derived from Broadcast ephemeris, then the position is already w.r.t.
     * the antenna phase center. However, if SatellitePvt is derived from other modeled orbits,
     * such as long-term-orbits, or precise orbits, then the orbits may have been computed w.r.t.
     * the satellite center of mass, and then GNSS vendors are expected to correct for the effect
     * on different phase centers (can differ by meters) of different GNSS signals (e.g. L1, L5)
     * on the reported satellite position. Accordingly, we might observe a different satellite
     * position reported for L1 GnssMeasurement struct compared to L5 GnssMeasurement struct.
     *
     * If receivedSvTimeNs is not fully decoded, Satellite PVT could still be reported and
     * receivedSvTimeNs uncertainty field would be used to provide confidence.
     */
    SatellitePvt satellitePvt;

    /**
     * A list of Correlation Vectors with each vector corresponding to a frequency offset.
     *
     * To represent correlation values over a 2D spaces (delay and frequency), a CorrelationVector
     * is required per frequency offset, and each CorrelationVector contains correlation values
     * at equally spaced spatial offsets.
     */
    CorrelationVector[] correlationVectors;
}
