# Voyager Communication Library
Simple and lightweight inter-process communication library for supporting the observer software design pattern

## Overview
The [observer software design pattern](https://refactoring.guru/design-patterns/observer) is one where modules (also referred to as processes) publish data to 1 or more subscribers and act on the delivered data as required. A key advantage of the observer pattern is that it enables the design of loosely coupled software architecture, promoting reusability and abstraction between various software modules. 

Many solutions in desktop programming environments exist to provide inter-process communication (popularly, ROS's messaging stack is a go-to example) to help enable this design pattern. However, I struggled to find a simple, lightweight solution that would work on an embedded system that gave me lightweight inter-process communication. This library is a small C++ header-file implementation that allows a user to define channels that enable modules with a reference to a specific channel to publish and receive messages of a particular type in a callback in real-time.

![image](https://github.com/sahil-kale/voyager-comm-lib/assets/32375512/b9833b94-486e-463a-b31b-e1b0bb379fe3)

A note that due to its simplistic nature, there is potential **significant** misuse if attention is not paid to data synchronization, particularly in concurrent/real-time environments. I have listed the pitfalls of the library below and some solutions that the user may opt to implement.

## Library
The library works by defining channels that will support the transport of a specific message type `T`. A user is free to define multiple channels of the same message type, to support the composition of an application that may reuse the same message type but in different contexts.

### API
```template <typename U> SubscribeStatus Subscribe(MemberFunctionCallback<U>& callback)```
The `Subscribe` function permits a callback to a class's member function to be registered. The callback is registered by defining a struct `MemberFunctionCallback` which contains a pointer to the desired function callback and a context pointer that stores the object instance. See Usage Example for more information on defining this struct.

```SubscribeStatus SubscribeNoContext(Callback cb)```
The `SubscribeNoContext` function permits a callback to a regular free-function, function-pointer style. 

```SubscribeResult Unsubscribe(subscriptionidx_t index)```
The `Unsubscribe` function removes a callback registration given a particular subscription ID. 

```PublishStatus Publish(const T& msg)```
The `Publish` function broadcasts a message to the subscribers of a channel in the order that they are subscribed to.

### Pitfalls
1. The communication library iterates through each valid callback (order is not guaranteed) when a publish request is called. As a result, the publish event occurs in the current execution context and without delay. As a result, **protections must be employed when publishing and receving messages in concurrent environments**, especially regarding shared data management and task timing if applicable. 
2. The publishing interface is synchronous with the calling task - as a result, processing should be kept minimal in a callback (ideally, limited to copying the relevant message contents for processing later). 'Looping' should be paid attention to minimize the time spent outside the scope of the module and ending up with a run-away condition in the software (i.e. don't have a callback be able to trigger a condition that sends a message via another channel, or at least acknowledge that this is occurring within a system and be aware of its impact on the WCET of every module that publishes a message).

A general rule of thumb to avoid dealing with the issues incurred by using this simplistic inter-processor communication library is to treat the subscription of a message as though it is received through an ISR and employ similar techniques to minimize concurrency, processing and latency issues.

## Usage Example
See https://github.com/sahil-kale/voyager-comm-lib/blob/main/test/test_comm_api.cpp for examples of using the API from class-based and free-function examples.
