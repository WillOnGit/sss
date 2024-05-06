#include <stdio.h>
#include <string.h>

#include "libsss.h"

/*
 * - read stdin or first argument filename
 * - encode first character, if there is one
 */
int main(int argc, char **argv)
{
	int enc, dec;
	char *f1, *f2;

	enc = dec = 0;

	/* handle arguments */
	if (argc < 2) {
		/* no args */
		fprintf(stderr, "Not enough arguments\n");
		return 1;
	}

	if (argc == 2) {
		/* encode from stdin and output to argv[1] */
		enc = 1;
		f1 = "-";
		f2 = argv[1];
	} else if (argc == 3) {
		/* encode from argv[1] and output to argv[2] */
		enc = 1;
		f1 = argv[1];
		f2 = argv[2];
	} else {
		 /* encode/decode (argc[1]) from argv[2] and output to argv[3] */
		if (!strcmp("-d", argv[1]) || !strcmp("--decode", argv[1])) {
			dec = 1;
		} else if (!strcmp("-e", argv[1]) || !strcmp("--encode", argv[1])) {
			enc = 1;
		} else {
			fprintf(stderr, "Bad operation\n");
			return 1;
		}

		f1 = argv[2];
		f2 = argv[3];
	}

	if (enc) {
		sss_enc(f1, f2);
	}

	if (dec) {
		sss_dec(f1, f2);
	}

	return 0;
}
