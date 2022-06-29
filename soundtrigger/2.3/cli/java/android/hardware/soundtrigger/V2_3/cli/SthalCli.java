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
package android.hardware.soundtrigger.V2_3.cli;

import android.hardware.soundtrigger.V2_0.PhraseRecognitionExtra;
import android.hardware.soundtrigger.V2_0.RecognitionMode;
import android.hardware.soundtrigger.V2_0.SoundModelType;
import android.hardware.soundtrigger.V2_3.OptionalModelParameterRange;
import android.hardware.soundtrigger.V2_3.ISoundTriggerHw;
import android.hardware.soundtrigger.V2_1.ISoundTriggerHwCallback;
import android.os.HidlMemoryUtil;
import android.os.HwBinder;
import android.os.RemoteException;
import android.os.SystemProperties;

import java.util.Scanner;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;

/**
 * This is a quick-and-dirty sound trigger HAL console mock.
 *
 * It would only work on userdebug builds.
 *
 * When this app is started, it will initially:
 * - Register a ISoundTriggerHw HAL with an instance name "mock".
 * - Set a sysprop that tells SoundTriggerMiddlewareService to try to connect to the mock instance
 * rather than the default one.
 * - Reboot the real (default) HAL.
 *
 * In response to that, SoundTriggerMiddlewareService is going to connect to the mock HAL and resume
 * normal operation.
 *
 * Our mock HAL will print to stdout every call it receives as well as expose a basic set of
 * operations for sending event callbacks to the client. This allows us to simulate the frameworks
 * behavior in response to different HAL behaviors.
 */
public class SthalCli {
    private static SoundTriggerImpl mService;
    private static final Scanner mScanner = new Scanner(System.in);

    public static void main(String[] args) {
        try {
            System.out.println("Registering mock STHAL");
            HwBinder.setTrebleTestingOverride(true);
            mService = new SoundTriggerImpl();
            mService.registerAsService("mock");

            System.out.println("Rebooting STHAL");
            SystemProperties.set("debug.soundtrigger_middleware.use_mock_hal", "2");
            SystemProperties.set("sys.audio.restart.hal", "1");

            while (processCommand()) ;
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            cleanup();
        }
    }

    private static void cleanup() {
        System.out.println("Cleaning up.");
        SystemProperties.set("debug.soundtrigger_middleware.use_mock_hal", null);
        HwBinder.setTrebleTestingOverride(false);
    }

    private static boolean processCommand() {
        String line = mScanner.nextLine();
        String[] tokens = line.split("\\s+");
        if (tokens.length < 1) {
            return false;
        }
        switch (tokens[0]) {
            case "q":
                return false;

            case "r":
                mService.sendRecognitionEvent(Integer.parseInt(tokens[1]),
                        Integer.parseInt(tokens[2]));
                return true;

            case "p":
                mService.sendPhraseRecognitionEvent(Integer.parseInt(tokens[1]),
                        Integer.parseInt(tokens[2]));
                return true;

            case "d":
                mService.dumpModels();
                return true;

            case "h":
                System.out.print("Available commands:\n" + "h - help\n" + "q - quit\n"
                        + "r <model> <status> - send recognitionEvent\n"
                        + "p <model> <status> - send phraseRecognitionEvent\n"
                        + "d - dump models\n");

            default:
                return true;
        }
    }

    private static class SoundTriggerImpl extends ISoundTriggerHw.Stub {
        static class Model {
            final ISoundTriggerHwCallback callback;
            final SoundModel model;
            final PhraseSoundModel phraseModel;
            public android.hardware.soundtrigger.V2_3.RecognitionConfig config = null;

            Model(ISoundTriggerHwCallback callback, SoundModel model) {
                this.callback = callback;
                this.model = model;
                this.phraseModel = null;
            }

            Model(ISoundTriggerHwCallback callback, PhraseSoundModel model) {
                this.callback = callback;
                this.model = null;
                this.phraseModel = model;
            }
        }

        private final ConcurrentMap<Integer, Model> mLoadedModels = new ConcurrentHashMap<>();
        private int mHandleCounter = 1;

        public void dumpModels() {
            mLoadedModels.forEach((handle, model) -> {
                System.out.println("+++ Model " + handle);
                System.out.println("    config = " + model.config);
                android.hardware.soundtrigger.V2_3.RecognitionConfig recognitionConfig =
                        model.config;
                if (recognitionConfig != null) {
                    System.out.println("    ACTIVE recognitionConfig = " + recognitionConfig);
                } else {
                    System.out.println("    INACTIVE");
                }
            });
        }

        public void sendRecognitionEvent(int modelHandle, int status) {
            Model model = mLoadedModels.get(modelHandle);
            if (model != null && model.config != null) {
                android.hardware.soundtrigger.V2_1.ISoundTriggerHwCallback.RecognitionEvent event =
                        new android.hardware.soundtrigger.V2_1.ISoundTriggerHwCallback.RecognitionEvent();
                event.header.model = modelHandle;
                event.header.type = SoundModelType.GENERIC;
                event.header.status = status;
                event.header.captureSession = model.config.base.header.captureHandle;
                event.header.captureAvailable = true;
                event.header.audioConfig.channelMask = 16;
                event.header.audioConfig.format = 1;
                event.header.audioConfig.sampleRateHz = 16000;
                event.data = HidlMemoryUtil.byteArrayToHidlMemory(new byte[0]);
                try {
                    model.callback.recognitionCallback_2_1(event, 0);
                } catch (RemoteException e) {
                    e.printStackTrace();
                }
                model.config = null;
            }
        }

        public void sendPhraseRecognitionEvent(int modelHandle, int status) {
            Model model = mLoadedModels.get(modelHandle);
            if (model != null && model.config != null) {
                android.hardware.soundtrigger.V2_1.ISoundTriggerHwCallback.PhraseRecognitionEvent
                        event =
                        new android.hardware.soundtrigger.V2_1.ISoundTriggerHwCallback.PhraseRecognitionEvent();
                event.common.header.model = modelHandle;
                event.common.header.type = SoundModelType.KEYPHRASE;
                event.common.header.status = status;
                event.common.header.captureSession = model.config.base.header.captureHandle;
                event.common.header.captureAvailable = true;
                event.common.header.audioConfig.channelMask = 16;
                event.common.header.audioConfig.format = 1;
                event.common.header.audioConfig.sampleRateHz = 16000;
                event.common.data = HidlMemoryUtil.byteArrayToHidlMemory(new byte[0]);
                if (!model.phraseModel.phrases.isEmpty()) {
                    PhraseRecognitionExtra extra = new PhraseRecognitionExtra();
                    extra.id = model.phraseModel.phrases.get(0).id;
                    extra.confidenceLevel = 100;
                    extra.recognitionModes = model.phraseModel.phrases.get(0).recognitionModes;
                    event.phraseExtras.add(extra);
                }
                try {
                    model.callback.phraseRecognitionCallback_2_1(event, 0);
                } catch (RemoteException e) {
                    e.printStackTrace();
                }
                model.config = null;
            }
        }

        @Override
        public void loadSoundModel_2_1(SoundModel soundModel,
                android.hardware.soundtrigger.V2_1.ISoundTriggerHwCallback callback, int cookie,
                loadSoundModel_2_1Callback _hidl_cb) {
            int handle = mHandleCounter++;
            System.out.printf("loadSoundModel_2_1(soundModel=%s) -> %d%n", soundModel, handle);
            mLoadedModels.put(handle, new Model(callback, soundModel));
            _hidl_cb.onValues(0, handle);
        }

        @Override
        public void loadPhraseSoundModel_2_1(PhraseSoundModel soundModel,
                android.hardware.soundtrigger.V2_1.ISoundTriggerHwCallback callback, int cookie,
                loadPhraseSoundModel_2_1Callback _hidl_cb) {
             int handle = mHandleCounter++;
            System.out.printf("loadPhraseSoundModel_2_1(soundModel=%s) -> %d%n", soundModel,
                    handle);
            mLoadedModels.put(handle, new Model(callback, soundModel));
            _hidl_cb.onValues(0, handle);
        }

        @Override
        public int startRecognition_2_3(int modelHandle,
                android.hardware.soundtrigger.V2_3.RecognitionConfig config) {
            System.out.printf("startRecognition_2_3(modelHandle=%d)%n", modelHandle);
            Model model = mLoadedModels.get(modelHandle);
            if (model != null) {
                model.config = config;
            }
            return 0;
        }

        @Override
        public void getProperties_2_3(getProperties_2_3Callback _hidl_cb) {
            System.out.println("getProperties_2_3()");
            android.hardware.soundtrigger.V2_3.Properties properties =
                    new android.hardware.soundtrigger.V2_3.Properties();
            properties.base.implementor = "Android";
            properties.base.description = "Mock STHAL";
            properties.base.maxSoundModels = 2;
            properties.base.maxKeyPhrases = 1;
            properties.base.recognitionModes =
                    RecognitionMode.VOICE_TRIGGER | RecognitionMode.GENERIC_TRIGGER;
            _hidl_cb.onValues(0, properties);
        }

        @Override
        public void queryParameter(int modelHandle, int modelParam,
                queryParameterCallback _hidl_cb) {
            _hidl_cb.onValues(0, new OptionalModelParameterRange());
        }

        @Override
        public int getModelState(int modelHandle) {
            System.out.printf("getModelState(modelHandle=%d)%n", modelHandle);
            return 0;
        }

        @Override
        public int unloadSoundModel(int modelHandle) {
            System.out.printf("unloadSoundModel(modelHandle=%d)%n", modelHandle);
            return 0;
        }

        @Override
        public int stopRecognition(int modelHandle) {
            System.out.printf("stopRecognition(modelHandle=%d)%n", modelHandle);
            Model model = mLoadedModels.get(modelHandle);
            if (model != null) {
                model.config = null;
            }
            return 0;
        }

        @Override
        public void debug(android.os.NativeHandle fd, java.util.ArrayList<String> options) {
            if (!options.isEmpty()) {
                switch (options.get(0)) {
                    case "reboot":
                        System.out.println("Received a reboot request. Exiting.");
                        cleanup();
                        System.exit(1);
                }
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Everything below is not implemented and not expected to be called.

        @Override
        public int setParameter(int modelHandle, int modelParam, int value) {
            throw new UnsupportedOperationException();
        }

        @Override
        public void getParameter(int modelHandle, int modelParam, getParameterCallback _hidl_cb) {
            throw new UnsupportedOperationException();
        }

        @Override
        public int startRecognition_2_1(int modelHandle, RecognitionConfig config,
                android.hardware.soundtrigger.V2_1.ISoundTriggerHwCallback callback, int cookie) {
            throw new UnsupportedOperationException();
        }

        @Override
        public void getProperties(getPropertiesCallback _hidl_cb) {
            throw new UnsupportedOperationException();
        }

        @Override
        public void loadSoundModel(
                android.hardware.soundtrigger.V2_0.ISoundTriggerHw.SoundModel soundModel,
                android.hardware.soundtrigger.V2_0.ISoundTriggerHwCallback callback, int cookie,
                loadSoundModelCallback _hidl_cb) {
            throw new UnsupportedOperationException();
        }

        @Override
        public void loadPhraseSoundModel(
                android.hardware.soundtrigger.V2_0.ISoundTriggerHw.PhraseSoundModel soundModel,
                android.hardware.soundtrigger.V2_0.ISoundTriggerHwCallback callback, int cookie,
                loadPhraseSoundModelCallback _hidl_cb) {
            throw new UnsupportedOperationException();
        }

        @Override
        public int startRecognition(int modelHandle,
                android.hardware.soundtrigger.V2_0.ISoundTriggerHw.RecognitionConfig config,
                android.hardware.soundtrigger.V2_0.ISoundTriggerHwCallback callback, int cookie) {
            throw new UnsupportedOperationException();
        }

        @Override
        public int stopAllRecognitions() {
            throw new UnsupportedOperationException();
        }
    }
}
