/* 
 * PICDEM 2 PLUS DEMO BOARD
 * PIC18F4520
 * 
 * This project is to use I2C to interface with two I2C devices.
 * The MSSP module is configured in I2C mode.
 * An MCP23008 and MCP23017 is used:
 * The GPIO of MCP23008 (set to all outputs) will output whatever
 * is on GPIOA of MCP23017 (set to all inputs).
 * 
 * References:
 *   https://cdn-shop.adafruit.com/datasheets/MCP23008.pdf
 *   http://ww1.microchip.com/downloads/en/DeviceDoc/20001952C.pdf
 *   https://github.com/adafruit/Adafruit-MCP23008-library/blob/master/Adafruit_MCP23008.h
 *   https://github.com/adafruit/Adafruit-MCP23017-Arduino-Library/blob/master/Adafruit_MCP23017.h
 */

#include <p18f4520.h>
#include <GenericTypeDefs.h>
#include <delays.h>
#include "MCP_Addresses.h"

#define HIGH_SPEED_I2C

/**
 * Waits until I2C is idling
 */
void I2C_idle() {
    while (SSPSTATbits.BF); // Wait until buffer is empty
    while (SSPSTATbits.R_W || SSPCON2 & 0x1F); // Wait until transmission done
}

/**
 * Transmits a byte over I2C
 */
void I2C_transmit(UINT8 buffer) {
    SSPCON2bits.ACKSTAT = 1;
    SSPCON2bits.RCEN = 0; // 0 = Receive Idle
    // I2C_idle();
    SSPBUF = buffer;
    I2C_idle();
    // while (SSPCON2bits.ACKSTAT); // wait until slave ack
}

/**
 * Begin communicating with I2C by transmitting the device 
 * address in bits 1-7, along with read/write in bit 0.
 */
void I2C_begin(char address, BOOL write) {
    I2C_idle();
    
    // START BIT
    SSPCON2bits.SEN = 1; // 1 = Initiates Start condition on SDA and SCL pins. Automatically cleared by hardware
    while(SSPCON2bits.SEN); // Wait until start bit is sent
    
    // TRANSMIT ADDRESS
    // Bit 0 for WRITE is 0, READ is 1
    I2C_transmit((address << 1) + !write);
}

/**
 * Send a stop bit to close the communication
 */
void I2C_end(){
    // STOP BIT
    SSPCON2bits.PEN = 1; // 1 = Initiates Stop condition on SDA and SCL pins. Automatically cleared by hardware. 
    while(SSPCON2bits.PEN); // Wait until stop bit is sent
}

/**
 * Write to devices using these steps:
 * - Address the device
 * - Select the register
 * - Send the value
 */
void I2C_write(char addr, char reg, char val) {
    I2C_begin(addr, TRUE); // TRUE for writing
    I2C_transmit(reg);
    I2C_transmit(val);
    I2C_end();
}


/**
 * Read from devices using these steps:
 * - Address the device with write bit
 * - Select the register
 * - Address the device with read bit (repeated start)
 * - Wait until the buffer is ful
 * - Get the value
 */

UINT8 I2C_read(char addr, char reg) {
    UINT8 result;
    
    /* Write the register we want */
    I2C_begin(addr, TRUE); // TRUE for writing
    I2C_transmit(reg); // Specify register to read
    
    /* Read from the register */
    I2C_begin(addr, FALSE); // FALSE for reading
    I2C_idle();
    SSPCON2bits.RCEN = 1; //1 = Enables Receive mode for I2C 
    while (!SSPSTATbits.BF);      // wait until byte received  
    result = SSPBUF;              // return with read byte  
    
    I2C_end();
    return result;
}

void MCP23008_write(char reg, char val) {
    I2C_write(MCP23008_ADDRESS, reg, val);
}

void MCP23017_write(char reg, char val) {
    I2C_write(MCP23017_ADDRESS, reg, val);
}

UINT8 MCP23017_read(char reg) {
    return I2C_read(MCP23017_ADDRESS, reg);
}

void main(void) {
    /* Set input for I2C pins */
    TRISCbits.TRISC3 = 1; // Serial clock (SCL) - RC3/SCK/SCL
    TRISCbits.TRISC4 = 1; // Serial data (SDA) - RC4/SDI/SDA
    
    /* RB[0:3] as output for debug */
    TRISB &= ~0x0f;
    LATB = 0x00;

    /* Configure I2C Master mode */
    SSPCON1bits.SSPEN = 1; // 1 = Enables the serial port and configures the SDA and SCL pins as the serial port pins 
    SSPCON1bits.SSPM = 0b1000; // 1000 = I2C Master mode, clock = FOSC/(4 * (SSPADD + 1))
    
    // When the MSSP is configured in Master mode, the lower seven
    // bits of SSPADD act as the Baud Rate Generator reload value.
#ifdef HIGH_SPEED_I2C
    /* clock = FOSC / (4 * (SSPADD + 1))
     * 100kbps = 10MHz / (4 * (SSPADD + 1))
     * SSPADD = 10MHz / 400kbps / 4 - 1
     * SSPADD = 5.25
     */
    SSPADD = 5; // 400KHz for Fosc = 10MHz
    SSPSTATbits.SMP = 0; // 0 = Slew rate control enabled for High-Speed mode (400 kHz)
#else
    /* clock = FOSC / (4 * (SSPADD + 1))
     * 100kbps = 10MHz / (4 * (SSPADD + 1))
     * SSPADD = 10MHz / 100kbps / 4 - 1
     * SSPADD = 24
     */
    SSPADD = 24; // 100KHz for Fosc = 10MHz
    SSPSTATbits.SMP = 1; // 1 = Slew rate control disabled for Standard Speed mode (100 kHz and 1 MHz) 
#endif

    /* Set I/O direction register to all output */
    MCP23008_write(MCP23008_IODIR, 0x00);
    
    /* Set I/O direction register to all input */
    MCP23017_write(MCP23017_IODIRA, 0xFF);
    
    /* Send value on startup for debugging */
    MCP23008_write(MCP23008_GPIO, 0xAA);
    Delay10KTCYx(100);

    while(1) {
        UINT8 result;

        /* Read value from MCP23017 GPIOA and output it on MCP23008 */
        result = MCP23017_read(MCP23017_GPIOA);
        MCP23008_write(MCP23008_GPIO, result);
        
        LATB ^= 0x01; // blink LED for debugging
        Delay10KTCYx(100);
    }
}