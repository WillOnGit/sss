#define	SBUF_SIZE	32
#define	SSS_VERSION	"2.0dev"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

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

static struct option long_options[] =
{
	{"version", no_argument,       0, 'v'},
	{"help",    no_argument,       0, 'h'},
	{"encode",  no_argument,       0, 'e'},
	{"decode",  no_argument,       0, 'd'},
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
	int enc, dec, opt, opt_index;
	char *n1, *n2;
	signed char sbuf[SBUF_SIZE] = { 0 };

	enc = 1;
	dec = 0;

	n1 = n2 = NULL;

	/*
	 * handle arguments
	 */
	while (1) {
		/* get option */
		opt_index = 0;
		opt = getopt_long (argc, argv, "vhed",
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
			/* changeme */
			enc = 1;
			dec = 0;
			break;
		case 'd':
			/* changeme */
			dec = 1;
			enc = 0;
			break;
		case '?':
			/* getopt_long already printed an error message */
			break;
		default:
			printf("Hmm\n");
			abort();
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
		FILE *infile, *sf1, *sf2, *sf3;
		FILE *outfiles[3];

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

		outfiles[0] = sf1;
		outfiles[1] = sf2;
		outfiles[2] = sf3;

		return sss_enc(sbuf, 3, outfiles);
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
