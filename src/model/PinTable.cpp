/*
 * Copyright 2026, air/OS contributors.
 * Distributed under the terms of the MIT License.
 *
 * The Raspberry Pi's header layout, pin colours and the choice of
 * configurable pins follow pigg (https://github.com/andrewdavidmackenzie/pigg,
 * Apache-2.0). Its alternate functions are those of the BCM2711 ARM
 * Peripherals datasheet, section 5.3.
 *
 * The Radxa Cubie A7S's headers are those of its schematic (RS504 v1.10,
 * sheet 14); the A733's pin functions are those of the schematic's pin
 * names in the order of Allwinner's pinctrl tables (pinctrl-sun60iw2.c and
 * pinctrl-sun60iw2-r.c of Radxa's BSP), which give the multiplexer values.
 */

#include "PinTable.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


namespace airpins {

namespace {

Board sBoard = Board::RaspberryPi4;


//	#pragma mark - Raspberry Pi 4


const int kPiPinCount = 40;
const int kBcmPinCount = 28;		// GPIO 0..27 reach the header

const HeaderPin kPiHeader[kPiPinCount] = {
	{ 1, -1, PinKind::Power3V3, "3V3", 0, NULL },
	{ 2, -1, PinKind::Power5V, "5V", 0, NULL },
	{ 3, 2, PinKind::Gpio, "GPIO2", 0, NULL },
	{ 4, -1, PinKind::Power5V, "5V", 0, NULL },
	{ 5, 3, PinKind::Gpio, "GPIO3", 0, NULL },
	{ 6, -1, PinKind::Ground, "Ground", 0, NULL },
	{ 7, 4, PinKind::Gpio, "GPIO4", 0, NULL },
	{ 8, 14, PinKind::Gpio, "GPIO14", 0, NULL },
	{ 9, -1, PinKind::Ground, "Ground", 0, NULL },
	{ 10, 15, PinKind::Gpio, "GPIO15", 0, NULL },
	{ 11, 17, PinKind::Gpio, "GPIO17", 0, NULL },
	{ 12, 18, PinKind::Gpio, "GPIO18", 0, NULL },
	{ 13, 27, PinKind::Gpio, "GPIO27", 0, NULL },
	{ 14, -1, PinKind::Ground, "Ground", 0, NULL },
	{ 15, 22, PinKind::Gpio, "GPIO22", 0, NULL },
	{ 16, 23, PinKind::Gpio, "GPIO23", 0, NULL },
	{ 17, -1, PinKind::Power3V3, "3V3", 0, NULL },
	{ 18, 24, PinKind::Gpio, "GPIO24", 0, NULL },
	{ 19, 10, PinKind::Gpio, "GPIO10", 0, NULL },
	{ 20, -1, PinKind::Ground, "Ground", 0, NULL },
	{ 21, 9, PinKind::Gpio, "GPIO9", 0, NULL },
	{ 22, 25, PinKind::Gpio, "GPIO25", 0, NULL },
	{ 23, 11, PinKind::Gpio, "GPIO11", 0, NULL },
	{ 24, 8, PinKind::Gpio, "GPIO8", 0, NULL },
	{ 25, -1, PinKind::Ground, "Ground", 0, NULL },
	{ 26, 7, PinKind::Gpio, "GPIO7", 0, NULL },
	{ 27, 0, PinKind::Reserved, "ID_SD", 0,
		"GPIO0: the HAT ID EEPROM's I2C bus" },
	{ 28, 1, PinKind::Reserved, "ID_SC", 0,
		"GPIO1: the HAT ID EEPROM's I2C bus" },
	{ 29, 5, PinKind::Gpio, "GPIO5", 0, NULL },
	{ 30, -1, PinKind::Ground, "Ground", 0, NULL },
	{ 31, 6, PinKind::Gpio, "GPIO6", 0, NULL },
	{ 32, 12, PinKind::Gpio, "GPIO12", 0, NULL },
	{ 33, 13, PinKind::Gpio, "GPIO13", 0, NULL },
	{ 34, -1, PinKind::Ground, "Ground", 0, NULL },
	{ 35, 19, PinKind::Gpio, "GPIO19", 0, NULL },
	{ 36, 16, PinKind::Gpio, "GPIO16", 0, NULL },
	{ 37, 26, PinKind::Gpio, "GPIO26", 0, NULL },
	{ 38, 20, PinKind::Gpio, "GPIO20", 0, NULL },
	{ 39, -1, PinKind::Ground, "Ground", 0, NULL },
	{ 40, 21, PinKind::Gpio, "GPIO21", 0, NULL },
};

const Connector kPiConnectors[] = {
	{ NULL, 0, kPiPinCount, 2 }
};

// ALT0 .. ALT5 of GPIO 0 to 27; NULL: reserved
const char* const kPiAlt[kBcmPinCount][6] = {
	{ "SDA0", "SA5", "PCLK", "SPI3_CE0_N", "TXD2", "SDA6" },
	{ "SCL0", "SA4", "DE", "SPI3_MISO", "RXD2", "SCL6" },
	{ "SDA1", "SA3", "LCD_VSYNC", "SPI3_MOSI", "CTS2", "SDA3" },
	{ "SCL1", "SA2", "LCD_HSYNC", "SPI3_SCLK", "RTS2", "SCL3" },
	{ "GPCLK0", "SA1", "DPI_D0", "SPI4_CE0_N", "TXD3", "SDA3" },
	{ "GPCLK1", "SA0", "DPI_D1", "SPI4_MISO", "RXD3", "SCL3" },
	{ "GPCLK2", "SOE_N", "DPI_D2", "SPI4_MOSI", "CTS3", "SDA4" },
	{ "SPI0_CE1_N", "SWE_N", "DPI_D3", "SPI4_SCLK", "RTS3", "SCL4" },
	{ "SPI0_CE0_N", "SD0", "DPI_D4", "BSCSL_CE_N", "TXD4", "SDA4" },
	{ "SPI0_MISO", "SD1", "DPI_D5", "BSCSL_MISO", "RXD4", "SCL4" },
	{ "SPI0_MOSI", "SD2", "DPI_D6", "BSCSL_SDA", "CTS4", "SDA5" },
	{ "SPI0_SCLK", "SD3", "DPI_D7", "BSCSL_SCL", "RTS4", "SCL5" },
	{ "PWM0_0", "SD4", "DPI_D8", "SPI5_CE0_N", "TXD5", "SDA5" },
	{ "PWM0_1", "SD5", "DPI_D9", "SPI5_MISO", "RXD5", "SCL5" },
	{ "TXD0", "SD6", "DPI_D10", "SPI5_MOSI", "CTS5", "TXD1" },
	{ "RXD0", "SD7", "DPI_D11", "SPI5_SCLK", "RTS5", "RXD1" },
	{ NULL, "SD8", "DPI_D12", "CTS0", "SPI1_CE2_N", "CTS1" },
	{ NULL, "SD9", "DPI_D13", "RTS0", "SPI1_CE1_N", "RTS1" },
	{ "PCM_CLK", "SD10", "DPI_D14", "SPI6_CE0_N", "SPI1_CE0_N", "PWM0_0" },
	{ "PCM_FS", "SD11", "DPI_D15", "SPI6_MISO", "SPI1_MISO", "PWM0_1" },
	{ "PCM_DIN", "SD12", "DPI_D16", "SPI6_MOSI", "SPI1_MOSI", "GPCLK0" },
	{ "PCM_DOUT", "SD13", "DPI_D17", "SPI6_SCLK", "SPI1_SCLK", "GPCLK1" },
	{ "SD0_CLK", "SD14", "DPI_D18", "SD1_CLK", "ARM_TRST", "SDA6" },
	{ "SD0_CMD", "SD15", "DPI_D19", "SD1_CMD", "ARM_RTCK", "SCL6" },
	{ "SD0_DAT0", "SD16", "DPI_D20", "SD1_DAT0", "ARM_TDO", "SPI3_CE1_N" },
	{ "SD0_DAT1", "SD17", "DPI_D21", "SD1_DAT1", "ARM_TCK", "SPI4_CE1_N" },
	{ "SD0_DAT2", NULL, "DPI_D22", "SD1_DAT2", "ARM_TDI", "SPI5_CE1_N" },
	{ "SD0_DAT3", NULL, "DPI_D23", "SD1_DAT3", "ARM_TMS", "SPI6_CE1_N" },
};

struct Group {
	const char* prefix;
	const char* name;
};

// longest prefixes first where they overlap
const Group kPiGroups[] = {
	{ "SPI0_", "SPI0" }, { "SPI1_", "SPI1" }, { "SPI3_", "SPI3" },
	{ "SPI4_", "SPI4" }, { "SPI5_", "SPI5" }, { "SPI6_", "SPI6" },
	{ "BSCSL", "I2C/SPI slave" }, { "PCM_", "PCM audio" },
	{ "PWM0_", "PWM0" }, { "GPCLK", "general purpose clock" },
	{ "DPI_", "parallel display" }, { "LCD_", "parallel display" },
	{ "PCLK", "parallel display" }, { "DE", "parallel display" },
	{ "SD0_", "SD host 0" }, { "SD1_", "SD host 1" }, { "ARM_", "JTAG" },
	{ "SDA0", "I2C0" }, { "SCL0", "I2C0" }, { "SDA1", "I2C1" },
	{ "SCL1", "I2C1" }, { "SDA3", "I2C3" }, { "SCL3", "I2C3" },
	{ "SDA4", "I2C4" }, { "SCL4", "I2C4" }, { "SDA5", "I2C5" },
	{ "SCL5", "I2C5" }, { "SDA6", "I2C6" }, { "SCL6", "I2C6" },
	{ "TXD0", "UART0" }, { "RXD0", "UART0" }, { "CTS0", "UART0" },
	{ "RTS0", "UART0" }, { "TXD1", "mini UART" }, { "RXD1", "mini UART" },
	{ "CTS1", "mini UART" }, { "RTS1", "mini UART" },
	{ "TXD2", "UART2" }, { "RXD2", "UART2" }, { "CTS2", "UART2" },
	{ "RTS2", "UART2" }, { "TXD3", "UART3" }, { "RXD3", "UART3" },
	{ "CTS3", "UART3" }, { "RTS3", "UART3" }, { "TXD4", "UART4" },
	{ "RXD4", "UART4" }, { "CTS4", "UART4" }, { "RTS4", "UART4" },
	{ "TXD5", "UART5" }, { "RXD5", "UART5" }, { "CTS5", "UART5" },
	{ "RTS5", "UART5" },
	{ "SA", "secondary memory" }, { "SD", "secondary memory" },
	{ "SOE_N", "secondary memory" }, { "SWE_N", "secondary memory" },
};


PinColor
PiColor(const HeaderPin& pin)
{
	switch (pin.gpio) {
		case 2: case 3:								// I2C1: light blue
			return { 173, 216, 230, 0, 0, 0 };
		case 7: case 8: case 9: case 10: case 11:	// SPI0: violet
			return { 238, 130, 238, 255, 255, 255 };
		case 14: case 15:							// UART: green
			return { 0, 128, 0, 255, 255, 255 };
		default:									// orange
			return { 255, 165, 0, 255, 255, 255 };
	}
}


int
PiAltIndex(int function)
{
	switch (function) {
		case kFunctionAlt0: return 0;
		case kFunctionAlt1: return 1;
		case kFunctionAlt2: return 2;
		case kFunctionAlt3: return 3;
		case kFunctionAlt4: return 4;
		case kFunctionAlt5: return 5;
		default: return -1;
	}
}


//	#pragma mark - Radxa Cubie A7S


/*	The driver's pin numbers are header positions: J11 pin n is n, J6 pin n
	is 30 + n. The 30-pin J11 has the Raspberry Pi's first 30 positions'
	power, ground and console pins. */
const int kCubiePinCount = 45;
const int kJ6 = 30;

const HeaderPin kCubieHeader[kCubiePinCount] = {
	{ 1, -1, PinKind::Power3V3, "3V3", 0, NULL },
	{ 2, -1, PinKind::Power5V, "5V", 0, NULL },
	{ 3, 3, PinKind::Gpio, "PJ23", 0, "TWI7 SDA on the board" },
	{ 4, -1, PinKind::Power5V, "5V", 0, NULL },
	{ 5, 5, PinKind::Gpio, "PJ22", 0, "TWI7 SCK on the board" },
	{ 6, -1, PinKind::Ground, "Ground", 0, NULL },
	{ 7, 7, PinKind::Gpio, "PB0", 0, NULL },
	{ 8, 8, PinKind::Reserved, "PB9", 0,
		"UART0 TX: the system's serial console" },
	{ 9, -1, PinKind::Ground, "Ground", 0, NULL },
	{ 10, 10, PinKind::Reserved, "PB10", 0,
		"UART0 RX: the system's serial console" },
	{ 11, 11, PinKind::Gpio, "PB1", 0, NULL },
	{ 12, 12, PinKind::Gpio, "PB5", 0, "I2S0 BCLK on the board" },
	{ 13, 13, PinKind::Gpio, "PL6", 0, NULL },
	{ 14, -1, PinKind::Ground, "Ground", 0, NULL },
	{ 15, 15, PinKind::Gpio, "PL7", 0, NULL },
	{ 16, 16, PinKind::Gpio, "PJ24", 0, NULL },
	{ 17, -1, PinKind::Power3V3, "3V3", 0, NULL },
	{ 18, 18, PinKind::Gpio, "PJ25", 0, NULL },
	{ 19, 19, PinKind::Gpio, "PD12", 0, "SPI1 MOSI on the board" },
	{ 20, -1, PinKind::Ground, "Ground", 0, NULL },
	{ 21, 21, PinKind::Gpio, "PD13", 0, "SPI1 MISO on the board" },
	{ 22, 22, PinKind::Gpio, "PL5", 0, NULL },
	{ 23, 23, PinKind::Gpio, "PD11", 0, "SPI1 CLK on the board" },
	{ 24, 24, PinKind::Gpio, "PD10", 0, "SPI1 CS0 on the board" },
	{ 25, -1, PinKind::Ground, "Ground", 0, NULL },
	{ 26, 26, PinKind::Gpio, "PD14", 0, NULL },
	{ 27, 27, PinKind::Gpio, "PD17", 0, "TWI2 SDA on the board" },
	{ 28, 28, PinKind::Gpio, "PD16", 0, "TWI2 SCK on the board" },
	{ 29, 29, PinKind::Gpio, "PB2", 0, NULL },
	{ 30, -1, PinKind::Ground, "Ground", 0, NULL },

	{ 1, kJ6 + 1, PinKind::Gpio, "PB3", 1, NULL },
	{ 2, kJ6 + 2, PinKind::Gpio, "PM3", 1, NULL },
	{ 3, kJ6 + 3, PinKind::Gpio, "PM4", 1, NULL },
	{ 4, -1, PinKind::Ground, "Ground", 1, NULL },
	{ 5, kJ6 + 5, PinKind::Gpio, "PB6", 1, "I2S0 LRCK on the board" },
	{ 6, kJ6 + 6, PinKind::Gpio, "PB4", 1, "I2S0 MCLK on the board" },
	{ 7, kJ6 + 7, PinKind::Gpio, "PB8", 1, "I2S0 DIN0 on the board" },
	{ 8, kJ6 + 8, PinKind::Gpio, "PB7", 1, "I2S0 DOUT0 on the board" },
	{ 9, -1, PinKind::Ground, "Ground", 1, NULL },
	{ 10, kJ6 + 10, PinKind::Gpio, "PG0", 1, NULL },
	{ 11, kJ6 + 11, PinKind::Gpio, "PG1", 1, NULL },
	{ 12, kJ6 + 12, PinKind::Gpio, "PG2", 1, NULL },
	{ 13, kJ6 + 13, PinKind::Gpio, "PG3", 1, NULL },
	{ 14, kJ6 + 14, PinKind::Gpio, "PG4", 1, NULL },
	{ 15, kJ6 + 15, PinKind::Gpio, "PG5", 1, NULL },
};

const Connector kCubieConnectors[] = {
	{ "J11", 0, 30, 2 },
	{ "J6", 30, 15, 1 }
};

// the A733's functions 2 to 13, by multiplexer value
const int kCubieAltCount = 13;		// and 14, the interrupt

struct CubieFunctions {
	int8_t		gpio;
	const char*	signals[12];
};

const CubieFunctions kCubieFunctions[] = {
	{ 3, { "LCD1", "PWM1-5", "UART3-RX", "UART2-CTS", "TWI7-SDA", "TWI3-SDA",
		"TWI11-SDA", "LCD0-D9" } },
	{ 5, { "LCD1", "PWM1-4", "UART3-TX", "UART2-RTS", "TWI7-SCK", "TWI3-SCK",
		"TWI11-SCK", "LCD0-D8" } },
	{ 7, { "UART2-TX", "UART0-TX", "SPI2-CS0", "DSI-TRIG-LCD-TE1", "LCD0-D0",
		"JTAG-MS" } },
	{ 8, { "UART0-TX", "I2S0-DIN2", "I2S0-DOUT2", "PWM1-1", "WATCHDOG-SIG",
		"LCD0-D16", "TWI8-SCK", "TWI0-SCK" } },
	{ 10, { "UART0-RX", "I2S0-DIN3", "I2S0-DOUT3", "PWM1-2", "PLL-LOCK-DBG",
		"LCD0-D17", "TWI8-SDA", "TWI0-SDA" } },
	{ 11, { "UART2-RX", "UART0-RX", "SPI2-CLK", NULL, "LCD0-D1",
		"JTAG-CK" } },
	{ 12, { NULL, "I2S0-BCLK", "SPI2-CS2", "PWM0-1", "LCD0-D17",
		"TRACE-DATA0", "TWI1-SDA" } },
	{ 13, { "S-JTAG-DO", "S-UART0-TX", "S-SPI0-MOSI", "S-IR-RX",
		"S-PWM0-4" } },
	{ 15, { "S-JTAG-DI", "S-UART0-RX", "S-SPI0-MISO", NULL, "S-PWM0-5" } },
	{ 16, { "LCD1", "PWM1-6", "UART4-TX", NULL, NULL, "TWI4-SCK",
		"SPI3-CLK" } },
	{ 18, { "LCD1", "PWM1-7", "UART4-RX", NULL, NULL, "TWI4-SDA",
		"SPI3-MOSI" } },
	{ 19, { "LCD0-D18", "LVDS1-D1P", "DSI1-D1P", "EINK-D12", "SPI1-MOSI",
		"PWM1-2" } },
	{ 21, { "LCD0-D19", "LVDS1-D1N", "DSI1-D1N", "EINK-D13", "SPI1-MISO",
		"PWM1-3" } },
	{ 22, { "S-JTAG-CK", "S-TWI2-SDA", "S-SPI0-CLK", NULL, "S-PWM0-3" } },
	{ 23, { "LCD0-D15", "LVDS1-D0N", "DSI1-D0N", "EINK-D11", "SPI1-CLK",
		"PWM1-1" } },
	{ 24, { "LCD0-D14", "LVDS1-D0P", "DSI1-D0P", "EINK-D10", "SPI1-CS0",
		"PWM1-0" } },
	{ 26, { "LCD0-D20", "LVDS1-D2P", "DSI1-CKP", "EINK-D14", "SPI1-HOLD",
		"UART3-RTS" } },
	{ 27, { "LCD0-D23", "LVDS1-CKN", "DSI1-D2N", "EINK-LEH", "TWI2-SDA",
		"UART3-RX" } },
	{ 28, { "LCD0-D22", "LVDS1-CKP", "DSI1-D2P", "EINK-OEH", "TWI2-SCK",
		"UART3-TX" } },
	{ 29, { "UART2-RTS", NULL, "SPI2-MOSI", "HDMI-SCL", "LCD0-D8", "JTAG-DO",
		"TWI0-SCK" } },
	{ kJ6 + 1, { "UART2-CTS", NULL, "SPI2-MISO", "HDMI-SDA", "LCD0-D9",
		"JTAG-DI", "TWI0-SDA" } },
	{ kJ6 + 2, { "S-JTAG-DI", "S-SPI0-MISO", "S-UART0-RX", "S-UART1-RX",
		"S-PWM0-5" } },
	{ kJ6 + 3, { "S-UART0-TX", "S-TWI2-SCK", "S-TWI1-SCK", "S-UART1-TX",
		"S-PWM0-0", "S-IR-RX" } },
	{ kJ6 + 5, { "CLK-FANOUT1", "I2S0-LRCK", "SPI2-CS3", "PWM0-2", "PWM0-8",
		"TRACE-DATA1" } },
	{ kJ6 + 6, { "PWM0-0", "I2S0-MCLK", "SPI2-CS1", "HDMI-CEC", "LCD0-D16",
		"TRACE-CLK", "TWI1-SCK" } },
	{ kJ6 + 7, { "CLK-FANOUT3", "I2S0-DIN0", "I2S0-DOUT1", "PWM1-0",
		"OWA0-OUT", "TRACE-DATA3", "TWI1-SDA" } },
	{ kJ6 + 8, { "CLK-FANOUT2", "I2S0-DOUT0", "I2S0-DIN1", "PWM0-9",
		"OWA0-IN", "TRACE-DATA2", "TWI1-SCK" } },
	{ kJ6 + 10, { "SDC1-CLK", "LCD0-D0", "PWM1-1", "LPC-LAD0" } },
	{ kJ6 + 11, { "SDC1-CMD", "LCD0-D1", "PWM1-2", "LPC-LPME" } },
	{ kJ6 + 12, { "SDC1-D0", "LCD0-D8", "PWM1-3", "LPC-LPCPD" } },
	{ kJ6 + 13, { "SDC1-D1", "LCD0-D9", "PWM1-4", "LPC-LFRAME" } },
	{ kJ6 + 14, { "SDC1-D2", "LCD0-D16", "PWM1-5", "LPC-LSMI" } },
	{ kJ6 + 15, { "SDC1-D3", "LCD0-D17", "PWM1-6", "LPC-LCLK" } },
};

// what the A733's signal names' prefixes stand for
const Group kCubieGroups[] = {
	{ "TWI", "I2C" }, { "S-TWI", "I2C" }, { "SDC", "SD host" },
	{ "OWA", "S/PDIF" }, { "LCD", "parallel display" },
	{ "EINK", "e-ink display" }, { "LPC", "LPC bus" },
};


PinColor
CubieColor(const HeaderPin& pin)
{
	switch (pin.gpio) {
		case 3: case 5: case 27: case 28:			// I2C: light blue
			return { 173, 216, 230, 0, 0, 0 };
		case 19: case 21: case 23: case 24:			// SPI1: violet
			return { 238, 130, 238, 255, 255, 255 };
		default:									// orange
			return { 255, 165, 0, 255, 255, 255 };
	}
}


int
CubieAltIndex(int function)
{
	if (function >= 2 && function <= kSunxiFunctionInterrupt)
		return function - 2;
	return -1;
}


//	#pragma mark -


const HeaderPin*
Header(int& count)
{
	if (sBoard == Board::CubieA7S) {
		count = kCubiePinCount;
		return kCubieHeader;
	}
	count = kPiPinCount;
	return kPiHeader;
}

}	// namespace


void
SetBoard(Board board)
{
	sBoard = board;
}


Board
CurrentBoard()
{
	return sBoard;
}


const char*
BoardName(Board board)
{
	return board == Board::CubieA7S ? "Radxa Cubie A7S" : "Raspberry Pi 4";
}


const char*
BoardId(Board board)
{
	return board == Board::CubieA7S ? "cubie-a7s" : "";
}


bool
BoardForId(const std::string& id, Board& board)
{
	if (id.empty())
		board = Board::RaspberryPi4;
	else if (id == "cubie-a7s")
		board = Board::CubieA7S;
	else
		return false;
	return true;
}


std::string
ConfigurablePinsText()
{
	int count = 0;
	for (int i = 0; i < HeaderPinCount(); i++) {
		if (HeaderPinAt(i).kind == PinKind::Gpio)
			count++;
	}
	char text[128];
	if (sBoard == Board::CubieA7S) {
		snprintf(text, sizeof(text), "%d (on the 30-pin J11 and the 15-pin J6"
			" headers)", count);
	} else
		snprintf(text, sizeof(text), "%d (GPIO 2 to 27 on the 40-pin header)",
			count);
	return text;
}


const char*
BoardLayoutTip()
{
	return sBoard == Board::CubieA7S
		? "Board pin layout: the pins as on the J11 and J6 headers"
		: "Board pin layout: the pins as on the 40-pin header";
}


const char*
GpioOrderName()
{
	return sBoard == Board::CubieA7S ? "Port" : "BCM";
}


int
ConnectorCount()
{
	return sBoard == Board::CubieA7S ? 2 : 1;
}


const Connector&
ConnectorAt(int index)
{
	if (sBoard == Board::CubieA7S)
		return kCubieConnectors[index == 1 ? 1 : 0];
	return kPiConnectors[0];
}


int
HeaderPinCount()
{
	int count;
	Header(count);
	return count;
}


const HeaderPin&
HeaderPinAt(int index)
{
	int count;
	const HeaderPin* header = Header(count);
	if (index < 0 || index >= count)
		index = 0;
	return header[index];
}


const HeaderPin*
HeaderPinForGpio(int gpio)
{
	if (gpio < 0)
		return NULL;
	int count;
	const HeaderPin* header = Header(count);
	for (int i = 0; i < count; i++) {
		if (header[i].gpio == gpio)
			return &header[i];
	}
	return NULL;
}


bool
IsConfigurable(int gpio)
{
	const HeaderPin* pin = HeaderPinForGpio(gpio);
	return pin != NULL && pin->kind == PinKind::Gpio;
}


std::vector<int>
GpioOrder()
{
	std::vector<int> pins;
	if (sBoard == Board::CubieA7S) {
		// by port, then number: PB0, PB1 ... PM4
		std::vector<const HeaderPin*> sorted;
		for (const HeaderPin& pin : kCubieHeader) {
			if (pin.kind == PinKind::Gpio)
				sorted.push_back(&pin);
		}
		for (size_t i = 1; i < sorted.size(); i++) {
			for (size_t j = i; j > 0; j--) {
				const HeaderPin* a = sorted[j - 1];
				const HeaderPin* b = sorted[j];
				bool later = a->name[1] > b->name[1]
					|| (a->name[1] == b->name[1]
						&& atoi(a->name + 2) > atoi(b->name + 2));
				if (!later)
					break;
				sorted[j - 1] = b;
				sorted[j] = a;
			}
		}
		for (const HeaderPin* pin : sorted)
			pins.push_back(pin->gpio);
		return pins;
	}

	for (int gpio = 0; gpio < kBcmPinCount; gpio++) {
		if (IsConfigurable(gpio))
			pins.push_back(gpio);
	}
	return pins;
}


std::string
PinLabel(int gpio)
{
	const HeaderPin* pin = HeaderPinForGpio(gpio);
	char label[64];
	if (sBoard == Board::CubieA7S) {
		if (pin == NULL)
			snprintf(label, sizeof(label), "pin %d", gpio);
		else {
			snprintf(label, sizeof(label), "%s (%s pin %u)", pin->name,
				ConnectorAt(pin->connector).name, pin->board);
		}
	} else {
		snprintf(label, sizeof(label), "GPIO%d (pin %u)", gpio,
			pin != NULL ? pin->board : 0);
	}
	return label;
}


std::string
PinPosition(const HeaderPin& pin)
{
	char text[32];
	const char* connector = ConnectorAt(pin.connector).name;
	if (connector != NULL)
		snprintf(text, sizeof(text), "%s pin %u", connector, pin.board);
	else
		snprintf(text, sizeof(text), "Pin %u", pin.board);
	return text;
}


PinColor
ColorFor(const HeaderPin& pin)
{
	switch (pin.kind) {
		case PinKind::Power3V3:
			return { 255, 235, 4, 0, 0, 0 };		// yellow
		case PinKind::Power5V:
			return { 230, 30, 30, 255, 255, 255 };	// red
		case PinKind::Ground:
			return { 20, 20, 20, 255, 255, 255 };	// black
		case PinKind::Reserved:
			return { 150, 150, 150, 0, 0, 0 };		// grey: reserved
		case PinKind::Gpio:
			break;
	}
	return sBoard == Board::CubieA7S ? CubieColor(pin) : PiColor(pin);
}


int
AltCount()
{
	return sBoard == Board::CubieA7S ? kCubieAltCount : 6;
}


const char*
AltName(int alt)
{
	static const char* const kPiNames[] = {
		"ALT0", "ALT1", "ALT2", "ALT3", "ALT4", "ALT5"
	};
	static const char* const kCubieNames[] = {
		"F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
		"F13", "EINT"
	};
	if (alt < 0 || alt >= AltCount())
		return NULL;
	return sBoard == Board::CubieA7S ? kCubieNames[alt] : kPiNames[alt];
}


int
AltIndex(int function)
{
	return sBoard == Board::CubieA7S ? CubieAltIndex(function)
		: PiAltIndex(function);
}


const char*
AltSignal(int gpio, int alt)
{
	if (sBoard == Board::CubieA7S) {
		// the interrupt function (alt 12) has no signal of its own
		if (alt < 0 || alt >= 12)
			return NULL;
		for (const CubieFunctions& functions : kCubieFunctions) {
			if (functions.gpio == gpio)
				return functions.signals[alt];
		}
		return NULL;
	}

	if (gpio < 0 || gpio >= kBcmPinCount || alt < 0 || alt > 5)
		return NULL;
	return kPiAlt[gpio][alt];
}


const char*
FunctionSelectName(int function)
{
	return AltName(AltIndex(function));
}


bool
IsSwitchedOff(int function)
{
	return sBoard == Board::CubieA7S && function == kSunxiFunctionOff;
}


std::string
SignalGroup(const char* signal)
{
	if (signal == NULL)
		return "";

	if (sBoard == Board::CubieA7S) {
		// the name up to the last dash: "UART2-TX" is UART2's
		std::string name = signal;
		size_t dash = name.rfind('-');
		if (dash == std::string::npos || dash == 0)
			return "";
		std::string group = name.substr(0, dash);
		for (const Group& known : kCubieGroups) {
			if (strncmp(signal, known.prefix, strlen(known.prefix)) == 0) {
				group += " (";
				group += known.name;
				group += ")";
				break;
			}
		}
		return group;
	}

	for (const Group& group : kPiGroups) {
		if (strncmp(signal, group.prefix, strlen(group.prefix)) == 0)
			return group.name;
	}
	return "";
}


bool
IsConsole(int gpio, int function)
{
	if (sBoard == Board::CubieA7S)
		return gpio == 8 || gpio == 10;
	return (gpio == 14 || gpio == 15) && AltIndex(function) == 0;
}


Pull
DefaultPull(int gpio)
{
	// the A733's pins have no pull after reset
	if (sBoard == Board::CubieA7S)
		return Pull::None;
	return gpio >= 0 && gpio <= 8 ? Pull::Up : Pull::Down;
}

}	// namespace airpins
