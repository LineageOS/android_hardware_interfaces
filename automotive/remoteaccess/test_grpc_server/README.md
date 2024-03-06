# Test GRPC Server.

A test GRPC server that implements wakeup_client.proto. This test server acts
as a reference implementation for a remote wakeup client running on TCU. The
test server does not communicate with any actual network server. It has the
following behavior:

* It starts a GRPC server on 'DGRPC_SERVICE_ADDRESS' compile flag which is
  localhost:50051. The GRPC server provides the service according to
  hardware/interfaces/automotive/remoteaccess/hal/default/proto/wakeup_client.proto.

  In real implementation, DGRPC_SERVICE_ADDRESS can be specified to any IP
  address where the TCU can be exposed to Application Processor. The default
  remote access HAL implementation
  (hardware/interfaces/automotive/remoteaccess/hal/default/Android.bp) also
  uses DGRPC_SERVICE_ADDRESS to find this GRPC server, so it must have the
  same IP address.

* It generates a fake task using FakeTaskGenerator every 'kTaskIntervalInMs' ms.

  In real implementation, it should receive task from the remote server.

* Each fake task has an increasing unique client ID. The task data is always
  what's defined for 'DATA' variable.

  In real implementation, the client ID and task data should come from the
  remote server.

* The generated tasks are put into a task queue which is a priority queue sorted
  by task received time.

  In real implementation, if the server provides a task timestamp, then this
  queue can be sorted by that task timestamp instead.

* When the Application processor is started, the remote access HAL running on
  Android will call 'GetRemoteTasks' to establish a long-live connection. This
  connection is used to deliver all task data from remote wakeup client to
  remote access HAL, which eventually to car service and applications.

  When the 'GetRemoteTasks' is called, the wakeup client must send all the
  pending tasks through the 'ServerWriter'. If no task is pending, then it must
  block and wait for a new task to arrive.

  If one task data fails to be sent through the channel, it likely means
  the other side (Application processor) is shutting down or has closed the
  channel. The wakeup client must put the task back to the pending queue and
  wait for a new 'GetRemoteTasks' request to retry sending the task.

* When a new task arrives, if 'WakeupRequired' is true, then try to wakeup
  the Application Processor by sending a specific CAN message. It is possible that
  the waking up is already in progress. This is okay since Vehicle Processor
  should ignore wakeup message if a wakeup is already in progress.

* When 'WakeupRequired' is updated from false to true, if there are unexpired
  pending tasks in the task queue, try to wakeup Application Processor.

  This is to handle the situation when a task arrives while the device is
  shutting down. During the device shutdown, the channel to deliver the remote
  tasks to Application Processor is shutdown so the new task will be added to the
  task queue. 'WakeupRequired' will be set to false to prevent the wakeup
  message preventing the shutdown. After the shutdown is complete,
  'WakeupRequired' will be set to true and this wakeup client must try to wake
  up the device again to execute the pending tasks.

* Every pending task has a timeout: 'kTaskTimeoutInMs'. If the pending task
  is not delivered to remote access HAL before the timeout (through
  GetRemoteTasks), the task timed out and a warning message is logged.

  In real implementation, this kTaskTimeoutInMs has to be set long enough to
  allow an Android bootup to happen. 20s is a reasonable value. When a task
  timed out, the wakeup client should also report to remote task server about
  the task timeout failure.

## How to build the test wakeup client

* Under android root: `source build/envsetup.sh`

* Add
  ```
  PRODUCT_SOONG_NAMESPACES += hardware/interfaces/automotive/remoteaccess/test_grpc_server/lib`
  ```

  to `device/generic/car/common/car.mk`.

* `lunch sdk_car_x86_64-userdebug`

* `make -j TestWakeupClientServer`

* `make -j ApPOwerControlLib`

## How to push the test wakeup client to a TCU which runs Android.

* Make the target device writable:

  `adb root`

  `adb remount` (remount might take a while)

  `adb reboot`

  `adb root`

  `adb remount`

* Under android root: `cd $ANDROID_PRODUCT_OUT`

* `adb push vendor/bin/TestWakeupClientServer /vendor/bin`

* `adb push vendor/lib64/ApPowerControlLib.so /vendor/lib64`

* `adb shell`

* `su`

* `/vendor/bin/TestWakeupClientServer`

## How to build and test the test wakeup client using one car emulator.

In this test setup we will use one car emulator
(sdk_car_x86_64-userdebug). We assume both the TCU and the remote access HAL
runs on the same Android system, and they communicate through local loopback
interface.

* Under android root, `source build/envsetup.sh`

* Add
  ```
  PRODUCT_SOONG_NAMESPACES += hardware/interfaces/automotive/remoteaccess/test_grpc_server/lib`
  ```

  to `device/generic/car/common/car.mk`.

* `lunch sdk_car_x86_64-userdebug`

* `m -j`

* Run the emulator, the '-read-only' flag is required to run multiple instances:

  `emulator -writable-system -read-only`

* The android lunch target: sdk_car_x86_64-userdebug and
  cf_x86_64_auto-userdebug already contains the default remote access HAL. For
  other lunch target, you can add the default remote access HAL by adding
  'android.hardware.automotive.remoteaccess@V2-default-service' to
  'PRODUCT_PACKAGES' variable in mk file, see `device/generic/car/common/car.mk`
  as example.

  To verify whether remote access HAL is running, you can use the following
  command to check:

  `dumpsys android.hardware.automotive.remoteaccess.IRemoteAccess/default`

* Make the target device writable:

  `adb root`

  `adb remount` (remount might take a while)

  `adb reboot`

  `adb root`

  `adb remount`

* `make -j TestWakeupClientServer`

* `make -j ApPOwerControlLib`

* `adb push $ANDROID_PRODUCT_OUT/vendor/bin/TestWakeupClientServer /vendor/bin`

* `adb push $ANDROID_PRODUCT_OUT/vendor/lib64/ApPowerControlLib.so /vendor/lib64`

* `adb shell`

* `emulator_car_x86_64:/ # su`

* `emulator_car_x86_64:/ # /vendor/bin/TestWakeupClientServer`

* Remote access HAL should start by default when the car emulator starts. Now
  the test wake up client should also be running and generating fake tasks.

  Start a new session under android root

  `source build/envsetup.sh`

  `lunch sdk_car_x86_64-userdebug`

  `adb shell`

  `emulator_car_x86_64:/ # su`

* Issue the command to start a simple debug callback that will capture all the
  received tasks at the remote access HAL side:

  `emulator_car_x86_64:/ # dumpsys android.hardware.automotive.remoteaccess.IRemoteAccess/default --start-debug-callback`

* Issue the following debug command to remote access HAL to establish the
  communication channel between it and the test wakeup client. This command
  also notifies that wakeup is not required:

  `emulator_car_x86_64:/ # dumpsys android.hardware.automotive.remoteaccess.IRemoteAccess/default --set-ap-state 1 0`

* Wait for a while, issue the following command to show the received fake tasks:

  `emulator_car_x86_64:/ # dumpsys android.hardware.automotive.remoteaccess.IRemoteAccess/default --show-task`

  You should expect to see some received tasks printed out.

* Simulate the Application Processor is shutting down by issuing the following
  command:

  `emulator_car_x86_64:/ # dumpsys android.hardware.automotive.remoteaccess.IRemoteAccess/default --set-ap-state 0 0`

* Wait for a while, issue the following command to show received tasks again:

  `emulator_car_x86_64:/ # dumpsys android.hardware.automotive.remoteaccess.IRemoteAccess/default --show-task`

  You should expect to see no new tasks received since remote access HAL already
  closed the communication channel.

* Simulate the Application Processor is already shutdown and wake up is required
  now:

  `emulator_car_x86_64:/ # dumpsys android.hardware.automotive.remoteaccess.IRemoteAccess/default --set-ap-state 0 1`

  Now you should expect to see the test wakeup client printing out messages
  that it is trying to wake up application processor.

* Simulate the Application Processor is waken up:

  `emulator_car_x86_64:/ # dumpsys android.hardware.automotive.remoteaccess.IRemoteAccess/default --set-ap-state 1 0`

* A new communication channel should have been established and all pending
  non-expired tasks should be delivered to the remote access HAL.

  `emulator_car_x86_64:/ # dumpsys android.hardware.automotive.remoteaccess.IRemoteAccess/default --show-task`

* Now you can issue `ctrl c` on the first adb shell to stop the test wakeup
  client.

* After the test, you can use `ctrl D` to exit the adb shell.

## How to build and test the test wakeup client using two car emulators.

In this test case, we are going to use two car emulators, one as the
Application Processor, one as the TCU.

* Change the IP address to allow IP communication between different emulator
  instances. For detail about why we change it this way, see [interconnecting
  emulator instance](https://developer.android.com/studio/run/emulator-networking#connecting).

  Change 'DGRPC_SERVICE_ADDRESS' in `[android_root]/hardware/interfaces/automotive/remoteaccess/test_grpc_server/impl/Android.bp` to
  `10.0.2.15:50051`.

  Change `DGRPC_SERVICE_ADDRESS` in '[android_root]/hardware/interfaces/automotive/remoteaccess/hal/defaut/Android.bp' to
  `10.0.2.2:50051`.

* Under android root: `source build/envsetup.sh`

* `lunch sdk_car_x86_64-userdebug`

*  `m -j`

* Start one car emulator as TCU

  `emulator -writable-system -read-only`

* Start a new shell session. Connect to the emulator's console,
  see [Start and stop a console session](https://developer.android.com/studio/run/emulator-console#console-session)
  for detail.

  `telnet localhost 5554`

* `auth auth_token` where auth_token must match the contents of the
  `~/.emulator_console_auth_token` file.

* `redir add tcp:50051:50051`

* Exit the telnet session using 'ctrl-C'

  Make the target device writable:

  Under android root:

  `source build/envsetup.sh`

  `lunch sdk_car_x86_64-userdebug`

  `adb root`

  `adb remount` (remount might take a while)

  `adb reboot`

  `adb root`

  `adb remount`

* `make -j TestWakeupClientServer`

* `adb push $ANDROID_PRODUCT_OUT/vendor/bin/TestWakeupClientServer /vendor/bin`

* `adb shell`

* `emulator_car_x86_64:/ # su`

* `emulator_car_x86_64:/ # /vendor/bin/TestWakeupClientServer`

* Start a new shell under android root, start another car emulator as the Application Processor:

  `source build/envsetup.sh`

  `lunch sdk_car_x86_64-userdebug`

  `emulator -writable-system -read-only`

* Open a new shell under android root:

  `source build/envsetup.sh`

  `lunch sdk_car_x86_64-userdebug`

* Connect to adb shell for the application processor:

  `adb -s emulator-5556 shell`

  `emulator_car_x86_64:/ # su`

* Follow the test instructions for one car emulator using the 'dumpsys'
  commands.

* After the test, you can use `ctrl D` to exit the adb shell.
