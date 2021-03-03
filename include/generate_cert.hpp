#pragma once 

#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <mutex>

#define EXT_FILENAME "domains.ext"
#define CRT_FILENAME "localhost.crt"
#define CSR_FILENAME "localhost.csr"
#define ROOT_KEYNAME "RootCA.key"
#define ROOT_CRTNAME "RootCA.pem"
#define CERTS_DIRNAME "certs"
#define MAIN_PATH "/Users/aleks/Desktop/myproxy/certs"

namespace fs = std::filesystem;

std::mutex mut;

std::string build_path(const std::string& crt_dirpath,
                         const std::string& domain_name, 
                           const std::string& crt_filename) {
    return crt_dirpath + "/" + domain_name + "/" + crt_filename;
}

std::string build_main_path(const std::string& filename) {
    return MAIN_PATH + std::string("/") + filename;
}

std::string build_path_command(const std::string& crt_dirpath,
                                const std::string& domain_name) {
    std::stringstream ss;
    ss << "openssl x509 -req -sha256 -days 1024" << " "
       << "-in " << build_main_path(CSR_FILENAME) << " "
       << "-CA " << build_main_path(ROOT_CRTNAME) << " " 
       << "-CAkey " << build_main_path(ROOT_KEYNAME) << " -CAcreateserial" << " "
       << "-extfile " << build_path(crt_dirpath, domain_name, EXT_FILENAME) << " "
       << "-out " << build_path(crt_dirpath, domain_name, CRT_FILENAME) << " &";

    return ss.str();
}

std::string generate_cert(const std::string& domain_name) {
    std::lock_guard<std::mutex> m(mut);

    const std::string crt_dirpath = MAIN_PATH + std::string("/") + CERTS_DIRNAME;

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

    fs::copy(MAIN_PATH + std::string("/") + EXT_FILENAME, ext_filepath); {
        std::ofstream file(ext_filepath, std::ios::app);
        file << "DNS.1 = " + domain_name;
    }
    std::string command = build_path_command(crt_dirpath, domain_name);

    int a = std::system(command.c_str());

    return build_path(crt_dirpath, domain_name, CRT_FILENAME);
}