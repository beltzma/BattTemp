

#define one_rst 0xC5                //Reset with flex Speed
#define one_data_mode 0xE1            //In Datamode versetzten
#define one_command_mode 0xE3        //In Command Mode versetzen
#define one_read_rom_command 0x33       //??
#define one_rom_id 0xFF
#define one_search_rom 0xF0   // send search rom
#define one_search_acc_on 0xB1
#define one_search_acc_off 0xA1

#define pdscr 0x17          // pdscrc auf 1,37 V/us setzten
#define w1ld 0x45           // w1ld auf 10 us setzten
#define w0rt 0x5B           // w0rt auf 8 us setzten
#define rbr 0x0F             // Baudrate auf 9600 setzten
#define one_wire_bit 0x91    // 1-Wire Bit setzten

// So sollte es vielleicht sein:
// Write:    0 Parameter    Wert   1
// Response: 0 Paramter     Wert   0

//Bits: 7     6-4       3-1           0
//      0   Parameter   Wert   0=schreiben 1= lesen




/*  1 Wire Controller Setup
 *  Configurates the DC2480B
 *  Parameters: Void
 *  Return: - 0 Success
 *          - 1 Error
 */
int one_wire_setup();



/*  1 Wire Adress Search
 *  Searches the One-Wire-Bus for ROM ID's and writes them into the ROM_ID[]
 *  Parameters: Void
 *  Return: Number of found ROM ID's
 */
int one_wire_adress();



/*  Temperature-Sensor Converting Signal
 *  Sends the Converting Signal to the Sensors.
 *  Parameters: Void
 *  Return: - 0 Success
 *          - 1 Error
 */
int one_wire_convert();


/*  Temperature of one Sensor
 *  Get the checked Temperature of one Sensor
 *  Parameters: Sensor_ID (probably 0-144)
 *  Return: Temperature of the Sensor
 */
int one_wire_gettemp(int sensor_ID);


/*  Set the resolution of the Temp-Sensors
 *  Set resolution to 9 Bit for fast conversion and saves it to the EEPROM
 *  This needs to be done everytime a new Sensor is added
 *  Parameters: Void
 *  Return: - 0 Success
 *          - 1 Error
 */
int one_wire_resolution();
