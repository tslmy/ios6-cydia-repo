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
| `ca-certificates` | Mozilla CA bundle at `/etc/ssl/certs/cacert.pem`. Version is the bundle date. |

## Not yet tested on a device

The debs have been unpacked and listed on the Mac only. Before publishing, install both on the iPod and check:
`Depends: curl, ca-certificates` names match what Cydia's repos call them, `dpkg -i` over the hand-installed
copy works (preinst handles the old local diversion), and `dpkg -r apt-https-curl` restores the stock method.
