/* 
 * PICDEM 2 PLUS DEMO BOARD
 * PIC18F4520
 * 
 * Generate a 500kHz PWM waveform using CCP2 on RC1.
 * Push button on RB0 will trigger an interrupt and
 * increment the duty cycle from 0% to 100% in 
 * steps of 10%. The clock is set to HSPLL with a
 * 10MHz crystal (Fosc = 40MHz)
 */

#include <p18f4520.h>
#include <delays.h>
#include <GenericTypeDefs.h>

/* Calculation for PWM Period
 *   PWM Period = [(PR2) + 1] • 4 • TOSC • (TMR2 Prescale Value)
 *   500kHz freq = 2us
 *   2e-6 = ()PR2+1) * 4 * (1/40*e6) * prescale of 1
 *   PR2+1 = 20 -> PR2 = 19dec
 */

#define round(x) ((x) + 0.5)
#define PWM_PERIOD (19)	
BOOL RB0_Pressed = FALSE;

void ISR();
void updateCCP2DutyCycle(int pwm_percentage);

void main(void) {
	int pwm_percentage;
	
	// Set up RB0 push button external interrupt
	TRISB = 1<<0; // RB0 as input
	INTCONbits.INT0IE = 1; // INT0 enabled	
	INTCON2bits.INTEDG0 = 0; // Interrupt on falling edge
	INTCONbits.GIEH = 1;// Enable global interrupts
	
	/****************************************************
	The following steps should be taken when configuring
	the CCP module for PWM operation:
	1. Set the PWM period by writing to the PR2 register.
	2. Set the PWM duty cycle by writing to the
	CCPRxL register and CCPxCON<5:4> bits.
	3. Make the CCPx pin an output by clearing the
	appropriate TRIS bit.
	4. Set the TMR2 prescale value, then enable
	Timer2 by writing to T2CON.
	5. Configure the CCPx module for PWM operation.
	****************************************************/
	
	// 1. Set the PWM period by writing to the PR2 register.
	PR2 = PWM_PERIOD;
	
	// 2. Set the PWM duty cycle by writing to the CCPRxL register and CCPxCON<5:4> bits.
	pwm_percentage = 10;
	updateCCP2DutyCycle(pwm_percentage); // Set PWM duty cycle to 10%
	
	// 3. Make the CCPx pin an output by clearing the appropriate TRIS bit.
	// Set RC1 to output: RC1/T1OSI/CCP2(1)
	TRISCbits.RC1 = 0;
	
	// 4. Set up TMR2
	T2CONbits.T2CKPS = 0b00; // 00 = Prescaler is 1 
	T2CONbits.TMR2ON = 1; // 1 = Timer2 is on
	
	// 5. Configure the CCPx module for PWM operation.
	CCP2CONbits.CCP2M = 0b1100; // CCP2 as PWM mode -> 11xx = PWM mode
	
	while (1) {
		if (RB0_Pressed) {
			RB0_Pressed = FALSE;
			if (pwm_percentage >= 100) { // reset back to 0 if already 100%
				pwm_percentage = 0;
			} else { // increment duty cycle by 10%
				pwm_percentage += 10;
			}
			updateCCP2DutyCycle(pwm_percentage);
		}
	}
}

void updateCCP2DutyCycle(int pwm_percentage) {
	/* Calculation for PWM Duty Cycle
	 *   PWM Duty Cycle = (CCPRXL:CCPXCON<5:4>) • TOSC • (TMR2 Prescale Value)
	 *   PWM Period = [(PR2) + 1] • 4 • TOSC • (TMR2 Prescale Value)
	 *   PWM Ratio = Duty Cycle / Period
	 *   PWM Ratio = (CCPRXL:CCPXCON<5:4>) / [(PR2) + 1] • 4]
	 *   (CCPRXL:CCPXCON<5:4>) = Ratio * [(PR2) + 1] • 4]
	 */
	// round off the float
	int pwm_duty_cycle = round( (pwm_percentage * 0.01f) * (PWM_PERIOD + 1) * 4 ); 
	CCP2CONbits.DC2B = 0b11 & pwm_duty_cycle; // DCxB<1:0>: bits 0, 1
	CCPR2L = pwm_duty_cycle >> 2;
}

//----------------------------------------------------------------------------
// High priority interrupt vector

#pragma code InterruptVectorHigh = 0x08
void InterruptVectorHigh(void) {
	_asm
	goto ISR
	_endasm
}

//----------------------------------------------------------------------------
// High priority interrupt routine

#pragma code
#pragma interrupt ISR

void ISR(void) {
	if (INTCONbits.INT0IF) {
		INTCONbits.INT0IF = 0;
		RB0_Pressed = TRUE;
	}
}

//----------------------------------------------------------------------------
