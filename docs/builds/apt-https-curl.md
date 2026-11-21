# apt-https-curl

Our own code: [`src/apt-https-curl/https.c`](../../src/apt-https-curl/https.c). It speaks apt's method protocol and runs
the device's curl (OpenSSL 1.1.1) for every download, because iOS 6's system TLS has no AES-GCM. Set up the
[toolchain](../toolchain.md) first.

```sh
cd src/apt-https-curl
armv7-ios6-clang -Os -Wall -include stdarg.h -include signal.h -o https-curl https.c
ldid -S https-curl
cp https-curl ../../packages/apt-https-curl/usr/lib/apt/methods/https
```

The package diverts apt's stock `https` method to `https.cfnetwork` (`preinst`) and restores it in `postrm`.
Restoring in `prerm` doesn't work: dpkg deletes the package's files after `prerm` and would remove the restored file.

Protocol gotchas (apt 0.7 hangs if a message's terminator arrives in a separate `read()`, so every reply is a single
`write()`; `X-Unique-ID` is a dummy because apt.bingner.com 404s without the header) are in the comments in `https.c`.
