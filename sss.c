#define	SBUF_SIZE	32
#define	SSS_VERSION	"0.1dev"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libsss.h"

const char * const sss_version = SSS_VERSION;
const char * const helptext =
	"sss - shamir's secret sharing with libsss (by WillOnGit)\n"
	"\n"
	"Usage: sss [--encode | -e] [in filename] [out directory]\n"
	"or:    sss [--decode | -d] [sharefile1 sharefile2]\n"
	"or:    sss [--version | -v]\n"
	"or:    sss [--help | -h]\n"
	"\n"
	"If none of [--encode | -e] or [--decode | -d] are specified, encoding is\n"
	"the default operation.\n"
	"\n"
	"When encoding shares, the input file defaults to stdin and the out\n"
	"directory defaults to the working directory. Three shares will be\n"
	"written to out directory, named share1, share2 and share3.\n"
	"\n"
	"When decoding from shares, if no filenames are supplied they will\n"
	"default to share1 and share2. Supplying only one filename is an error.\n"
	"Output is to stdout.\n"
	"\n"
	"[--version | -v] prints the program and library versions then exits.\n"
	"\n"
	"[--help | -h] prints this message then exits.\n"
	;

/*
 * basic wrapper around libsss encoding & decoding
 *
 * encoding:
 * - input from stdin or file
 * - output to share files in given directory
 *
 * decoding:
 * - input from files
 * - output to stdout
 */
int main(int argc, char **argv)
{
	int enc, dec;
	char *n1, *n2;
	signed char sbuf[SBUF_SIZE] = { 0 };

	enc = 1;
	dec = 0;

	n1 = n2 = NULL;

	/*
	 * handle arguments
	 *
	 * TODO: deal with files named '-e' '-d' or '-v'
	 */
	for (int i = 1; i < argc; i++) {
		if (!strcmp("-v", argv[i]) || !strcmp("--version", argv[i])) {
			/* emit version info and exit immediately */
			printf("sss version: %s\nlibsss version: %s\n", sss_version, sss_libver);
			return 0;
		}

		if (!strcmp("-h", argv[i]) || !strcmp("--help", argv[i])) {
			/* emit help text and exit immediately */
			printf("%s", helptext);
			return 0;
		}

		if (!strcmp("-d", argv[i]) || !strcmp("--decode", argv[i])) {
			dec = 1;
			enc = 0;
			continue;
		}

		if (!strcmp("-e", argv[i]) || !strcmp("--encode", argv[i])) {
			enc = 1;
			dec = 0;
			continue;
		}

		/* pick up anything else as a file/directory argument, ignoring any beyond the first 2 */
		if (n1 == NULL) {
			n1 = argv[i];
		} else if (n2 == NULL) {
			n2 = argv[i];
		}
	}

	/* options parsed, call relevant lib function */
	if (enc) {
		int c, sp;
		char *outf;

		/* set defaults if not specified or check for empty strings */
		if (n1 == NULL) {
			n1 = "-";
		} else if (!strcmp(n1, "")) {
			fprintf(stderr, "Please supply a nonempty input filename\n");
			return 1;
		}

		if (n2 == NULL) {
			n2 = "./";
		} else if (!strcmp(n2, "")) {
			fprintf(stderr, "Please supply a nonempty directory name\n");
			return 1;
		}

		/* open files */
		FILE *infile, *sf1, *sf2, *sf3;

		if (!strcmp(n1, "-")) {
			infile = stdin;
		} else {
			infile = fopen(n1, "r");
			if (infile == NULL) {
				fprintf(stderr, "Unable to open file %s\n", n1);
				return 1;
			}
		}

		/* construct share filenames */
		sp = strlen(n2);
		outf = malloc(sp + 8);
		if (outf == NULL)
			return 1;

		memcpy(outf, n2, sp * sizeof(char));
		if (outf[sp - 1] != '/') {
			outf[sp++] = '/';
		}

		strcpy(&outf[sp], "share0");
		sp += 6;
		outf[sp] = '\0';

		/* start */
		outf[sp - 1] = '1';
		sf1 = fopen(outf, "w");
		outf[sp - 1] = '2';
		sf2 = fopen(outf, "w");
		outf[sp - 1] = '3';
		sf3 = fopen(outf, "w");

		if (sf1 == NULL || sf2 == NULL || sf3 == NULL) {
			fprintf(stderr, "Unable to open some files\n");
			return 1;
		}

		/* fill the buffer and create shares */
		for (int i = 0; i < SBUF_SIZE; i++) {
			c = getc(infile);
			if (c == EOF)
				break;
			sbuf[i] = c;
		}

		return sss_enc(sbuf, sf1, sf2, sf3);
	} else if (dec) {
		int result;

		/* set defaults only if nothing specified */
		if (n1 == NULL && n2 == NULL) {
			n1 = "share1";
			n2 = "share2";
		} else if (n1 == NULL || n2 == NULL) {
		    fprintf(stderr, "Please supply two shares\n");
		    return 1;
		}

		FILE *sf1, *sf2;

		sf1 = fopen(n1, "r");
		sf2 = fopen(n2, "r");

		if (sf1 == NULL || sf2 == NULL) {
			fprintf(stderr, "Unable to open some files\n");
			return 1;
		}

		result = sss_dec(sbuf, sf1, sf2);

		switch (result) {
		case 0:
			for (int i = 0; i < SBUF_SIZE; i++)
				putchar(sbuf[i]);
			return 0;
		case 1:
			printf("Bad input share\n");
			return 1;
		case 2:
			printf("Duplicated coordinates\n");
			return 1;
		}
	}

	/* shouldn't happen */
	fprintf(stderr, "No option specified\n");
	return 1;
}
