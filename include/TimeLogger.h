#pragma once

#include <chrono>
#include <ostream>

template <typename T>
class TimeLogger {
 public:
    explicit TimeLogger(std::ostream& stream) : stream(stream) {
        start = std::chrono::system_clock::now();
    }

    ~TimeLogger() {
        auto end = std::chrono::system_clock::now();
        stream << "Elapsed time: " 
               << std::chrono::duration_cast<T>(end - start).count() 
               << std::endl;
    }
 private:
    std::chrono::system_clock::time_point start;
    std::ostream& stream;
};