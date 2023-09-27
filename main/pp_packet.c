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
#include "esp_log.h"

#include "pp_packet.h"

#include "pp_queue.h"
#include "crc.h"

extern bool rx_sync;

static const char *TAG = "> pp_packet";

bool pp_rx_sync_flag = false;		/* No sync yet */


/* Set the staus of the communication from pypilot remote to local servo */

void pp_set_rx_sync( bool flag ){
	pp_rx_sync_flag = flag;
}

/* Get a received packet out of the the the queue */

bool pp_get_rx_packet( pypi_packet *packet ) {
	if( queue_get_rx( packet ) ) {
		if( crc8( packet->byte, 3 ) == packet->byte[3] ) {
			//ESP_LOGI(TAG, "valid packet from queue: %hx %hx %hx %hx", packet->byte[0], packet->byte[1], packet->byte[2], packet->byte[3] );
			return( true );
		}
		pp_set_rx_sync( false );
	}
	return( false );
}

/* Put a packet to be transmitted into the queue */

void pp_put_tx_packet( pypi_packet packet ) {
	uint8_t c;
	c = packet.byte[1];
	packet.byte[1] = packet.byte[2];
	packet.byte[2] = c;
	packet.byte[3] = crc8( packet.byte, 3 );
	queue_put_tx( (void *) &packet );
}


char temp_buffer[128*2];
int temp = 0;
char* p, * e;

void shift_out( char* buffer, int shift, int length ) {
	for ( int i=0; i<length; i++ ) {
		buffer[i] = buffer[i+shift];
	}
}

void pp_decode( char* buffer, int len ) {
	uint8_t c;
	//ESP_LOGI(TAG, "start, len = %d", len);
	
	if( len + temp > sizeof(temp_buffer) ){				// test if we have room for the new bytes
		ESP_LOGI(TAG, "shift, temp = %d", temp);
		shift_out( temp_buffer, temp + len - sizeof(temp_buffer), sizeof(temp_buffer) );		// lose the first received bytes, test for calculations!
		temp = sizeof(temp_buffer);
	}
	// copy received bytes in our buffer
	memcpy( temp_buffer + temp, buffer, len );
	temp += len;
	if( temp < 4 )													// do we have enough for a packet?
		return;														// no
	
	p = temp_buffer;
	e = p + temp;

	while ( crc8( (uint8_t*) p, 3 ) != p[3] ) {			//find valid packet
		p++;
		if( p > e - 3 ) {											// out of received bytes? need 4 for a packet
			ESP_LOGE(TAG, "no sync");
			return;													// wait for more
		}
	}
	//ESP_LOGI(TAG, "sync");
	c = p[1];
	p[1] = p[2];
	p[2] = c;

	if ( queue_put_rx( p ) ) {								// packet
		p += 4;
	}
	while( p < e ) {												// test for more packets
		if( crc8( (uint8_t*) p, 3 ) == p[3] ) {
			//ESP_LOGI(TAG, "sync");
			c = p[1];
			p[1] = p[2];
			p[2] = c;
			if ( queue_put_rx( p ) ) {
				p += 4;
			} else {
				return;
			}
		}
	}
	shift_out( temp_buffer, p - temp_buffer, e - p );	// shift buffer
	temp = e - p;
}
