/* 
 * PICDEM 2 PLUS DEMO BOARD
 * PIC18F4520
 * 
 * External Interrupts
 * A push button on RB0 will trigger an external 
 * interrupt to toggle an LED on RB1.
 */

#include <p18f4520.h>
#include <GenericTypeDefs.h>
 
BOOL RB0_Pressed = FALSE;

void ISR(void);
void delay(void);

void main(void) {
	// RB0 as input, RB1:3 as output 
	TRISB = 0x01;
	LATB = 0;  
	
	// Setup External interrupt 0 -> RB0 / INT0
	INTCONbits.GIEH = 1; // Enable global interrupt
	INTCONbits.INT0IE = 1; // INT0 enabled	
	INTCON2bits.INTEDG0 = 0; // Interrupt on falling edge
	
	while (1) {
		if (RB0_Pressed) {
			RB0_Pressed = FALSE;
			LATB ^= 1<<1; // Toggle on RB1 LED
		}
		delay(); /* Long program */
		LATB ^= 1<<2;
	}
}

void delay() {
	UINT16 i;
	for (i = 0; i < 10000; i++) {
		Nop();
	}
}

#pragma code InterruptVectorHigh = 0x08 
void InterruptVectorHigh(void) {
	_asm
	goto ISR
	_endasm
}
#pragma code
#pragma interrupt ISR
void ISR(void) {
	if (INTCONbits.INT0IF == 1) {
		INTCONbits.INT0IF = 0;
		RB0_Pressed = TRUE;
	}
}

