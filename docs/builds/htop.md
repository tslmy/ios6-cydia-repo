# htop

Built from upstream [htop-dev/htop](https://github.com/htop-dev/htop). iOS 6.1 support was merged in commit `347084b8`
([#1828](https://github.com/htop-dev/htop/pull/1828)) and is part of release 3.5.0 and later, so no fork is involved.
Set up the [toolchain](../toolchain.md) first.

**Provenance of the shipped binary:** it was built from `eda4f742`, the head of that pull request, which is the second parent
of `347084b8`. The three files the PR changed (`configure.ac`, `darwin/DarwinMachine.c`, `darwin/Platform.c`) are identical in
`347084b8`; the rest of the tree differs by 8 unrelated upstream files. The next rebuild should use `347084b8` or the `3.5.0` tag
and bump the package revision.

## Patch the SDK

htop needs IOKit for battery, disk and GPU stats, but the iPhoneOS 6.1 SDK has no IOKit headers. Work on a copy of the SDK
(the DMG is read-only) and add the headers from the simulator SDK, plus one header from the MacOSX 10.7 SDK:

```sh
P=$XCODE/Platforms
cp -r $P/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator6.1.sdk/System/Library/Frameworks/IOKit.framework/Versions/A/Headers \
  $IOS6_SDK_PATH/System/Library/Frameworks/IOKit.framework/
cp $P/MacOSX.platform/Developer/SDKs/MacOSX10.7.sdk/System/Library/Frameworks/IOKit.framework/Versions/A/Headers/storage/IOBlockStorageDriver.h \
  $IOS6_SDK_PATH/System/Library/Frameworks/IOKit.framework/Headers/storage/
```

Headers dumped by other people for newer iOS versions don't work (they use iOS 7 availability macros). `libproc.h` is not in the
device SDK either, so the build adds the simulator SDK's `usr/include` to the include path. Don't unmount the DMG until the build is done.

## Build

```sh
git clone https://github.com/htop-dev/htop && cd htop && git checkout 347084b8
./autogen.sh
./configure --host=armv7-apple-darwin --disable-unicode --disable-sensors --disable-hwloc \
  CFLAGS="-arch armv7 -miphoneos-version-min=6.1 -isysroot $IOS6_SDK_PATH \
          -I$SIM_SDK/usr/include -I/opt/homebrew/opt/ncurses/include"
make
ldid -S htop
```

Copy `htop` and `htop.1` into `packages/htop/`. The binary links only IOKit, CoreFoundation, ncurses (from the dyld shared
cache) and libSystem. `htop --version` on the device reports `3.5.0-dev-3.4.1-246-gba98c8e-dirty` because the build tree had
local changes.
