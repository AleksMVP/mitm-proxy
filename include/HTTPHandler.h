#pragma once 

#include "IHandler.h"
#include "Utils.h"
#include "MakeHTTPRequest.h"
#include "HTTPSHandler.h"

namespace fs = std::filesystem;
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = net::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


template <typename T>
class HTTPHandler : public IHandler<T> {
 public: 
    HTTPHandler(const fs::path& resolve_path, const fs::path& main_dirpath);
    void handle(T&& data) override;

 private:
    template<class Body, class Allocator, class Send>
    void send_response(
            http::request<Body, http::basic_fields<Allocator>>&& req,
            Send&& send);

    template<class Body, class Allocator>
    void save_request(const http::request<Body, http::basic_fields<Allocator>>& req);

 private:
    fs::path resolve_path;
    HTTPSHandler<T> https_handler;
};


template <typename T>
HTTPHandler<T>::HTTPHandler(const fs::path& resolve_path, const fs::path& main_dirpath) : 
        resolve_path(resolve_path),
        https_handler(resolve_path, main_dirpath) {}


template <typename T>
void HTTPHandler<T>::handle(T&& client) {
    beast::error_code ec;
    beast::flat_buffer buffer;
    http::request<http::string_body> request;

    http::read(client.get_socket(), buffer, request, ec);

    if (ec) {
        return fail(ec, "init-read");
    }

    // This means we've got https proxy request
    if (request.method_string() == "CONNECT") {
        https_handler.handle(std::move(client), request);
        return;
    }

    bool close = false;
    send_lambda<tcp::socket&> lambda{client.get_socket(), close, ec};
    for(;;) { 
        save_request(request);
        send_response(std::move(request), lambda);

        if (ec) {
            fail(ec, "write");
            return;
        }
        if (close) {
            return;
        }

        http::read(client.get_socket(), buffer, request, ec);

        if (ec == http::error::end_of_stream) {
            return;
        }
        if (ec) {
            fail(ec, "read");
            return;
        }
    }

}


template <typename T>
template<class Body, class Allocator>
void HTTPHandler<T>::save_request(const http::request<Body, http::basic_fields<Allocator>>& req) {
    std::stringstream req_convert;
    req_convert << req;

    std::stringstream ss;
    ss << req.method_string()
       << req.at(http::field::host)
       << sha256(req_convert.str());

    fs::current_path(resolve_path);
    std::ofstream outfile(ss.str());
    outfile << req;
}


template <typename T>
template <typename Body, typename Allocator, typename Send>
void HTTPHandler<T>::send_response(
            http::request<Body, http::basic_fields<Allocator>>&& req,
            Send&& send) {   

    auto response = make_http_request(std::move(req));
    return send(std::move(response));
}
 