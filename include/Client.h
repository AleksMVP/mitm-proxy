#pragma once 

#include <memory>
#include <boost/asio.hpp>

using tcp = boost::asio::ip::tcp;

class Client {
 public:
    explicit Client(std::unique_ptr<tcp::socket> socket);
    Client(const Client& c) = delete;
    Client(Client&& c);

    Client& operator=(Client&& rhs);
    Client& operator=(const Client& rhs) = delete;

    size_t read(char* buffer, size_t size);

    template <typename T=boost::asio::streambuf>
    std::unique_ptr<T> read_until(const std::string& pattern);

    void write(const std::string& data);
    void write(char* buffer, size_t size);

    tcp::socket& get_socket();

    Client& operator<<(const std::string& data);

    ~Client();

 private:
    std::unique_ptr<tcp::socket> socket;
};

template <typename T>
std::unique_ptr<T> Client::read_until(const std::string& pattern) {
    std::unique_ptr<T> response(std::make_unique<T>());

    boost::system::error_code ec;
    boost::asio::read_until(*socket, *response, pattern, ec);

    if (ec) {
        return nullptr;
    }
    
    return response;
}