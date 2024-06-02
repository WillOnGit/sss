
sss: sss.c libsss.c
	cc -O2 -Wall -Werror -Werror=implicit -std=gnu11 -lgmp -o sss sss.c libsss.c

debug: sss.c libsss.c
	cc -O0 -Wall -Werror -Werror=implicit -std=gnu11 -lgmp -g -o debug sss.c libsss.c
