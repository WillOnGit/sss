#define	SSS_VERSION	"2.0dev"
#define	SBUF_SIZE	32
#define	MIN_SHARES	2
#define	MAX_SHARES	9

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "libsss.h"

const char * const sss_version = SSS_VERSION;
const char * const helptext =
	"sss - Shamir's secret sharing with libsss (by WillOnGit)\n"
	"\n"
	"Usage: sss [--encode | -e] [--shares | -n num] [infile] [outdir]\n"
	"or:    sss [--decode | -d] [sharefile1 sharefile2]\n"
	"or:    sss [--version | -v]\n"
	"or:    sss [--help | -h]\n"
	"\n"
	"If none of [--encode | -e] or [--decode | -d] are specified, encoding is\n"
	"the default operation.\n"
	"\n"
	"When encoding shares, the input file defaults to stdin ('-') and the out\n"
	"directory defaults to the working directory. [--shares | -n] (default 3)\n"
	"shares will be generated and written to out directory, named share1,\n"
	"share2, etc.\n"
	"\n"
	"When decoding from shares, if no filenames are supplied they will\n"
	"default to share1 and share2. Supplying only one filename is an error.\n"
	"Output is to stdout.\n"
	"\n"
	"[--version | -v] prints the program and library versions then exits.\n"
	"\n"
	"[--help | -h] prints this message then exits.\n"
	;

static struct option long_options[] =
{
	{"version", no_argument,       0, 'v'},
	{"help",    no_argument,       0, 'h'},
	{"encode",  no_argument,       0, 'e'},
	{"decode",  no_argument,       0, 'd'},
	{"shares",  required_argument, 0, 'n'},
	{0, 0, 0, 0}
};

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
	int enc, dec, n, opt, opt_index;
	char *n1, *n2;
	signed char sbuf[SBUF_SIZE] = { 0 };

	enc = 1;
	dec = 0;
	n = 3;

	n1 = n2 = NULL;

	/*
	 * handle arguments
	 */
	while (1) {
		/* get option */
		opt_index = 0;
		opt = getopt_long (argc, argv, "vhedn:",
				long_options, &opt_index);

		if (opt == -1)
			/* no more options */
			break;

		/* handle an option */
		switch (opt) {
		case 'v':
			/* emit version info and exit immediately */
			printf("sss version: %s\nlibsss version: %s\n", sss_version, libsss_version);
			return 0;
		case 'h':
			/* emit help text and exit immediately */
			printf("%s", helptext);
			return 0;
		case 'e':
			enc = 1;
			dec = 0;
			break;
		case 'd':
			dec = 1;
			enc = 0;
			break;
		case 'n':
			n = strtol(optarg, NULL, 10);

			/* handle parsing errors and invalid values together */
			if (n < MIN_SHARES || n > MAX_SHARES) {
				fprintf(stderr, "number of shares to generate must be between %d and %d, inclusive\n", MIN_SHARES, MAX_SHARES);
				return 1;
			}
			break;
		case '?':
			/* getopt_long already printed an error message */
			printf("%s", helptext);
		default:
			return 1;
		}
	}

	/* assign any remaining args */
	switch (argc - optind) {
	case 2:
		n1 = argv[optind++];
		n2 = argv[optind];
		break;
	case 1:
		n1 = argv[optind];
		break;
	default:
		;
	}

	/* options parsed, call relevant lib function with args */
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
		FILE *infile, *sharef;
		FILE *outfiles[n];

		if (!strcmp(n1, "-")) {
			infile = stdin;
		} else {
			infile = fopen(n1, "r");
			if (infile == NULL) {
				fprintf(stderr, "Unable to open file %s\n", n1);
				return 1;
			}
		}

		/* construct share filenames - only valid with MAX_SHARES <= 9 */
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
		outf[sp--] = '\0';

		/* start */
		for (int i = 0; i < n; i++) {
			outf[sp] = '1' + i;// dubious, but very K&R
			sharef = fopen(outf, "w");

			if (sharef == NULL) {
				fprintf(stderr, "Unable to open %s for writing\n", outf);
				return 1;
			}

			outfiles[i] = sharef;
		}

		/* fill the buffer and create shares */
		for (int i = 0; i < SBUF_SIZE; i++) {
			c = getc(infile);
			if (c == EOF)
				break;
			sbuf[i] = c;
		}

		return sss_enc(sbuf, n, outfiles);
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

		FILE *sf[2];

		sf[0] = fopen(n1, "r");
		sf[1] = fopen(n2, "r");

		if (sf[0] == NULL || sf[1] == NULL) {
			fprintf(stderr, "Unable to open some files\n");
			return 1;
		}

		result = sss_dec(sbuf, 2, sf);

		switch (result) {
		case 0:
			for (int i = 0; i < SBUF_SIZE; i++)
				putchar(sbuf[i]);
			return 0;
		case 1:
			fprintf(stderr, "Bad input share\n");
			return 1;
		case 2:
			fprintf(stderr, "Duplicated coordinates\n");
			return 1;
		}
	}

	/* shouldn't happen */
	fprintf(stderr, "No option specified\n");
	return 1;
}
