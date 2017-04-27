## Codec OMX Tests
---

The current directory contains the following folders: audio, common, component, master
and video.

Besides common, all other folders contain basic OMX unit tests for testing audio and video decoder-encoder
components. common constitutes files that are used across test applications.

## master
Enumerates all the omx components (and their roles) available in android media framework.

Usage:

VtsHalMediaOmxV1\_0TargetMasterTest -I default

## component
This folder includes test fixtures that tests aspects common to all OMX compatible components. For instance, port enabling/disabling, enumerating port formats, state transitions, flush etc. stay common to all components irrespective of the service they offer. In a way this tests the OMX Core. Every standard OMX compatible component is expected to pass these tests.


Usage:

VtsHalMediaOmxV1\_0TargetComponentTest -I default -C <component name> -R <component role>

## audio
This folder includes test fixtures associated with audio encoder/decoder components such as encoding/decoding, EOS test, timestamp test etc. These tests are aimed towards testing the component specific aspects.

Usage:

VtsHalMediaOmxV1\_0TargetAudioDecTest -I default -C <component name> -R audio_decoder.<class>

VtsHalMediaOmxV1\_0TargetAudioEncTest -I default -C <component name> -R audio_encoder.<class>

## video
This folder includes test fixtures associated with video encoder/decoder components like encoding/decoding, EOS test, timestamp test etc. These tests are aimed towards testing the component specific aspects.

Usage:

VtsHalMediaOmxV1\_0TargetVideoDecTest -I default -C <component name> -R video_decoder.<class>

VtsHalMediaOmxV1\_0TargetVideoEncTest -I default -C <component name> -R video_encoder.<class>

## notes
Every component shall be tested by two applications,

* ComponentTest.

* AudioDecTest/AudioEncTest/VideoDecTest/VideoEncTest depending on the component class.

