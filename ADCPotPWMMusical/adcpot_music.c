/* 
 * PICDEM 2 PLUS DEMO BOARD
 * PIC18F4520
 * 
 * A music tone will be produced for the buzzer on RC1,
 * and its tone varies linearly with the pot on RA0.
 *
 * A PWM waveform is produced using CPP2 on RC1.
 * The frequency of the tone is linearly interpolated
 * according to the ADC value on RA0 pot.
 *
 * The duty cycle is also recalculated on-the-fly
 * to maintain the same duty cycle regardless of the
 * PWM period.
 *
 * The clock is set to HS with a 10MHz crystal (Fosc = 10MHz)
 */

#include <p18f4520.h>
#include <GenericTypeDefs.h>

void main(void);
void setPWMFrequency(float freq);
void setPWMDutyCycleCCP2(int pwm_percentage);
void InterruptHandlerHigh();

UINT16 ADCResult = 0;

void main(void) {
    // Output LED on RB0
    TRISBbits.TRISB0 = 0;

    /***************************************************************************/
    // Setup ADC on RA0
    /***************************************************************************/
    // Setup input on RA0
    TRISAbits.TRISA0 = 1;
    
    // 1. Configure the A/D module:
    ADCON0bits.CHS = 0b0000; // Analog Channel Select RA0/AN0
    
    ADCON1bits.PCFG = 0b1110; // AN0 as analog input
    ADCON1bits.VCFG1 = 0; // Vss as Vref-
    ADCON1bits.VCFG0 = 0; // Vdd as Vref+
    
    ADCON2bits.ADFM = 1; // Right Justified
    ADCON2bits.ACQT = 0b001; // 001 = 2 TAD Acquisition Time
    ADCON2bits.ADCS = 0b001; // A/D Conversion Clock -> 001 = FOSC/8
    
    ADCON0bits.ADON = 1; // 1 = A/D Converter module is enabled
    
    // 2. Configure A/D interrupt (if desired):
    PIR1bits.ADIF = 0; // Clear ADIF bit
    PIE1bits.ADIE = 1; // Set ADIE bit 
    INTCONbits.GIE = 1; // Set GIE bit 
    INTCONbits.PEIE = 1; // Set peripheral bit 
    
    // 4. Start conversion:
    ADCON0bits.GO = 1; // Set GO/DONE bit (ADCON0 register)

    /***************************************************************************/
    // Setup PWM on RC1
    /***************************************************************************/
    // Setup output on RA0
    TRISCbits.RC1 = 0;

    // 1. Set the PWM period by writing to the PR2 register.
    setPWMFrequency(1046.50);
    
    // 2. Set the PWM duty cycle by writing to the CCPRxL register and CCPxCON<5:4> bits.
    setPWMDutyCycleCCP2(10); // Set PWM duty cycle
    
    // 3. Make the CCPx pin an output by clearing the appropriate TRIS bit.
    TRISCbits.RC1 = 0; // Set RC1 to output: RC1/T1OSI/CCP2(1)
    
    // 4. Set up TMR2
    T2CONbits.T2CKPS = 0b10; // 1x = Prescaler is 16 
    T2CONbits.TMR2ON = 1; // 1 = Timer2 is on
    
    // 5. Configure the CCPx module for PWM operation.
    CCP2CONbits.CCP2M = 0b1100; // CCP2 as PWM mode -> 11xx = PWM mode
    
    while (1) {
        // Range of freq is 1046.50 (C6) to 2093.00 (C7)
        float freq = (ADCResult / 1024.0 * 1046.5) + 1046.50;
        setPWMFrequency(freq);
    }
}


/* Calculation for PWM Period
 *   PWM Period = [(PR2) + 1] • 4 • TOSC • (TMR2 Prescale Value)
 *   1/PWM freq = [(PR2) + 1] • 4 • (1/FOSC) • (TMR2 Prescale Value)
 *   (1/PWM freq) / 4 / (1/FOSC) / (TMR2 Prescale Value) = [(PR2) + 1]
 *   0.25 / PWM freq * FOSC / (TMR2 Prescale Value) = [(PR2) + 1]
 *   0.25 * FOSC / PWM freq / (TMR2 Prescale Value) - 1 = PR2
 *   PR2 = 0.25 * FOSC / freq / 8 - 1
 *
 * Calculation for Max Freq & Min Freq
 *   (256 / 0.25 * Prescale / FOSC)^-1 = freq min 
 *   (1 / 0.25 * Prescale / FOSC)^-1 = freq max
 */

#define FOSC (10e6) // 10MHz HS mode
#define PWM_PRESCALE (16)
#define round(x) ((x) + 0.5)
//Calculated for 10MHz, 16 prescaler
//Freq min = 610 Hz
//Freq max = 156 kHz
void setPWMFrequency(float freq) {
    PR2 = 0.25  * FOSC / freq / PWM_PRESCALE - 1;
}

void setPWMDutyCycleCCP2(int pwm_percentage) {
    /* Calculation for PWM Duty Cycle
     *   PWM Duty Cycle = (CCPRXL:CCPXCON<5:4>) • TOSC • (TMR2 Prescale Value)
     *   PWM Period = [(PR2) + 1] • 4 • TOSC • (TMR2 Prescale Value)
     *   PWM Ratio = Duty Cycle / Period
     *   PWM Ratio = (CCPRXL:CCPXCON<5:4>) / [(PR2) + 1] • 4]
     *   (CCPRXL:CCPXCON<5:4>) = Ratio * [(PR2) + 1] • 4]
     */
    int pwm_period = PR2;
    // round off the float
    int pwm_duty_cycle = round( (pwm_percentage * 0.01f) * (pwm_period + 1) * 4 ); 
    CCP2CONbits.DC2B = 0b11 & pwm_duty_cycle; // DCxB<1:0>: bits 0, 1
    CCPR2L = pwm_duty_cycle >> 2;
}


#pragma code InterruptVectorHigh = 0x08
void InterruptVectorHigh (void) {
    _asm
        goto InterruptHandlerHigh //jump to interrupt routine
    _endasm
}

//----------------------------------------------------------------------------
// High priority interrupt routine

#pragma code
#pragma interrupt InterruptHandlerHigh
void InterruptHandlerHigh() {
    if (PIR1bits.ADIF) {
        if (!ADCON0bits.GO_DONE) { // if done conversion
            PIR1bits.ADIF = 0; // Clear ADIF bit
            // Alternatively: ADCResult = ADRES;
            ADCResult = ((int) ADRESH) << 8; // Must cast as the register is 8-bits wide
            ADCResult |= ADRESL;
            // result is loaded into the ADRESH:ADRESL register pair
            ADCON0bits.GO = 1;
        }
        LATBbits.LATB0 = !LATBbits.LATB0; //toggle LED on RB0
    }
}