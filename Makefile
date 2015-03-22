# enable this for armv7 builds, lazily using iPhone SDK
#CFLAGS = -I /Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS5.0.sdk/usr/include -arch armv7 -Os
CFLAGS = -Os -Wall -Wextra -Wno-unknown-pragmas -Werror-implicit-function-declaration -Werror -Wno-unused-parameter -Wdeclaration-after-statement -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes

all: cntest

test: cntest
	env MallocStackLogging=true ./cntest >new.out
	-diff new.out expected.out

cntest: test.c cbor.h cn-cbor.h cn-cbor.c cn-manip.c
	clang $(CFLAGS) cn-cbor.c cn-error.c cn-manip.c test.c -o cntest

size: cn-cbor.o
	size cn-cbor.o
	size -m cn-cbor.o

cn-cbor.o: cn-cbor.c cn-cbor.h cbor.h
	clang $(CFLAGS) -c cn-cbor.c

cn-cbor-play.zip: Makefile cbor.h cn-cbor.c cn-cbor.h expected.out test.c
	zip $@ $^

clean:
	$(RM) cntest *.o new.out
