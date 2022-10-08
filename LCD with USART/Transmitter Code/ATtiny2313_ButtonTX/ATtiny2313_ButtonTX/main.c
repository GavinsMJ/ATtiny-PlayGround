/*
 * ATtiny2313_ButtonTX.c
 * 
 * Author : Gavins Maragia 
 
 *	RX  (PD0)
 *	TX  (PD1) 
 *  Push button connected to pin PB2
 *  PB0 AND PB1 Debug LEDs
 
    This program sends the text "PRINT" via USART with even parity
 
 */  //    TRANSMITTER CODE

#define F_CPU 16000000UL    // 16 MHz clock freq 

#include <avr/io.h>        
#include <util/delay.h>  


// Baud rate set at 9600 -- 0.2% error at 16000000
#define BAUDRATE 9600                    // 9600 baud rate
#define BRC ((F_CPU/16UL/BAUDRATE) -1 ) // calculates to 103 for  U2X = 0

                                   
char PRINT[5] =  {'P', 'R', 'I', 'N', 'T'};     // Command to be sent when button is pressed
 
 // function to initialize UART at 9600 bauds
void uartInit() {    
	
	// Setting the baud rate 9600 -- 0.2% error at 16000000
	UBRRH = (unsigned char)(BRC >> 8); // high byte placed in UBRRH
	UBRRL = (unsigned char) BRC;       // Low byte in UBRRL
	
	UCSRA  = 0x00;
	UCSRB |=  (1 << TXEN) | (1 << UCSZ2);  
     // Enabling Transmitter 9 data bits 

	UCSRC |=  (1 << UCSZ0) | (1 << UCSZ1) | (1 << UPM1) | (0 << UPM0) ; 
	// Set to Asynchronous Op 9 data bits, 1 stop bit, even parity  
}
 
int main(void)
{
	
	DDRB = (1 << PINB0) | (1 << PINB1);  // setting pins connected to debug LEDs as output
	DDRB = (0 << PINB2);                 // setting pin connected to push button as input
	
	uartInit();                   // Initialize the UART
	
    while(1)
    {
        
	 if ((PINB&(1<<PINB2)))       // Compare and wait for button press
	  { 
		 // If button is pressed then,  
		 PORTB = (1 << PINB0) | (0 << PINB1);       // light LED ON PINB0 if button is pressed
		     
		 for (int i=0; i <5; i++) {	  // for loop to loop through and print our command array
		 
			  UDR = PRINT[i];	  // Send current selected byte from 'PRINT' Array
			  _delay_ms(5);       // delay for transmission
		 }
                                          
         _delay_ms(20); 
	     
		} else {            
		
		PORTB = (0 << PINB0) | (1 << PINB1);        // light LED ON PINB1 if button is NOT pressed
		_delay_ms(20);
	}  
   }
}
