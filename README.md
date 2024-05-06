# SSS
Toy implementation of Shamir's Secret Sharing.

## Quickstart
```
make
./sss -e output_directory
```
One character will be read from stdin. You can read from a file instead with

```
./sss -e infile output_directory
```
Shares will be generated in the output directory.
Use:

```
./sss -d output_directory/shareX output_directory/shareY
```
to recover the secret character.

## Current limitations
* k=2, n=3 is the only scheme currently supported.
* secrets are limited to a single byte
* polynomial coefficients are not random, so a secret can be recovered from a single share
