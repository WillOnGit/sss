#include <stdio.h>
#include <string.h>

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

	while ((c = getc(infile)) != EOF)
		putchar(c);

	return 0;
}
