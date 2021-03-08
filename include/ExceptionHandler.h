#pragma once 

#include "IHandler.h"

#include <iostream>

template <typename T>
class ExceptionHandler : public IHandler<T> {
 public:
    explicit ExceptionHandler(IHandler<T>& next_handler_);
    void handle(T&& client) override;

 private:
    IHandler<T>& next_handler;
};

template <typename T>
ExceptionHandler<T>::ExceptionHandler(IHandler<T>& next_handler_) : 
        next_handler(next_handler_) {}

template <typename T>
void ExceptionHandler<T>::ExceptionHandler::handle(T&& client) {
    try {
        next_handler.handle(std::move(client));
    } catch(std::exception& ex) {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }
}