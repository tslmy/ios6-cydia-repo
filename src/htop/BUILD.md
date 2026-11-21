# htop for iOS 6

Built from [tslmy/htop](https://github.com/tslmy/htop), branch `ios6` (commit `eda4f742`). The iOS 6.1 support is merged upstream
in [htop-dev/htop#1828](https://github.com/htop-dev/htop/pull/1828).

```sh
./autogen.sh
./configure --host=armv7-apple-darwin --disable-unicode --disable-sensors --disable-hwloc \
  CFLAGS="-arch armv7 -miphoneos-version-min=6.1 -isysroot <iPhoneOS6.1.sdk>"
make
ldid -S htop
```

The binary links only system libraries (IOKit, CoreFoundation, ncurses from the dyld shared cache, libSystem).
