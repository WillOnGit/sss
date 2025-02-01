.PHONY: all clean debug fmt install lastflags mac release test uninstall
.SILENT: lastflags test

# variables
CFLAGS = -Wall -Werror -Werror=implicit -std=gnu11
LDLIBS = -lgmp
OBJECTS = sss.o libsss.o
LIBVERSION = 2.0

# main build targets
all: release libsss.a(libsss.o)
mac: all libsss.dylib

# build: bins
release: CFLAGS += -O2
release: lastflags sss

debug: CFLAGS += -O0 -g
debug: lastflags sss

lastflags:
	touch .flags
	if [[ $$(cat .flags) != "$(CFLAGS)" ]]; then echo "rebuilding object files"; rm -f $(OBJECTS); fi
	echo "$(CFLAGS)" > .flags

sss: $(OBJECTS)

# build: libs
libsss.a(libsss.o): libsss.o

libsss.dylib: CFLAGS += -O2 -dynamiclib -current_version $(LIBVERSION) -compatibility_version $(LIBVERSION) -install_name /usr/local/lib/libsss.dylib
libsss.dylib: libsss.c
	$(CC) $(CFLAGS) $(LDLIBS) libsss.c -o libsss.dylib

# other targets
test: sss
	echo "testing the string input \"testing\""
	echo "testing" | ./sss
	./sss -d share1 share2
	./sss -d share1 share3
	./sss -d share2 share1
	./sss -d share2 share3
	./sss -d share3 share1
	./sss -d share3 share2
	echo "testing all zero bytes"
	echo "0000000000000000000000000000000000000000000000000000000000000000" \
		| xxd -r -p \
		| ./sss
	./sss -d share1 share2 | xxd
	./sss -d share1 share3 | xxd
	./sss -d share2 share1 | xxd
	./sss -d share2 share3 | xxd
	./sss -d share3 share1 | xxd
	./sss -d share3 share2 | xxd
	echo "testing all FF bytes"
	echo "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff" \
		| xxd -r -p \
		| ./sss
	./sss -d share1 share2 | xxd
	./sss -d share1 share3 | xxd
	./sss -d share2 share1 | xxd
	./sss -d share2 share3 | xxd
	./sss -d share3 share1 | xxd
	./sss -d share3 share2 | xxd
	echo "testing invocations with string inputs \"helloX\""
	echo "hello1" | ./sss
	./sss -d
	./sss --decode
	./sss -d share1 share2
	echo "hello2" | ./sss -
	./sss -d
	echo "hello3" | ./sss - ./
	./sss -d
	echo "hello4" | ./sss -e - ./
	./sss -d
	echo "hello5" | ./sss --encode
	./sss -d
	echo "hello6" | ./sss --encode -
	./sss -d
	echo "hello7" | ./sss - ./ --encode
	./sss share1 share2 -d
	echo "hello8" | ./sss - . --encode
	./sss share1 share2 -d

install: all
	sudo install -C -m 0644 libsss.a /usr/local/lib/
	sudo install -C -m 0755 sss /usr/local/bin/
	if [[ -f libsss.dylib ]]; then sudo install -C -m 0755 libsss.dylib /usr/local/lib/; fi;

uninstall:
	sudo rm -f /usr/local/lib/libsss.a /usr/local/lib/libsss.dylib /usr/local/bin/sss

fmt:
	clang-format -i *.c *.h

clean:
	rm -rf $(OBJECTS) .flags libsss.a libsss.dylib share* sss sss.dSYM/
