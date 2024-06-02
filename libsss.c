#define	SBUF_SIZE	32

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gmp.h>

#include "libsss.h"

/*
 * from input 256-bit value, generate shares in the following scheme:
 *
 * k = 2 (implies polynomial degree 1)
 * n = 3
 *
 * prime p = 115792089237316195423570985008687907853269984665640564039457584007913129640233
 * q(x) = secret + 57x
 *
 * yes, there are many deficiencies here, this is to get started.
 */
int sss_enc(char* infilename, char* outdirname)
{
	FILE *infile;
	FILE *outfiles[3];
	char *outfilename;
	char sbuf[SBUF_SIZE] = { 0 };
	int sp, c;
	mpz_t y1, y2, y3, p, secret;

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
	mpz_inits(y1, y2, y3, secret, NULL);
	mpz_init_set_str(p, "115792089237316195423570985008687907853269984665640564039457584007913129640233", 10);

	for (int i = 0; i < SBUF_SIZE; i++) {
		c = getc(infile);
		if (c == EOF)
			break;
		sbuf[i] = c;
	}

	/* 0 <= secret < p */
	mpz_import(secret, SBUF_SIZE, -1, sizeof(char), 0, 0, sbuf);

	/* arithmetic */
	mpz_add_ui(y1, secret, 57 * 1);
	mpz_mod(y1, y1, p);
	mpz_add_ui(y2, secret, 57 * 2);
	mpz_mod(y2, y2, p);
	mpz_add_ui(y3, secret, 57 * 3);
	mpz_mod(y3, y3, p);

	gmp_fprintf(outfiles[0], "(1, %Zd)\n", y1);
	gmp_fprintf(outfiles[1], "(2, %Zd)\n", y2);
	gmp_fprintf(outfiles[2], "(3, %Zd)\n", y3);

	return 0;
}

/*
 * reconstruct secret from shares in above scheme.
 * shares are read from files and must be entered with
 * "(", "," and ")" characters; whitespace is ignored.
 */
int sss_dec(char *f1, char *f2)
{
	FILE *s1, *s2;
	char sbuf[SBUF_SIZE] = { 0 };
	int matches;
	mpz_t x1, x2, y1, y2, x_diff, y_diff, p;

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

	mpz_inits(x1, x2, y1, y2, x_diff, y_diff, NULL);
	mpz_init_set_str(p, "115792089237316195423570985008687907853269984665640564039457584007913129640233", 10);

	matches = gmp_fscanf(s1, " ( %Zd , %Zd ) ", &x1, &y1);
	if (matches != 2) {
		printf("Bad input in %s\n", f1);
		return 1;
	}

	matches = gmp_fscanf(s2, " ( %Zd , %Zd ) ", &x2, &y2);
	if (matches != 2) {
		printf("Bad input in %s\n", f2);
		return 1;
	}

	if (! (mpz_cmp(x1, x2) && mpz_cmp(y1, y2)) ){
		printf("Duplicated coordinates\n");
		return 1;
	}

	/*
	 * we have well-defined distinct points
	 * and 0 <= x1, x2, y1, y2 < p
	 *
	 * the coefficient of x "a" in q(x) can be recovered as follows:
	 *
	 * (y2 - y1) * (x2 - x1)^-1 ~= a	mod p
	 */
	mpz_sub(y_diff, y2, y1);
	mpz_sub(x_diff, x2, x1);

	mpz_invert(x_diff, x_diff, p);
	mpz_mul(y_diff, y_diff, x_diff);

	mpz_submul(y1, x1, y_diff);
	mpz_mod(y1, y1, p);

	mpz_export(sbuf, NULL, -1, sizeof(char), 0, 0, y1);

	for (int i = 0; i < SBUF_SIZE; i++)
		putchar(sbuf[i]);

	return 0;
}
