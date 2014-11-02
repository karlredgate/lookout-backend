
PWD := $(shell pwd)
PACKAGE = lookout-backend

default: rpm

rpm: build
	rm -rf rpm
	mkdir -p rpm/BUILD rpm/RPMS rpm/BUILDROOT
	rpmbuild --quiet -bb --define="_topdir $(PWD)/rpm" $(PACKAGE).spec

build: lookout-backend

test: encode decode lookout-backend send-event
	./encode $$(sha256sum encode | sed -e 's/ .*//') 123456789 | ./decode

CLEANS += lookout-backend
lookout-backend: lookout.pb-c.o lookout-backend.o
	cc -o lookout-backend $^ -lprotobuf-c

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
