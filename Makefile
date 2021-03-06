
PWD := $(shell pwd)
ARCH := $(shell arch)
PACKAGE = lookout-backend

INCS=-I/opt/local/include
CFLAGS += $(INCS)

default: rpm

rpm: build
	rm -rf rpm
	mkdir -p rpm/BUILD rpm/RPMS rpm/BUILDROOT
	rpmbuild --quiet -bb --define="_topdir $(PWD)/rpm" $(PACKAGE).spec

build: lookout-backend test-client

test: encode decode lookout-backend send-event test-client
	./encode $$(sha256sum encode | sed -e 's/ .*//') 123456789 | ./decode

# this is just for testing installs during development
update: rpm
	sudo yum --enablerepo=epel update -y rpm/RPMS/$(ARCH)/lookout-backend-*.$(ARCH).rpm

CLEANS += test-client
test-client: lookout.pb-c.o test-client.o
	cc $(INCS) -o test-client $^ -L/opt/local/lib -lprotobuf-c

CLEANS += lookout-backend
lookout-backend: lookout.pb-c.o lookout-backend.o
	cc $(INCS) -o lookout-backend $^ -L/opt/local/lib -lprotobuf-c

CLEANS += send-event
send-event: lookout.pb-c.o send-event.o
	cc $(INCS) -o send-event $^ -lprotobuf-c

CLEANS += decode
decode: lookout.pb-c.o decode.o
	cc $(INCS) -o decode $^ -lprotobuf-c

CLEANS += encode
encode: lookout.pb-c.o encode.o
	cc $(INCS) -o encode $^ -lprotobuf-c

lookout.pb-c.o: lookout.pb-c.h

CLEANS += lookout.pb-c.h
CLEANS += lookout.pb-c.c
lookout.pb-c.h lookout.pb-c.c: lookout.proto
	protoc-c --c_out=. $^

clean:
	rm -f $(CLEANS) *.o
