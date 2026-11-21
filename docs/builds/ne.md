# ne (not yet packaged)

[vigna/ne](https://github.com/vigna/ne) needs a makefile fix before it honors command-line flags: `src/makefile` assigns
`CFLAGS=` and `LDFLAGS=` with `=`, which discards the cross-compile flags. Changing them to `+=` fixes it
(upstream PR in progress). Set up the [toolchain](../toolchain.md) first, then:

```sh
cd ne/src
make clean && make \
  CC="$IOS6_TOOLCHAIN/usr/bin/clang" \
  OPTS="-isysroot $IOS6_SDK_PATH -arch armv7 -miphoneos-version-min=6.1 -I/opt/homebrew/opt/ncurses/include"
ldid -S ne
```

A package also has to install the runtime files (`syntax/`, `macros/`, `extensions`, `doc/`); that is still to do.
