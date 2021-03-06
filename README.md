Example backend for IpEvent service
===================================

The service is structured in two parts, the UDP recevier and the REST
APT service.  This structure allows for the recevier to be scaled to
mutliple hosts with DNS round robin load balancing to the client devices,
while providing a single interface for the API.

The UDP receiver is very small and has been tested with the provided
client as is, and also with the sleep between messages reduced by
two orders of magnitude.  The test was run on a t2.micro instance
over the localhost interface.  Given it started at 33K/sec according
to the doc, that should have been 3.3N/sec, but I did not observe
the client providing that level of load.

The communication between the receiver and the REST API is a tree of
files in the filesystem.  There is a command line tool that given a
SHA hash returns the JSON data for that app.  This tool is used by the
API to service the `/events/:sha` request.  Using the external tool
also allows for testing of the receiver data outside of the API.

## Installation

This package is installed from an RPM.  The RPM is built on an AWS
t2.micro instance running the basic AWS Linux AMI, augmented with
a build server rpm found in a secondary github repo at:

```
http://github.com/karlredgate/build-server
```

The build server can be generated from an RPM in a release from the
build server repo.  There is a `cloud-init` script in the build
server repo that can be used in creating the build server either
from the console or from the command line:

```
https://raw.githubusercontent.com/karlredgate/build-server/master/cloud-init
```

The backend server can also be installed using a `cloud-init` from the
backend repo:

```
https://raw.githubusercontent.com/karlredgate/lookout-backend/master/cloud-init
```

## Testing

The `test-client` tests sending 30% randomly corrupted packets to
the server.

Also, use the provided ruby client but decrease the delay between
messages until the drop rate is determined.  In this case we get
to at least 3.3M messages per second over the looback interface.
Removing the sleep entirely caused a 13% drop rate.

Run the server under `valgrind --leak-yes` to detect memory leaks.
There are a couple leaks that are reported, but they are data
structures whose lifetime is the lifetime of the process.

## Scaling

To scale this service add an inotify watcher for the data stored on each
receiver server and have it push its data to S3.  The REST API service
would then change to access the S3 data instead of the local filesystem
data.

The receiver hosts could be in an autoscale group to allow then to
grow dynamically.

## References

The UDP code in this is picked up from my `sumner-tunnel` repo, which is a
very simple TAP/UDP over IPv4/IPv6 ethernet tunnel.  (Also on github)

<!--
Web sites referenced
https://github.com/lookout/backend-coding-questions
-- protobuf
https://code.google.com/p/protobuf-c/wiki/Examples
https://github.com/protobuf-c/protobuf-c
-- S3 libraries
https://github.com/qoobaa/s3
https://github.com/marcel/aws-s3
https://github.com/bji/libs3
 -->

<!-- vim: set autoindent expandtab sw=4: -->
