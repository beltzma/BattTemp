
#include <math.h>
#include <stdint.h>
#include <avr/delay.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include "uart.h"
#include "one_wire_controll.h"
#include "defines.h"
extern uint64_t ROM_ID[144];
extern uint8_t ROM_CNT;
extern debug_on;

uint8_t _crc_ibutton_update(uint8_t crc, uint8_t data)
{
    uint8_t i;

    crc = crc ^ data;
    for (i = 0; i < 8; i++)
    {
        if (crc & 0x01)
            crc = (crc >> 1) ^ 0x8C;
        else
            crc >>= 1;
    }

    return crc;
}

int8_t check_crc(uint8_t *data)
{
    uint8_t crc = 0, i;

    for (i = 0; i < 9; i++)
        crc = _crc_ibutton_update(crc, data[i]);
    //uart1_puts_hex(crc);
    return crc; // must be 0
}


int one_wire_setup()
{
uint16_t rec_data; //the received 16bits
uint8_t data_only; //only the lower 8bits, containing data
char hex_string[5]; //debug output
uart1_puts("\tInit started\n\tByte send: ");

uint8_t error = 0;                  //if an error occurs and which
uart_putc(one_command_mode);
_delay_ms(globaldelay);
uart_putc(one_rst);             //Reset
uart1_puts_hex(one_rst);
  _delay_ms(globaldelay);
rec_data = uart_getc();
data_only = rec_data; // cut off the lower 8bit with data

        if(rec_data & UART_NO_DATA)
        {
            uart1_puts("\n\tNO DATA\n");
            return 66;
        }
        else
        {
            uart1_puts("\n\tGot response: ");
            uart1_puts_hex(data_only);
            uart1_putc('\n');
            if((data_only != 0xCD) && (data_only != 0xED))
                {return 1;}                   //Exit with Error 1
        }


uart1_puts("\n\tSet to command mode\n");
uart_putc(one_command_mode);    //set to command mode

uart_putc(pdscr);               // set PDSCR
uart1_puts("\n\tsend: ");
uart1_puts_hex(pdscr);
  _delay_ms(globaldelay);
data_only=uart_getc();
            uart1_puts("\n\trec: ");
            uart1_puts_hex(data_only);
            uart1_putc('\n\n');
if (0x16 != data_only)        //PDSCR correct?
error += 0x01;



uart_putc(w1ld);                // set W1ld
uart1_puts("\n\tsend: ");
uart1_puts_hex(w1ld);
  _delay_ms(globaldelay);
data_only=uart_getc();
            uart1_puts("\n\trec: ");
            uart1_puts_hex(data_only);
            uart1_putc('\n');
if (0x44 != data_only)        // W1LD correct?
error += 0x02;

uart_putc(w0rt);                // set W0RT
uart1_puts("\n\tsend: ");
uart1_puts_hex(w0rt);
  _delay_ms(globaldelay);
data_only=uart_getc();
            uart1_puts("\n\trec: ");
            uart1_puts_hex(data_only);
            uart1_putc('\n');
if (0x5A != data_only)       //WORT correct?
error += 0x04;

uart_putc(rbr);                 // set Baudrate
_delay_ms(globaldelay);
data_only=uart_getc();
if (0x00 != data_only)       //Baudrate correct?
error += 0x08;

uart_putc(one_wire_bit);        // set 1-Wire Bit
_delay_ms(globaldelay);
data_only=uart_getc();
if (0x93 != data_only)        // 1-Wire correct?
error += 0x0F;

uart1_puts("\tOne-Wire init finished - Errors: ");

return error;
}

int one_wire_adress() //only for one device
{
    char package[3]={0xC1,0xE1,0x33};
    uint8_t i;

    uart_putc(0xC1);
    _delay_ms(10);
    uart_putc(0xE1);
    _delay_ms(10);
    uart_putc(0x33);
    _delay_ms(10);
    for(i=0;i<8;i++) uart_getc(); //clear uart buffer
    for(i=0;i<8;i++)
    {
        uart_putc(0xff);
        _delay_ms(10);
        uart1_puts_hex(uart_getc());
        uart1_putc(' ');
    }
    uart_putc(0xE3);
    _delay_ms(10);
    uart_putc(0xC1);
    uart1_putc('\n');
}


int one_wire_search_rom()
{
    uart1_puts("\nSearching ROM ID\n");

    uint8_t data_only,i,rom_cnt=0;
    char path[16]={0x00};
    uint8_t receive[16]={0x00};
    uint8_t col_bit=99,byte_cnt;//collision bit and byte number
    uint8_t last_col=99; //collsion in last run
    for(;;)
    {
        uart_putc(one_command_mode);
        _delay_ms(globaldelay);

        for(i=0;i<8;i++) uart_getc(); //clear uart buffer

        uart_putc(one_rst);             //Reset
        _delay_ms(globaldelay);

        data_only=uart_getc();
        if((data_only != 0xCD) && (data_only != 0xED))
                return 1;              //Exit with Error 1

        uart_putc(one_data_mode);
        uart_putc(one_search_rom);            //(issue the 'ROM search' command)
        uart_putc(one_command_mode);
        uart_putc(one_search_acc_on);
        uart_putc(one_data_mode);
        _delay_ms(globaldelay);
        //algo

        for(i=0;i<10;i++) uart_getc(); //clear uart buffer

            //debug loop
            uart1_puts("\nPath: ");
            for(i=0;i<16;i++)
            {
                uart_putc(path[i]);
                uart1_puts_hex(path[i]);
                uart1_putc('-');
            }
            uart1_puts("\nReceived:");
            //end loop

            _delay_ms(globaldelay);
            for(i=0;i<16;i++){
                receive[i]=uart_getc();
                uart1_puts_hex(receive[i]);
                uart1_putc('-');
                _delay_ms(globaldelay);
            }
            //todo: check for errors
            if(receive[7] & 0xC0)
                uart1_puts("\nScan error, try again\n");
            else
            {

                for(byte_cnt=15;byte_cnt;byte_cnt--)
                {
                    if(receive[byte_cnt] & 0x40 )
                    {
                        col_bit=6;
                        break;
                    }
                    if(receive[byte_cnt] & 0x10 )
                    {
                        col_bit=4;
                        break;
                    }
                    if(receive[byte_cnt] & 0x4 )
                    {
                        col_bit=2;
                        break;
                    }
                    if(receive[byte_cnt] & 0x1 )
                    {
                        col_bit=0;
                        break;
                    }
                }
                //todo store rom id


                uint8_t tmp_cnt=0;
                uint8_t rom_tmp[9]={0x00};
                for(i=0;i<16;i++)
                {

                    if(!(i%2))
                    {
                        rom_tmp[tmp_cnt]|=((receive[i] & 0x02)>>1);
                        rom_tmp[tmp_cnt]|=((receive[i] & 0x08)>>2);
                        rom_tmp[tmp_cnt]|=((receive[i] & 0x20)>>3);
                        rom_tmp[tmp_cnt]|=((receive[i] & 0x80)>>4);
                    }
                    else
                    {
                        rom_tmp[tmp_cnt]|=((receive[i] & 0x02)<<3);
                        rom_tmp[tmp_cnt]|=((receive[i] & 0x08)<<2);
                        rom_tmp[tmp_cnt]|=((receive[i] & 0x20)<<1);
                        rom_tmp[tmp_cnt]|=(receive[i] & 0x80);
                        tmp_cnt++;
                    }

                }
                uint64_t tmp=0;
                ROM_ID[rom_cnt]=0;
                for(i=0;i<8;i++)
                {
                    tmp=rom_tmp[i];
                    ROM_ID[rom_cnt]|=(tmp<<i*8);
                }

                uart1_puts("\n ROM ID ");
                uart1_puts_int(rom_cnt);
                uart1_puts(" : ");
                uart1_puts_romid(rom_cnt);

                if(last_col==col_bit)
                    break;
                 last_col=col_bit;

                uart1_puts("\n collision bit & byte:");
                uart1_puts_int(col_bit);
                uart1_putc(',');
                uart1_puts_int(byte_cnt);
                uart1_putc('\n');
                col_bit++;
                path[byte_cnt]|=(1<<col_bit); //set new path-bit for next search

                for(col_bit+=2;col_bit<6;col_bit+=2)
                    path[byte_cnt]&=~(1<<col_bit); //set all higher bits 0
                for(byte_cnt++;byte_cnt<8;byte_cnt++)
                    path[byte_cnt]=0x00;  //set all higher bits 0


            }
            uart_putc(one_command_mode);
            uart_putc(one_search_acc_off);

            if(rom_cnt<144)
                    rom_cnt++;
            else
                break;
    }

    uart1_puts("\nSearch end.\n");
    uart_putc(one_command_mode);
    uart_putc(one_search_acc_off);
    uart_putc(one_rst);

    rom_cnt++; //start counting at 1 instead of 0
    ROM_CNT=rom_cnt;
    eeprom_update_block((const void*)&ROM_ID,(void*)eeprom_romid,144);
    eeprom_update_byte((uint8_t*)eeprom_romcnt,ROM_CNT);
                        uart1_puts_P("\tupdating EEPROM done.\n");

    return rom_cnt;
}

int one_wire_convert()
{
uint8_t data_only;
uint8_t i;

if(debug_on=='d')
    uart1_puts("\nConverting started \n");

uart_putc(one_command_mode);
_delay_ms(globaldelay);
//uart_putc(0x39);                   //Set pullup dur. = 524ms
uart_putc(0x3B);                   //Set pullup dur. = 1048ms

_delay_ms(globaldelay);
for(i=0;i<8;i++) uart_getc(); //clear uart buffer
uart_putc(one_rst);             //Reset
_delay_ms(globaldelay);
data_only=uart_getc();
if((data_only != 0xCD) && (data_only != 0xED))
        return 1;              //Exit with Error 1

uart_putc(one_data_mode);          //Set Data Mode
_delay_ms(globaldelay);
uart_putc(0xCC);                   // Skip ROM Command (To all Devides)
_delay_ms(globaldelay);
uart_putc(one_command_mode);       //Set Command Mode
_delay_ms(globaldelay);
uart_putc(0xEF);                   // Arm Strong Pullup
_delay_ms(globaldelay);
uart_putc(0xF1);                   //Terminate Pulse
_delay_ms(globaldelay);
uart_putc(one_data_mode);          //Set Data Mode
_delay_ms(globaldelay);

uart_putc(0x44);                   //Convert Temperature
_delay_ms(globaldelay);

for(i=0;i<8;i++) uart_getc(); //clear uart buffer

while(uart_getc()&UART_NO_DATA)//wait for response pulse
    {
        if(debug_on=='d')
            uart1_putc('.');
    }
uart_putc(one_command_mode);        //Set Command mode
_delay_ms(globaldelay);
uart_putc(0xED);                    //Disarm Strong Pullup
_delay_ms(globaldelay);
uart_putc(0xF1);                    //Terminate Pulse
_delay_ms(globaldelay);
for(i=0;i<8;i++) uart_getc(); //clear uart buffer

uart_putc(one_rst);              //Reset
_delay_ms(globaldelay);

data_only=uart_getc();
if((data_only != 0xCD) && (data_only != 0xED))
        return 1;              //Exit with Error 1
if(debug_on=='d')
    uart1_puts("Converting finished \n");

return 0;
}

int one_wire_gettemp(int sensor_num)
{
uint8_t data_only;

if(debug_on=='d')
{
    uart1_puts("\n\nGet Temperature started\nSensor num: ");
    uart1_puts_int(sensor_num);
    uart1_puts("\nROM ID: ");
}



uint16_t temp_c = 0;                 //Temperature in °C
int tempsign = 1;               //"multi" is for Negative temperatures
int i = 0, counter = 0;
uint8_t full_data[8];
uint64_t schieber;
//Get ROM_ID from sensor_num

char rom_id_byte[8];
for(i=0; i<8; i++)
    rom_id_byte[i]=ROM_ID[sensor_num]>>i*8;

for(i=0;i<8;i++) uart_getc(); //clear uart buffer
uart_putc(one_command_mode);
_delay_ms(globaldelay);
uart_putc(one_rst);             //Reset
_delay_ms(globaldelay);
data_only=uart_getc();
if((data_only != 0xCD) && (data_only != 0xED))
        return -99;              //Exit with Error
uart_putc(one_data_mode);
_delay_ms(globaldelay);

uart_putc(0x55);                   //ROM Command
_delay_ms(globaldelay);

for(i=0;i<8;i++)
{
    uart_putc(rom_id_byte[i]);
    if(debug_on=='d')
    {
        uart1_puts_hex(rom_id_byte[i]);
        uart1_putc('-');
    }
    _delay_ms(globaldelay);
}

_delay_ms(globaldelay);
uart_putc(0xBE);                    //Read Scratchpad
_delay_ms(globaldelay);
for(i=0;i<10;i++) uart_getc(); //clear uart buffer

if(debug_on=='d')
    uart1_puts("\nScratchpad: ");

for(counter = 0; counter<9; counter++)      // Get 9 Byte of Scratchpad, Byte 0 LSB - Byte 1 MSB
{
    uart_putc(0xFF);                        //next byte
    _delay_ms(globaldelay);
    full_data[counter] = uart_getc();
    if(debug_on=='d')
    {
        uart1_puts_hex(full_data[counter]);
        uart1_putc(' ');
    }

}

if(check_crc(full_data)) //crc check
{
    if(debug_on=='d')
        uart1_puts("\nCRC check failed!\n");
    return -99;
}

uint16_t temp_hex;
temp_hex=full_data[0]|(full_data[1]<<8); //set right order
temp_c=(temp_hex/16); //divede through 16 to get the °C value in decimal (unsigned!)

if(debug_on=='d')
    uart1_puts("\nGet Temperature finished \n");

return temp_c;
}


int one_wire_resolution() //not used & not working
{
//desc: this function should set the resolution to 9bit (default 12bit) to reduce convertion-time
uint8_t data_only;
uart1_puts("Set resolution started \n");
    //Set resolution to 9 Bit
uart_putc(one_rst);             //Reset
_delay_ms(globaldelay);
data_only=uart_getc();
if((data_only != 0xCD)&&(data_only != 0xED)) //Response is not CD or ED?
        return 1;              //Exit with Error 1

    uart_putc(one_data_mode);          //Set Data Mode
    _delay_ms(globaldelay);
    uart_putc(0xCC);                   //Skip ROM Command (To all Devides)
    _delay_ms(globaldelay);
    uart_putc(0x4E);                   //Write to Scratchpad
    _delay_ms(globaldelay);
    uart_putc(0x00);                   //Write Byte 2
    _delay_ms(globaldelay);
    uart_putc(0x00);                   //Write Byte 3
    _delay_ms(globaldelay);
    uart_putc(0xF1);                   //Write Byte 4 LSB !! oder 1F?!
    _delay_ms(globaldelay);
    uart_putc(0x48);                   //Save Data to EEPROM
    _delay_ms(globaldelay);
    uart_putc(0xB8);                   //Save EEPROM to Scratchpad

uart1_puts("Set resolution finished \n");
    return 0;
}
