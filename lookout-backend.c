
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>

#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <sys/ioctl.h>
#include <syslog.h>

#include <search.h>
#include <glob.h>
#include <libgen.h>

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

/*
 */
static void
write_count( char *sha, long count ) {
    static const int PATHLEN = 1024;
    int bytes;

    char path[PATHLEN];
    bytes = snprintf( path, sizeof(path), "events/%s/.count", sha );
    if ( bytes > PATHLEN ) return; // Bad path

    char tmppath[PATHLEN];
    bytes = snprintf( tmppath, sizeof(tmppath), "events/%s/.new", sha );
    if ( bytes > PATHLEN ) return; // Bad path

    FILE *f = fopen( tmppath, "w" );
    /* if the file cannot be opened, treat it as a dropped message */
    if ( f == NULL ) return;
    fprintf( f, "%ld", count );
    fclose( f );

    rename( tmppath, path );
}

/*
 */
static void
idle() {
    glob_t g;
    int error = glob( "events/*", GLOB_NOSORT, NULL, &g );
    if ( error != 0 ) {
        syslog( LOG_ERR, "failed to glob the events dir" );
        return;
    }

    int i;
    for ( i = 0 ; i < g.gl_pathc ; ++i ) {
        ENTRY entry;
        entry.key = basename( g.gl_pathv[i] );
        ENTRY *e = hsearch( entry, FIND );
        if ( e == NULL ) continue;

        struct message_stats *stats = (struct message_stats *)( e->data );
        if ( stats == NULL ) continue;

        if ( stats->messages == stats->written ) continue;
        write_count( e->key, stats->messages );
        stats->written = stats->messages;
    }

    globfree( &g );
}

/*
 */
static void
intern( char *sha, int64_t ip ) {
    static const int PATHLEN = 1024;
    char path[PATHLEN];

    int bytes = snprintf( path, sizeof(path), "events/%s", sha );
    if ( bytes > PATHLEN ) return; // Bad path

    /* ignore errors - if directory already exists, it is fine */
    mkdir( path, 0755 );

    uint8_t octet1 = (ip>>24) & 0xFF;
    uint8_t octet2 = (ip>>16) & 0xFF;
    uint8_t octet3 = (ip>> 8) & 0xFF;
    uint8_t octet4 =  ip      & 0xFF;

    bytes = snprintf( path, sizeof(path),
                      "events/%s/%02x%02x%02x%02x",
                      sha, octet1, octet2, octet3, octet4 );
    if ( bytes > PATHLEN ) return; // Bad path

    struct stat s;
    if ( stat(path, &s) == 0 ) { // cache hit - do nothing
        return;
    }

    int fd = open(path, O_CREAT|O_WRONLY|O_TRUNC, 0666 );
    if ( fd < 0 ) {
        syslog( LOG_ERR, "could not create <%s>", path );
        return;
    }

    close( fd );
}

/*
 */
static ssize_t
process( int in ) {
    unsigned char buffer[2048];

    ssize_t octets = read( in, buffer, sizeof(buffer) );

    Lookout__BackendCodingQuestions__Q1__IpEvent *message;

    message = lookout__backend_coding_questions__q1__ip_event__unpack( NULL, octets, buffer );

    if ( message == NULL ) {
        fprintf( stderr, "cannot unpack message\n" );
        errors++;
        return octets;
    }

    intern( message->app_sha256, message->ip );

    ENTRY entry;
    entry.key = message->app_sha256;
    entry.data = NULL;

    ENTRY *e = hsearch( entry, ENTER );

    if ( e != NULL ) {
        if ( e->key == entry.key ) {
            e->key = strdup( e->key );
        }

        if ( e->data == NULL ) {
            e->data = calloc( 1, sizeof(struct message_stats) );
        }
        struct message_stats *stats = (struct message_stats *)( e->data );
        stats->messages += 1;
    }

    lookout__backend_coding_questions__q1__ip_event__free_unpacked( message, NULL );

    return octets;
}

/*
 */
static void
loop( int sock ) {
    struct timeval timeout;
    fd_set fds;

    for (;;) {
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        FD_ZERO( &fds );
        FD_SET( sock, &fds );

        int result = select( 32, &fds, NULL, NULL, &timeout );
        if ( result == -1 ) {
            perror( "select" );
            continue;
        }

        if ( result == 0 ) {
            idle();
            continue;
        }

        if ( FD_ISSET(sock, &fds) ) {
            ssize_t octets = process( sock );
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
static void
tune_socket( int sock ) {
    int oldbuf = 0;
    socklen_t optlen = sizeof(oldbuf);
    getsockopt( sock, SOL_SOCKET, SO_RCVBUF, &oldbuf, &optlen );

    int newbuf = 10 * 1024 * 1024;
    int error = setsockopt( sock, SOL_SOCKET, SO_RCVBUF, &newbuf, sizeof(newbuf) );
    if ( error < 0 ) {
        syslog( LOG_ERR, "could not increase socket buffer size" );
        return;
    }

    syslog( LOG_NOTICE, "increased socket buffer size from %d to %d", oldbuf, newbuf );
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

    tune_socket( sock );

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

    if ( isatty(0) )  debug = 1;
    openlog( "lookout", LOG_PID, LOG_DAEMON );
    hcreate( 64 );

    /*
     * check for cache dir - and die if not present - since this
     * user should not be able to create it
     */
    if ( chdir("/var/run/lookout") != 0 ) {
        syslog( LOG_NOTICE, "run dir missing - cannot run" );
        exit( -1 );
    }

    mkdir( "events", 0755 );
    int sock = socket_open( argv[1], argv[2] );
    loop( sock );

    return 0;
}

/* vim: set autoindent expandtab sw=4 : */
