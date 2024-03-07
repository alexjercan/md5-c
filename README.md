# MD5 Hash/Bruteforce tool

MD5 hash clone tool. It follows the instructions from [ Wikipedia
](https://en.wikipedia.org/wiki/MD5)

## Quickstart

```console
make
./build/main -h
```

To use the hashing functionality you can provide a string either as stdin or
from a file.

```console
make
echo -n "mystring" | ./build/main
```

To use the bruteforce algorithm you need a dictionary file with possible words
separated by newlines.

```console
make
echo -n "my_md5_hash_digest" | ./build/main --bruteforce -f dictionary.txt
```
