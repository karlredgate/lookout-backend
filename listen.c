
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
#include <net/if.h>

#include "lookout.pb-c.h"

static int debug = 0;

struct dirstats {
    long int pkts;
    long int octets;
    long int dropped;
};

struct tunnel_stats {
    int epoch;
    int written;
    struct dirstats rx;
    struct dirstats tx;
};

/*
 */
static void
idle( struct tunnel_stats *s ) {
    if ( debug ) printf( "idle\n" );
    if ( s->epoch == s->written ) return;
    FILE *f = fopen( ".link-stats", "w" );
    if ( f == NULL ) {
        return;
    }
    fprintf( f, "%ld %ld %ld %ld\n", s->tx.pkts, s->tx.octets,
                                     s->rx.pkts, s->rx.octets );
    fclose( f );
    rename( ".link-stats", "link-stats" );
    s->written = s->epoch;
    printf( "stats updated\n" );
}

/*
 */
static ssize_t
forward( int in, struct dirstats *s ) {
    unsigned char buffer[2048];

    ssize_t octets = read( in, buffer, sizeof(buffer) );

    Lookout__BackendCodingQuestions__Q1__IpEvent *message;

    message = lookout__backend_coding_questions__q1__ip_event__unpack( NULL, octets, buffer );

    if ( message == NULL ) {
        fprintf( stderr, "cannot unpack message\n" );
        /* count errors? */
        return octets;
    }

    // fprintf( stderr, "aws dynamodb put-item --table-name IpEvent --item  <%s> <0x%0llx>\n", message->app_sha256, message->ip );

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
    struct tunnel_stats s;
    memset( &s, 0, sizeof(s) );

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
            idle( &s );
            continue;
        }

        if ( FD_ISSET(sock, &fds) ) {
            ssize_t octets = forward( sock, &(s.rx) );
            s.epoch += 1;
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

    int sock = socket_open( argv[1], argv[2] );

    loop( sock );

    return 0;
}

/*
 * vim:autoindent
 * vim:expandtab
 */
