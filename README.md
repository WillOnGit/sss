# (lib)sss
Toy implementation of [Shamir's Secret Sharing](https://en.wikipedia.org/wiki/Shamir%27s_secret_sharing).

## Quickstart
```
make
./sss
```

Up to 32 characters (ASCII) will be read from stdin - use CTRL-D (twice if not after a newline) to end input.
Your input will be split into three pieces (`share1`, `share2` and `share3`), any two of which can be used to reconstruct it.

To recover the secret use
```
./sss -d
```
which is a shortcut for `./sss -d share1 share2`.

Run `./sss -h` for more info or `make install` to install.

## Development notes
This repo includes two main components:

* secret sharing functionality in a library `libsss`
* a program to encode and decode secrets using said library, `sss`.

By default, the library is only built and installed as a static archive.
On macOS it can also be built as a dylib by running `make mac` instead of just `make`.
You can then `make install` as normal.

Running `make test` will run some basic tests against the library via the `sss` program.

## Current limitations
* the demo program `sss` can only generate up to nine shares (the library has no such constraints)
* secrets must be 32 bytes - smaller secrets will be right-padded with zero-bytes
* only supports an install prefix of `/usr/local`
* only supported on unix-like OSes

## Dependencies
* C compiler and `make`
* [The GNU Multiple Precision Arithmetic Library](https://gmplib.org/) - tested with version 6.3.0
* `xxd` - for tests only
