# KeyMint HAL

This directory contains the HAL definition for KeyMint. KeyMint provides
cryptographic services in a hardware-isolated environment.

Note that the `IRemotelyProvisionedComponent` HAL, and it's associated types,
used to also be defined in this directory. As of Android U, this HAL has been
moved to a different directory (../rkp). This move is ABI compatible, as the
interfaces have been maintained. The build is split so that the generated
code may be built with different options.
