### Mac OS Build

docker run -dp 7888:7888 getting-started
cmake .. -DOPENSSL_ROOT_DIR="/usr/local/opt/openssl@1.1" -DOPENSSL_LIBRARIES="/usr/local/opt/openssl@1.1/lib"