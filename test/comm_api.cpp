#include <gtest/gtest.h>

#include "voyager_comm.hpp"

using namespace voyager_comm;

class TestMessage0 {
   public:
    TestMessage0() : data_(0) {}

    uint32_t data_;
};

TEST(CommApi, Publish) {
    Channel<TestMessage0> channel;
    TestMessage0 msg;
    EXPECT_EQ(channel.Publish(msg), Channel<TestMessage0>::PublishStatus::SUCCESS);
}

class FakeSubscriber {
   public:
    FakeSubscriber() : num_messages_(0) {}

    void Callback(const TestMessage0& msg) {
        (void)msg;
        num_messages_++;
        most_recent_msg_ = msg;
    }
    TestMessage0 most_recent_msg_;
    uint32_t num_messages_;
};

// Publish a message and ensure the callback is called.
TEST(CommApi, PublishCallback) {
    Channel<TestMessage0> channel;
    FakeSubscriber subscriber;
    Channel<TestMessage0>::MemberFunctionCallback<FakeSubscriber> callback{&FakeSubscriber::Callback, &subscriber};

    EXPECT_EQ(channel.Subscribe(callback), Channel<TestMessage0>::SubscribeStatus::SUCCESS);
    TestMessage0 msg;
    EXPECT_EQ(channel.Publish(msg), Channel<TestMessage0>::PublishStatus::SUCCESS);
    EXPECT_EQ(subscriber.num_messages_, 1);
}

// Publish a message and ensure the callback is called without a fake subscriber (use a free function).
static TestMessage0 most_recent_msg;
void FreeFunctionCallback(const TestMessage0& msg) {
    (void)msg;
    most_recent_msg = msg;
}

TEST(CommApi, PublishFreeFunctionCallback) {
    Channel<TestMessage0> channel;
    EXPECT_EQ(channel.SubscribeNoContext(&FreeFunctionCallback), Channel<TestMessage0>::SubscribeStatus::SUCCESS);
    TestMessage0 msg;
    msg.data_ = 0x42;
    EXPECT_EQ(channel.Publish(msg), Channel<TestMessage0>::PublishStatus::SUCCESS);
    EXPECT_EQ(most_recent_msg.data_, 0x42);
}

// Test a null callback
TEST(CommApi, PublishNullCallback) {
    Channel<TestMessage0> channel;
    EXPECT_EQ(channel.SubscribeNoContext(nullptr), Channel<TestMessage0>::SubscribeStatus::INVALID_PARAMETERS);
}

// Test multiple callbacks with fake subscribers
TEST(CommApi, PublishMultipleCallbacks) {
    Channel<TestMessage0> channel;
    FakeSubscriber subscriber1;
    FakeSubscriber subscriber2;
    Channel<TestMessage0>::MemberFunctionCallback<FakeSubscriber> callback1{&FakeSubscriber::Callback, &subscriber1};
    Channel<TestMessage0>::MemberFunctionCallback<FakeSubscriber> callback2{&FakeSubscriber::Callback, &subscriber2};

    EXPECT_EQ(channel.Subscribe(callback1), Channel<TestMessage0>::SubscribeStatus::SUCCESS);
    EXPECT_EQ(channel.Subscribe(callback2), Channel<TestMessage0>::SubscribeStatus::SUCCESS);
    TestMessage0 msg;
    EXPECT_EQ(channel.Publish(msg), Channel<TestMessage0>::PublishStatus::SUCCESS);
    EXPECT_EQ(subscriber1.num_messages_, 1);
    EXPECT_EQ(subscriber2.num_messages_, 1);
}

// Test multiple callbacks with free functions
TEST(CommApi, PublishMultipleFreeFunctionCallbacks) {
    Channel<TestMessage0> channel;
    EXPECT_EQ(channel.SubscribeNoContext(&FreeFunctionCallback), Channel<TestMessage0>::SubscribeStatus::SUCCESS);
    EXPECT_EQ(channel.SubscribeNoContext(&FreeFunctionCallback), Channel<TestMessage0>::SubscribeStatus::SUCCESS);
    TestMessage0 msg;
    msg.data_ = 0x42;
    EXPECT_EQ(channel.Publish(msg), Channel<TestMessage0>::PublishStatus::SUCCESS);
    EXPECT_EQ(most_recent_msg.data_, 0x42);
}

// Test multiple publishing
TEST(CommApi, PublishMultiple) {
    Channel<TestMessage0> channel;
    FakeSubscriber subscriber;
    Channel<TestMessage0>::MemberFunctionCallback<FakeSubscriber> callback{&FakeSubscriber::Callback, &subscriber};

    EXPECT_EQ(channel.Subscribe(callback), Channel<TestMessage0>::SubscribeStatus::SUCCESS);
    TestMessage0 msg;
    EXPECT_EQ(channel.Publish(msg), Channel<TestMessage0>::PublishStatus::SUCCESS);
    EXPECT_EQ(subscriber.num_messages_, 1);
    EXPECT_EQ(channel.Publish(msg), Channel<TestMessage0>::PublishStatus::SUCCESS);
    EXPECT_EQ(subscriber.num_messages_, 2);
}