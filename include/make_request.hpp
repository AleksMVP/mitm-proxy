#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <sstream>

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http;   // from <boost/beast/http.hpp>
namespace net = boost::asio;    // from <boost/asio.hpp>
namespace ssl = net::ssl;       // from <boost/asio/ssl.hpp>
using tcp = net::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

auto make_request(http::request<http::string_body>&& request) {
    int version = 11;
    std::string host = request.at(http::field::host).to_string();
    std::string port = "443";
    auto pos = host.find(":");
    if (pos != std::string::npos) {
        port = host.substr(pos + 1, host.length());
        host = host.substr(0, pos);
    }

    // The io_context is required for all I/O
    net::io_context ioc;

    // The SSL context is required, and holds certificates
    ssl::context ctx(ssl::context::tlsv12_client);

    // These objects perform our I/O
    tcp::resolver resolver(ioc);
    beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

    // Set SNI Hostname (many hosts need this to handshake successfully)
    if (! SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
        beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
        throw beast::system_error{ec};
    }

    // Look up the domain name
    // msg.at
    auto const results = resolver.resolve(host.c_str(), port);

    // Make the connection on the IP address we get from a lookup
    beast::get_lowest_layer(stream).connect(results);

    // Perform the SSL handshake
    stream.handshake(ssl::stream_base::client);

    // Send the HTTP request to the remote host
    http::write(stream, request);

    // This buffer is used for reading and must be persisted
    beast::flat_buffer buffer;

    // Declare a container to hold the response
    http::response<http::dynamic_body> res;

    // Receive the HTTP response
    http::read(stream, buffer, res);

    // Gracefully close the stream
    beast::error_code ec;

    stream.shutdown(ec);
    if (ec == net::error::eof) {
        // Rationale:
        // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
        ec = {};
    }

    return res;
}