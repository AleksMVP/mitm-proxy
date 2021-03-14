#pragma once

#include "Utils.h"
#include "MakeHTTPSRequest.h"
#include "LoadCertificate.h"

#include <thread>

#define CERT_GENERATE_LOAD_LIMIT 100

namespace fs = std::filesystem;
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp; 
using namespace std::chrono_literals;
using namespace std::chrono;

template <typename T>
class HTTPSHandler {
 public:
    explicit HTTPSHandler(const fs::path& path, const fs::path& main_dirpath);

    template<class Body, class Allocator>
    void handle(T&& client, 
                const http::request<Body, http::basic_fields<Allocator>>& request);

 private:
    template<class Body, class Allocator, class Send>
    void send_response(
            http::request<Body, http::basic_fields<Allocator>>&& req,
            Send&& send);

    template <class Body, class Allocator>
    void save_request(const http::request<Body, http::basic_fields<Allocator>>& req);
 private:
    fs::path resolve_path;
    fs::path main_dirpath;
};

template <typename T>
HTTPSHandler<T>::HTTPSHandler(const fs::path& resolve_path, const fs::path& main_dirpath) : 
        resolve_path(resolve_path),
        main_dirpath(main_dirpath) {}

template <typename T>
template<class Body, class Allocator>
void HTTPSHandler<T>::handle(T&& client, 
                            const http::request<Body, http::basic_fields<Allocator>>& request) {
    std::string host = parse_host(request);

    std::cout << "Host: " << host << std::endl;

    std::cout << "---------------request----------------" << std::endl;
    std::cout << request << std::endl;
    std::cout << "--------------------------------------" << std::endl;

    beast::error_code ec;
    boost::asio::write(
        client.get_socket(), 
        boost::asio::buffer(
            std::string(
                "HTTP/1.0 200 Connection established\r\n"
                "Proxy-agent: node.js-proxy\r\n\r\n"
            )
        ), 
        ec
    );

    if(ec) {
        return fail(ec, "init-write");
    }

    // Generate for each socket individual certificate
    std::string cert_path = generate_cert(host, main_dirpath.string());
    ssl::context ctx{ssl::context::tlsv12};

    int count = 0;
    while (true) {  // We need this to wait cert generate
        try {
            count++;
            load_server_certificate(
                ctx, 
                cert_path,
                main_dirpath.string()
            );
            break;
        } catch (std::exception& e) {
            if (count > CERT_GENERATE_LOAD_LIMIT) {
                std::cerr << "Exception: Certificate load limit" << std::endl;
                throw e;
            }
            std::cerr << "Exception: " << e.what() << std::endl;
            std::this_thread::sleep_for(100ms);
        }
    }

    beast::ssl_stream<tcp::socket&> stream{client.get_socket(), ctx};
    stream.handshake(ssl::stream_base::server, ec);
    if (ec) {
        return fail(ec, "handshake");
    }

    bool close = false;
    beast::flat_buffer buffer;
    send_lambda<beast::ssl_stream<tcp::socket&>> lambda{stream, close, ec};
    for(;;) {
            
        http::request<http::string_body> req;
        http::read(stream, buffer, req, ec);

        if (ec == http::error::end_of_stream) {
            break;
        }
        if (ec) {
            return fail(ec, "read");
        } 

        save_request(req);
        send_response(std::move(req), lambda);

        if (ec) {
            return fail(ec, "write");
        }
        if (close) {
            break;
        }

    }

    stream.shutdown(ec);
    if (ec) {
        return fail(ec, "shutdown");
    }
}

template <typename T>
template <class Body, class Allocator>
void HTTPSHandler<T>::save_request(const http::request<Body, http::basic_fields<Allocator>>& req) {
    std::stringstream req_convert;
    req_convert << req;

    std::stringstream ss;
    ss  << req.method_string()
        << req.at(http::field::host)
        << sha256(req_convert.str());

    fs::current_path(resolve_path);
    std::ofstream outfile(ss.str());
    outfile << req;
}

template <typename T>
template<class Body, class Allocator, class Send>
void HTTPSHandler<T>::send_response(
            http::request<Body, http::basic_fields<Allocator>>&& req,
            Send&& send) {  

    auto response = make_https_request(std::move(req));
    return send(std::move(response));
}