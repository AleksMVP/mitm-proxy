#pragma once

#include <boost/asio.hpp>
#include <memory>

using tcp = boost::asio::ip::tcp;

template <typename T>
class Acceptor {
 public:
    explicit Acceptor(int port);

    T accept();

 private:
    boost::asio::io_context io_context;
    tcp::acceptor acceptor;
};

template <typename T>
Acceptor<T>::Acceptor(int port) :
        acceptor(io_context, tcp::endpoint(tcp::v4(), port)) {}

template <typename T>
T Acceptor<T>::accept() {
    std::unique_ptr<tcp::socket> socket(std::make_unique<tcp::socket>(io_context));
    acceptor.accept(*socket);

    T cl(std::move(socket));

    return cl;
}