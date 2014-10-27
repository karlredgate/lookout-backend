
#include <stdlib.h>
#include <stdio.h>

#include "lookout.pb-c.h"

int main( int argc, char ** argv ) {
    Lookout__BackendCodingQuestions__Q1__IpEvent message = LOOKOUT__BACKEND_CODING_QUESTIONS__Q1__IP_EVENT__INIT;

    char *buffer;
    unsigned int length;

    message.app_sha256 = argv[1];
    message.ip = atoll( argv[2] );

    fprintf( stderr, "encoding <%s> <0x%0llx>\n", message.app_sha256, message.ip );

    length = lookout__backend_coding_questions__q1__ip_event__get_packed_size( &message );
    buffer = malloc( length );
    lookout__backend_coding_questions__q1__ip_event__pack( &message, buffer );

    fwrite( buffer, length, 1, stdout );

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
