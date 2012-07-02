
//requires uart.h + one_wire_controll.h
#include <stdint.h>


void uart1_puts_hex(int hex_value)
{
    char hex_string[5];
    sprintf(hex_string, "%x", hex_value); //convert to string
    uart1_puts(hex_string); //output
}

void uart1_puts_int(int int_value)
{
    char int_string[6];
    sprintf(int_string,"%d", int_value); //conver to string
    uart1_puts(int_string); //output
}

extern uint64_t ROM_ID[144];
void uart1_puts_romid(int id)
{
    char hex64_string[9];
    uint8_t i,tmp;

    for(i=0;i<8;i++)
    {
        tmp=ROM_ID[id]>>i*8;
        uart1_puts_hex(tmp);
        uart1_putc('-');
    }
}
