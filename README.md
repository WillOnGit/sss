# SSS
Toy implementation of Shamir's Secret Sharing.

## Quickstart
```
make
./sss
```

Up to 32 characters (ASCII) will be read from stdin.
Three shares will be generated in the current directory named `share1`, `share2` and `share3`.

You can read from a file instead with
```
./sss infile optional_directory
```

If you specify `optional_directory` then shares will be placed there instead of the current directory.

To recover the secret use
```
./sss -d shareX shareY
```

or just `./sss -d` if `share1` and `share2` are present in the current directory.

Run
```
./sss -h
```

for more info.

## Current limitations
* k=2, n=3 is the only scheme currently supported
* secrets must be 32 bytes/256 bits

## Dependencies
* [The GNU Multiple Precision Arithmetic Library](https://gmplib.org/)
* `xxd` for tests only
