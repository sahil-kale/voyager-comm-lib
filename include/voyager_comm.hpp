#ifndef VOYAGER_COMM_HPP
#define VOYAGER_COMM_HPP
#include <stdint.h>

namespace voyager_comm {

// Create a new class that is templated on the message type
// This class is a 'pipe' that can be used to send and receive messages
// of the templated type.
template <typename T>
class Channel {
   public:
    enum class PublishStatus {
        SUCCESS,
        FULL,
    };

    enum class SubscribeStatus {
        SUCCESS,
        FULL,
        INVALID_PARAMETERS,
    };

    typedef uint32_t subscriptionidx_t;

    class SubscribeResult {
       public:
        SubscribeResult(SubscribeStatus status, subscriptionidx_t num_subscribers)
            : result(status), num_subscribers(num_subscribers) {}
        SubscribeStatus result = 0;
        subscriptionidx_t index = 0;  // Index of the callback in the callback table, only valid if result == SUCCESS and the
                                      // operation is a subscription
        subscriptionidx_t num_subscribers = 0;
    };

    Channel() = default;
    ~Channel() = default;

    // Delete the copy constructor and copy assignment operator
    // This prevents the Channel from being copied, which can be
    // done by accident and cause the channel to silently fail.
    // There is no reason to copy a channel, so we delete these

    // @note If you end up with a compiler error here, you are trying to copy a channel instead of
    // pass a reference (or pointer) to it. This is a common mistake.
    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    /**************API***********/
    // Define a callback function type
    typedef void (*Callback)(const T&);

    // Define a trampoline function type
    typedef void (*Trampoline)(const T&, void*);

    // Template struct for holding member function callback
    template <typename U>
    struct MemberFunctionCallback {
        void (U::*func)(const T&);  // Pointer to the member function callback (class U is the class type, T is the message type)
        U* obj;                     // Pointer to the class instance object that the callback function belongs to
    };

    /**
     * @brief Subscribe to the channel
     * @param callback The callback function to subscribe, must be a member function of class U
     * @return SubscribeStatus::SUCCESS if the callback was subscribed successfully
     * @note This method is used for subscribing to a member function callback and requires
     *     a context parameter
     */
    template <typename U>
    SubscribeResult Subscribe(MemberFunctionCallback<U>& callback) {
        // Call the default Subscribe method with a pointer to the trampoline
        // function and a pointer to the member function callback.

        Callback cb = reinterpret_cast<Callback>(&callback);
        // Trampoline function
        // What we want to do is bridge the gap between the member function callback
        // and the default Subscribe method. We can do this by creating a trampoline
        // function that takes the message and the context (the member function callback)
        // as parameters. The trampoline function can then call the member function
        // callback with the message.
        Trampoline trampoline = [](const T& msg, void* context) {
            auto* cb = reinterpret_cast<MemberFunctionCallback<U>*>(context);

            // Calls the member function callback with the message via the object's pointer
            (cb->obj->*cb->func)(msg);
        };
        return SubscribeBase(cb, trampoline);
    }

    /**
     * @brief Subscribe to the channel
     * @param cb The callback function to subscribe
     * @return SubscribeStatus::SUCCESS if the callback was subscribed successfully
     * @note This method is used for subscribing to a free function callback and does not
     *      require a context parameter
     */
    SubscribeResult SubscribeNoContext(Callback cb) { return SubscribeBase(cb, nullptr); }

    /**
     * @brief Publish a message to all subscribers
     * @param msg The message to publish
     * @return PublishStatus::SUCCESS if the message was published successfully
     */
    PublishStatus Publish(const T& msg) {
        for (uint8_t i = 0; i < kMaxCallbacks; i++) {
            CallbackTableEntry& entry = callbacks_[i];
            if (entry.valid) {
                if (entry.trampoline == nullptr) {
                    entry.cb(msg);
                } else {
                    entry.trampoline(msg, reinterpret_cast<void*>(entry.cb));
                }
            }
        }
        return PublishStatus::SUCCESS;
    }

    /**
     * @brief Unsubscribe from the channel
     * @param index The index of the callback to unsubscribe
     * @return SubscribeStatus::SUCCESS if the callback was unsubscribed successfully
     * @note The index field of the SubscribeResult is not valid for this method
     */
    SubscribeResult Unsubscribe(subscriptionidx_t index) {
        SubscribeResult result(SubscribeStatus::SUCCESS, num_callbacks_);
        do {
            if (index >= kMaxCallbacks) {
                result.result = SubscribeStatus::INVALID_PARAMETERS;
                break;
            }

            // Check if the callback table entry is valid
            if (callbacks_[index].valid == false) {
                result.result = SubscribeStatus::INVALID_PARAMETERS;
                break;
            }

            // Make the callback table entry invalid
            callbacks_[index].valid = false;
            callbacks_[index].cb = nullptr;
            callbacks_[index].trampoline = nullptr;
            num_callbacks_--;
            result.num_subscribers = num_callbacks_;

        } while (0);
        return result;
    }

    static constexpr uint32_t kMaxCallbacks = 32;

    /**
     * @brief Get the number of callbacks subscribed to the channel
     * @return The number of callbacks subscribed to the channel
     * @note This method is intended for testing purposes only
     */
    subscriptionidx_t GetNumCallbacks() { return num_callbacks_; }

   private:
    typedef struct {
        bool valid = false;
        Callback cb = nullptr;
        Trampoline trampoline = nullptr;
    } CallbackTableEntry;

    CallbackTableEntry callbacks_[kMaxCallbacks] = {};
    uint8_t num_callbacks_ = 0;  // Number of callbacks subscribed

    /**
     * @brief Base Subscribe method
     * @param cb The callback function to subscribe
     * @param trampoline The trampoline function to use
     * @return SubscribeStatus::SUCCESS if the callback was subscribed successfully
     */
    SubscribeResult SubscribeBase(Callback cb, Trampoline trampoline) {
        SubscribeResult result(SubscribeStatus::SUCCESS, num_callbacks_);
        do {
            if (num_callbacks_ >= kMaxCallbacks) {
                result.result = SubscribeStatus::FULL;
                break;
            }
            if (cb == nullptr) {
                result.result = SubscribeStatus::INVALID_PARAMETERS;
                break;
            }

            callbacks_[num_callbacks_].cb = cb;
            callbacks_[num_callbacks_].trampoline = trampoline;
            callbacks_[num_callbacks_].valid = true;
            result.index = num_callbacks_;
            num_callbacks_++;
            result.num_subscribers = num_callbacks_;
        } while (0);
        return result;
    }
};

}  // namespace voyager_comm

#endif  // VOYAGER_COMM_HPP
