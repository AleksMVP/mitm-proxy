#include "server_certificate.hpp"
#include "make_request.hpp"
#include "generate_cert.hpp"
#include "make_http_request.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/config.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#define FAIL 1
#define OK 0

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
using namespace std::chrono_literals;

// This function produces an HTTP response for the given
// request. The type of the response object depends on the
// contents of the request, so the interface requires the
// caller to pass a generic lambda for receiving the response.
template<class Body, class Allocator, class Send>
void handle_request(
            http::request<Body, http::basic_fields<Allocator>>&& req,
            Send&& send) {   

    // std::cout << req << std::endl;
    auto response = make_request(std::move(req));
    // std::cout << response << std::endl;
    return send(std::move(response));
}

template<class Body, class Allocator, class Send>
void handle_http_request(
            http::request<Body, http::basic_fields<Allocator>>&& req,
            Send&& send) {   

    // std::cout << req << std::endl;
    auto response = make_http_request(std::move(req));
    // std::cout << response << std::endl;
    return send(std::move(response));
}


//------------------------------------------------------------------------------

// Report a failure
void fail(beast::error_code ec, char const* what) {
    std::cerr << what << ": " << ec.message() << "\n";
}

// This is the C++11 equivalent of a generic lambda.
// The function object is used to send an HTTP message.
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

template <typename T>
int process_request(T& stream) {
    bool close = false;
    // This buffer is required to persist across reads
    beast::flat_buffer buffer;

    beast::error_code ec;

    // This lambda is used to send messages
    send_lambda<T> lambda{stream, close, ec};
    for(;;) {
            
        // Read a request
        http::request<http::string_body> req;
        http::read(stream, buffer, req, ec);

        if (ec == http::error::end_of_stream) {
            break;
        }
        if (ec) {
            fail(ec, "read");
            return FAIL;
        } 

        // Send the response
        handle_request(std::move(req), lambda);

        if (ec) {
            fail(ec, "write");
            return FAIL;
        }
        if (close) {
            // This means we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
            break;
        }
    }

    return OK;
}

// Handles an HTTP server connection
void do_session(tcp::socket& socket) {
    try {
        beast::error_code ec;
        beast::flat_buffer tmp_buffer;
        http::request<http::string_body> proxy_request;

        // We need this to catch https proxy request
        http::read(socket, tmp_buffer, proxy_request, ec);

        if (ec) {
            return fail(ec, "init-read");
        }

        std::cout << proxy_request.method_string() << std::endl;
        // This means we've got http request
        if (proxy_request.method_string() != "CONNECT") {
            bool close = false;
            send_lambda<tcp::socket&> lambda{socket, close, ec};
            for(;;) { 
                // Send the response
                handle_http_request(std::move(proxy_request), lambda);

                if (ec) {
                    fail(ec, "write");
                    return;
                }
                if (close) {
                    // This means we should close the connection, usually because
                    // the response indicated the "Connection: close" semantic.
                    break;
                }

                // Read a request
                http::read(socket, tmp_buffer, proxy_request, ec);

                if (ec == http::error::end_of_stream) {
                    break;
                }
                if (ec) {
                    fail(ec, "read");
                    return;
                }
            }
        }

        std::string host = proxy_request.at(http::field::host).to_string();
        size_t pos = host.find(":");
        if (pos != std::string::npos) {
            host = host.substr(0, pos);
        }
        std::cout << host << std::endl;

        boost::asio::write(
            socket, 
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
        std::string cert_path = generate_cert(host);
        std::cout << cert_path << std::endl;
        ssl::context ctx{ssl::context::tlsv12};
        while (true) {  // We need this to wait cert generate
            try {
                load_server_certificate(
                    ctx, 
                    cert_path,
                    "/Users/aleks/Desktop/myproxy/certs/localhost.key"
                );
                break;
            } catch (std::exception& e) {
                std::cerr << "Exception: " << e.what() << std::endl;
                std::this_thread::sleep_for(100ms);
            }
        }

        // Construct the stream around the socket
        beast::ssl_stream<tcp::socket&> stream{socket, ctx};

        // Perform the SSL handshake
        stream.handshake(ssl::stream_base::server, ec);

        if (ec) {
            return fail(ec, "handshake");
        }

        if (process_request(stream)) {
            return;
        }

        // Perform the SSL shutdown
        stream.shutdown(ec);
        if (ec) {
            return fail(ec, "shutdown");
        }
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    // At this point the connection is closed gracefully
}

//------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    try
    {
        // Check command line arguments.
        if (argc != 4)
        {
            std::cerr <<
                "Usage: http-server-sync-ssl <address> <port> <doc_root>\n" <<
                "Example:\n" <<
                "    http-server-sync-ssl 0.0.0.0 8080 .\n";
            return EXIT_FAILURE;
        }
        auto const address = net::ip::make_address(argv[1]);
        auto const port = static_cast<unsigned short>(std::atoi(argv[2]));

        // The io_context is required for all I/O
        net::io_context ioc{1};

        // This holds the self-signed certificate used by the server
        // The acceptor receives incoming connections
        tcp::acceptor acceptor{ioc, {address, port}};
        for(;;)
        {
            // This will receive the new connection
            tcp::socket socket{ioc};

            // Block until we get a connection
            acceptor.accept(socket);

            // Launch the session, transferring ownership of the socket
            std::thread{std::bind(
                &do_session,
                std::move(socket))}.detach();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}