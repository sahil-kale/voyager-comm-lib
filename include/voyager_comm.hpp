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

    // Modified Subscribe method
    template <typename U>
    SubscribeStatus Subscribe(MemberFunctionCallback<U>& callback) {
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

    SubscribeStatus SubscribeNoContext(Callback cb) { return SubscribeBase(cb, nullptr); }

    PublishStatus Publish(const T& msg) {
        for (uint8_t i = 0; i < num_callbacks_; i++) {
            if (callbacks_[i].trampoline == nullptr) {
                callbacks_[i].cb(msg);
            } else {
                callbacks_[i].trampoline(msg, reinterpret_cast<void*>(callbacks_[i].cb));
            }
        }
        return PublishStatus::SUCCESS;
    }

    static constexpr uint32_t kMaxCallbacks = 32;

   private:
    typedef struct {
        Callback cb;
        Trampoline trampoline = nullptr;
    } CallbackTableEntry;

    CallbackTableEntry callbacks_[kMaxCallbacks] = {};
    uint8_t num_callbacks_ = 0;

    SubscribeStatus SubscribeBase(Callback cb, Trampoline trampoline) {
        SubscribeStatus status = SubscribeStatus::SUCCESS;
        do {
            if (num_callbacks_ >= kMaxCallbacks) {
                status = SubscribeStatus::FULL;
                break;
            }
            if (cb == nullptr) {
                status = SubscribeStatus::INVALID_PARAMETERS;
                break;
            }

            callbacks_[num_callbacks_].cb = cb;
            callbacks_[num_callbacks_].trampoline = trampoline;
            num_callbacks_++;
        } while (0);
        return status;
    }
};

}  // namespace voyager_comm

#endif  // VOYAGER_COMM_HPP
