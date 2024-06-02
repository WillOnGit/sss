.PHONY: test clean

sss: sss.c libsss.c
	cc -O2 -Wall -Werror -Werror=implicit -std=gnu11 -lgmp -o sss sss.c libsss.c

debug: sss.c libsss.c
	cc -O0 -Wall -Werror -Werror=implicit -std=gnu11 -lgmp -g -o debug sss.c libsss.c

test: sss
	@echo "testing the string input \"testing\""
	@echo "testing" | ./sss ./
	@./sss -d share1 share2
	@./sss -d share1 share3
	@./sss -d share2 share1
	@./sss -d share2 share3
	@./sss -d share3 share1
	@./sss -d share3 share2
	@echo "testing all zero bytes"
	@echo "0000000000000000000000000000000000000000000000000000000000000000" \
		| xxd -r -p \
		| ./sss ./
	@./sss -d share1 share2 | xxd
	@./sss -d share1 share3 | xxd
	@./sss -d share2 share1 | xxd
	@./sss -d share2 share3 | xxd
	@./sss -d share3 share1 | xxd
	@./sss -d share3 share2 | xxd
	@echo "testing all FF bytes"
	@echo "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff" \
		| xxd -r -p \
		| ./sss ./
	@./sss -d share1 share2 | xxd
	@./sss -d share1 share3 | xxd
	@./sss -d share2 share1 | xxd
	@./sss -d share2 share3 | xxd
	@./sss -d share3 share1 | xxd
	@./sss -d share3 share2 | xxd

clean:
	rm -rf sss debug debug.dSYM/ share{1,2,3}
