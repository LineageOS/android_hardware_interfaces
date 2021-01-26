# NNAPI Conversions

`convert` fails if either the source type or the destination type is invalid, and it yields a valid
object if the conversion succeeds. For example, let's say that an enumeration in the current version
has fewer possible values than the "same" canonical enumeration, such as `OperationType`. The new
value of `HARD_SWISH` (introduced in Android R / NN HAL 1.3) does not map to any valid existing
value in `OperationType`, but an older value of `ADD` (introduced in Android OC-MR1 / NN HAL 1.0) is
valid. This can be seen in the following model conversions:

```cpp
// Unsuccessful conversion
const nn::Model canonicalModel = createModelWhichHasV1_3Operations();
const nn::Result<V1_0::Model> maybeVersionedModel = V1_0::utils::convert(canonicalModel);
EXPECT_FALSE(maybeVersionedModel.has_value());
```
```cpp
// Successful conversion
const nn::Model canonicalModel = createModelWhichHasOnlyV1_0Operations();
const nn::Result<V1_0::Model> maybeVersionedModel = V1_0::utils::convert(canonicalModel);
ASSERT_TRUE(maybeVersionedModel.has_value());
const V1_0::Model& versionedModel = maybeVersionedModel.value();
EXPECT_TRUE(V1_0::utils::valid(versionedModel));
```

`V1_X::utils::convert` does not guarantee that all information is preserved. For example, In the
case of `nn::ErrorStatus`, the new value of `MISSED_DEADLINE_TRANSIENT` can be represented by the
existing value of `V1_0::GENERAL_FAILURE`:

```cpp
// Lossy Canonical -> HAL -> Canonical conversion
const nn::ErrorStatus canonicalBefore = nn::ErrorStatus::MISSED_DEADLINE_TRANSIENT;
const V1_0::ErrorStatus versioned = V1_0::utils::convert(canonicalBefore).value();
const nn::ErrorStatus canonicalAfter = nn::convert(versioned).value();
EXPECT_NE(canonicalBefore, canonicalAfter);
```

However, `nn::convert` is guaranteed to preserve all information:

```cpp
// Lossless HAL -> Canonical -> HAL conversion
const V1_0::ErrorStatus versionedBefore = V1_0::ErrorStatus::GENERAL_FAILURE;
const nn::ErrorStatus canonical = nn::convert(versionedBefore).value();
const V1_0::ErrorStatus versionedAfter = V1_0::utils::convert(canonical).value();
EXPECT_EQ(versionedBefore, versionedAfter);
```

The `convert` functions operate only on types that used in a HIDL method call directly. The
`unvalidatedConvert` functions operate on types that are either used in a HIDL method call directly
(i.e., not as a nested class) or used in a subsequent version of the NN HAL. Prefer using `convert`
over `unvalidatedConvert`.

# Interface Lifetimes across Processes

## HIDL

Some notes about HIDL interface objects and lifetimes across processes:

All HIDL interface objects inherit from `IBase`, which itself inherits from `::android::RefBase`. As
such, all HIDL interface objects are reference counted and must be owned through `::android::sp` (or
referenced through `::android::wp`). Allocating `RefBase` objects on the stack will log errors and
may result in crashes, and deleting a `RefBase` object through another means (e.g., "delete",
"free", or RAII-cleanup through `std::unique_ptr` or some equivalent) will result in double-free
and/or use-after-free undefined behavior.

HIDL/Binder manages the reference count of HIDL interface objects automatically across processes. If
a process that references (but did not create) the HIDL interface object dies, HIDL/Binder ensures
any reference count it held is properly released. (Caveat: it might be possible that HIDL/Binder
behave strangely with `::android::wp` references.)

If the process which created the HIDL interface object dies, any call on this object from another
process will result in a HIDL transport error with the code `DEAD_OBJECT`.

## AIDL

We use NDK backend for AIDL interfaces. Handling of lifetimes is generally the same with the
following differences:
* Interfaces inherit from `ndk::ICInterface`, which inherits from `ndk::SharedRefBase`. The latter
  is an analog of `::android::RefBase` using `std::shared_ptr` for reference counting.
* AIDL calls return `ndk::ScopedAStatus` which wraps fields of types `binder_status_t` and
  `binder_exception_t`. In case the call is made on a dead object, the call will return
  `ndk::ScopedAStatus` with exception `EX_TRANSACTION_FAILED` and binder status
  `STATUS_DEAD_OBJECT`.

# Protecting Asynchronous Calls

## Across HIDL

Some notes about asynchronous calls across HIDL:

For synchronous calls across HIDL, if an error occurs after the function was called but before it
returns, HIDL will return a transport error. For example, if the message cannot be delivered to the
server process or if the server process dies before returning a result, HIDL will return from the
function with the appropriate transport error in the `Return<>` object, which can be queried with
`Return<>::isOk()`, `Return<>::isDeadObject()`, `Return<>::description()`, etc.

However, HIDL offers no such error management in the case of asynchronous calls. By default, if the
client launches an asynchronous task and the server fails to return a result through the callback,
the client will be left waiting indefinitely for a result it will never receive.

In the NNAPI, `IDevice::prepareModel*` and `IPreparedModel::execute*` (but not
`IPreparedModel::executeSynchronously*`) are asynchronous calls across HIDL. Specifically, these
asynchronous functions are called with a HIDL interface callback object (`IPrepareModelCallback` for
`IDevice::prepareModel*` and `IExecutionCallback` for `IPreparedModel::execute*`) and are expected
to quickly return, and the results are returned at a later time through these callback objects.

To protect against the case when the server dies after the asynchronous task was called successfully
but before the results could be returned, HIDL provides an object called a "`hidl_death_recipient`,"
which can be used to detect when an interface object (and more generally, the server process) has
died. nnapi/hal/ProtectCallback.h's `DeathHandler` uses `hidl_death_recipient`s to detect when the
driver process has died, and `DeathHandler` will unblock any thread waiting on the results of an
`IProtectedCallback` callback object that may otherwise not be signaled. In order for this to work,
the `IProtectedCallback` object must have been registered via `DeathHandler::protectCallback()`.

## Across AIDL

We use NDK backend for AIDL interfaces. Handling of asynchronous calls is generally the same with
the following differences:
* AIDL calls return `ndk::ScopedAStatus` which wraps fields of types `binder_status_t` and
  `binder_exception_t`. In case the call is made on a dead object, the call will return
  `ndk::ScopedAStatus` with exception `EX_TRANSACTION_FAILED` and binder status
  `STATUS_DEAD_OBJECT`.
* AIDL interface doesn't contain asynchronous `IPreparedModel::execute`.
* Service death is handled using `AIBinder_DeathRecipient` object which is linked to an interface
  object using `AIBinder_linkToDeath`. nnapi/hal/aidl/ProtectCallback.h provides `DeathHandler`
  object that is a direct analog of HIDL `DeathHandler`, only using libbinder_ndk objects for
  implementation.
