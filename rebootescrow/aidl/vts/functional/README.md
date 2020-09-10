Many of the tests in this directory may require that TEE Keymaster
"EARLY_BOOT_ONLY" keys be usable when this test runs. In order to accomplish
this, a build of "vold" that omits the call to "earlyBootEnded()" function
should be made. Then these DISABLED tests may be run successfully.

The CTS test ResumeOnRebootHostTests will test the functionality without a
special build.
