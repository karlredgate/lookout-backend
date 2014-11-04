
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <time.h>

#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <sys/ioctl.h>
#include <syslog.h>

#include "lookout.pb-c.h"

struct message_stats {
    int epoch;
    int written;
    int messages;
};

static int debug = 0;
static int errors = 0;

void dbgprintf( const char *fmt, ... ) {
    va_list ap;
    va_start( ap, fmt );
    if ( debug > 0 )  vprintf( fmt, ap );
    va_end( ap );
}

char *good_sha[] = { NULL, NULL, NULL };

/*
 */
static ssize_t
transmit_good( int out ) {
    Lookout__BackendCodingQuestions__Q1__IpEvent message = LOOKOUT__BACKEND_CODING_QUESTIONS__Q1__IP_EVENT__INIT;

    char *buffer;
    unsigned int length;

    message.app_sha256 = good_sha[ random() % 3 ];
    message.ip = random();

    length = lookout__backend_coding_questions__q1__ip_event__get_packed_size( &message );
    buffer = malloc( length );
    lookout__backend_coding_questions__q1__ip_event__pack( &message, buffer );

    write( out, buffer, length );
    free( buffer );

    return length;
}

/*
 */
static ssize_t
transmit_corrupted( int out ) {
    Lookout__BackendCodingQuestions__Q1__IpEvent message = LOOKOUT__BACKEND_CODING_QUESTIONS__Q1__IP_EVENT__INIT;

    char *buffer;
    unsigned int length;

    message.app_sha256 = good_sha[ random() % 3 ];
    message.ip = random();

    length = lookout__backend_coding_questions__q1__ip_event__get_packed_size( &message );
    buffer = malloc( length );
    lookout__backend_coding_questions__q1__ip_event__pack( &message, buffer );

    int corrupted_bytes = random() % length;
    int i;
    for ( i = 0 ; i < corrupted_bytes ; ++i ) {
        int byte = random() % length;
        buffer[byte] = random() % 256;
    }

    write( out, buffer, length );
    free( buffer );

    return length;
}

/*
 */
static ssize_t
transmit( int sock ) {
    switch ( random() % 10 ) {
    case 0: /* transmit more good messages than bad ones */
    case 1:
    case 2:
    case 4:
    case 5:
    case 6:
        transmit_good( sock );
        break;
    case 7:
    case 8:
    case 9:
        transmit_corrupted( sock );
        break;
    }
}

/*
 */
static void
loop( int sock ) {
    struct timespec delay;

    struct timeval timeout;
    fd_set fds;

    int limit = random() % 1000000;
    int i;

    printf( "About to send %d messages\n", limit );
    for ( i = 0 ; i < limit ; ++i ) {
        if ( (i % 10000) == 0 ) {
            fputc( '.', stdout );
            fflush( stdout );
        }
        delay.tv_sec = 0;
        delay.tv_nsec = random() % 1000;
        nanosleep( &delay, NULL );
        transmit( sock );
    }
    printf( "\nDone\n" );
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
tune_socket( int sock ) {
    int oldbuf = 0;
    socklen_t optlen = sizeof(oldbuf);
    getsockopt( sock, SOL_SOCKET, SO_RCVBUF, &oldbuf, &optlen );

    int newbuf = 10 * 1024 * 1024;
    int error = setsockopt( sock, SOL_SOCKET, SO_SNDBUF, &newbuf, sizeof(newbuf) );
    if ( error < 0 ) {
        syslog( LOG_ERR, "could not increase socket buffer size" );
        return;
    }

    syslog( LOG_NOTICE, "increased socket buffer size from %d to %d", oldbuf, newbuf );
}

/*
 */
static int
socket_open( char *raddr, char *rport ) {
    int error;

    struct addrinfo *remote  = parse( raddr, rport );

    int sock = socket( remote->ai_family, remote->ai_socktype, remote->ai_protocol );
    if ( sock < 0 ) {
        perror( "socket" );
        exit( -1 );
    }

    tune_socket( sock );

    /* connect to remote address/port */
    error = connect( sock, remote->ai_addr, remote->ai_addrlen );
    if ( error < 0 ) {
        perror( "connect" );
        exit( -1 );
    }

    return sock;
}

/*
 */
int
main( int argc, char **argv ) {
    if ( argc < 6 ) {
        fprintf( stderr, "usage: %s remote-address remote-port sha1 sha2 sha3\n", argv[0] );
        exit( -1 );
    }

    if ( isatty(0) )  debug = 1;
    srandom( time(NULL) );

    int i;
    for ( i = 0 ; i < 3 ; ++i ) {
        good_sha[i] = argv[ i + 3 ];
    }

    int sock = socket_open( argv[1], argv[2] );
    loop( sock );

    return 0;
}

/*
 * vim:autoindent
 * vim:expandtab
 */
