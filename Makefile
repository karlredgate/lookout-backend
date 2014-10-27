
test: encode decode udp
	./encode $$(sha256sum encode | sed -e 's/ .*//') 123456789 | ./decode

CLEANS += udp
udp: udp.o
	cc -o udp $^

CLEANS += decode
decode: lookout.pb-c.o decode.o
	cc -o decode $^ -lprotobuf-c

CLEANS += encode
encode: lookout.pb-c.o encode.o
	cc -o encode $^ -lprotobuf-c

lookout.pb-c.o: lookout.pb-c.h

CLEANS += lookout.pb-c.h
CLEANS += lookout.pb-c.c
lookout.pb-c.h lookout.pb-c.c: lookout.proto
	protoc-c --c_out=. $^

clean:
	rm -f $(CLEANS) *.o
