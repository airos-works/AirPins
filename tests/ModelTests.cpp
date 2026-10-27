/*
 * Copyright 2026, air/OS contributors.
 * Distributed under the terms of the MIT License.
 *
 * Tests of the parts of AirPins that do not need Haiku: they build and run
 * on any host (make check-host) as well as on Haiku (make check).
 */

#include <stdio.h>
#include <string.h>

#include "BoardInfo.h"
#include "Config.h"
#include "Json.h"
#include "PinHistory.h"
#include "PinTable.h"

using namespace airpins;

static int sFailures;
static int sChecks;

#define CHECK(condition) \
	do { \
		sChecks++; \
		if (!(condition)) { \
			sFailures++; \
			fprintf(stderr, "%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, \
				#condition); \
		} \
	} while (false)


static void
TestPiggFiles()
{
	// configs/andrews_board.pigg from pigg's repository
	Configuration config;
	std::string error;
	CHECK(ParseConfiguration(
		"{\"pin_functions\":{\"26\":{\"Input\":\"PullUp\"},"
		"\"17\":{\"Output\":true}}}", config, &error));
	CHECK(config.pins.size() == 2);
	CHECK(config.pins[26].mode == PinMode::Input);
	CHECK(config.pins[26].pull == Pull::Up);
	CHECK(config.pins[17].mode == PinMode::Output);
	CHECK(config.pins[17].level == 1);

	// the other spellings serde writes
	CHECK(ParseConfiguration(" {\n \"pin_functions\" : {\n"
		"  \"4\": {\"Input\": null},\n  \"5\": {\"Input\": \"None\"},\n"
		"  \"6\": {\"Input\": \"PullDown\"},\n  \"12\": {\"Output\": null},\n"
		"  \"13\": {\"Output\": false}\n } }\n", config, &error));
	CHECK(config.pins.size() == 5);
	CHECK(config.pins[4].pull == Pull::None);
	CHECK(config.pins[5].pull == Pull::None);
	CHECK(config.pins[6].pull == Pull::Down);
	CHECK(config.pins[12].mode == PinMode::Output && config.pins[12].level == -1);
	CHECK(config.pins[13].level == 0);

	// early pigg versions
	CHECK(ParseConfiguration("{\"configured_pins\":{\"21\":\"Output\"}}",
		config));
	CHECK(config.pins.size() == 1 && config.pins[21].mode == PinMode::Output);

	// pins that are not configurable on the header are skipped
	std::vector<int> skipped;
	CHECK(ParseConfiguration("{\"pin_functions\":{\"0\":{\"Input\":null},"
		"\"30\":{\"Output\":true},\"22\":{\"Output\":true}}}", config, &error,
		&skipped));
	CHECK(config.pins.size() == 1 && config.pins.count(22) == 1);
	CHECK(skipped.size() == 2);
}


static void
TestBadFiles()
{
	Configuration config;
	config.pins[2].mode = PinMode::Input;
	std::string error;
	CHECK(!ParseConfiguration("", config, &error));
	CHECK(!error.empty());
	CHECK(!ParseConfiguration("[1,2]", config, &error));
	CHECK(!ParseConfiguration("{\"pin_functions\":{\"x\":{\"Input\":null}}}",
		config, &error));
	CHECK(!ParseConfiguration("{\"pin_functions\":{\"2\":{\"Pwm\":1}}}",
		config, &error));
	CHECK(!ParseConfiguration("{\"pin_functions\":{\"2\":{\"Input\":\"Up\"}}}",
		config, &error));
	CHECK(!ParseConfiguration("{\"pin_functions\":{}", config, &error));
	CHECK(!ParseConfiguration("{\"pin_functions\":{}} x", config, &error));
	// a failed parse leaves the configuration alone
	CHECK(config.pins.size() == 1 && config.pins[2].mode == PinMode::Input);
}


static void
TestFormat()
{
	Configuration config;
	CHECK(FormatConfiguration(config) == "{\"pin_functions\":{}}");

	config.pins[26].mode = PinMode::Input;
	config.pins[26].pull = Pull::Up;
	config.pins[17].mode = PinMode::Output;
	config.pins[17].level = 1;
	config.pins[4].mode = PinMode::Input;
	config.pins[5].mode = PinMode::Output;
	config.pins[6].mode = PinMode::Output;
	config.pins[6].level = 0;
	std::string text = FormatConfiguration(config);
	CHECK(text == "{\"pin_functions\":{\"4\":{\"Input\":null},"
		"\"5\":{\"Output\":null},\"6\":{\"Output\":false},"
		"\"17\":{\"Output\":true},\"26\":{\"Input\":\"PullUp\"}}}");

	Configuration again;
	CHECK(ParseConfiguration(text, again));
	CHECK(again == config);
}


static void
TestJson()
{
	JsonValue value;
	CHECK(JsonValue::Parse("{\"a\":[1,-2.5e1,\"x\\u00e9\\n\"],\"b\":{}}",
		value));
	const JsonValue* a = value.Find("a");
	CHECK(a != NULL && a->IsArray() && a->Array().size() == 3);
	CHECK(a->Array()[1].Number() == -25);
	CHECK(a->Array()[2].String() == "x\xc3\xa9\n");
	CHECK(JsonValue::Quote("a\"b\\\n") == "\"a\\\"b\\\\\\n\"");

	std::string deep(100, '[');
	CHECK(!JsonValue::Parse(deep, value));
}


static void
TestBoards()
{
	BoardInfo info = DecodeRevision(0xc03115);
	CHECK(info.known);
	CHECK(info.model == "Raspberry Pi 4 Model B Rev 1.5");
	CHECK(info.processor == "BCM2711");
	CHECK(info.memory == "4 GB");
	CHECK(info.manufacturer == "Sony UK");
	CHECK(info.revisionCode == "c03115");

	info = DecodeRevision(0xd03114);
	CHECK(info.model == "Raspberry Pi 4 Model B Rev 1.4");
	CHECK(info.memory == "8 GB");

	info = DecodeRevision(0xc03130);
	CHECK(info.model == "Raspberry Pi 400 Rev 1.0");

	info = DecodeRevision(0x902120);
	CHECK(info.model == "Raspberry Pi Zero 2 W Rev 1.0");
	CHECK(info.memory == "512 MB" && info.processor == "BCM2837");

	// old-style codes and nothing at all
	CHECK(!DecodeRevision(0x000e).known);
	CHECK(!DecodeRevision(0).known);
}


static void
TestPinTable()
{
	SetBoard(Board::RaspberryPi4);
	int configurable = 0;
	for (int i = 0; i < HeaderPinCount(); i++) {
		const HeaderPin& pin = HeaderPinAt(i);
		CHECK(pin.board == i + 1);
		if (pin.kind == PinKind::Gpio) {
			configurable++;
			CHECK(IsConfigurable(pin.gpio));
			CHECK(HeaderPinForGpio(pin.gpio) == &pin);
		}
	}
	CHECK(HeaderPinCount() == 40 && ConnectorCount() == 1);
	CHECK(configurable == 26);
	CHECK(GpioOrder().size() == 26 && GpioOrder().front() == 2);
	CHECK(!IsConfigurable(0) && !IsConfigurable(1) && !IsConfigurable(28));
	CHECK(HeaderPinForGpio(14)->board == 8);
	CHECK(HeaderPinForGpio(21)->board == 40);
	CHECK(PinLabel(17) == "GPIO17 (pin 11)");
	CHECK(strcmp(AltSignal(14, 0), "TXD0") == 0);
	CHECK(strcmp(AltSignal(15, 5), "RXD1") == 0);
	CHECK(AltSignal(16, 0) == NULL);
	CHECK(SignalGroup("TXD0") == "UART0");
	CHECK(SignalGroup("SDA1") == "I2C1");
	CHECK(SignalGroup("SPI0_MOSI") == "SPI0");
	CHECK(SignalGroup("SD0_CLK") == "SD host 0");
	CHECK(SignalGroup("SD5") == "secondary memory");
	CHECK(SignalGroup(NULL).empty());
	CHECK(AltIndex(kFunctionAlt0) == 0 && AltIndex(kFunctionAlt5) == 5);
	CHECK(AltIndex(kFunctionInput) == -1);
	CHECK(strcmp(FunctionSelectName(kFunctionAlt4), "ALT4") == 0);
	CHECK(DefaultPull(8) == Pull::Up && DefaultPull(9) == Pull::Down);
	CHECK(IsConsole(14, kFunctionAlt0) && !IsConsole(14, kFunctionInput));
	CHECK(!IsSwitchedOff(15));

	PinColor ground = ColorFor(HeaderPinAt(5));
	CHECK(HeaderPinAt(5).kind == PinKind::Ground);
	CHECK(ground.red == 20);
}


static void
TestCubieTable()
{
	SetBoard(Board::CubieA7S);
	CHECK(HeaderPinCount() == 45 && ConnectorCount() == 2);
	int configurable = 0;
	for (int c = 0; c < ConnectorCount(); c++) {
		const Connector& connector = ConnectorAt(c);
		for (int i = 0; i < connector.pinCount; i++) {
			const HeaderPin& pin = HeaderPinAt(connector.first + i);
			CHECK(pin.connector == c && pin.board == i + 1);
			// the driver numbers pins by position: J6 pin n is 30 + n
			if (pin.gpio >= 0)
				CHECK(pin.gpio == (c == 0 ? 0 : 30) + pin.board);
			if (pin.kind == PinKind::Gpio) {
				configurable++;
				CHECK(HeaderPinForGpio(pin.gpio) == &pin);
				// every pin's functions are known
				bool any = false;
				for (int alt = 0; alt < AltCount(); alt++)
					any |= AltSignal(pin.gpio, alt) != NULL;
				CHECK(any);
			}
		}
	}
	CHECK(configurable == 31);
	CHECK(ConfigurablePinsText().compare(0, 3, "31 ") == 0);
	CHECK(GpioOrder().size() == 31);
	CHECK(GpioOrder().front() == 7);		// PB0
	CHECK(GpioOrder().back() == 33);		// PM4
	CHECK(!IsConfigurable(8) && !IsConfigurable(10));	// the console
	CHECK(HeaderPinForGpio(8)->kind == PinKind::Reserved);
	CHECK(!IsConfigurable(1) && !IsConfigurable(34) && !IsConfigurable(0));
	CHECK(PinLabel(7) == "PB0 (J11 pin 7)");
	CHECK(PinLabel(40) == "PG0 (J6 pin 10)");
	CHECK(PinPosition(*HeaderPinForGpio(40)) == "J6 pin 10");
	CHECK(strcmp(AltSignal(3, AltIndex(6)), "TWI7-SDA") == 0);
	CHECK(strcmp(AltSignal(12, AltIndex(3)), "I2S0-BCLK") == 0);
	CHECK(AltSignal(12, AltIndex(2)) == NULL);
	CHECK(SignalGroup("UART2-TX") == "UART2");
	CHECK(SignalGroup("TWI7-SDA") == "TWI7 (I2C)");
	CHECK(SignalGroup("S-UART0-TX") == "S-UART0");
	CHECK(SignalGroup("LCD1").empty());
	CHECK(AltIndex(kFunctionInput) == -1 && AltIndex(kFunctionOutput) == -1);
	CHECK(AltIndex(kSunxiFunctionOff) == -1 && IsSwitchedOff(15));
	CHECK(strcmp(FunctionSelectName(2), "F2") == 0);
	CHECK(strcmp(FunctionSelectName(kSunxiFunctionInterrupt), "EINT") == 0);
	CHECK(DefaultPull(7) == Pull::None);
	CHECK(IsConsole(8, 2) && !IsConsole(7, 2));

	// configurations name the board
	Configuration config;
	config.pins[26].mode = PinMode::Output;
	config.pins[26].level = 1;
	std::string text = FormatConfiguration(config);
	CHECK(text == "{\"board\":\"cubie-a7s\",\"pin_functions\":"
		"{\"26\":{\"Output\":true}}}");
	Configuration read;
	std::string error;
	CHECK(ParseConfiguration(text, read, &error) && read == config);
	// a Pi's file does not open here, nor this one on the Pi
	CHECK(!ParseConfiguration("{\"pin_functions\":{\"26\":{\"Output\":"
		"true}}}", read, &error));
	CHECK(error.find("Raspberry Pi 4") != std::string::npos);
	CHECK(!ParseConfiguration("{\"board\":\"x\",\"pin_functions\":{}}",
		read, &error));
	SetBoard(Board::RaspberryPi4);
	CHECK(!ParseConfiguration(text, read, &error));
	CHECK(FormatConfiguration(config).find("board") == std::string::npos);
}


static void
TestHistory()
{
	PinHistory history(8);
	CHECK(!history.HasLevel());
	CHECK(history.LevelAt(100) == -1);

	history.Add(100, true);
	history.Add(150, true);		// no change
	history.Add(200, false);
	history.Add(300, true);
	CHECK(history.Changes().size() == 3);
	CHECK(history.Level());
	CHECK(history.LevelAt(99) == -1);
	CHECK(history.LevelAt(100) == 1);
	CHECK(history.LevelAt(250) == 0);
	CHECK(history.LevelAt(1000) == 1);
	CHECK(history.ChangesIn(100, 300) == 2);
	CHECK(history.ChangesIn(0, 1000) == 3);

	// a pulse: two changes at the same time
	history.Add(400, false);
	history.Add(400, true);
	CHECK(history.Changes().size() == 5);
	CHECK(history.ChangesIn(399, 400) == 2);

	// time does not go back
	history.Add(350, false);
	CHECK(history.LastChange() == 400 && !history.Level());

	history.Trim(250);
	CHECK(history.Changes().front().time == 200);
	CHECK(history.LevelAt(250) == 0);

	for (int i = 0; i < 20; i++)
		history.Add(1000 + i, i % 2 == 0);
	CHECK(history.Changes().size() == 8);
}


int
main()
{
	TestPiggFiles();
	TestBadFiles();
	TestFormat();
	TestJson();
	TestBoards();
	TestPinTable();
	TestCubieTable();
	TestHistory();

	printf("%d checks, %d failed\n", sChecks, sFailures);
	return sFailures == 0 ? 0 : 1;
}
