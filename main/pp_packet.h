#include <stdio.h>

#define PP_SIZE 4

typedef union { uint32_t packed;
                uint8_t byte[4];
                struct { uint8_t command;
                         uint16_t value;
                         uint8_t crc;
                       } p;
} pypi_packet;


bool pp_get_rx_packet( pypi_packet *packet );
void pp_put_tx_packet( pypi_packet packet );