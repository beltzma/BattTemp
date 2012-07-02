/*
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <avr/eeprom.h>
#include <util/delay.h>

#include "uart.h"
#include "one_wire_controll.h"
#include "defines.h"


uint64_t ROM_ID[144];           // 144 Sensor IDs, each 64bit (RAM: 1152byte)
uint8_t ROM_CNT = 0;

#include "debug.h"
uint8_t debug_on=0;

uint8_t shutdown_temp = 60;     // shutdown temperature in °C
uint8_t warning_temp = 50;      // warning temperature in °C

//uint8_t debug_mode;

uint8_t error_code = 0;              //global errorcodes
/*
*******ERROR CODES:*******
10 - setup/init error
    15 - no sensor

20 - 1-Wire error
    25 - no sensor
    26 - more than 10% of sensors offline

30 - CAN-Error
40 - UART error
**************************

*******LED Status******
green:          measurement running
green&red:      debugmode (measurement stopped)
red:            overheat -> shutdown
red blinking:   error    -> shutdown

see debug-uart for live details
***********************

*/

void shutdown_power()
{
    uart1_puts_P("\n***POWER OFF***\n");
    PORTD &= ~(1<<PD6); //set Green off
    PORTD |= (1<<PD7); //set Red LED on

    PORTC |= (1<<PC2); // PIN PC2 auf "1" setzten => Relais schaltet durch => Powercut

}

void throw_error(uint8_t error_code)
{
    if(error_code<10)
    {
        uart1_puts_P("\n***NO ERROR***\n");
    }
    else
    {
        shutdown_power();

        char cerr[3];
        sprintf(cerr,"%d", error_code); //convert int to chararray

        uart1_puts_P("\n***PROGRAM STOPPED***\n");

        if(error_code>=10&&error_code<20){
            uart1_puts_P("\n***ERROR: Setup ");
            uart1_puts(cerr);
            uart1_puts_P("***\n");
        }
        if(error_code>=20&&error_code<30){
            uart1_puts_P("\n***ERROR: 1-Wire ");
            uart1_puts(cerr);
            uart1_puts_P("***\n");
        }
        if(error_code>=30&&error_code<40){
            uart1_puts_P("\n***ERROR: CAN ");
            uart1_puts(cerr);
            uart1_puts_P("***\n");
        }
        if(error_code>=40&&error_code<50){
            uart1_puts_P("\n***ERROR: UART ");
            uart1_puts(cerr);
            uart1_puts_P("***\n");
        }

        if(error_code>50){
            uart1_puts_P("\n***ERROR: UNKOWN ");
            uart1_puts(cerr);
            uart1_puts_P("***\n");
        }
        while(1)
        {
            PORTD |= (1<<PD7); //set Red LED on
            _delay_ms(200);
            PORTD &= ~(1<<PD7); //set Red LED off
            _delay_ms(200);
        }
    }// end else
}

int setup(void)
{
    uint8_t ret;
    DDRD |=  (1<<PD6); //init Green LED
	DDRD |=  (1<<PD7); //init Red LED
	DDRC |=  (1<<PC2); //init relay output

	PORTD &= ~(1<<PD6); //set Green off
    PORTD |= (1<<PD7); //set Red LED on

    uart_init( UART_BAUD_SELECT(UART_BAUD_RATE_ONE_WIRE,F_CPU) ); //initialisiere hardware uart 0 with baurate 9600 - [One Wire]
    uart1_init( UART_BAUD_SELECT(UART_BAUD_RATE_RS232,F_CPU) ); //initialisiere hardware uart 1 with baurate 57600 - [Debug]

    sei();

    _delay_ms(globaldelay); // simulate setup time
    uart1_puts_P("\n\nSetup started...\n"); //Debug output

    uart1_puts_P("\treading eeprom ...\n");
    eeprom_read_block((void*)&ROM_ID,(const void*)eeprom_romid,144);
    ROM_CNT=eeprom_read_byte((const uint8_t*)eeprom_romcnt);
    warning_temp=eeprom_read_byte((const uint8_t*)eeprom_warntemp);
    shutdown_temp=eeprom_read_byte((const uint8_t*)eeprom_shuttemp);

    uart1_puts_P("\tinit One-Wire ...\n");
    ret=one_wire_setup();
    if(ret)
        throw_error(15);

    uart1_puts_P("\nSetup done.\n\n*Press 'd' for debug-mode*\n\n"); //Debug output
	PORTD |= (1<<PD6); //Green LED on
    PORTD &= ~(1<<PD7); //Red LED off

    return 0;
}


void debug() //debug menu & functions
{
        static uint8_t debug_mode;
        static uint8_t page_drawn;

        //used in case 'e' error test
        static uint8_t num_char;
        static uint8_t input_errorcode;
        static char hex_string[3];
        //////

        char ch = 0; //debug input char

        if(debug_mode==0)
        {
            debug_mode='m';
            PORTD |= (1<<PD7); //set Red LED on

            uart1_puts_P("\n\n----DEBUG-MODE----\n");
            uart1_puts_P("\tt - change temperatures\n");
            uart1_puts_P("\tw - one-wire\n");
            uart1_puts_P("\te - error test\n");
            uart1_puts_P("\td - direct one-wire access\n");
            uart1_puts_P("\tp - eeprom access\n");
            uart1_puts_P("q - quit debug-mode\n");
            uart1_puts_P("------------------\n\n");


        }

        ch = uart1_getc();         //get char from uart
        if ( ch & UART_NO_DATA )   //nothing to get
        {

        }
        else
        {
            /*
             * new data available from UART
             * check for Frame or Overrun error
             */
            if ( ch & UART_FRAME_ERROR )
            {
                /* Framing Error detected, i.e no stop bit detected */
                uart1_puts_P("\tUART Frame Error: \n");
            }
            if ( ch & UART_OVERRUN_ERROR )
            {
                /*
                 * Overrun, a character already present in the UART UDR register was
                 * not read by the interrupt handler before the next character arrived,
                 * one or more received characters have been dropped
                 */
                uart_puts_P("\tUART Overrun Error: \n");
            }
            if ( ch & UART_BUFFER_OVERFLOW )
            {
                /*
                 * We are not reading the receive buffer fast enough,
                 * one or more received character have been dropped
                 */
                uart_puts_P("\tBuffer overflow error: \n");
            }

            char cint[5]; //temporary var for conversion
            //uart1_putc(ch);

            switch(debug_mode)
            {

            case 'm': //options in main menu
                    if(ch=='t') //press t to change temps
                    {
                        debug_mode='t';
                        page_drawn=0;
                    }

                    if(ch=='e') //press e to test errors
                    {
                        debug_mode='e';
                        page_drawn=0;
                    }
                     if(ch=='w') //press w to get in one-wire submenu
                    {
                        debug_mode='w';
                        page_drawn=0;
                    }
                     if(ch=='d') //press d to access one-wire
                    {
                        debug_mode='d';
                        page_drawn=0;
                    }
                     if(ch=='p') //press p to access eeprom
                    {
                        debug_mode='p';
                        page_drawn=0;
                    }
                     if(ch=='q') //press q to quit debug-mode
                    {
                        PORTD &= ~(1<<PD7); //set Red LED off
                        debug_mode=0;
                        debug_on=0;
                    }
            break;

            case 't':           //enter temp change-mode

                    if(ch=='+')
                    {
                        shutdown_temp++;
                        page_drawn=0;
                    }
                    if(ch=='-')
                    {
                        shutdown_temp--;
                        page_drawn=0;
                    }

                    warning_temp=shutdown_temp-10;

                    if(!page_drawn)
                    {
                        uart1_puts_P("----Change Temps----\n");
                        uart1_puts_P("\tShutdown Temp: ");
                        sprintf(cint,"%d", shutdown_temp); //convert int to chararray
                        uart1_puts(cint);
                        uart1_puts("\n");

                        uart1_puts_P("\tWarning Temp: ");
                        sprintf(cint,"%d", warning_temp); //convert int to chararray
                        uart1_puts(cint);
                        uart1_puts_P("\n\nPress + or -\n");
                        uart1_puts_P("\nb - back & save\n");
                        uart1_puts_P("---------------------\n\n");
                        page_drawn=1;
                    }
                    if(ch=='b') //save and back to main menu
                    {
                        eeprom_update_byte((uint8_t*)eeprom_warntemp,warning_temp);
                        eeprom_update_byte((uint8_t*)eeprom_shuttemp,shutdown_temp);

                        debug_mode=0;
                        page_drawn=0;
                    }
            break;


            /*
            case 'u':
                    if(!page_drawn)
                    {
                        uart1_puts_P("----UPDATE ROM----\n");
                        uart1_puts_P("\tNot implemented yet\n");
                        uart1_puts_P("\nb - back\n");
                        uart1_puts_P("------------------\n\n");
                        page_drawn=1;
                    }

                    if(ch=='b') //back to main menu
                    {
                        debug_mode=0;
                        page_drawn=0;
                        break;
                    }
            break;
            */

            case 'e':

                    if(!page_drawn)
                    {

                        uart1_puts_P("----ERROR TEST----\n");
                        uart1_puts_P("\tType errorcode (2 digits)\n");
                        uart1_puts_P("\nb - back\n");
                        uart1_puts_P("------------------\n\n");
                        input_errorcode=0; //reset
                        num_char=0; //reset
                        page_drawn=1;
                    }
                    if(ch>='0'&&ch<='9'&&num_char<2)//only numbers up to 2 digits
                    {
                        uart1_putc(ch);
                        if(num_char==0)
                            input_errorcode+=(ch-48)*10;
                        else
                            input_errorcode+=(ch-48);
                        num_char++;
                    }
                    if(ch==13&&num_char==2)//press enter after typing 2 digits
                    {
                        throw_error(input_errorcode);
                        page_drawn=0;
                    }

                    if(ch=='b') //back to main menu
                    {
                        debug_mode=0;
                        page_drawn=0;
                        break;
                    }
            break;

            case 'w':
                    if(!page_drawn)
                    {
                        uart1_puts_P("----ONE-WIRE----\n");
                        uart1_puts_P("\t1 - Setup\n");
                        uart1_puts_P("\t2 - Set resolution\n");
                        uart1_puts_P("\t3 - Search ROM IDs\n");
                        uart1_puts_P("\t4 - Convert\n");
                        uart1_puts_P("\t5 - Get all temps in °C\n");
                        uart1_puts_P("\nb - back\n");
                        uart1_puts_P("-----------------\n\n");
                        num_char=0; //reset
                        page_drawn=1;
                    }

                    if(ch=='1')
                        uart1_puts_hex(one_wire_setup());

                    if(ch=='2')
                        uart1_puts_hex(one_wire_resolution());

                    if(ch=='3'){
                        uart1_puts_int(one_wire_search_rom());
                    }
                    if(ch=='4')
                        uart1_puts_hex(one_wire_convert());

                    if(ch=='5')
                    {
                        uint8_t i;
                        for(i=0;i<ROM_CNT;i++)
                        {
                            uart1_puts_int(one_wire_gettemp(i));
                            uart1_puts(" °C\n");
                        }
                    }

                    if(ch=='b') //back to main menu
                    {
                        debug_mode=0;
                        page_drawn=0;
                        break;
                    }
            break;

            case 'd':

                    if(!page_drawn)
                    {

                        uart1_puts_P("----DIRECT ONE WIRE----\n");
                        uart1_puts_P("\tType hexcode to send (2 digits)\n");
                        uart1_puts_P("\nq - quit\n");
                        uart1_puts_P("----------------------\n\n");
                        input_errorcode=0; //reset
                        num_char=0; //reset
                        page_drawn=1;
                    }
                    if(((ch>='0'&&ch<='9')||(ch>='a'&&ch<='f'))&&num_char<2)//only hex-numbers up to 2 digits
                    {
                        uart1_putc(ch);
                        if(ch>='0'&&ch<='9')
                        {
                            if(num_char==0)
                            input_errorcode+=(ch-48)*16;
                            else
                            input_errorcode+=(ch-48);
                        }else{
                            if(num_char==0)
                            input_errorcode+=(ch-87)*16;
                            else
                            input_errorcode+=(ch-87);

                        }

                        num_char++;
                    }
                    if(ch==13&&num_char==2)//press enter after typing 2 digits
                    {
                        uint8_t i;
                        for(i=0;i<8;i++) uart_getc(); //clear uart buffer
                        uart_putc(input_errorcode);
                        _delay_ms(globaldelay);
                        input_errorcode=uart_getc();
                        uart1_puts(" response: ");
                        uart1_puts_hex(input_errorcode);
                        uart1_puts("\n\n");
                        page_drawn=0;
                    }

                    if(ch=='q') //back to main menu
                    {
                        debug_mode=0;
                        page_drawn=0;
                        break;
                    }

            break;

            case 'p':
                    if(!page_drawn)
                    {

                        uart1_puts_P("----EEPROM ACCESS----\n");
                        uart1_puts_P("\tr - read content\n");
                        uart1_puts_P("\tu - update content\n");
                        uart1_puts_P("\nb - back\n");
                        uart1_puts_P("----------------------\n\n");
                        input_errorcode=0; //reset
                        num_char=0; //reset
                        page_drawn=1;
                    }
                    if(ch=='r')
                    {
                        eeprom_read_block((void*)&ROM_ID,(const void*)eeprom_romid,144);
                        ROM_CNT=eeprom_read_byte((const uint8_t*)eeprom_romcnt);
                        warning_temp=eeprom_read_byte((const uint8_t*)eeprom_warntemp);
                        shutdown_temp=eeprom_read_byte((const uint8_t*)eeprom_shuttemp);
                        uart1_puts_P("\treading done.\nROM_CNT: ");
                        uart1_puts_int(ROM_CNT);
                        uart1_puts_P("\t\nwarning_temp: ");
                        uart1_puts_int(warning_temp);
                        uart1_puts_P("\t\nshutdown_temp: ");
                        uart1_puts_int(shutdown_temp);

                    }
                    if(ch=='u')
                    {
                        eeprom_update_block((const void*)&ROM_ID,(void*)eeprom_romid,144);
                        eeprom_update_byte((uint8_t*)eeprom_romcnt,ROM_CNT);
                        eeprom_update_byte((uint8_t*)eeprom_warntemp,warning_temp);
                        eeprom_update_byte((uint8_t*)eeprom_shuttemp,shutdown_temp);
                        uart1_puts_P("\tupdating done.\nROM_CNT: ");
                        uart1_puts_int(ROM_CNT);
                        uart1_puts_P("\t\nwarning_temp: ");
                        uart1_puts_int(warning_temp);
                        uart1_puts_P("\t\nshutdown_temp: ");
                        uart1_puts_int(shutdown_temp);
                    }
                    if(ch=='b') //back to main menu
                    {
                        debug_mode=0;
                        page_drawn=0;
                        break;
                    }
            break;

            }

        }
}


void get_all_temps()
{
	uint8_t temp_mid=0,temp_max =0,tempsend[2]={0};
	uint8_t sens_count,sens_temp[144]={0},invalid_count=0;
	int tmp=0;
    uart1_puts("\n\nTemps: ");
		for(sens_count=0;sens_count<ROM_CNT;sens_count++)								//Temperaturabfrage der Sensoren startet
		{

            if((tmp=one_wire_gettemp(sens_count))!=-99) //no error
            {
                sens_temp[sens_count] = tmp;
                uart1_puts_int(sens_temp[sens_count]);
                temp_mid +=sens_temp[sens_count];							//Mittelwert berechnen
            }
            else
            {
                uart1_puts("x,");
                invalid_count++;
                continue;
            }
            uart1_putc(',');
			if(sens_temp[sens_count]>temp_max)									//Maximaltemperatur überprüfen
			{
				if(sens_temp[sens_count]>=shutdown_temp)
				{

					uart1_puts("\nMaximal Temperatur überschritten\n");

					shutdown_power();											//Maximaletemperaturüberschreitung
																				//Funktionsaufruf shutdown

                    for(;;){} // stop program
				}
				temp_max = sens_temp[sens_count];

			}

		};

		temp_mid = temp_mid/(sens_count-invalid_count);

		tempsend[0]=temp_mid;
		tempsend[1]=temp_max;
            uart1_puts("\navg: ");
            uart1_puts_int(temp_mid);
            uart1_puts("\nmax: ");
            uart1_puts_int(temp_max);

        if(invalid_count>(sens_count*0.1)) //more than 10% sensors offline
            throw_error(26);
}


void main(void)
{
    uint8_t shutdown_timer=5;

    setup();

    _delay_ms(1000); //time to get in debug mode
    debug_on=uart1_getc();

    for(;;)
    {

        if(debug_on=='d')
            debug();
        else
        {
            if(one_wire_convert()==0)
            {
                get_all_temps();
                shutdown_timer=5;
            }
            else
            {
                uart1_puts_P("\nno sensor! shutdown: ");
                uart1_puts_int(shutdown_timer);
                shutdown_timer--;
                _delay_ms(800);
            }

            if(shutdown_timer==0)
                throw_error(25);


            _delay_ms(200);
            debug_on=uart1_getc();
        }

    };
   //never reached
}
