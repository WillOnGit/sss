#include <stdio.h>
#include <string.h>

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
 * - read stdin or first argument filename
 * - encode first character, if there is one
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

	return 0;
}
