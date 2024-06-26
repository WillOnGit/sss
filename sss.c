#define	SSS_VERSION	"0.1dev"

#include <stdio.h>
#include <string.h>

#include "libsss.h"

const char * const sss_version = SSS_VERSION;

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
	char *f1, *f2;

	enc = 1;
	dec = 0;

	f1 = f2 = NULL;

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

		/* pick up anything else as a file/directory argument, ignoring if 2 */
		if (f1 == NULL) {
			f1 = argv[i];
		} else if (f2 == NULL) {
			f2 = argv[i];
		}
	}

	/* options parsed, call relevant lib function */
	if (enc) {
		/* set defaults if not specified */
		if (f1 == NULL) {
			/* f2 will also be NULL if f1 is */
			return sss_enc("-", "./");
		} else if (f2 == NULL) {
			return sss_enc(f1, "./");
		} else {
			return sss_enc(f1, f2);
		}
	} else if (dec) {
		/* set defaults only if nothing specified */
		if (f1 == NULL && f2 == NULL) {
			f1 = "share1";
			f2 = "share2";
		} else if (f1 == NULL || f2 == NULL) {
		    fprintf(stderr, "Please supply two shares\n");
		    return 1;
		}

		return sss_dec(f1, f2);
	}

	/* shouldn't happen */
	fprintf(stderr, "No option specified\n");
	return 1;
}
