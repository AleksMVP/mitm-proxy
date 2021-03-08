#pragma once

#include <boost/asio/buffer.hpp>
#include <boost/asio/ssl/context.hpp>
#include <cstddef>
#include <memory>

void load_server_certificate(boost::asio::ssl::context& ctx, 
                             const std::string& cert_path, 
                             const std::string& key_path) {
    ctx.set_options(
        boost::asio::ssl::context::default_workarounds |
        boost::asio::ssl::context::no_sslv2
    );
    ctx.use_certificate_chain_file(cert_path);
    ctx.use_rsa_private_key_file(key_path, boost::asio::ssl::context::pem);
}