### Mac OS build

```
git clone https://github.com/AleksMVP/mitm-proxy.git
cd mitm-proxy
sh gen.sh
mkdir build
cd build
cmake .. -DOPENSSL_ROOT_DIR="/usr/local/opt/openssl@1.1" -DOPENSSL_LIBRARIES="/usr/local/opt/openssl@1.1/lib"
make
```

### Ubunty build
```
git clone https://github.com/AleksMVP/mitm-proxy.git
cd mitm-proxy
sh gen.sh
mkdir build
cd build
cmake ..
make
```

> For build you need [boost](https://www.boost.org/), [openssl](https://www.openssl.org/)

### Docker build
```
git clone https://github.com/AleksMVP/mitm-proxy.git
cd mitm-proxy
docker build -t boberproxy .
docker run -p 7888:7888 boberproxy
```

### Script usage
```
usage: script.py [-h] -f  [-b] [-q]

Search vulnarabilities

optional arguments:
  -h, --help        show this help message and exit
  -f, --filepath    path to file with request
  -b, --body        flag to check body params
  -q, --query       flag to check query params
 ```

### Script work example
```
> python3 script.py --filepath requests/FileWithRequest --query --body
New query params: {'kek': '\'"><img src="" onerror=alert("")>', 'bob': 'fjdjfdkfjdjfdj', 'lol': '434343'}
New query params: {'kek': 'jdjdjdjdd', 'bob': '\'"><img src="" onerror=alert("")>', 'lol': '434343'}
New query params: {'kek': 'jdjdjdjdd', 'bob': 'fjdjfdkfjdjfdj', 'lol': '\'"><img src="" onerror=alert("")>'}
New body params: {'username': '\'"><img src="" onerror=alert("")>', 'Domain': 'mail.ru', 'Login': 'kek', 'Password': 'lol'}
New body params: {'username': 'bob', 'Domain': '\'"><img src="" onerror=alert("")>', 'Login': 'kek', 'Password': 'lol'}
New body params: {'username': 'bob', 'Domain': 'mail.ru', 'Login': '\'"><img src="" onerror=alert("")>', 'Password': 'lol'}
New body params: {'username': 'bob', 'Domain': 'mail.ru', 'Login': 'kek', 'Password': '\'"><img src="" onerror=alert("")>'}
```
