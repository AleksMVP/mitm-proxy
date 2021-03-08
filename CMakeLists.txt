cmake_minimum_required(VERSION 3.15)

project(MyProject)

set(CMAKE_CXX_STANDARD 17)

include_directories("/usr/local/include")
include_directories("include")
include_directories("/usr/local/opt/openssl/include")
link_directories("/usr/lib")
link_directories("/usr/local/lib")
link_directories("/usr/local/opt/openssl/lib")


find_package(Boost REQUIRED system)
find_package(OpenSSL REQUIRED)

add_executable(MyProject src/main.cpp src/Client.cpp)
target_link_libraries (MyProject -lboost_coroutine -lboost_chrono -lboost_thread-mt -lboost_system OpenSSL::SSL OpenSSL::Crypto)
