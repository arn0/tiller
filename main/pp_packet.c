// Communication between TCP and pypilot control.

/*
The program uses a simple protocol to ensure only
correct data can be received and to ensure that
false/incorrect or random data is very unlikely to
produce motor movement.

The input and output over uart has 4 byte packets

The first byte is the command or register, the next
two bytes is a 16bit value (signed or unsigned)
the last byte is a crc8 of the first 3 bytes

If incoming data has the correct crc for a few frames
the command can be recognized.
*/

#include <stdbool.h>

#include "pp_packet.h"

#include "pp_queue.h"
#include "crc.h"

extern bool rx_sync;

static const char *TAG = "> pp_packet";

bool pp_rx_sync_flag = false;		/* No sync yet */


/* Set the staus of the communication from pypilot remote to local servo */

void pp_set_rx_sync( bool flag ){
	pp_rx_sync_flag = flag;
	rx_sync = flag;
}

/* Get a received packet out of the the the queue */

bool pp_get_rx_packet( pypi_packet *packet ) {
	if( queue_get_rx( packet ) ) {
		if( crc8( packet->byte, 3 ) == packet->byte[3] ) {
			return( true );
		}
		pp_set_rx_sync( false );
	}
	return( false );
}

/* Put a packet to be transmitted into the queue */

void pp_put_tx_packet( pypi_packet packet ) {
	packet.byte[3] = crc8( packet.byte, 3 );
	queue_send_tx( (void *) &packet );
}
