# Adding and publishing a package

## Layout

Each `packages/<name>/` directory is a package root: `DEBIAN/control` (plus any `preinst`, `postinst`, `postrm`) and the files
to install, at their final paths. `python3 build.py` turns them into `site/` (debs, `Packages`, `Packages.gz`, `Packages.bz2`,
`Release`, `index.html`). It needs only Python 3, and it does not use dpkg, so it runs the same locally and in CI.

```sh
python3 build.py && python3 -m http.server -d site 8000
```

## Rules we learned the hard way

- **Architecture** is `iphoneos-arm`. Binaries are armv7 for iOS 6.1 and pseudo-signed with `ldid -S`.
- **Licence:** ship the licence text and a source pointer (repo, branch, commit) in `usr/share/doc/<name>/copyright`. GPL
  programs must stay reproducible from the cited source. We don't host anything derived from Apple's firmware or SDK files.
- **Version:** `<upstream>-<revision>`; for a git snapshot use `<next version>~dev+g<sha>-<revision>`.
- **Diversions** (replacing a file another package owns): add them in `preinst` with `dpkg-divert --package <name> --rename`,
  and remove them in `postrm`, never `prerm`. dpkg deletes the package's files between the two.
- **Names:** if Bingner's or another famous repo pins the same name at a high priority, `apt-get upgrade` will downgrade
  our package. Use a distinct name and `Provides`/`Replaces`/`Conflicts`.
- **Dependencies:** `ncurses` for terminal programs. The ncurses library itself comes from the iOS 6 dyld shared cache.
- `/usr/share` on the jailbroken iPod is a symlink into `/var/stash`; dpkg installs through it fine.

## Test on a device before merging

1. `scp` the deb to the iPod, then `dpkg -i`, run the program, `dpkg -r`, and `dpkg -i` again.
2. `ls -ld /usr/share` should still show the symlink, and the package's files should be gone after removal.
3. After the PR is merged and Pages has deployed: `apt-get update && apt-get install <name>` over the hosted repo
   (the repo is unsigned, so apt asks for confirmation; Cydia doesn't).
4. Never run `apt-get update` with `-o Dir::Etc::SourceList=...`; it deletes every other repo's saved index.

## Publish

Branch `<user>/<description>`, open a PR, and merge once pre-commit.ci passes. Every push to `main` runs
`.github/workflows/pages.yml`, which rebuilds `site/` and deploys it to GitHub Pages.
