/*
 * Copyright (C) 2021 The Android Open Source Project
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

#ifndef android_hardware_automotive_vehicle_utils_include_VehicleObjectPool_H_
#define android_hardware_automotive_vehicle_utils_include_VehicleObjectPool_H_

#include <deque>
#include <map>
#include <memory>
#include <mutex>

#include <VehicleHalTypes.h>

#include <android-base/thread_annotations.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

// Handy metric mostly for unit tests and debug.
#define INC_METRIC_IF_DEBUG(val) PoolStats::instance()->val++;

struct PoolStats {
    std::atomic<uint32_t> Obtained{0};
    std::atomic<uint32_t> Created{0};
    std::atomic<uint32_t> Recycled{0};
    std::atomic<uint32_t> Deleted{0};

    static PoolStats* instance() {
        static PoolStats inst;
        return &inst;
    }
};

template <typename T>
struct Deleter {
    using OnDeleteFunc = std::function<void(T*)>;

    explicit Deleter(const OnDeleteFunc& f) : mOnDelete(f){};

    Deleter() = default;
    Deleter(const Deleter&) = default;

    void operator()(T* o) { mOnDelete(o); }

  private:
    OnDeleteFunc mOnDelete;
};

// This is std::unique_ptr<> with custom delete operation that typically moves the pointer it holds
// back to ObjectPool.
template <typename T>
using recyclable_ptr = typename std::unique_ptr<T, Deleter<T>>;

// Generic abstract object pool class. Users of this class must implement {@Code createObject}.
//
// This class is thread-safe. Concurrent calls to {@Code obtain} from multiple threads is OK, also
// client can obtain an object in one thread and then move ownership to another thread.
template <typename T>
class ObjectPool {
  public:
    using GetSizeFunc = std::function<size_t(const T&)>;

    ObjectPool(size_t maxPoolObjectsSize, GetSizeFunc getSizeFunc)
        : mMaxPoolObjectsSize(maxPoolObjectsSize), mGetSizeFunc(getSizeFunc){};
    virtual ~ObjectPool() = default;

    virtual recyclable_ptr<T> obtain() {
        std::scoped_lock<std::mutex> lock(mLock);
        INC_METRIC_IF_DEBUG(Obtained)
        if (mObjects.empty()) {
            INC_METRIC_IF_DEBUG(Created)
            return wrap(createObject());
        }

        auto o = wrap(mObjects.front().release());
        mObjects.pop_front();
        mPoolObjectsSize -= mGetSizeFunc(*o);
        return o;
    }

    ObjectPool& operator=(const ObjectPool&) = delete;
    ObjectPool(const ObjectPool&) = delete;

  protected:
    virtual T* createObject() = 0;

    virtual void recycle(T* o) {
        std::scoped_lock<std::mutex> lock(mLock);
        size_t objectSize = mGetSizeFunc(*o);

        if (objectSize > mMaxPoolObjectsSize ||
            mPoolObjectsSize > mMaxPoolObjectsSize - objectSize) {
            INC_METRIC_IF_DEBUG(Deleted)

            // We have no space left in the pool.
            delete o;
            return;
        }

        INC_METRIC_IF_DEBUG(Recycled)

        mObjects.push_back(std::unique_ptr<T>{o});
        mPoolObjectsSize += objectSize;
    }

    const size_t mMaxPoolObjectsSize;

  private:
    const Deleter<T>& getDeleter() {
        if (!mDeleter.get()) {
            Deleter<T>* d =
                    new Deleter<T>(std::bind(&ObjectPool::recycle, this, std::placeholders::_1));
            mDeleter.reset(d);
        }
        return *mDeleter.get();
    }

    recyclable_ptr<T> wrap(T* raw) { return recyclable_ptr<T>{raw, getDeleter()}; }

    mutable std::mutex mLock;
    std::deque<std::unique_ptr<T>> mObjects GUARDED_BY(mLock);
    std::unique_ptr<Deleter<T>> mDeleter;
    size_t mPoolObjectsSize GUARDED_BY(mLock);
    GetSizeFunc mGetSizeFunc;
};

#undef INC_METRIC_IF_DEBUG

// This class provides a pool of recyclable VehiclePropertyValue objects.
//
// It has only one overloaded public method - obtain(...), users must call this method when new
// object is needed with given VehiclePropertyType and vector size (for vector properties). This
// method returns a recyclable smart pointer to VehiclePropertyValue, essentially this is a
// std::unique_ptr with custom delete function, so recyclable object has only one owner and
// developers can safely pass it around. Once this object goes out of scope, it will be returned to
// the object pool.
//
// Some objects are not recyclable: strings and vector data types with vector
// length > maxRecyclableVectorSize (provided in the constructor). These objects will be deleted
// immediately once the go out of scope. There's no synchronization penalty for these objects since
// we do not store them in the pool.
//
// This class is thread-safe. Users can obtain an object in one thread and pass it to another.
//
// Sample usage:
//
//   VehiclePropValuePool pool;
//   auto v = pool.obtain(VehiclePropertyType::INT32);
//   v->propId = VehicleProperty::HVAC_FAN_SPEED;
//   v->areaId = VehicleAreaSeat::ROW_1_LEFT;
//   v->timestamp = elapsedRealtimeNano();
//   v->value->int32Values[0] = 42;
class VehiclePropValuePool {
  public:
    using RecyclableType =
            recyclable_ptr<aidl::android::hardware::automotive::vehicle::VehiclePropValue>;

    // Creates VehiclePropValuePool
    //
    // @param maxRecyclableVectorSize - vector value types (e.g. VehiclePropertyType::INT32_VEC)
    // with size equal or less to this value will be stored in the pool. If users tries to obtain
    // value with vector size greater than maxRecyclableVectorSize, user will receive a regular
    // unique pointer instead of a recyclable pointer. The object would not be recycled once it
    // goes out of scope, but would be deleted.
    // @param maxPoolObjectsSize - The approximate upper bound of memory each internal recycling
    // pool could take. We have 4 different type pools, each with 4 different vector size, so
    // approximately this pool would at-most take 4 * 4 * 10240 = 160k memory.
    VehiclePropValuePool(size_t maxRecyclableVectorSize = 4, size_t maxPoolObjectsSize = 10240)
        : mMaxRecyclableVectorSize(maxRecyclableVectorSize),
          mMaxPoolObjectsSize(maxPoolObjectsSize){};

    // Obtain a recyclable VehiclePropertyValue object from the pool for the given type. If the
    // given type is not MIXED or STRING, the internal value vector size would be set to 1.
    // If the given type is MIXED or STRING, all the internal vector sizes would be initialized to
    // 0.
    RecyclableType obtain(aidl::android::hardware::automotive::vehicle::VehiclePropertyType type);

    // Obtain a recyclable VehiclePropertyValue object from the pool for the given type. If the
    // given type is *_VEC or BYTES, the internal value vector size would be set to vectorSize. If
    // the given type is BOOLEAN, INT32, FLOAT, or INT64, the internal value vector size would be
    // set to 1. If the given type is MIXED or STRING, all the internal value vector sizes would be
    // set to 0. vectorSize must be larger than 0.
    RecyclableType obtain(aidl::android::hardware::automotive::vehicle::VehiclePropertyType type,
                          size_t vectorSize);
    // Obtain a recyclable VehicePropertyValue object that is a copy of src. If src does not contain
    // any value or the src property type is not valid, this function would return an empty
    // VehiclePropValue.
    RecyclableType obtain(
            const aidl::android::hardware::automotive::vehicle::VehiclePropValue& src);
    // Obtain a recyclable boolean object.
    RecyclableType obtainBoolean(bool value);
    // Obtain a recyclable int32 object.
    RecyclableType obtainInt32(int32_t value);
    // Obtain a recyclable int64 object.
    RecyclableType obtainInt64(int64_t value);
    // Obtain a recyclable float object.
    RecyclableType obtainFloat(float value);
    // Obtain a recyclable float object.
    RecyclableType obtainString(const char* cstr);
    // Obtain a recyclable mixed object.
    RecyclableType obtainComplex();

    VehiclePropValuePool(VehiclePropValuePool&) = delete;
    VehiclePropValuePool& operator=(VehiclePropValuePool&) = delete;

  private:
    static inline bool isSingleValueType(
            aidl::android::hardware::automotive::vehicle::VehiclePropertyType type) {
        return type == aidl::android::hardware::automotive::vehicle::VehiclePropertyType::BOOLEAN ||
               type == aidl::android::hardware::automotive::vehicle::VehiclePropertyType::INT32 ||
               type == aidl::android::hardware::automotive::vehicle::VehiclePropertyType::INT64 ||
               type == aidl::android::hardware::automotive::vehicle::VehiclePropertyType::FLOAT;
    }

    static inline bool isComplexType(
            aidl::android::hardware::automotive::vehicle::VehiclePropertyType type) {
        return type == aidl::android::hardware::automotive::vehicle::VehiclePropertyType::MIXED ||
               type == aidl::android::hardware::automotive::vehicle::VehiclePropertyType::STRING;
    }

    bool isDisposable(aidl::android::hardware::automotive::vehicle::VehiclePropertyType type,
                      size_t vectorSize) const {
        return vectorSize == 0 || vectorSize > mMaxRecyclableVectorSize || isComplexType(type);
    }

    RecyclableType obtainDisposable(
            aidl::android::hardware::automotive::vehicle::VehiclePropertyType valueType,
            size_t vectorSize) const;
    RecyclableType obtainRecyclable(
            aidl::android::hardware::automotive::vehicle::VehiclePropertyType type,
            size_t vectorSize);

    class InternalPool
        : public ObjectPool<aidl::android::hardware::automotive::vehicle::VehiclePropValue> {
      public:
        InternalPool(aidl::android::hardware::automotive::vehicle::VehiclePropertyType type,
                     size_t vectorSize, size_t maxPoolObjectsSize,
                     ObjectPool::GetSizeFunc getSizeFunc)
            : ObjectPool(maxPoolObjectsSize, getSizeFunc),
              mPropType(type),
              mVectorSize(vectorSize) {}

      protected:
        aidl::android::hardware::automotive::vehicle::VehiclePropValue* createObject() override;
        void recycle(aidl::android::hardware::automotive::vehicle::VehiclePropValue* o) override;

      private:
        bool check(aidl::android::hardware::automotive::vehicle::RawPropValues* v);

        template <typename VecType>
        bool check(std::vector<VecType>* vec, bool isVectorType) {
            return vec->size() == (isVectorType ? mVectorSize : 0);
        }

      private:
        aidl::android::hardware::automotive::vehicle::VehiclePropertyType mPropType;
        size_t mVectorSize;
    };
    const Deleter<aidl::android::hardware::automotive::vehicle::VehiclePropValue>
            mDisposableDeleter{
                    [](aidl::android::hardware::automotive::vehicle::VehiclePropValue* v) {
                        delete v;
                    }};

    mutable std::mutex mLock;
    const size_t mMaxRecyclableVectorSize;
    const size_t mMaxPoolObjectsSize;
    // A map with 'property_type' | 'value_vector_size' as key and a recyclable object pool as
    // value. We would create a recyclable pool for each property type and vector size combination.
    std::map<int32_t, std::unique_ptr<InternalPool>> mValueTypePools GUARDED_BY(mLock);
};

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_utils_include_VehicleObjectPool_H_
