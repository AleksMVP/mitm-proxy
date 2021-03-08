#pragma once

template <typename T>
class IHandler {
 public:
    virtual void handle(T&& data) = 0;
    // virtual ~IHandler() = 0;
};