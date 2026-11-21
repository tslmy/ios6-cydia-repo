# Legacy iOS Cydia repo

Packages for jailbroken iOS 6 (armv7) devices, published as a flat apt repo on GitHub Pages.

- `packages/<name>/`: one directory per package (`DEBIAN/` plus the files to install).
- `src/`: sources for prebuilt binaries. Cross-compile on a Mac, `ldid -S`, then copy the binary into `packages/`.
- `build.py`: builds `site/` (debs, `Packages`, `Packages.gz`, `Packages.bz2`, `Release`, `index.html`). Python 3 only, no dpkg needed.
- `.github/workflows/pages.yml`: runs `build.py` on every push to `main` and deploys `site/`.

```sh
python3 build.py && python3 -m http.server -d site 8000
```

## Packages

| Package | What it does |
| --- | --- |
| `apt-https-curl` | apt `https` method that runs curl, so Cydia can reach GCM-only HTTPS repos. Rebuild: see `src/apt-https-curl/https.c` (header comment) and the project README. |
| `htop` | Interactive process viewer, built from [tslmy/htop](https://github.com/tslmy/htop) (iOS 6.1 support merged upstream in htop-dev/htop#1828). |
| `tmux` | Terminal multiplexer, unmodified upstream (`next-3.7`), static libevent. |
| `cmatrix` | Falling-characters terminal screensaver, unmodified upstream. |
| `ca-certificates-mozilla` | Mozilla CA bundle at `/etc/ssl/certs/cacert.pem`; provides and replaces `ca-certificates` (Bingner pins that name at priority 1001, which would downgrade it). Version is the bundle date. |

## Documentation

How to reproduce every binary and how to add a package: [docs/toolchain.md](docs/toolchain.md), [docs/packaging.md](docs/packaging.md),
and one recipe per package in [docs/builds/](docs/builds/). Apple's toolchain, SDK and headers are not in this repo; you extract them yourself.

## Tested on a device

Both packages were installed on an iPod touch 4G (iOS 6.1.6): `dpkg -i` over the earlier hand-made setup, upgrade, remove (restores
the stock `https` method), reinstall, `apt-get update` over HTTPS, and installing from this repo through apt.
`ca-certificates-mozilla` replaces Bingner's `ca-certificates`; `apt-get upgrade` no longer wants to downgrade the bundle.
