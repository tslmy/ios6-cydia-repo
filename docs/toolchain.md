# Toolchain for iOS 6.1 armv7 (from an Apple Silicon Mac)

Every binary in this repo is cross-compiled with Apple's own Xcode 4.6.3 toolchain, because modern clang can't
produce iOS 6 binaries. None of Apple's files are redistributed here; you extract them yourself.

## What you need

- Xcode 4.6.3 DMG, mounted at `/Volumes/Xcode` (it contains the iPhoneOS 6.1 SDK and the old clang).
- Homebrew packages: `ncurses` (headers only; the iOS 6.1 SDK ships none), `automake`, `autoconf`, `libtool`, `ldid`.
- `ldid -S <binary>` pseudo-signs each binary, which the jailbreak needs.

## One-time setup

Copy the toolchain and SDK out of the DMG. `Xcode.app`'s signature has expired, so macOS refuses to run it from the
DMG, and the SDK sometimes needs edits (see [htop](builds/htop.md)).

```sh
export XCODE=/Volumes/Xcode/Xcode.app/Contents/Developer
mkdir -p ~/ios6
cp -r $XCODE/Toolchains/XcodeDefault.xctoolchain  ~/ios6/
cp -r $XCODE/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS6.1.sdk  ~/ios6/

export IOS6_TOOLCHAIN=~/ios6/XcodeDefault.xctoolchain
export IOS6_SDK_PATH=~/ios6/iPhoneOS6.1.sdk
export SIM_SDK=$XCODE/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator6.1.sdk
export PATH="$PWD/tools:$PATH"        # from the repo root: gives you armv7-ios6-clang
```

[`tools/armv7-ios6-clang`](../tools/armv7-ios6-clang) wraps the old clang with `-arch armv7 -miphoneos-version-min=6.1 -isysroot $IOS6_SDK_PATH`.

Check it: `armv7-ios6-clang -x c - -o /tmp/t <<< 'int main(){return 0;}' && file /tmp/t` should say `Mach-O executable arm_v7`.

The simulator SDK in the same DMG (`$SIM_SDK` above) supplies a few headers the device SDK lacks.

## Getting a binary onto an iPod

```sh
scp -i ~/.ssh/<key> <binary> root@<ipod>:/var/root/
```

Anything you want to share goes through [packaging](packaging.md) instead of `/usr/local/bin`.

## Not yet reproducible

- **micro** (Go): Go 1.14 is the last release with `darwin/arm`, and cgo builds failed against the old clang in the
  original notes. No iOS 6 build exists yet.
- **fastfetch**: needs patches that aren't committed anywhere yet.
