# tmux

Unmodified upstream [tmux/tmux](https://github.com/tmux/tmux) (package built from commit `e9afd2bb`, reports `next-3.7`).
Needs a static libevent. Set up the [toolchain](../toolchain.md) first. Recipe from the original build notes.

## libevent 2.1.12 (static)

```sh
curl -L -O https://github.com/libevent/libevent/releases/download/release-2.1.12-stable/libevent-2.1.12-stable.tar.gz
tar xzf libevent-2.1.12-stable.tar.gz && cd libevent-2.1.12-stable
export LIBEVENT_PATH=$HOME/ios6/libevent-ios
./configure --host=arm-apple-darwin --prefix=$LIBEVENT_PATH --disable-shared --enable-static \
  CFLAGS="-arch armv7 -miphoneos-version-min=6.1 -isysroot $IOS6_SDK_PATH" \
  LDFLAGS="-arch armv7 -miphoneos-version-min=6.1 -isysroot $IOS6_SDK_PATH"
make install-exec install-data    # not plain `make install`: it fails trying to link the sample binaries
```

## tmux

```sh
git clone https://github.com/tmux/tmux && cd tmux && git checkout e9afd2bb
sh autogen.sh
./configure --host=arm-apple-darwin --disable-utf8proc \
  CC="clang -arch armv7 -miphoneos-version-min=6.1 -isysroot $IOS6_SDK_PATH" \
  CFLAGS="-O2 -I$LIBEVENT_PATH/include -I$SIM_SDK/usr/include -I/opt/homebrew/opt/ncurses/include" \
  LDFLAGS="-L$LIBEVENT_PATH/lib"
make -j4
ldid -S tmux
```

Then copy `tmux` to `packages/tmux/usr/bin/tmux` and `tmux.1` to `packages/tmux/usr/share/man/man1/`.
The only runtime dependency is the `ncurses` package. Smoke test on the device:
`TERM=xterm-256color tmux -f /dev/null new-session -d -s t 'sleep 20'; tmux ls; tmux kill-server`.
