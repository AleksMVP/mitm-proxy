#pragma once 

#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <mutex>

namespace fs = std::filesystem;

std::mutex mut;
const std::string start_path = "/Users/aleks/Desktop/myproxy/certs/certs";
const std::string main_path = "/Users/aleks/Desktop/myproxy/certs";

std::string generate_cert(const std::string& domain_name) {
    std::lock_guard<std::mutex> m(mut);

    for(const auto& p : fs::directory_iterator(start_path)) {
        fs::path path = p.path();
        std::string path_string = path.string();
        size_t pos = path_string.rfind("/");
        if (fs::is_directory(path) && 
                path_string.substr(pos + 1, path_string.length()) == domain_name) {
            return start_path + "/" + domain_name + "/localhost.crt";
        }
    }
    fs::current_path(start_path);
    fs::create_directories(domain_name);
    fs::current_path(main_path);
    std::string ext_filepath = start_path + "/" + domain_name + "/domains.ext";
    // std::ofstream file(ext_filepath);
    fs::copy("domains.ext", ext_filepath);
    {
        std::ofstream file(ext_filepath, std::ios::app);
        file << "DNS.1 = " + domain_name;
    }

    fs::current_path(main_path);
    std::string command = "openssl x509 -req -sha256 -days 1024 -in localhost.csr -CA RootCA.pem -CAkey RootCA.key -CAcreateserial -extfile ./certs/" + domain_name + "/domains.ext -out ./certs/" + domain_name + "/localhost.crt &";

    int a = std::system(command.c_str());

    return start_path + "/" + domain_name + "/" + "localhost.crt";
}