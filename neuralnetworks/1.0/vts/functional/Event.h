#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_0_EVENT_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_0_EVENT_H

#include <android/hardware/neuralnetworks/1.0/IEvent.h>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <mutex>
#include <thread>

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

using ReturnedStatus = ::android::hardware::neuralnetworks::V1_0::Status;

/**
 * The Event class is used internally by the Neuralnetworks runtime to
 * synchronize between different threads. An asynchronous task is launched
 * paired with an event object. When a client thread requires the output being
 * processed by the asynchronous task, the client thread can wait for the result
 * and be blocked until it has completed or a timeout condition has been
 * reached, or poll the result periodically. Both poll and wait* may safely be
 * called concurrently, even on the same event. When the server thread has
 * completed, it should immediately call "notify" to indicate the corresponding
 * output has been produced and awaken any client threads waiting on the event.
 *
 * This class exists to enable synchronization across HIDL. When synchronization
 * is only required in the same process, consider using std::future, std::mutex,
 * std::condition_variable, or std::experimental::latch instead.
 */
struct Event : public IEvent {
    Event();
    ~Event() override;

    /**
     * Event::Status::WAITING -- The corresponding asynchronous execution has
     *                           not yet finished.
     * Event::Status::SUCCESS -- The corresponding asynchronous execution has
     *                           succeeded and the output is ready to be
     *                           consumed.
     * Event::Status::TIMEOUT -- The calling thread has waited longer than the
     *                           user has specified. This only applies to the
     *                           methods Event::wait_for and Event::wait_until.
     * Event::Status::ERROR   -- The corresponding asynchronous execution has
     *                           failed to properly execute.
     */
    enum class Status : uint32_t {
        WAITING,
        SUCCESS,
        TIMEOUT,
        ERROR,
    };

    /**
     * IEvent::notify marks the event with the return status of the
     * asynchronous call the event is paired with and enables all
     * prior and future wait calls on the Event object to proceed. The
     * call to IEvent::notify happens before any wait* calls on
     * this event return (except in the case of TIMEOUT) and before
     * any poll calls that see the resulting status. The asynchronous
     * call the event is paired with must ensure that any update to
     * state that should be visible to the caller of wait* or poll
     * happens before the call to IEvent::notify.
     *
     * IEvent::notify can be called at most once on a given event.
     *
     * @param neuralnetworks::V1_0::Status SUCCESS or ERROR
     */
    Return<void> notify(ReturnedStatus status) override;

    /**
     * Event::poll returns the current status of the event.
     *
     * @return Status SUCCESS, ERROR, or WAITING
     */
    Event::Status poll();

    /**
     * Event::wait blocks until the event has been signaled.
     *
     * @return Status SUCCESS or ERROR
     */
    Event::Status wait();

    /**
     * Event::wait_for blocks until the event has been signaled or the time
     * duration from the time the wait_for function was called has expired,
     * whichever comes first.
     *
     * @return Status SUCCESS, ERROR, or TIMEOUT
     */
    template<class Rep, class Period>
    Event::Status wait_for(const std::chrono::duration<Rep,Period>& timeout_duration);

    /**
     * Event::wait_until blocks until the event has been signaled or a certain
     * time has been reached, whichever comes first.
     *
     * @return Status SUCCESS, ERROR, or TIMEOUT
     */
    template<class Clock, class Duration>
    Event::Status wait_until(const std::chrono::time_point<Clock,Duration>& timeout_duration);

    /**
     * Event::on_finish binds a callback function to the event. The
     * callback will be executed when IEvent::notify is called, before
     * any calls to wait* return. (Note that wait_for or wait_until
     * can return TIMEOUT before IEvent::notify is called for the
     * first time, and hence before the callback is executed.)
     *
     * The callback function must not synchronize with or otherwise
     * access the event object it is bound to.
     *
     * Event::on_finish can be called at most once on a given event.
     *
     * @param callback Function to be invoked the first time IEvent::notify is
     *                 called. Must have a target -- i.e., must not compare equal
     *                 to nullptr. Callback returns true if it successfully
     *                 completes, false if it fails.
     * @return bool True if the callback was successfully bound, false if
     *              unsuccessful.
     *
     * TODO: What if notify has already been called before on_finish?
     * TODO: Why does the return value of the callback matter?
     */
    bool on_finish(std::function<bool(void)> callback);

    /**
     * Event::bind_thread binds a thread to the event for later use by
     * Event::join_thread.
     *
     * The thread must be passed using std::move.
     *
     * Once a thread is bound with Event::bind_thread, the client code
     * should ensure that one of the following occurs before the event is
     * destroyed:
     * - Event::join_thread has been called.
     * - Event::wait has been called.
     * - Event::wait_for has been called and returned other than TIMEOUT.
     * - Event::wait_until has been called and returned other than TIMEOUT.
     *
     * The bound thread shall not call any Event method with the exception of
     * IEvent::notify, which it will call when the thread has finished its
     * computation.
     *
     * Event::bind_thread can be called at most once on a given event.
     *
     * @param asyncThread Thread to be bound to the event. The thread object
     *                    must represent a thread of execution -- i.e.,
     *                    asyncThread.joinable() must be true.
     * @return bool True if successful, false if thread was not properly bound.
     */
    bool bind_thread(std::thread&& asyncThread);

    /**
     * Event::join_thread ensures that the thread (if any) bound to
     * this event with Event::bind_thread has fully finished and
     * cleaned its resources. It is legal to call this function
     * multiple times, concurrently or sequentially.
     */
    void join_thread();

 private:
    // Same as Event::join_thread but assumes we already hold a lock on mMutex.
    void join_thread_locked();

    Status                    mStatus;
    std::mutex                mMutex;
    std::condition_variable   mCondition;
    std::function<bool(void)> mCallback;
    std::thread               mThread;
};


// template function implementations

template<class Rep, class Period>
Event::Status Event::wait_for(const std::chrono::duration<Rep,Period>& timeout_duration) {
    std::unique_lock<std::mutex> lock(mMutex);
    std::cv_status status = mCondition.wait_for(lock, timeout_duration,
                                                [this]{return mStatus != Status::WAITING;});
    if (status != std::cv_status::timeout) {
        join_thread_locked();
    }
    return status != std::cv_status::timeout ? mStatus : Status::TIMEOUT;
}

template<class Clock, class Duration>
Event::Status Event::wait_until(const std::chrono::time_point<Clock,Duration>& timeout_time) {
    std::unique_lock<std::mutex> lock(mMutex);
    std::cv_status status = mCondition.wait_until(lock, timeout_time,
                                                  [this]{return mStatus != Status::WAITING;});
    if (status != std::cv_status::timeout) {
        join_thread_locked();
    }
    return status != std::cv_status::timeout ? mStatus : Status::TIMEOUT;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_NEURALNETWORKS_V1_0_EVENT_H
