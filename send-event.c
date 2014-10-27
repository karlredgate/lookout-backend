
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

/*
 */
static ssize_t
transmit( int out, char *shasum, char *ip_address ) {
    Lookout__BackendCodingQuestions__Q1__IpEvent message = LOOKOUT__BACKEND_CODING_QUESTIONS__Q1__IP_EVENT__INIT;

    message.app_sha256 = shasum;
    message.ip = atoll( ip_address );

    unsigned int length = lookout__backend_coding_questions__q1__ip_event__get_packed_size( &message );
    char *buffer = malloc( length );
    lookout__backend_coding_questions__q1__ip_event__pack( &message, buffer );

    ssize_t written = write( out, buffer, length );

    if ( written < 0 ) {
        perror( "write" );
    }

    if ( written < length ) {
        perror( "write" );
    }

    return written;
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
socket_open( char *raddr, char *rport ) {
    int error;

    struct addrinfo *remote = parse( raddr, rport );

    int sock = socket( remote->ai_family, remote->ai_socktype, remote->ai_protocol );
    if ( sock < 0 ) {
        perror( "socket" );
        exit( -1 );
    }

    /* Bind to remote port */
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
    if ( argc < 5 ) {
        fprintf( stderr, "usage: %s "
                                  " remote-address remote-port"
                                  " shasum address\n", argv[0] );
        exit( -1 );
    }

    int sock = socket_open( argv[1], argv[2] );

    transmit( sock, argv[3], argv[4]  );

    return 0;
}

/*
 * vim:autoindent
 * vim:expandtab
 */
