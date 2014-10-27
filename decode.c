
#include <stdlib.h>
#include <stdio.h>

#include "lookout.pb-c.h"


static size_t
read_buffer( unsigned max_length, uint8_t *out ) {
    size_t nread;
    size_t cur_len = 0;
    uint8_t c;

    while ( (nread=fread(out + cur_len, 1, max_length - cur_len, stdin)) != 0 ) {
        cur_len += nread;
        if ( cur_len == max_length ) {
            fprintf(stderr, "max message length exceeded\n");
            exit(1);
        }
    }

    return cur_len;
}


int
main( int argc, char ** argv ) {
    Lookout__BackendCodingQuestions__Q1__IpEvent *message;

    uint8_t buffer[1024];
    size_t length = read_buffer( sizeof(buffer), buffer );
    message = lookout__backend_coding_questions__q1__ip_event__unpack( NULL, length, buffer );

    if ( message == NULL ) {
        fprintf( stderr, "cannot unpack message\n" );
        exit( 1 );
    }

    fprintf( stderr, "decoding <%s> <0x%0llx>\n", message->app_sha256, message->ip );

    return 0;
}

/*
package lookout.backend_coding_questions.q1;

message IpEvent {
  required string app_sha256 = 1;
  required int64 ip = 2;
}
 */

/* vim: set autoindent expandtab sw=4: */
