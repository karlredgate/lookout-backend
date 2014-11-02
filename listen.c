
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>

#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ioctl.h>
#include <syslog.h>

#include "lookout.pb-c.h"

#define CACHEDIR "/var/run/lookout"

static int debug = 0;

struct message_stats {
    int epoch;
    int written;
    int messages;
    int errors;
};

/*
 * change to syslog how many seen upto idle
 */
static void
idle( struct message_stats *s ) {
    if ( debug ) printf( "idle\n" );
    if ( s->epoch == s->written ) return;
    s->written = s->epoch;
    syslog( LOG_NOTICE, "prcoessed %d requests", s->messages );
    if ( debug ) printf( "stats updated\n" );
}

/*
 */
static ssize_t
intern( char *appsha, int64_t ip ) {
    // how to translate int64 to IP
    //

    // how to determine if the IP is bad

    // create path
    // path -- ./apps/<sha>/good/<ip-address>
    // path -- ./apps/<sha>/bad/<ip-address>
    char path[1024]; // what is the max string length of a protobuf
    // perhaps who cares - if it is longer than a sha256 then it is an error

    struct stat s;
    if ( stat(path, &s) == 0 ) {
        // cache hit - do nothing
    }
}

/*
 */
static ssize_t
process( int in, struct message_stats *s ) {
    unsigned char buffer[2048];

    ssize_t octets = read( in, buffer, sizeof(buffer) );

    Lookout__BackendCodingQuestions__Q1__IpEvent *message;

    message = lookout__backend_coding_questions__q1__ip_event__unpack( NULL, octets, buffer );

    if ( message == NULL ) {
        fprintf( stderr, "cannot unpack message\n" );
        s->errors++;
        return octets;
    }

    // fprintf( stderr, "aws dynamodb put-item --table-name IpEvent --item  <%s> <0x%0llx>\n", message->app_sha256, message->ip );
    intern( message->app_sha256, message->ip );

/*
package lookout.backend_coding_questions.q1;

message IpEvent {
  required string app_sha256 = 1;
  required int64 ip = 2;
}
 */

    return octets;
}

/*
 */
static void
loop( int sock ) {
    struct message_stats stats;
    memset( &stats, 0, sizeof(stats) );

    struct timeval timeout;
    fd_set fds;

    for (;;) {
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        FD_ZERO( &fds );
        FD_SET( sock, &fds );

        int result = select( 32, &fds, NULL, NULL, &timeout );
        if ( result == -1 ) {
            perror( "select" );
            continue;
        }

        if ( result == 0 ) {
            idle( &stats );
            continue;
        }

        if ( FD_ISSET(sock, &fds) ) {
            ssize_t octets = process( sock, &stats );
            stats.epoch += 1;
            if ( debug ) printf( "sock->tap %zd octets\n", octets );
        }
    }
}

/*
 */
static struct addrinfo*
parse( char *address, char *service ) {
    struct addrinfo hints = {
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_DGRAM,
        .ai_protocol = 0,
        .ai_flags = (AI_NUMERICSERV | AI_ADDRCONFIG),
    };

    struct addrinfo *info;
    int error = getaddrinfo( address, service, &hints, &info );

    switch ( error ) {
    case 0: break;
    case EAI_SERVICE:
        fprintf( stderr, "bad service '%s'\n", service );
        exit( -1 );
    case EAI_NONAME:
        fprintf( stderr, "bad address '%s'\n", address );
        exit( -1 );
    default:
        fprintf( stderr, "address parse error = %d\n", error );
        exit( -1 );
    }

    if ( info == NULL ) {
        fprintf( stderr, "cannot translate: %s:%s\n", address, service );
        exit( -1 );
    }

    return info;
}

/*
 */
static int
socket_open( char *laddr, char *lport ) {

    int error;

    struct addrinfo *local  = parse( laddr, lport );

    int sock = socket( local->ai_family, local->ai_socktype, local->ai_protocol );
    if ( sock < 0 ) {
        perror( "socket" );
        exit( -1 );
    }

    /* Bind to local port */
    error = bind( sock, local->ai_addr, local->ai_addrlen );
    if ( error < 0 ) {
        perror( "bind" );
        exit( -1 );
    }

    return sock;
}

/*
 */
int
main( int argc, char **argv ) {
    if ( argc < 3 ) {
        fprintf( stderr, "usage: %s local-address local-port\n", argv[0] );
        exit( -1 );
    }

    openlog( "lookout", LOG_PID, LOG_DAEMON );

    // check for cache dir - and die if not present - since this user should not be able to create it
    if ( chdir("/var/run/lookout") != 0 ) {
        syslog( LOG_NOTICE, "run dir missing - cannot run" );
        exit( -1 );
    }

    mkdir( "apps", 0755 );

    int sock = socket_open( argv[1], argv[2] );

    loop( sock );

    return 0;
}

/*
 * vim:autoindent
 * vim:expandtab
 */
