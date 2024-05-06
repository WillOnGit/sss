#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libsss.h"

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
int sss_enc(char* infilename, char* outdirname)
{
	int sp, y1, y2, y3, p, inc;
	FILE *outfiles[3];
	FILE *infile;
	char *outfilename;

	/* next step */
	if (!strcmp(infilename, "-")) {
		infile = stdin;
	} else {
		infile = fopen(infilename, "r");
		if (!infile) {
			fprintf(stderr, "Unable to open file %s\n", infilename);
			return 1;
		}
	}

	/* outfiles will be share1, share2, etc. */
	sp = strlen(outdirname);
	outfilename = malloc(sp + 8);
	if (outfilename == NULL)
		return 1;

	memcpy(outfilename, outdirname, sp);

	/* add / if required */
	if (outfilename[sp - 1] != '/') {
		outfilename[sp++] = '/';
	}

	/* there must be a better way to do this? */
	outfilename[sp++] = 's';
	outfilename[sp++] = 'h';
	outfilename[sp++] = 'a';
	outfilename[sp++] = 'r';
	outfilename[sp++] = 'e';
	outfilename[sp+1] = '\0';

	for (int i = 0; i < 3; i++) {
		outfilename[sp] = '1' + i;
		outfiles[i] = fopen(outfilename, "w");
		if (outfiles[i] == NULL)
			return 1;
	}

	/* actually do the thing */
	inc = getc(infile);
	p = 257;

	y1 = (inc + 57 * 1) % p;
	y2 = (inc + 57 * 2) % p;
	y3 = (inc + 57 * 3) % p;

	fprintf(outfiles[0], "(1, %d)\n", y1);
	fprintf(outfiles[1], "(2, %d)\n", y2);
	fprintf(outfiles[2], "(3, %d)\n", y3);

	return 0;
}

/*
 * reconstruct secret from shares in above scheme.
 * shares are read from files and must be entered with
 * "(", "," and ")" characters; whitespace is ignored.
 */
int sss_dec(char *f1, char *f2)
{
	int x1, y1, x2, y2, matches, y_diff, x_diff, x_co;
	char secret;
	FILE *s1, *s2;

	s1 = fopen(f1, "r");
	s2 = fopen(f2, "r");
	if (s1 == NULL) {
		fprintf(stderr, "Unable to open file %s\n", f1);
		return 1;
	}
	if (s2 == NULL) {
		fprintf(stderr, "Unable to open file %s\n", f2);
		return 1;
	}

	matches = fscanf(s1, " ( %d , %d ) ", &x1, &y1);
	if (matches != 2) {
		printf("bad input in %s\n", f1);
		return 0;
	}

	matches = fscanf(s2, " ( %d , %d ) ", &x2, &y2);
	if (matches != 2) {
		printf("bad input in %s\n", f2);
		return 0;
	}

	if (x1 == x2 || y1 == y2){
		printf("duplicated coordinates\n");
		return 0;
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

	return 0;
}
