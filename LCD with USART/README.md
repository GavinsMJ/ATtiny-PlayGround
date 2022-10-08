# LCD with USART

Two ATtiny 2313 controllers communicate with USART, even parity and an LCD display to print data saved on EEPROM once a PRINT command is sent from first uP. Sim via Proteus.

LCD and EEPROM libraries were NOT used 

Add USART RX interrupt for faster communication, I left this out to not use the avr/interrupt.h library

Starting Program 
![Schematic](/Assets/Startup.png)

Running : button not pressed yet
![Sim with Button unpressed](/Assets/bPressed.png)

Running : button pressed
![Sim with Button pressed](/Assets/bUnPressed.png)

## To-Do

...