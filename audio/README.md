# Audio HAL

Directory structure of the audio HAL related code.

## Directory Structure for AIDL audio HAL

The AIDL version is located inside `aidl` directory. The tree below explains
the role of each subdirectory:

* `aidl_api` — snapshots of the API created each Android release. Every
  release, the current version of the API becomes "frozen" and gets assigned
  the next version number. If the API needs further modifications, they are
  made on the "current" version. After making modifications, run
  `m <package name>-update-api` to update the snapshot of the "current"
  version.
* `android/hardware/audio/common` — data structures and interfaces shared
  between various HALs: BT HAL, core and effects audio HALs.
* `android/hardware/audio/core` — data structures and interfaces of the
  core audio HAL.
* `default` — the default, reference implementation of the audio HAL service.
* `vts` — VTS tests for the AIDL HAL.

## Directory Structure for HIDL audio HAL

Run `common/all-versions/copyHAL.sh` to create a new version of the HIDL audio
HAL based on an existing one. Note that this isn't possible since Android T
release. Android U and above uses AIDL audio HAL.

* `2.0` — version 2.0 of the core HIDL API. Note that `.hal` files
  can not be moved into the `core` directory because that would change
  its namespace and include path.
   - `config` — the XSD schema for the Audio Policy Manager
     configuration file.
* `4.0` — version 4.0 of the core HIDL API.
* ...
* `common` — common types for audio core and effect HIDL API.
   - `2.0` — version 2.0 of the common types HIDL API.
   - `4.0` — version 4.0.
   - ...
   - `7.0` — version 7.0.
      - `example` — example implementation of the core and effect
        V7.0 API. It represents a "fake" audio HAL that doesn't
        actually communicate with hardware.
   - `all-versions` — code common to all version of both core and effect API.
      - `default` — shared code of the default implementation.
         - `service` — vendor HAL service for hosting the default
           implementation.
      - `test` — utilities used by tests.
      - `util` — utilities used by both implementation and tests.
* `core` — VTS tests and the default implementation of the core API
  (not HIDL API, it's in `audio/N.M`).
   - `7.0` — code specific to version V7.0 of the core HIDL API
   - `all-versions` — the code is common between all versions,
     version-specific parts are enclosed into conditional directives
     of preprocessor or reside in dedicated files.
       - `default` — code that wraps the legacy API (from
         `hardware/libhardware`).
         - `util` — utilities for the default implementation.
       - `vts` VTS tests for the core HIDL API.
* `effect` — same for the effect HIDL API.
   - `2.0`
      - `config` — the XSD schema for the Audio Effects configuration file.
   - `4.0`
   - ...
   - `all-versions`
       - `default` — code that wraps the legacy API (from
         `hardware/libhardware`).
         - `util` — utilities for the default implementation.
       - `vts` VTS tests for the effect HIDL API.
* `policy` — Configurable Audio Policy schemes.
   - `1.0` — note that versions of CAP are not linked to the versions
     of audio HAL.
      - `vts` — VTS tests for validating actual configuration files.
      - `xml` — XSD schemas for CAP configuration files.
