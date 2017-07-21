/* 
 * PICDEM 2 PLUS DEMO BOARD
 * PIC18F4520
 * 
 * This project is to use I2C to interface with MCP23008, which
 * is an I2C I/O port expander. The MSSP module is configured in I2C mode.
 * A seven segment display is connected to the outputs of the MCP23008.
 * 
 * References:
 *   https://cdn-shop.adafruit.com/datasheets/MCP23008.pdf
 *   https://github.com/adafruit/Adafruit-MCP23008-library/blob/master/Adafruit_MCP23008.h
 */

#include <p18f4520.h>
#include <GenericTypeDefs.h>
#include <delays.h>

#define HIGH_SPEED_I2C

/* Seven Segment Active-Low Hex Values */
const char SEVEN_SEG_CA[] = {
        0xc0, 0xf9, 0xa4, 0xb0, // 0, 1, 2, 3
        0x99, 0x92, 0x82, 0xf8, // 4, 5, 6, 7
        0x80, 0x90, 0x88, 0x83, // 8, 9, A, B
        0xc6, 0xa1, 0x86, 0x8e, // C, D, E, F
};

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
	// SSPCON2bits.RCEN = 0; // 0 = Receive Idle
	
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
 * Write to MCP23008 using these steps:
 * - Address the device
 * - Select the register
 * - Send the value
 */
void MCP23008_write(char reg, char val) {
	// 0x20 is address of MCP23008
	I2C_begin(0x20, TRUE);
	I2C_transmit(reg);
	I2C_transmit(val);
	I2C_end();
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

	/* Set I/O direction register to all output
	 * IODIR address = 0x00 */
	MCP23008_write(0x00, 0x00);
	
	/* Display Hex digits (0-F) on the seven segment.
	 * Count down, followed by count up */
	while(1) {
		INT8 i;
		for (i = -16; i <= 16; i++) {
			char sevseg = SEVEN_SEG_CA[ (i < 0) ? -i : i ];
			/* Set GPIO register to our value
			 * GPIO address = 0x09 */
			MCP23008_write(0x09, sevseg);
			Delay10KTCYx(100);

			LATB ^= 0x01; // blink LED for debugging
		}
	}
}