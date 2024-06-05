# SSS
Toy implementation of Shamir's Secret Sharing.

## Quickstart
```
make
./sss output_directory
```
Up to 32 characters (ASCII only) will be read from stdin.
You can read from a file instead with

```
./sss infile output_directory
```
Shares will be generated in the output directory.
Use:

```
./sss -d output_directory/shareX output_directory/shareY
```
to recover the secret.

## Current limitations
* k=2, n=3 is the only scheme currently supported
* secrets must be 32 bytes/256 bits

## Dependencies
* [The GNU Multiple Precision Arithmetic Library](https://gmplib.org/)
* `xxd` for tests only
