#include "IHandler.h"
#include "LoggingHandler.h"
#include "HTTPHandler.h"
#include "ExceptionHandler.h"
#include "Server.h"
#include "Client.h"

#include <filesystem>

namespace fs = std::filesystem;

const fs::path RESOLVE_PATH = "/Users/aleks/Desktop/tp-thirdsem/myproxy/requests";
const fs::path CERTS_PATH = "/Users/aleks/Desktop/tp-thirdsem/myproxy/certs";

const int QUEUE_SIZE = 1000;
const int THREADS_NUMBER = 100;
const int PORT = 7888;

int main() {
    Server<Client>::Config config(
        THREADS_NUMBER, 
        QUEUE_SIZE,
        PORT
    );

    HTTPHandler<Client> http_handler(RESOLVE_PATH, CERTS_PATH);
    ExceptionHandler<Client> exception_handler(http_handler);
    LoggingHandler<Client> logging_handler(exception_handler);
    Server<Client> server(logging_handler, config);

    server.start();
}