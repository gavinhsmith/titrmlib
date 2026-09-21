# titrmlib
Terminal-Styled UI Framework for the TI-84 Plus CE

## Layout

- `src/` — library source and headers
- `test/` — standalone test programs that link against `src/` to exercise the library
- `bin/` — build output (`.8xp` files), populated by `make`

## Building

Requires the [CE C/C++ Toolchain](https://github.com/CE-Programming/toolchain) (`CEdev`) on `PATH`.

```sh
make        # build all test programs into bin/
make clean  # remove build artifacts
```

Each program under `test/` can also be built individually, e.g. `make -C test/basic`.
