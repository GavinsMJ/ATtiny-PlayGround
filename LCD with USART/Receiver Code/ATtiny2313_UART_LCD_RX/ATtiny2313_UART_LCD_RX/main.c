/*
 * ATtiny2313UART_RX.c
 *  
 
 * Author : Gavins Maragia 
 
 *	RX: pin 2 (PD0)
 *	TX: pin 3 (PD1)
 *  PortB connected to LCD 
 * 
 *    At startup, some data is written to EEPROM
 *  
 *    when a "PRINT" command is received via UART, the program checks for parity,
 *    if its okay, an LED at pin  PB0 is turned on and the controller retrieves
 *    the data from the EEPROM and prints it on the LCD.
 *    the LCD displays the data for a second after the button is pressed then clears it
 */ 

#define F_CPU 16000000UL    // 16 MHz clock freq

#include <avr/io.h>
#include <util/delay.h> 

// Baud rate set at 9600 -- 0.2% error at 16000000
#define BAUDRATE 9600        // 9600 baud rate
#define BRC ((F_CPU/16UL/BAUDRATE) -1 ) // calculates to 103 for  U2X = 0

//LCD connection
#define LCD PORTB // 8 Bit data pins
#define RS  PD3   //Register select (RS)
#define RW  PD4   //Read/Write Pin
#define EN1 PD5   //Enable for First 80 Characters
#define EN2 PD6   //Enable for Second 80 Characters

char keyword[6] = "PRINT";  //Keyword to compare received data with 
char Received_data[5];       // holds Received data
int Showing_details  = 0;    // Flag to indicate if data is being displayed on LCD
int index_ = 0;              // index for Received_data array

int Parity = 0; // If no data has been received hence no parity checked it'll be set to 0 
                // when parity is okay, this is set to 1
                // when parity is not okay, this is set to 2


   // Function to initialize all pins to be connected to the ATtiny2313
void PinModes_INIT(){
	// LED connected to pin PD2
	DDRD |= (1<<PD2); //set PD2 as output
	
	//Pins connected to LCD
	DDRB = 0xFF; // Data pins connected to LCD in 8-bit mode set as OUTPUT
	DDRD |= (1<<RS)|(1<<RW)|(1<<EN1)|(1<<EN2); // Setting RS, RW, EN1, EN2 AS OUTPUT

}

 // function to initialize UART at 9600 bauds
void uartInit() {   
	
	// Setting the baud rate 9600 -- 0.2% error at 16000000
	UBRRH = (unsigned char)(BRC >> 8); // high byte placed in UBRRH
	UBRRL = (unsigned char) BRC;       // Low byte in UBRRL
	
	UCSRA  = 0x00;
	UCSRB |= (1 << RXEN) | (1 << UCSZ2);  
     // Enabling  Receiver 9 data bits 

	UCSRC |=  (1 << UCSZ0) | (1 << UCSZ1) | (1 << UPM1) | (0 << UPM0) ; 
	// Set to Asynchronous Op 9 data bits, 1 stop bit, even parity 
    // UCSRC |=  (1 << UCSZ0) | (1 << UCSZ1) | (1 << UPM1) | (0 << UPM0) ;
	// Set to Asynchronous Op 8 data bits, 1 stop bit, even parity 
 
}

// Function for writing one byte to EEPROM memory address
void EEPROM_write(unsigned int uiAddress, unsigned char ucData)
{
/* Wait for completion of previous write */
while(EECR & (1<<EEPE))
;
/* Set up address and data registers */
EEAR = uiAddress;     // address to being written goes here 
EEDR = ucData;        // byte being written is placed in EEDR   

/* Write logical one to EEMPE */
EECR |= (1<<EEMPE);
/* Start eeprom write by setting EEPE */
EECR |= (1<<EEPE);
}


// Function for Reading one byte from EEPROM memory address
unsigned char EEPROM_read(unsigned int uiAddress)
{
/* Wait for completion of previous write */
while(EECR & (1<<EEPE))
;
/* Set up address register */
EEAR = uiAddress;   // address to READ goes here 
EECR |= (1<<EERE);
 
 _delay_ms(5);

/* Return data from data register */
return EEDR;
}

 // function for writing strings to EEPROM
void Prom_EEPROM_write(unsigned int StartAdress, char *Details, unsigned int Length)
{
    unsigned int i;  
    for(i=0; i<Length; i++)         
    {   // loop through all bits in data
        EEPROM_write(StartAdress+i, Details[i]);  // write to EEPROM
        _delay_us(10);
    }
}

// function for Reading strings from EEPROM
void Prom_EEPROM_read(unsigned int StartAdress, char *EEPROM_Data, unsigned int Length)
{
    unsigned int i;
    for(i=0; i<Length; i++)
    {   // loop through all bits in data
        EEPROM_Data[i] = EEPROM_read(StartAdress+i);  //Read from EEPROM
		_delay_us(10);
    }
}

// Function to select LCD section which will be displaying 1- first 80 characters or 2- second 80 characters
void LCD_Enable(unsigned int EN)
{
	// this pulses the enable pins of the LCD
	if(EN == 1){
		 // Enable first two lines for display when 1 is passed in
	
		PORTD |= (1<<EN1);   // logic 1 on enable 1 pin
		_delay_ms(1);        // short delay
		PORTD &= ~(1<<EN1);  // logic 0 on enable 1 pin
    } 
	else if(EN == 2){
		// Enable last two lines for display when 2 is passed in
	
		PORTD |= (1<<EN2);  // logic 1 on enable 2 pin
		_delay_ms(1);       // short delay
		PORTD &= ~(1<<EN2); // logic 0 on enable 2 pin
	}
	_delay_ms(1); 
}

	// This function passes the commands to LCD
void LCD_SEND_CMD(unsigned char CMD,unsigned int EN){
		
    LCD = CMD;           //  Send COMMAND TO LCD 

	PORTD &= ~(1<<RS);   // Register Select = 0 For LCD to take COMMAND input
	PORTD &= ~(1<<RW);   // RW = 0 FOR WRITE
    
	LCD_Enable(EN);       // LCD enable funtion to switch between first and second 80 characters
}

// This function passes data to LCD
void LCD_SEND_DATA(unsigned char data,unsigned int EN){
	
    LCD = data;         // SEND DATA TO LCD 
	
	PORTD |= (1<<RS);    // RS = 1 For LCD to take DATA input
	PORTD &= ~(1<<RW);   // RW = 0 FOR WRITE
	
	LCD_Enable(EN);      // LCD enable funtion to switch between first and second 80 characters
}

 // This function sends string data to LCD
void LCD_STRING(char *str,unsigned int EN){
	int i;
	for(i=0;str[i]!=0;i++)  
	{   // loop through each character in the character array
		LCD_SEND_DATA(str[i],EN); // send each character 
		_delay_ms(1);             // short delay
	}
}
 
 /// Function to initialize the LCD
void LCD_INIT(unsigned int lcd){
	
	PORTB = 0x00; // All Data pins to logic zero
	PORTD = 0x00; // All control pins to logic zero
	
    _delay_ms(10); // delay of 10 ms 
     
    LCD_SEND_CMD(0x38, lcd);     //2 lines 5X7 matrix(D0 - D7; 8Bit)
    _delay_ms(1);
    LCD_SEND_CMD(0x0F, lcd);     // Turn DISPLAY on , Set Cursor to Blink
    _delay_ms(1);
    LCD_SEND_CMD(0x01, lcd);     // clear LCD   
    _delay_ms(1);
	LCD_SEND_CMD(0x06, lcd);     // Shift LCD cursor to the right
    _delay_ms(1);
}


void LCD_mes(){
	
	char line_1[] = "Press button"; // data to show on line 1
	char line_2[] = "...";          // data to show on line 2
	
	LCD_SEND_CMD(0x01, 1);     // Clear screen()
	_delay_ms(1);
	LCD_SEND_CMD(0x0F, 1);     // Set Cursor not to Blink
	_delay_ms(1);
	LCD_SEND_CMD(0x80, 1);     // Cursor on 1st line
	LCD_STRING(line_1, 1);     // data to be written to first line

	LCD_SEND_CMD(0xC0, 1);     // Cursor on 2nd line ->C0
	_delay_ms(1);
	LCD_STRING(line_2, 1);     // data to be written to second line  
	_delay_ms(1);
   	
	char line_3[] = "uP & uC";     // data to show on line 3
	char line_4[] = "Dry run 1.0"; // data to show on line 4

    LCD_SEND_CMD(0x01, 2);     // Clear screen
	_delay_ms(1);
	LCD_SEND_CMD(0x86, 2);     // Cursor on 1st line
	LCD_STRING(line_3, 2);
	LCD_SEND_CMD(0xC0, 2);     // Cursor on 2nd line ->C0
	_delay_ms(1);
	LCD_STRING(line_4, 2); 
	_delay_ms(500);
	
	// Now switch up stuff
	LCD_SEND_CMD(0x01, 1);     // Clear screen() 1
	LCD_SEND_CMD(0xC0, 1);     // Cursor on 2nd line ->C0
	_delay_ms(1);
	LCD_STRING(line_2, 1);     // data to be written to second line
	_delay_ms(1);
	
	LCD_SEND_CMD(0x86, 2);     // Cursor on 1st line screen 2
	_delay_ms(1);
	LCD_STRING(line_1, 2);
	_delay_ms(1);
}


// Function for receiving data from the serial port
void ReceiveUARTData(){
	// when RXC is SET, then USART Receive data is Complete

	//check for parity error
	if ((UCSRA&(1<<UPE)))       // CHECK FOR ERROR FLAG
	{ 
		// We have an error in the received data
		Parity = 2;            // trigger parity error led to blink in switch case statement
		UCSRA = (0 << UPE);    // reset flag
		index_ = 0;            //reset index
		return;                // return to loop since data is corrupt and cannot be read
	}

     // Theres no parity error so read the data
	 Parity = 1; 			  // set parity error led to "PARITY OKAY"
	char receivedByte = UDR;  // UDR will "read" from the RX pin 
	_delay_ms(5);
	 
	
	if(receivedByte != '0' && index_ < 5){
        Received_data[index_] = receivedByte;   // append received data to array
        index_++;             					 // Add 1 to the index position
    } else {

        // data reveived, parity okay: checking for keyword "PRINT" 
		index_ = 0;          //reset index
		Showing_details = 0; //reset showing details

		// Checking whether received data is "PRINT"
		for(int i=0;keyword[i]!=0;i++)  
        {
		 // Check each character of the keyword and the received data
		 //                          if                              else:	
         Showing_details = ((Received_data[i] == keyword[i])  ?  1    :    0  );//shorthand if else statement
         if (Showing_details == 0) break;                                       // If the data does not match i.e if command sent is NOT "PRINT", break the loop
         } 

        if (Showing_details == 1) {    // If the data is okay, Read from EEPROM and print EEPROM data to LCD
             //Reading from EEPROM
            char Read_Data[32]; 
            Prom_EEPROM_read(0, Read_Data, 32);
            
			LCD_SEND_CMD(0x01, 1);     // Clear first 80 characters on screen
            _delay_ms(1);
			
            //loop through read data and print to LCD
            for (int i=0; i <32; i++) {  
                LCD_SEND_DATA(Read_Data[i], 1);
				if (i == 14) LCD_SEND_CMD(0xC0, 1);     // 2nd line data
                _delay_us(100);
            }
		    _delay_ms(1000);        
		    LCD_SEND_CMD(0x01, 1);     // Clears after a few milliseconds
            _delay_ms(1);
        } 
	}
}



int main(void)
{
	PinModes_INIT(); // Function to Initialize input and output pins
		
	LCD_INIT(1);    // Initialize LCD first 80 Characters
	LCD_INIT(2);    // Initialize LCD second 80 Characters 
	_delay_ms(5);   // delay 5ms
    
	uartInit();    // Initialize UART
 
    char MyNAME_REG[32] =  {"GaV ROCKS !! _  Data to display"};//string to be stored in the EEPROM -- 31 characters
                                                            // +1 for the null character
    
	LCD_mes();
     
    Prom_EEPROM_write(0, MyNAME_REG, 32); // Write data in character array to EEPR0M
     

    //Looping function
    while (1) 
    {  
		
		switch (Parity)
		{ 
	   		case 0:
				// No data received yet, no parity checked yet
				PORTD &= ~(1<<PD2); //set PD2 low
				break; 


		    case 1:
				// parity is okay  -- LED ON
				PORTD |= (1<<PD2); //set PD2 high
				break;


			case 2:
				// parity is not okay -- Blinking LED at rate of 200ms
				
				PORTD |= (1<<PD2);  //set PD2 high
				_delay_ms(200);     //delay 200ms
				PORTD &= ~(1<<PD2); //set PD2 low
				_delay_ms(200);     //delay 200ms 
				break;

			default:
				PORTD |= (1<<PD2); //set PD2 high
				break;

        }  
		
		// Uncomment below for scroll funct 
        LCD_SEND_CMD(0x1c, 1);
		
	    // I didnt use RX interrupt functionality -- use it for more efficient code
        // this was just to avoid using interrupt library

        //Check RXC register if USART data is available
		if ((UCSRA&(1<<RXC)))       // If data has been received ; fetch and check keyword
		{ 
			ReceiveUARTData();  // Function to receive data from the serial port
		}
    }
}
  