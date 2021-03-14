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