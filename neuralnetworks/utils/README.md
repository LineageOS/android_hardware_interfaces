# NNAPI Conversions

`convert` fails if either the source type or the destination type is invalid, and it yields a valid
object if the conversion succeeds. For example, let's say that an enumeration in the current
version has fewer possible values than the "same" canonical enumeration, such as `OperationType`.
The new value of `HARD_SWISH` (introduced in Android R / NN HAL 1.3) does not map to any valid
existing value in `OperationType`, but an older value of `ADD` (introduced in Android OC-MR1 / NN
HAL 1.0) is valid. This can be seen in the following model conversions:

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
