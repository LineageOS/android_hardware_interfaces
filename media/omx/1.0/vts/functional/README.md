## Omx Hal @ 1.0 tests ##
---
## Overview :
The scope of the tests presented here is not restricted solely to testing omx hal @ 1.0 API but also test to omx core functionality and to an extent omx components as well. The current directory contains the following folders: audio, common, component, master and video. Besides common all other folders contain test fixtures for testing AV decoder, encoder components. Common constitutes files that are used across by these test applications.

#### master :
Functionality of master is to enumerate all the omx components (and the roles it supports) available in android media framework.

usage: atest VtsHalMediaOmxV1\_0TargetMasterTest

#### component :
This folder includes test fixtures that tests aspects common to all omx compatible components. For instance, port enabling/disabling, enumerating port formats, state transitions, flush, ..., stay common to all components irrespective of the service they offer. Test fixtures here are directed towards testing these (omx core). Every standard OMX compatible component is expected to pass these tests.

usage: atest VtsHalMediaOmxV1\_0TargetComponentTest

#### audio :
This folder includes test fixtures associated with testing audio encoder and decoder components such as simple encoding of a raw clip or decoding of an elementary stream, end of stream test, timestamp deviations test, flush test and so on. These tests are aimed towards testing the plugin that connects the component to the omx core.

usage:

atest VtsHalMediaOmxV1\_0TargetAudioDecTest

atest VtsHalMediaOmxV1\_0TargetAudioEncTest

#### video :
This folder includes test fixtures associated with testing video encoder and decoder components such as simple encoding of a raw clip or decoding of an elementary stream, end of stream test, timestamp deviations test, flush test and so on. These tests are aimed towards testing the plugin that connects the component to the omx core.

usage:

atest VtsHalMediaOmxV1\_0TargetVideoDecTest

atest VtsHalMediaOmxV1\_0TargetVideoEncTest
