/*
 * Copyright 2026, air/OS contributors.
 * Distributed under the terms of the MIT License.
 *
 * The header layout, pin colours and the choice of configurable pins follow
 * pigg (https://github.com/andrewdavidmackenzie/pigg, Apache-2.0).
 */
#pragma once

#include <stdint.h>
#include <string>
#include <vector>

#include "Config.h"


namespace airpins {

/*!	The boards whose headers AirPins knows. The driver says which one it
	drives; the pins' numbers and function values are that board's.
*/
enum class Board {
	RaspberryPi4,	// /dev/misc/rpi_gpio: BCM GPIO numbers
	CubieA7S		// /dev/misc/sunxi_gpio: header positions (J6 n = 30 + n)
};

enum class PinKind {
	Power3V3,
	Power5V,
	Ground,
	Gpio,		// configurable
	Reserved	// a GPIO the system uses (the Pi's ID EEPROM, the console)
};

struct HeaderPin {
	uint8_t		board;		// the physical pin number on its connector
	int8_t		gpio;		// the driver's pin number, or -1
	PinKind		kind;
	const char*	name;		// "GPIO17", "PB0", "3V3", "Ground"...
	uint8_t		connector;	// index of ConnectorAt()
	const char*	note;		// what the board uses the pin for, or NULL
};

struct Connector {
	const char*	name;		// "J11", or NULL for the Pi's only header
	int			first;		// index of its first pin in HeaderPinAt()
	int			pinCount;
	int			columns;	// 2: odd pins left, even pins right
};

struct PinColor {
	uint8_t red, green, blue;
	uint8_t textRed, textGreen, textBlue;
};

//! The size of the driver's pin arrays: pin numbers are below it.
static const int kMaxGpioPins = 64;

// function values all boards share (the BCM2711's function select, the
// A733's pin multiplexer)
enum {
	kFunctionInput = 0,
	kFunctionOutput = 1
};

// the BCM2711's other function select values
enum {
	kFunctionAlt5 = 2,
	kFunctionAlt4 = 3,
	kFunctionAlt0 = 4,
	kFunctionAlt1 = 5,
	kFunctionAlt2 = 6,
	kFunctionAlt3 = 7
};

// the A733's: 2 to 13 peripherals, then these
enum {
	kSunxiFunctionInterrupt = 14,
	kSunxiFunctionOff = 15
};

//! The board the tables describe; the Raspberry Pi 4 until set.
void SetBoard(Board board);
Board CurrentBoard();
//! "Raspberry Pi 4", "Radxa Cubie A7S"
const char* BoardName(Board board);
//! The board's name in configuration files: "" for the Pi (pigg's files).
const char* BoardId(Board board);
bool BoardForId(const std::string& id, Board& board);
//! "26 (GPIO 2 to 27 on the 40-pin header)" style.
std::string ConfigurablePinsText();
//! What the board layout's tool tip says.
const char* BoardLayoutTip();
//! What the GPIO order layout is called: "BCM", "Port".
const char* GpioOrderName();

int ConnectorCount();
const Connector& ConnectorAt(int index);

int HeaderPinCount();
const HeaderPin& HeaderPinAt(int index);	// in connector, then board order
const HeaderPin* HeaderPinForGpio(int gpio);	// NULL if not on the header
bool IsConfigurable(int gpio);
//! The configurable pins in the board's GPIO order.
std::vector<int> GpioOrder();

//! "GPIO17 (pin 11)", "PB0 (J11 pin 7)"
std::string PinLabel(int gpio);
//! "Pin 11", "J11 pin 7"
std::string PinPosition(const HeaderPin& pin);

PinColor ColorFor(const HeaderPin& pin);

//! How many alternate functions a pin can have, and their names ("ALT0",
//! "F2").
int AltCount();
const char* AltName(int alt);
//! The alternate function of a function value, or -1: input, output, and
//! the A733's "off" are none.
int AltIndex(int function);
//! The signal of an alternate function of a GPIO ("TXD0"), or NULL.
const char* AltSignal(int gpio, int alt);
//! The name of a function value's alternate function ("ALT0"), or NULL.
const char* FunctionSelectName(int function);
//! Whether the function value is the A733's "off" (its state after reset).
bool IsSwitchedOff(int function);
//! A short description of what a signal belongs to ("UART0", "I2C1"), or
//! an empty string.
std::string SignalGroup(const char* signal);
//! Whether the pin in this function is the system's serial console.
bool IsConsole(int gpio, int function);

//! The pull resistor of a GPIO after reset.
Pull DefaultPull(int gpio);

}	// namespace airpins
