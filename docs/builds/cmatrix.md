# cmatrix

Unmodified upstream [abishekvashok/cmatrix](https://github.com/abishekvashok/cmatrix) at commit `5c082c6`. It is built
directly with clang (no configure), forcing the config macros that `configure` would normally set.
Set up the [toolchain](../toolchain.md) first.

```sh
git clone https://github.com/abishekvashok/cmatrix && cd cmatrix && git checkout 5c082c6
armv7-ios6-clang \
  -I/opt/homebrew/opt/ncurses/include \
  -DHAVE_NCURSES_H -DHAVE_SYS_IOCTL_H -DHAVE_UNISTD_H \
  -DVERSION=\"2.0\" -DEXCLUDE_CONFIG_H -D_XOPEN_SOURCE_EXTENDED \
  cmatrix.c -lncurses -L$IOS6_SDK_PATH/usr/lib -o cmatrix-armv7
xcrun strip -S cmatrix-armv7        # optional
ldid -S cmatrix-armv7
```

Copy it to `packages/cmatrix/usr/bin/cmatrix`. The Homebrew ncurses headers work for iOS even though they are built for
macOS; only the headers are used, and the library comes from the iOS SDK. The console font files in the upstream repo
are not installed.
