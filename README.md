### Mac OS Build

```
git clone https://github.com/AleksMVP/myproxy.git
cd myproxy
sh gen.sh
mkdir build
cd build
cmake .. -DOPENSSL_ROOT_DIR="/usr/local/opt/openssl@1.1" -DOPENSSL_LIBRARIES="/usr/local/opt/openssl@1.1/lib"
make
```