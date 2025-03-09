package android.hardware.gnss.gnss_assistance;

import android.hardware.gnss.gnss_assistance.KeplerianOrbitModel;

/**
 * Contains ephemeris parameters specific to Beidou satellites.
 *
 * @hide
 */
@VintfStability
parcelable BeidouSatelliteEphemeris {
    /*
     * Contains the set of parameters needed for Beidou satellite clock
     * correction.
     * This is defined in BDS-SIS-ICD-B1I-3.0, section 5.2.4.9, 5.2.4.10.
     */
    @VintfStability
    parcelable BeidouSatelliteClockModel {
        /**
         * Time of the clock in seconds since Beidou epoch.
         *
         * Represents the 'Epoch' field within the 'SV/EPOCH/SV CLK' record of GNSS
         * navigation message file in RINEX 3.05 Table A14.
         */
        long timeOfClockSeconds;

        /** SV clock bias in seconds. */
        double af0;

        /** SV clock drift in seconds per second. */
        double af1;

        /** Clock drift rate in seconds per second squared. */
        double af2;

        /** Group delay differential 1 B1/B3 in seconds. */
        double tgd1;

        /** Group delay differential 2 B2/B3 in seconds. */
        double tgd2;

        /**
         * Age of Data Clock and field range is: 0-31.
         * This is defined in BDS-SIS-ICD-B1I-3.0 Section 5.2.4.8 Table 5-6.
         */
        int aodc;
    }

    /** Contains information about Beidou health. */
    parcelable BeidouSatelliteHealth {
        /**
         * The autonomous satellite health flag (SatH1) occupies 1 bit. “0” means
         * broadcasting satellite is good and “1” means not.
         * This is defined in BDS-SIS-ICD-B1I-3.0 section 5.2.4.6.
         */
        int satH1;

        /**
         * SV accuracy in meters.
         * This is defined in the "BROADCAST ORBIT - 6" record of RINEX 3.05
         * Table A14, pp.78.
         */
        double svAccur;
    }

    /** Contains information about time of ephemeris */
    parcelable BeidouSatelliteEphemerisTime {
        /**
         * AODE Age of Data, Ephemeris.
         * This is as defined in BDS-SIS-ICD-B1I-3.0 section 5.2.4.11 Table 5-8.
         */
        int aode;

        /** Beidou week number without rollover. */
        int weekNumber;

        /**
         * Time of ephemeris in seconds.
         * This is defined in BDS-SIS-ICD-B1I-3.0 section 5.2.4.12.
         */
        int toeSeconds;
    }

    /** The PRN number of the Beidou satellite. */
    int prn;

    /** Satellite clock model. */
    BeidouSatelliteClockModel satelliteClockModel;

    /** Satellite orbit model. */
    KeplerianOrbitModel satelliteOrbitModel;

    /** Satellite health. */
    BeidouSatelliteHealth satelliteHealth;

    /** Satellite ephemeris time. */
    BeidouSatelliteEphemerisTime satelliteEphemerisTime;
}
