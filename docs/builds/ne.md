# ne (not yet packaged)

[vigna/ne](https://github.com/vigna/ne). Unmodified upstream builds for iOS 6 when the cross flags go through `OPTS`,
which `src/makefile` adds to both the compile and link lines. Set up the [toolchain](../toolchain.md) first, then:

```sh
cd ne/src
make clean && make \
  CC="$IOS6_TOOLCHAIN/usr/bin/clang" \
  OPTS="-isysroot $IOS6_SDK_PATH -arch armv7 -miphoneos-version-min=6.1 -I/opt/homebrew/opt/ncurses/include" \
  NE_TERMCAP=1 NE_ANSI=1
ldid -S ne
```

Notes:

- **Environment flags:** `CFLAGS=... LDFLAGS=... make` is ignored by upstream, because the makefile assigns with `=`.
  Upstream PR [vigna/ne#136](https://github.com/vigna/ne/pull/136) changes that to `+=`; the `OPTS` command above doesn't need it.
- **Generated sources:** a plain git checkout lacks `enums.h`, `hash.c`, `help.c` and `names.c`, which come from a native build.
  The build above was run in a tree that already had them, so build natively once first.
- **Packaging:** a package still has to install the runtime files (`syntax/`, `macros/`, `extensions`, `doc/`). Not done yet.
  The binary started on the iPod, but it has not been used interactively.
