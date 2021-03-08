#pragma once 

#include <iomanip>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/config.hpp>
#include <openssl/sha.h>


namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>

// Report a failure
void fail(beast::error_code ec, char const* what) {
    std::cerr << what << ": " << ec.message() << "\n";
}

template<class Stream>
struct send_lambda {
    Stream& stream_;
    bool& close_;
    beast::error_code& ec_;

    explicit send_lambda(
            Stream& stream,
            bool& close,
            beast::error_code& ec) : 
        stream_(stream), 
        close_(close), 
        ec_(ec) {}

    template<bool isRequest, class Body, class Fields>
    void operator()(http::message<isRequest, Body, Fields>&& msg) const {
        // Determine if we should close the connection after
        close_ = msg.need_eof();

        // We need the serializer here because the serializer requires
        // a non-const file_body, and the message oriented version of
        // http::write only works with const messages.
        http::serializer<isRequest, Body, Fields> sr{msg};
        http::write(stream_, sr, ec_);
    }
};

std::string sha256(const std::string& str) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(hash, &sha256);
    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

template<class Body, class Allocator>
std::string parse_host(const http::request<Body, http::basic_fields<Allocator>>& request) {
    std::string host = request.at(http::field::host).to_string();
    size_t pos = host.find(":");
    if (pos != std::string::npos) {
        host = host.substr(0, pos);
    }

    return host;
}