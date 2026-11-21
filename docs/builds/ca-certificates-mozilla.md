# ca-certificates-mozilla

No compilation: the package ships `/etc/ssl/certs/cacert.pem`, the Mozilla bundle as curl distributes it.

To refresh it:

```sh
cp /opt/homebrew/etc/ca-certificates/cert.pem packages/ca-certificates-mozilla/etc/ssl/certs/cacert.pem   # after `brew upgrade ca-certificates`
```

Then set `Version:` in `packages/ca-certificates-mozilla/DEBIAN/control` to the bundle's date (`YYYYMMDD`). The package is
named differently from `ca-certificates` on purpose: Bingner's repo pins that name at priority 1001 and would downgrade it.
It `Provides`, `Replaces` and `Conflicts` with `ca-certificates`.
