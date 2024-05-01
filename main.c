#include <stdio.h>
#include <string.h>

/*
 * macro?
 */
int f_norm(int input, int modulus)
{
	while (input < 0) {
		input += modulus;
	}

	return input % modulus;
}

/*
 * we'll do this properly later
 */
int finite_inverse(int input, int modulus)
{
	switch (input)
	{
		case 1:
			return 1;
		case 2:
			return 129;
		case 255:
			return 128;
		case 256:
			return 256;
	}

	return 666;
}

/*
 * from input 8-bit value, generate shares in the following scheme:
 *
 * k = 2 (implies polynomial degree 1)
 * n = 3
 *
 * prime p = 257
 * q(x) = secret + 57x
 *
 * yes, there are many deficiencies here, this is to get started.
 */
void shamir_encode(char inc)
{
	int y1, y2, y3, p;

	p = 257;

	y1 = (inc + 57 * 1) % p;
	y2 = (inc + 57 * 2) % p;
	y3 = (inc + 57 * 3) % p;

	printf("(1, %d)\n(2, %d)\n(3, %d)\n", y1, y2, y3);
}

/*
 * reconstruct secret from shares in above scheme.
 * shares are read from stdin and must be entered with
 * "(", "," and ")" characters; whitespace is ignored.
 */
void shamir_decode()
{
	int x1, y1, x2, y2, matches, y_diff, x_diff, x_co;
	char secret;

	matches = scanf(" ( %d , %d ) ( %d , %d ) ", &x1, &y1, &x2, &y2);
	if (matches != 4) {
		printf("bad input\n");
		return;
	}

	if (x1 == x2 || y1 == y2){
		printf("duplicated coordinates\n");
		return;
	}

	/* 
	 * we have well-defined distinct points
	 * and 0 <= x1, x2, y1, y2 < 257
	 *
	 * the coefficient of x "a" in q(x) can be recovered as follows:
	 *
	 * (y2 - y1) * (x2 - x1)^-1 ~= a	mod 257
	 */
	y_diff = (257 + y2 - y1) % 257;
	x_diff = (257 + x2 - x1) % 257;

	x_co = (y_diff * finite_inverse(x_diff, 257)) % 257;
	secret = f_norm(y1 - x_co * x1, 257);

	printf("Reconstructed secret: %c\n", secret);
}

/*
 * - read stdin or first argument filename
 * - encode first character, if there is one
 * - prompt for two shares to verify secret reconstruction
 */
int main(int argc, char **argv)
{
	FILE *infile;
	char *secretfilename;
	int c;

	if (argc > 1 && strcmp(*(argv + 1), "-")) {
		secretfilename = *(argv + 1);
		infile = fopen(secretfilename, "r");

		if (!infile) {
			fprintf(stderr, "Unable to open file %s\n", secretfilename);
			return 1;
		}

		fprintf(stderr, "Reading file %s\n", secretfilename);
	} else {
		fprintf(stderr, "Reading stdin\n");
		infile = stdin;
	}

	c = getc(infile);
	if (c != EOF)
		shamir_encode(c);

	fprintf(stderr, "Reenter two shares to validate:\n");
	shamir_decode();

	return 0;
}
