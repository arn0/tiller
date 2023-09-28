#include <stdio.h>

#define PP_SIZE 4

typedef union { uint32_t packed;
                uint8_t byte[4];
                struct { uint8_t command;
                         uint16_t do_not_use;
                         uint8_t crc;
                       } p;
} pypi_packet;

void pp_set_rx_sync( bool flag );
extern bool pp_rx_sync_flag;

void pp_decode( char* buffer, int len );

void pp_val_set( uint16_t value, pypi_packet *packet );
uint16_t pp_val_get( pypi_packet *packet );

bool pp_get_rx_packet( pypi_packet *packet );
void pp_put_tx_packet( pypi_packet packet );