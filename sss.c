#define SSS_VERSION   "2.0dev"
#define SBUF_SIZE     32
#define MIN_SHARES    2
#define MAX_SHARES    9
#define MIN_THRESHOLD 2
#define MAX_THRESHOLD 9

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "libsss.h"

const char *const sss_version = SSS_VERSION;
const char *const helptext =
	"sss - Shamir's secret sharing with libsss (by WillOnGit)\n"
	"\n"
	"Usage: sss [--encode | -e] [--threshold | -k num] [--shares | -n num]\n"
	"           [infile] [outdir]\n"
	"\n"
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
	"[--help | -h] prints this message then exits.\n";

static struct option long_options[] = {
	{ "version",   no_argument,       0, 'v' },
	{ "help",      no_argument,       0, 'h' },
	{ "encode",    no_argument,       0, 'e' },
	{ "decode",    no_argument,       0, 'd' },
	{ "shares",    required_argument, 0, 'n' },
	{ "threshold", required_argument, 0, 'k' },
	{ 0,           0,                 0, 0   }
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
	int enc, dec, n, k, opt, opt_index;
	signed char sbuf[SBUF_SIZE] = { 0 };

	enc = 1;
	dec = 0;
	n = 3;
	k = 2;

	/*
	 * handle arguments
	 */
	while (1) {
		/* get option */
		opt_index = 0;
		opt = getopt_long(argc, argv, "vhedn:k:", long_options,
		                  &opt_index);

		if (opt == -1)
			/* no more options */
			break;

		/* handle an option */
		switch (opt) {
		case 'v':
			/* emit version info and exit immediately */
			printf("sss version: %s\nlibsss version: %s\n",
			       sss_version, libsss_version);
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
				fprintf(stderr,
				        "number of shares to generate must be between %d and %d, inclusive\n",
				        MIN_SHARES, MAX_SHARES);
				return 1;
			}
			break;
		case 'k':
			k = strtol(optarg, NULL, 10);

			/* handle parsing errors and invalid values together */
			if (k < MIN_THRESHOLD || k > MAX_THRESHOLD) {
				fprintf(stderr,
				        "share threshold must be between %d and %d, inclusive\n",
				        MIN_THRESHOLD, MAX_THRESHOLD);
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

	/* options parsed, call relevant lib function with args */
	if (enc) {
		int c, sp;
		char *outf, *sharedir;
		FILE *infile, *sharef, *outfiles[n];

		/* check for invalid k, n */
		if (n < k) {
			fprintf(stderr,
			        "Number of shares to generate below recovery threshold\n");
			return 1;
		}

		/* set input file, with default and empty check */
		if (argc == optind || !strcmp(argv[optind], "-")) {
			infile = stdin;
		} else if (!strcmp(argv[optind], "")) {
			fprintf(stderr,
			        "Please supply a nonempty input filename\n");
			return 1;
		} else {
			infile = fopen(argv[optind], "r");
			if (infile == NULL) {
				fprintf(stderr,
				        "Unable to open file %s\n",
				        argv[optind]);
				return 1;
			}
		}

		/* set share output directory, with default and empty check */
		if (argc <= optind + 1) {
			sharedir = "./";
			sp = 2;
		} else if (!strcmp(argv[optind + 1], "")) {
			fprintf(stderr,
			        "Please supply a nonempty directory name\n");
			return 1;
		} else {
			sharedir = argv[optind + 1];
			sp = strlen(sharedir);
		}

		/* construct share filenames - only valid with MAX_SHARES <= 9 */
		outf = malloc(sp + 8);
		if (outf == NULL)
			return 1;

		memcpy(outf, sharedir, sp * sizeof(char));
		if (outf[sp - 1] != '/') {
			outf[sp++] = '/';
		}

		strcpy(&outf[sp], "share0");
		sp += 6;
		outf[sp--] =
			'\0'; /* can probably optimise this out with calloc */

		/* start */
		for (int i = 0; i < n; i++) {
			outf[sp] = '1' + i; /* dubious, but very K&R */
			sharef = fopen(outf, "w");

			if (sharef == NULL) {
				fprintf(stderr,
				        "Unable to open %s for writing\n",
				        outf);
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

		return sss_enc(sbuf, k, n, outfiles);
	} else if (dec) {
		int result, args, shares;
		FILE **sharefiles;

		args = argc - optind;

		/*
		 * set defaults only if nothing specified
		 *
		 * ALSO only valid if MAX_SHARES <= 9
		 */
		if (args == 0) {
			shares = 2;

			sharefiles = malloc(shares * sizeof(FILE *));
			if (sharefiles == NULL) {
				fprintf(stderr,
				        "Unable to allocate shares\n");
				return 1;
			}

			sharefiles[0] = fopen("share1", "r");
			sharefiles[1] = fopen("share2", "r");
			if (sharefiles[0] == NULL ||
			    sharefiles[1] == NULL) {
				fprintf(stderr,
				        "Unable to open some files\n");
				return 1;
			}
		} else if (args == 1) {
			fprintf(stderr,
			        "Please supply at least two shares\n");
			return 1;
		} else {
			shares = args;

			sharefiles = malloc(shares * sizeof(FILE *));
			if (sharefiles == NULL) {
				fprintf(stderr,
				        "Unable to allocate shares\n");
				return 1;
			}

			for (int i = 0; i < args; i++) {
				sharefiles[i] =
					fopen(argv[optind++], "r");
				if (sharefiles[i] == NULL) {
					fprintf(stderr,
					        "Unable to open file %s\n",
					        argv[optind]);
					return 1;
				}
			}
		}

		result = sss_dec(sbuf, shares, sharefiles);

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
		case 3:
			fprintf(stderr, "Invalid k\n");
			return 1;
		default:
			fprintf(stderr,
			        "An unknown error occurred decoding the secret\n");
			return 1;
		}
	}

	/* shouldn't happen */
	fprintf(stderr, "No option specified\n");
	return 1;
}
