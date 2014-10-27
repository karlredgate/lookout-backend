
test: encode decode listen send-event
	./encode $$(sha256sum encode | sed -e 's/ .*//') 123456789 | ./decode

CLEANS += listen
listen: lookout.pb-c.o listen.o
	cc -o listen $^ -lprotobuf-c

CLEANS += send-event
send-event: lookout.pb-c.o send-event.o
	cc -o send-event $^ -lprotobuf-c

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
