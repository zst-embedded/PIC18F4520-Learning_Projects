/* 
 * PICDEM 2 PLUS DEMO BOARD
 * PIC18F4520
 * 
 * A music tone will be produced for the buzzer on
 * RC1 when push button RB0 is pressed. For each
 * button press, a single note will be played in 
 * "Do, Re, Mi" sequence. It lasts for one second.
 *
 * A PWM waveform is produced using CPP2 on RC1.
 * Frequency of each note is looked up from an array
 * and the PWM period is calculated on-the-fly.
 * 
 * The duty cycle is also recalculated on-the-fly
 * to maintain the same duty cycle regardless of the
 * PWM period.
 *
 * The clock is set to HS with a 10MHz crystal (Fosc = 10MHz)
 */

#include <p18f4520.h>
#include <delays.h>
#include <GenericTypeDefs.h>

// Taken from http://www.phy.mtu.edu/~suits/notefreqs.html
float ToneFreq[] = {
    1046.50, //C6   
    1174.66, //D6
    1318.51, //E6
    1396.91, //F6
    1567.98, //G6
    1760.00, //A6
    1975.53, //B6
    2093.00, //C7
    -1 // End of array (if -1, then restart array index)
};


#define round(x) ((x) + 0.5)
#define FOSC (10e6) // 10MHz HS mode
#define PWM_PRESCALE (16)
#define PWM_DUTY_CYCLE (10) // Percentage of Duty Cycle

/* Calculation for Timer 0 as 1 sec
 *   10MHz clock, 256 prescaler, 8 bit mode
 *   Instruction freq = 10MHz / 4 = 2.5MHz
 *   Timer clock freq = 2.5MHz / 256 prescaler
 *   No of cycles = 1Hz / (2.5MHz / 256)^-1
 *   No of cycles = 2.5e6 / 256 = 9765.625
 *   Reset value = 65535 - 9765.625 = 55769.375
 *   Formula: 65535 - FOSC / 4 / PRESCALE 
 */ 
#define TIMER0_RESET_VAL (55769)


BOOL RB0_Pressed = FALSE;

void main(void);
void setPWMFrequency(float freq);
void setPWMDutyCycleCCP2(int pwm_percentage);
void ISR(void);
void resetTimer0OneSecond();

void main(void) {
    int array_index = 0;
    float freq;
    
    // Set up Timer 0
    INTCONbits.PEIE = 1; // enable peripheral interrupt
    INTCON2bits.TMR0IP = 1; // TMR0 high priority
    INTCONbits.TMR0IE = 1; // TMR0 interrupt enable
    T0CONbits.PSA = 0; // Assign prescaler
    T0CONbits.T0PS = 0b111; // Prescaler 1:256
    T0CONbits.T0CS = 0; // internal clock source
    T0CONbits.T08BIT = 0; // 0 = Timer0 is configured as a 16-bit timer/counter 
    T0CONbits.TMR0ON = 1; // Enable Timer 0
    
    // Set up external interrupt -> RB0 push button 
    TRISB = 1<<0; // RB0 as input
    INTCONbits.INT0IE = 1; // INT0 enabled  
    INTCON2bits.INTEDG0 = 0; // Interrupt on falling edge
    INTCONbits.GIEH = 1; // Enable global interrupts
    
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
    setPWMFrequency(ToneFreq[0]);
    
    // 2. Set the PWM duty cycle by writing to the CCPRxL register and CCPxCON<5:4> bits.
    setPWMDutyCycleCCP2(0); // Set PWM duty cycle to 0% on first boot
    
    // 3. Make the CCPx pin an output by clearing the appropriate TRIS bit.
    // Set RC1 to output: RC1/T1OSI/CCP2(1)
    TRISCbits.RC1 = 0;
    
    // 4. Set up TMR2
    T2CONbits.T2CKPS = 0b10; // 1x = Prescaler is 16 
    T2CONbits.TMR2ON = 1; // 1 = Timer2 is on
    
    // 5. Configure the CCPx module for PWM operation.
    CCP2CONbits.CCP2M = 0b1100; // CCP2 as PWM mode -> 11xx = PWM mode
    
    while (1) {
        if (RB0_Pressed) {
            freq = ToneFreq[++array_index];
            if (freq == -1) { // reset to first freq
                array_index = 0;
                freq = ToneFreq[array_index];
            }
            setPWMFrequency(freq);
            setPWMDutyCycleCCP2(PWM_DUTY_CYCLE);
            resetTimer0OneSecond();
            RB0_Pressed = FALSE;
        }
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
    
    if (INTCONbits.TMR0IF) {
        INTCONbits.TMR0IF = 0;
        setPWMDutyCycleCCP2(0);
    }
}

void resetTimer0OneSecond() {
    
    TMR0H = TIMER0_RESET_VAL >> 8; //reset timer
    TMR0L = TIMER0_RESET_VAL & 0xFF; //reset timer
}

//----------------------------------------------------------------------------
