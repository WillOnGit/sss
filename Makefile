test: main.c
	cc -O2 -Wall -Werror -Werror=implicit -std=gnu11 -o sss main.c

debug: main.c
	cc -O0 -Wall -Werror -Werror=implicit -std=gnu11 -g -o sss main.c
