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
package android.hardware.soundtrigger3.cli;

import android.annotation.NonNull;
import android.hardware.soundtrigger3.ISoundTriggerHw;
import android.hardware.soundtrigger3.ISoundTriggerHwCallback;
import android.hardware.soundtrigger3.ISoundTriggerHwGlobalCallback;
import android.media.audio.common.AudioChannelLayout;
import android.media.audio.common.AudioConfig;
import android.media.audio.common.AudioConfigBase;
import android.media.audio.common.AudioFormatDescription;
import android.media.audio.common.AudioFormatType;
import android.media.audio.common.PcmType;
import android.media.soundtrigger.ConfidenceLevel;
import android.media.soundtrigger.ModelParameterRange;
import android.media.soundtrigger.PhraseRecognitionEvent;
import android.media.soundtrigger.PhraseRecognitionExtra;
import android.media.soundtrigger.PhraseSoundModel;
import android.media.soundtrigger.Properties;
import android.media.soundtrigger.RecognitionConfig;
import android.media.soundtrigger.RecognitionEvent;
import android.media.soundtrigger.RecognitionMode;
import android.media.soundtrigger.SoundModel;
import android.media.soundtrigger.SoundModelType;
import android.os.HwBinder;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.os.ServiceManager;
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
    private static final Scanner scanner = new Scanner(System.in);

    public static void main(String[] args) {
        try {
            printUsage();

            System.out.println("Registering mock STHAL");
            mService = new SoundTriggerImpl();
            // This allows us to register the service, even if it is not declared in the manifest.
            mService.forceDowngradeToSystemStability();
            ServiceManager.addService(ISoundTriggerHw.class.getCanonicalName() + "/mock", mService);

            System.out.println("Rebooting STHAL");
            SystemProperties.set("debug.soundtrigger_middleware.use_mock_hal", "3");
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
        String line = scanner.nextLine();
        String[] tokens = line.split("\\s+");
        if (tokens.length < 1) {
            return false;
        }
        switch (tokens[0]) {
            case "q":
                return false;

            case "a":
                mService.sendOnResourcesAvailable();
                return true;

            case "u":
                mService.sendModelUnloaded(Integer.parseInt(tokens[1]));
                return true;

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

            default:
                printUsage();
                return true;
        }
    }

    private static void printUsage() {
        System.out.print(
                "Sound Trigger HAL v3 mock\n"
                + "Available commands:\n"
                + "h - help\n"
                + "q - quit\n"
                + "a - send onResourcesAvailable event\n"
                + "u <model> - send modelUnloaded event\n"
                + "r <model> <status> - send recognitionEvent\n"
                + "p <model> <status> - send phraseRecognitionEvent\n"
                + "d - dump models\n");
    }

    private static class SoundTriggerImpl extends ISoundTriggerHw.Stub {
        static class Model {
            final ISoundTriggerHwCallback callback;
            final SoundModel model;
            final PhraseSoundModel phraseModel;
            public RecognitionConfig config = null;

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

        private ISoundTriggerHwGlobalCallback mGlobalCallback;
        private final ConcurrentMap<Integer, Model> mLoadedModels = new ConcurrentHashMap<>();
        private int mHandleCounter = 1;

        public void dumpModels() {
            mLoadedModels.forEach((handle, model) -> {
                System.out.println("+++ Model " + handle);
                System.out.println("    config = " + model.config);
                RecognitionConfig recognitionConfig = model.config;
                if (recognitionConfig != null) {
                    System.out.println("    ACTIVE recognitionConfig = " + recognitionConfig);
                } else {
                    System.out.println("    INACTIVE");
                }
            });
        }

        public void sendOnResourcesAvailable() {
            if (mGlobalCallback != null) {
                try {
                    mGlobalCallback.onResourcesAvailable();
                } catch (RemoteException e) {
                    e.printStackTrace();
                }
            }
        }

        public void sendRecognitionEvent(int modelHandle, int status) {
            Model model = mLoadedModels.get(modelHandle);
            if (model != null && model.config != null) {
                RecognitionEvent event = new RecognitionEvent();
                event.type = SoundModelType.GENERIC;
                event.status = status;
                event.captureAvailable = true;
                event.audioConfig = createConfig();
                try {
                    model.callback.recognitionCallback(modelHandle, event);
                } catch (RemoteException e) {
                    e.printStackTrace();
                }
                model.config = null;
            }
        }

        public void sendPhraseRecognitionEvent(int modelHandle, int status) {
            Model model = mLoadedModels.get(modelHandle);
            if (model != null && model.config != null) {
                PhraseRecognitionEvent event = new PhraseRecognitionEvent();
                event.common = new RecognitionEvent();
                event.common.type = SoundModelType.KEYPHRASE;
                event.common.status = status;
                event.common.captureAvailable = true;
                event.common.audioConfig = createConfig();
                if (model.phraseModel.phrases.length > 0) {
                    PhraseRecognitionExtra extra = new PhraseRecognitionExtra();
                    extra.id = model.phraseModel.phrases[0].id;
                    extra.confidenceLevel = 100;
                    extra.recognitionModes = model.phraseModel.phrases[0].recognitionModes;
                    extra.levels = new ConfidenceLevel[0];
                    event.phraseExtras = new PhraseRecognitionExtra[]{extra};
                } else {
                    event.phraseExtras = new PhraseRecognitionExtra[0];
                }
                try {
                    model.callback.phraseRecognitionCallback(modelHandle, event);
                } catch (RemoteException e) {
                    e.printStackTrace();
                }
                model.config = null;
            }
        }

        public void sendModelUnloaded(int modelHandle) {
            Model model = mLoadedModels.remove(modelHandle);
            if (model != null) {
                try {
                    model.callback.modelUnloaded(modelHandle);
                } catch (RemoteException e) {
                    e.printStackTrace();
                }
            }
        }

        @Override
        public void registerGlobalCallback(ISoundTriggerHwGlobalCallback callback) {
            System.out.println("registerGlobalCallback()");
            mGlobalCallback = callback;
        }

        @Override
        public int loadSoundModel(SoundModel soundModel, ISoundTriggerHwCallback callback) {
            int handle = mHandleCounter++;
            System.out.printf("loadSoundModel(soundModel=%s) -> %d%n", soundModel, handle);
            mLoadedModels.put(handle, new Model(callback, soundModel));
            return handle;
        }

        @Override
        public int loadPhraseSoundModel(PhraseSoundModel soundModel,
                ISoundTriggerHwCallback callback) {
            int handle = mHandleCounter++;
            System.out.printf("loadPhraseSoundModel(soundModel=%s) -> %d%n", soundModel, handle);
            mLoadedModels.put(handle, new Model(callback, soundModel));
            return handle;
        }

        @Override
        public void startRecognition(int modelHandle, int deviceHandle, int ioHandle,
                RecognitionConfig config) {
            System.out.printf("startRecognition(modelHandle=%d, deviceHandle=%d, ioHandle=%d)%n",
                    modelHandle, deviceHandle, ioHandle);
            Model model = mLoadedModels.get(modelHandle);
            if (model != null) {
                model.config = config;
            }
        }

        @Override
        public Properties getProperties() {
            System.out.println("getProperties()");
            Properties properties = new Properties();
            properties.implementor = "Android";
            properties.description = "Mock STHAL";
            properties.uuid = "a5af2d2a-4cc4-4b69-a22f-c9d5b6d492c3";
            properties.supportedModelArch = "Mock arch";
            properties.maxSoundModels = 2;
            properties.maxKeyPhrases = 1;
            properties.recognitionModes =
                    RecognitionMode.VOICE_TRIGGER | RecognitionMode.GENERIC_TRIGGER;
            return properties;
        }

        @Override
        public ModelParameterRange queryParameter(int modelHandle, int modelParam) {
            System.out.printf("queryParameter(modelHandle=%d, modelParam=%d)%n", modelHandle,
                    modelParam);
            return null;
        }

        @Override
        public void forceRecognitionEvent(int modelHandle) {
            System.out.printf("getModelState(modelHandle=%d)%n", modelHandle);
        }

        @Override
        public void unloadSoundModel(int modelHandle) {
            System.out.printf("unloadSoundModel(modelHandle=%d)%n", modelHandle);
        }

        @Override
        public void stopRecognition(int modelHandle) {
            System.out.printf("stopRecognition(modelHandle=%d)%n", modelHandle);
            Model model = mLoadedModels.get(modelHandle);
            if (model != null) {
                model.config = null;
            }
        }

        @Override
        public int handleShellCommand(@NonNull ParcelFileDescriptor in,
                @NonNull ParcelFileDescriptor out, @NonNull ParcelFileDescriptor err,
                @NonNull String[] args) {
            if (args.length > 0) {
                switch (args[0]) {
                    case "reboot":
                        System.out.println("Received a reboot request. Exiting.");
                        cleanup();
                        System.exit(1);
                }
            }
            return 0;
        }

        @Override
        public void setParameter(int modelHandle, int modelParam, int value) {
            throw new IllegalArgumentException();
        }

        @Override
        public int getParameter(int modelHandle, int modelParam) {
            throw new IllegalArgumentException();
        }

        private static AudioConfig createConfig() {
            AudioConfig config = new AudioConfig();
            config.base = new AudioConfigBase();
            config.base.channelMask = AudioChannelLayout.layoutMask(AudioChannelLayout.LAYOUT_MONO);
            config.base.format = new AudioFormatDescription();
            config.base.format.type = AudioFormatType.PCM;
            config.base.format.pcm = PcmType.INT_16_BIT;
            config.base.sampleRate = 16000;
            return config;
        }

        @Override
        public int getInterfaceVersion() {
            return ISoundTriggerHw.VERSION;
        }

        @Override
        public String getInterfaceHash() {
            return ISoundTriggerHw.HASH;
        }
    }
}
