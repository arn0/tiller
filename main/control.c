#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_err.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_err.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include <inttypes.h>
#include "esp_chip_info.h"
#include "esp_flash.h"

#include "pp_packet.h"


static const char *TAG = "> control";


enum commands {COMMAND_CODE=0xc7, RESET_CODE=0xe7, MAX_CURRENT_CODE=0x1e, MAX_CONTROLLER_TEMP_CODE=0xa4, MAX_MOTOR_TEMP_CODE=0x5a, RUDDER_RANGE_CODE=0xb6, RUDDER_MIN_CODE=0x2b, RUDDER_MAX_CODE=0x4d, REPROGRAM_CODE=0x19, DISENGAGE_CODE=0x68, MAX_SLEW_CODE=0x71, EEPROM_READ_CODE=0x91, EEPROM_WRITE_CODE=0x53, CLUTCH_PWM_AND_BRAKE_CODE=0x36};

enum results {CURRENT_CODE=0x1c, VOLTAGE_CODE=0xb3, CONTROLLER_TEMP_CODE=0xf9, MOTOR_TEMP_CODE=0x48, RUDDER_SENSE_CODE=0xa7, FLAGS_CODE=0x8f, EEPROM_VALUE_CODE=0x9a};

enum {SYNC=1, OVERTEMP_FAULT=2, OVERCURRENT_FAULT=4, ENGAGED=8, INVALID=16, PORT_PIN_FAULT=32, STARBOARD_PIN_FAULT=64, BADVOLTAGE_FAULT=128, MIN_RUDDER_FAULT=256, MAX_RUDDER_FAULT=512, CURRENT_RANGE=1024, BAD_FUSES=2048, /* PORT_FAULT=4096  STARBOARD_FAULT=8192 */ REBOOTED=32768};



struct struct_control_settings { unsigned int low_current;
                          unsigned int max_current;
                          unsigned int max_controller_temp;
                          unsigned int max_motor_temp;
                          unsigned int rudder_min;
                          unsigned int rudder_max;
                          unsigned int max_slew_speed;
                          unsigned int max_slew_slow;
                          uint8_t clutch_pwm;
                          uint8_t clutch_start_time;
                          uint8_t use_brake;
                          uint8_t brake_on;
};


uint16_t flags = 0;
struct struct_control_settings control_settings;
uint8_t out_sync_b = 0, out_sync_pos = 0;
uint8_t low_current = 1;



// command is from 0 to 2000 with 1000 being neutral
uint16_t lastpos = 1000;
void position(uint16_t value)
{
    lastpos = value;
    if(value > 1100) {
        if(value > 1900) {
        }
    } else if(value < 900) {
        if(value < 100) {
        }
    }
}

uint16_t command_value = 1000; // range is 0 to 2000 for forward and backward

void stop( void ){
    control_settings.brake_on = 0;
    position(1000); // 1000 is stopped
    command_value = 1000;
}

void stop_port()
{
    if(lastpos > 1000)
       stop();
}

void stop_starboard()
{
    if(lastpos < 1000)
       stop();
}

void engage()
{
    if(flags & ENGAGED)
        return; // already engaged

    // do some pwm

    position(1000);

    // activate clutch
    //clutch_start_time = 20;
    //switch led on
    flags |= ENGAGED;
}

void process_packet( pypi_packet packet )
{
    flags |= SYNC;
    uint16_t value;

    value = pp_val_get( &packet );

    switch( packet.p.command ) {
        case REPROGRAM_CODE:
            ESP_LOGI( TAG, "Received REPROGRAM_CODE, value %d", value );
            break;
 
        case RESET_CODE:
            ESP_LOGI( TAG, "Received RESET_CODE, value %d", value );
                                                                // reset overcurrent flag
            flags &= ~OVERCURRENT_FAULT;
            break;
        
        case COMMAND_CODE:
            ESP_LOGI( TAG, "Received COMMAND_CODE, value %d", value );
            //  timeout = 0;
            //  if(serialin < 12)
            //      serialin+=4; // output at input rate
            if( value > 2000 );
                                                                // unused range, invalid!!!
            else if(flags & (OVERTEMP_FAULT | OVERCURRENT_FAULT | BADVOLTAGE_FAULT));
                                                                // no command because of overtemp or overcurrent or badvoltage
            else if((flags & (PORT_PIN_FAULT | MAX_RUDDER_FAULT)) && value > 1000)
                stop();                                         // no forward command if port fault
            else if((flags & (STARBOARD_PIN_FAULT | MIN_RUDDER_FAULT)) && value < 1000)
                stop();                                         // no starboard command if port fault
            else {
                control_settings.brake_on = control_settings.use_brake;
                command_value = value;
                engage();
            }
            break;

        case MAX_CURRENT_CODE:                                  // current in units of 10mA
            ESP_LOGI( TAG, "Received MAX_CURRENT_CODE, value %d", value );
            unsigned int max_max_current = control_settings.low_current ? 2000 : 5000;
            if( value > max_max_current)               // maximum is 20 or 50 amps
                value = max_max_current;
            control_settings.max_current =  value;
            break;
    
        case MAX_CONTROLLER_TEMP_CODE:
            ESP_LOGI( TAG, "Received MAX_CONTROLLER_TEMP_CODE, value %d", value );
            if( value > 10000 )            // maximum is 100C
                value = 10000;
            control_settings.max_controller_temp = value;
            break;

        case MAX_MOTOR_TEMP_CODE:
            ESP_LOGI( TAG, "Received MAX_MOTOR_TEMP_CODE, value %d", value );
            if( value > 10000)                       // maximum is 100C
                value = 10000;
            control_settings.max_motor_temp = value;
            break;

        case RUDDER_MIN_CODE:
            ESP_LOGI( TAG, "Received RUDDER_MIN_CODE, value %d", value );
            control_settings.rudder_min = value;
            break;

        case RUDDER_MAX_CODE:
            ESP_LOGI( TAG, "Received RUDDER_MAX_CODE, value %d", value );
            control_settings.rudder_max = value;
            break;

        case DISENGAGE_CODE:
            ESP_LOGI( TAG, "Received DISENGAGE_CODE, value %d", value );
            // if(serialin < 12)
                // serialin+=4; // output at input rate
            //disengage();
            break;

        case MAX_SLEW_CODE:
            ESP_LOGI( TAG, "Received MAX_SLEW_CODE, value %d", value );
            control_settings.max_slew_speed = packet.byte[1];
            control_settings.max_slew_slow = packet.byte[2];

            // if set at the end of range (up to 255)  no slew limit
            if( control_settings.max_slew_speed > 250 )
                control_settings.max_slew_speed = 250;
            if( control_settings.max_slew_slow > 250 )
                control_settings.max_slew_slow = 250;

            // must have some slew
            if( control_settings.max_slew_speed < 1 )
                control_settings.max_slew_speed = 1;
            if( control_settings.max_slew_slow < 1 )
                control_settings.max_slew_slow = 1;
            break;

        case EEPROM_READ_CODE:
            ESP_LOGI( TAG, "Received EEPROM_READ_CODE, value %d", value );
            break;

        case EEPROM_WRITE_CODE:
            ESP_LOGI( TAG, "Received EEPROM_WRITE_CODE, value %d", value );
            break;

        case CLUTCH_PWM_AND_BRAKE_CODE:
            ESP_LOGI( TAG, "Received CLUTCH_PWM_AND_BRAKE_CODE, value %d", value );
            control_settings.clutch_pwm = packet.byte[1];
            if( control_settings.clutch_pwm < 30 )
                control_settings.clutch_pwm = 30;
            else if( control_settings.clutch_pwm > 250 )
                control_settings.clutch_pwm = 255;
            control_settings.use_brake = packet.byte[2];
            break;
        default:
            ESP_LOGI( TAG, "Received unknown comman, command %d, value %d", packet.p.command, value );
    }
}

void send_packet()
{
        pypi_packet packet;
        uint16_t value;


        //  flags C R V C R ct C R mt flags  C  R  V  C  R EE  C  R mct flags  C  R  V  C  R  EE  C  R rr flags  C  R  V  C  R EE  C  R cc  C  R vc
        //  0     1 2 3 4 5  6 7 8  9    10 11 12 13 14 15 16 17 18  19    20 21 22 23 24 25  26 27 28 29    30 31 32 33 34 35 36 37 38 39 40 41 42
        
        // fla FLAGS_CODE flags
        // C   CURRENT_CODE
        // R   RUDDER_SENSE_CODE
        // V   VOLTAGE_CODE
        // ct  CONTROLLER_TEMP_CODE
        // mt  MOTOR_TEMP_CODE
        // EE  EEPROM_VALUE_CODE
        // mct -
        // rr  -
        // cc  -
        // vc  -

        //CURRENT_CODE=0x1c
        //VOLTAGE_CODE=0xb3
        //CONTROLLER_TEMP_CODE=0xf9
        //MOTOR_TEMP_CODE=0x48
        //RUDDER_SENSE_CODE=0xa7
        //FLAGS_CODE=0x8f
        //EEPROM_VALUE_CODE=0x9a

        
    switch(out_sync_pos++) {
    case 0: case 10: case 20: case 30:
        if(!low_current)
            flags |= CURRENT_RANGE;

        value = flags;
        flags &= ~REBOOTED;
        packet.p.command = FLAGS_CODE;
        ESP_LOGI(TAG, "Send FLAGS_CODE, value %d:", value);
        break;
    case 1: case 4: case 7: case 11: case 14: case 17: case 21: case 24: case 27: case 31: case 34: case 37: case 40:
        value = 1000;
        packet.p.command = CURRENT_CODE;
        ESP_LOGI(TAG, "Send CURRENT_CODE, value %d:", value);
        break;
    case 2: case 5: case 8: case 12: case 15: case 18: case 22: case 25: case 28: case 32: case 35: case 38: case 41:
        value = 1000;
        packet.p.command = RUDDER_SENSE_CODE;
        ESP_LOGI(TAG, "Send RUDDER_SENSE_CODE, value %d:", value);
        break;
    case 3: case 13: case 23: case 33:
        value = 1200;
        packet.p.command = VOLTAGE_CODE;
        ESP_LOGI(TAG, "Send VOLTAGE_CODE, value %d:", value);
        break;
    case 6:
        value = 6600;
        packet.p.command = CONTROLLER_TEMP_CODE;
        ESP_LOGI(TAG, "Send CONTROLLER_TEMP_CODE, value %d:", value);
        break;
    case 9:
        value = 4200;        // 1200 = 12C
        packet.p.command = MOTOR_TEMP_CODE;
        ESP_LOGI(TAG, "Send MOTOR_TEMP_CODE, value %d:", value);
        break;
    case 16: case 26: case 36: /* eeprom reads */
        return;
    default:
        if( out_sync_pos > 63 ) {
            out_sync_pos = 0;
        }
        return;
    }
    pp_val_set( value, &packet );
    pp_put_tx_packet( packet );
}

void control_loop(void *p) {
    control_settings.low_current = 2000;
    control_settings.clutch_pwm = 192;
    control_settings.use_brake = 0;
    control_settings.brake_on = 0;
    pypi_packet packet;

    while( true ) {
        if( pp_get_rx_packet( &packet ) ) {
            process_packet( packet );
            send_packet();
            vTaskDelay( 2 / portTICK_PERIOD_MS );
        } else {
            vTaskDelay( 20 / portTICK_PERIOD_MS );
        }
    }
    vTaskDelete(NULL);
}
/* Estimate timing based on default serial speed for Arduino servo
 * 38400 baud = 38400 bit/s
 * 8 databits + 1 startbit + 1 stopbit = 10 bits
 * 38400 / 10 = 3840 byte/s
 * 
 * uint8_t out_sync_pos = 0; counts continuesly, so 256 byte slots
 * of wich 43 (codes) * 4 (bytes / code) = 172 are actually used
 * 
 * with 3840 bit/s we can do max 3840 / 256 = 15 loop/s
 * one loop can be minimal 1 /15 = 66,6 ms
 * 
 * this loop sends 4 bytes, the original did 1 byte per loop
 * 
 * original 43 * 4 = 172 data 256 - 172 = 84 wait
 * this     43 packets data   256 - 43  = 213 wait
 * 
 * with 256 as max for out_sync_pos this does not work
 * 
 * original 84 wait = 84 /4 = 21 packets
 * so this loop should count to 43 + 21 = 64 (which is 256 / 4 and makes sense)
 */