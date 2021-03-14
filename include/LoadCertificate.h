#pragma once

#include <boost/asio/buffer.hpp>
#include <boost/asio/ssl/context.hpp>
#include <cstddef>
#include <memory>
#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <mutex>

#define EXT_FILENAME "domains.ext"
#define CRT_FILENAME "localhost.crt"
#define CSR_FILENAME "localhost.csr"
#define KEY_FILANAME "localhost.key"
#define ROOT_KEYNAME "RootCA.key"
#define ROOT_CRTNAME "RootCA.pem"
#define CERTS_DIRNAME "certs"

namespace fs = std::filesystem;

std::string build_path(const std::string& crt_dirpath,
                         const std::string& domain_name, 
                           const std::string& crt_filename) {
    return crt_dirpath + "/" + domain_name + "/" + crt_filename;
}

std::string build_main_path(const std::string& main_dirpath, 
                             const std::string& filename) {
    return main_dirpath + std::string("/") + filename;
}

std::string build_path_command(const std::string& crt_dirpath,
                                const std::string& domain_name,
                                 const std::string& main_dirpath ) {
    std::stringstream ss;
    ss << "openssl x509 -req -sha256 -days 1024" << " "
       << "-in " << build_main_path(main_dirpath, CSR_FILENAME) << " "
       << "-CA " << build_main_path(main_dirpath, ROOT_CRTNAME) << " " 
       << "-CAkey " << build_main_path(main_dirpath, ROOT_KEYNAME) << " -CAcreateserial" << " "
       << "-extfile " << build_path(crt_dirpath, domain_name, EXT_FILENAME) << " "
       << "-out " << build_path(crt_dirpath, domain_name, CRT_FILENAME) << " &";

    return ss.str();
}

std::mutex mut;
std::string generate_cert(const std::string& domain_name,
                            const std::string& main_dirpath) {
    std::lock_guard<std::mutex> m(mut);

    const std::string crt_dirpath = main_dirpath + std::string("/") + CERTS_DIRNAME;

    for(const auto& p : fs::directory_iterator(crt_dirpath)) {
        fs::path path = p.path();
        std::string path_string = path.string();
        size_t pos = path_string.rfind("/");
        if (fs::is_directory(path) && 
                path_string.substr(pos + 1, path_string.length()) == domain_name) {
            return build_path(crt_dirpath, domain_name, CRT_FILENAME);
        }
    }

    fs::create_directories(crt_dirpath + "/" + domain_name);

    std::string ext_filepath = build_path(crt_dirpath, domain_name, EXT_FILENAME);

    fs::copy(main_dirpath + std::string("/") + EXT_FILENAME, ext_filepath); {
        std::ofstream file(ext_filepath, std::ios::app);
        file << "DNS.1 = " + domain_name;
    }
    std::string command = build_path_command(crt_dirpath, domain_name, main_dirpath);

    int _ = std::system(command.c_str());

    return build_path(crt_dirpath, domain_name, CRT_FILENAME);
}

void load_server_certificate(boost::asio::ssl::context& ctx, 
                             const std::string& cert_path, 
                             const std::string& main_dirpath) {
    ctx.set_options(
        boost::asio::ssl::context::default_workarounds |
        boost::asio::ssl::context::no_sslv2
    );
    ctx.use_certificate_chain_file(cert_path);
    ctx.use_rsa_private_key_file(
        build_main_path(main_dirpath, KEY_FILANAME), 
        boost::asio::ssl::context::pem
    );
}