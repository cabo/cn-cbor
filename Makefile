all: size test
	env MallocStackLogging=true ./test >new.out
	-diff new.out expected.out

test: test.c cbor.h cn-cbor.h cn-cbor.c
	clang cn-cbor.c test.c -o test

size: cn-cbor.o
	size cn-cbor.o
	size -m cn-cbor.o

cn-cbor.o: cn-cbor.c cn-cbor.h cbor.h
	clang -I /Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS5.0.sdk/usr/include -arch armv7 -Os -c cn-cbor.c

cn-cbor-play.zip: Makefile cbor.h cn-cbor.c cn-cbor.h expected.out test.c
	zip $@ $^
