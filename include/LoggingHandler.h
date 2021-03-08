#pragma once 

#include "IHandler.h"
#include "TimeLogger.h"

#include <iostream>

template <typename T>
class LoggingHandler : public IHandler<T> {
 public:
    explicit LoggingHandler(IHandler<T>& next_handler_);
    void handle(T&& cl) override;

 private:
    IHandler<T>& next_handler;
};

template <typename T>
LoggingHandler<T>::LoggingHandler(IHandler<T>& next_handler_) : 
        next_handler(next_handler_) {}

template <typename T>
void LoggingHandler<T>::LoggingHandler::handle(T&& cl) {
    {
        TimeLogger<std::chrono::milliseconds> l(std::cout);
        next_handler.handle(std::move(cl));
    }
}