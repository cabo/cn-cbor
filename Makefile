# enable this for iphone builds
#CFLAGS = -I /Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS5.0.sdk/usr/include -arch armv7 -Os

all: cntest

test: cntest
	env MallocStackLogging=true ./cntest >new.out
	-diff new.out expected.out

cntest: test.c cbor.h cn-cbor.h cn-cbor.c
	clang cn-cbor.c test.c -o cntest

size: cn-cbor.o
	size cn-cbor.o
	size -m cn-cbor.o

cn-cbor.o: cn-cbor.c cn-cbor.h cbor.h
	clang $(CFLAGS) -c cn-cbor.c

cn-cbor-play.zip: Makefile cbor.h cn-cbor.c cn-cbor.h expected.out test.c
	zip $@ $^

clean:
	$(RM) cntest *.o new.out
