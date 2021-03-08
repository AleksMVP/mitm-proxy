#pragma once

#include <boost/asio.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace net = boost::asio;    // from <boost/asio.hpp>
namespace http = boost::beast::http;   // from <boost/beast/http.hpp>
using tcp = net::ip::tcp; 

class RequestMaker {
 public:
    template <class Body, class Allocator>
    auto make_http_request(http::request<Body, http::basic_fields<Allocator>>&& request);
    template <class Body, class Allocator>
    auto make_https_request(http::request<Body, http::basic_fields<Allocator>>&& request);

 private:
    std::string cut_host(std::string target);
    std::pair<std::string, std::string> get_host_port(const std::string& host);

    net::io_context ioc;
    int version = 11;
};

std::string RequestMaker::cut_host(std::string target) {
    // http://mail.ru/news/ -> /news/
    int i = 0;
    for (int j = 0; i < target.length(); i++) {
        if (target[i] == '/') {
            j++;
            if (j == 3) {
                break;
            }
        }
    }

    target = target.substr(i, target.length());

    return target;
}

template <class Body, class Allocator>
auto RequestMaker::make_http_request(http::request<Body, http::basic_fields<Allocator>>&& request) {
    std::string host = request.at(http::field::host).to_string();
    std::string port = "80";
    auto pos = host.find(":");

    if (pos != std::string::npos) {
        port = host.substr(pos + 1, host.length());
        host = host.substr(0, pos);
    }

    request.erase(http::field::proxy_connection);
    request.target(cut_host(request.target()));

    tcp::resolver resolver(ioc);
    beast::tcp_stream stream(ioc);

    auto const results = resolver.resolve(host.c_str(), port.c_str());
    beast::get_lowest_layer(stream).connect(results);
    http::write(stream, request);

    beast::flat_buffer buffer;
    http::response<http::dynamic_body> res;
    http::read(stream, buffer, res);

    return res;
}

template <class Body, class Allocator>
auto RequestMaker::make_https_request(http::request<Body, http::basic_fields<Allocator>>&& request) {
    std::string host = request.at(http::field::host).to_string();
    std::string port = "443";
    auto pos = host.find(":");
    // port detect
    if (pos != std::string::npos) {
        port = host.substr(pos + 1, host.length());
        host = host.substr(0, pos);
    }

    ssl::context ctx(ssl::context::tlsv12_client);

    tcp::resolver resolver(ioc);
    beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

    if (! SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
        beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
        throw beast::system_error{ec};
    }

    auto const results = resolver.resolve(host.c_str(), port);
    beast::get_lowest_layer(stream).connect(results);
    stream.handshake(ssl::stream_base::client);
    http::write(stream, request);

    beast::flat_buffer buffer;
    http::response<http::dynamic_body> res;
    http::read(stream, buffer, res);

    beast::error_code ec;
    stream.shutdown(ec);
    if (ec == net::error::eof) {
        // Rationale:
        // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
        ec = {};
    }

    return res;
}