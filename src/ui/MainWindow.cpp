/*
 * Copyright 2026, air/OS contributors.
 * Distributed under the terms of the MIT License.
 */

#include "MainWindow.h"

#include <Alert.h>
#include <Application.h>
#include <ControlLook.h>
#include <Directory.h>
#include <File.h>
#include <FilePanel.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <MessageRunner.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <StringView.h>
#include <Url.h>

#include <algorithm>
#include <stdio.h>
#include <string.h>

#include "BoardInfo.h"
#include "DeviceWindow.h"
#include "EventPump.h"
#include "Gpio.h"
#include "IconButton.h"
#include "PinsView.h"
#include "Settings.h"


namespace airpins {

namespace {

const bigtime_t kTickPeriod = 100000;		// waveforms move 10 times a second
const bigtime_t kRefreshPeriod = 1000000;	// the pins' functions, once a second
const bigtime_t kMessageTime = 6000000;
const bigtime_t kHistoryKept = 70000000;	// a little more than the longest span
const off_t kMaxFileSize = 1024 * 1024;

const int kSpans[] = { 4, 16, 30, 60 };


BString
Label(int gpio)
{
	return BString(airpins::PinLabel(gpio).c_str());
}


BString
OrderLayoutTip()
{
	BString tip;
	tip.SetToFormat("%s pin layout: the GPIOs in %s order", GpioOrderName(),
		CurrentBoard() == Board::CubieA7S ? "port" : "number");
	return tip;
}

}	// namespace


MainWindow::MainWindow(const BMessage& settings, bool simulate)
	:
	BWindow(BRect(60, 60, 900, 700), kAppName, B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS
			| B_QUIT_ON_WINDOW_CLOSE),
	fHardware(NULL),
	fPump(NULL),
	fSimulated(false),
	fHasFile(false),
	fQuitAfterSave(false),
	fApplying(false),
	fOpenPanel(NULL),
	fSavePanel(NULL),
	fTicker(NULL),
	fRefresher(NULL),
	fMessageClearer(NULL),
	fSpanSeconds(16)
{
	for (int gpio = 0; gpio < kMaxGpioPins; gpio++)
		fPins[gpio].gpio = gpio;

	BMenuBar* menuBar = new BMenuBar("menu bar");
	_BuildMenu(menuBar);

	auto toolButton = [this](const char* name, IconId icon, BMessage* message,
			const char* tip) {
		IconButton* button = new IconButton(name, NULL, icon, message);
		button->SetFlat(true);
		button->SetToolTip(tip);
		button->SetTarget(this);
		return button;
	};
	BMessage* board = new BMessage(kMsgSetLayout);
	board->AddInt32("layout", (int32)LayoutMode::Board);
	BMessage* order = new BMessage(kMsgSetLayout);
	order->AddInt32("layout", (int32)LayoutMode::Bcm);
	BMessage* compact = new BMessage(kMsgSetLayout);
	compact->AddInt32("layout", (int32)LayoutMode::Compact);
	fLayoutButtons[0] = toolButton("board", kIconBoardLayout, board,
		BoardLayoutTip());
	fLayoutButtons[1] = toolButton("bcm", kIconBcmLayout, order,
		OrderLayoutTip());
	fLayoutButtons[2] = toolButton("compact", kIconCompactLayout, compact,
		"Compact layout: only the configured pins");
	for (IconButton* button : fLayoutButtons)
		button->SetBehavior(BButton::B_TOGGLE_BEHAVIOR);

	fPinsView = new PinsView(fPins, this);
	BScrollView* scrollView = new BScrollView("scroll", fPinsView, 0, true,
		true, B_NO_BORDER);

	fStatusView = new BStringView("status", "");
	fMessageView = new BStringView("message", "");
	// the status takes what room there is, the message what it needs
	fStatusView->SetExplicitMinSize(BSize(50, B_SIZE_UNSET));
	fStatusView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	fMessageView->SetAlignment(B_ALIGN_RIGHT);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(menuBar)
		.AddGroup(B_HORIZONTAL, 0)
			.SetInsets(B_USE_HALF_ITEM_INSETS, 2, B_USE_HALF_ITEM_INSETS, 2)
			.Add(toolButton("open", kIconOpen, new BMessage(kMsgOpen),
				"Open a configuration"))
			.Add(toolButton("save", kIconSave, new BMessage(kMsgSave),
				"Save the configuration"))
			.AddStrut(be_control_look->DefaultItemSpacing())
			.Add(fLayoutButtons[0])
			.Add(fLayoutButtons[1])
			.Add(fLayoutButtons[2])
			.AddStrut(be_control_look->DefaultItemSpacing())
			.Add(toolButton("reset", kIconReset, new BMessage(kMsgResetPins),
				"Set every pin to Unused"))
			.Add(toolButton("device", kIconDevice,
				new BMessage(kMsgDeviceDetails), "Device details"))
			.AddGlue()
		.End()
		.Add(new BSeparatorView(B_HORIZONTAL))
		.Add(scrollView)
		.Add(new BSeparatorView(B_HORIZONTAL))
		.AddGroup(B_HORIZONTAL)
			.SetInsets(B_USE_WINDOW_SPACING, B_USE_HALF_ITEM_SPACING,
				B_USE_WINDOW_SPACING, B_USE_HALF_ITEM_SPACING)
			.Add(fStatusView, 10)
			.Add(fMessageView, 1)
		.End();

	int32 span;
	if (settings.FindInt32("span", &span) == B_OK)
		fSpanSeconds = span;
	_SetSpan(fSpanSeconds);
	int32 layout;
	if (settings.FindInt32("layout", &layout) != B_OK || layout < 0
		|| layout > 2) {
		layout = 0;
	}
	_SetLayoutMode((LayoutMode)layout);

	_UseHardware(simulate);

	BRect frame;
	if (settings.FindRect("frame", &frame) == B_OK && frame.IsValid()) {
		BRect screen = BScreen(this).Frame();
		if (screen.Contains(frame.LeftTop() + BPoint(40, 10))) {
			MoveTo(frame.LeftTop());
			ResizeTo(frame.Width(), frame.Height());
		} else
			_FitToContent();
	} else
		_FitToContent();

	BMessage tick(kMsgTick);
	fTicker = new BMessageRunner(BMessenger(this), &tick, kTickPeriod);
	BMessage refresh(kMsgRefreshState);
	fRefresher = new BMessageRunner(BMessenger(this), &refresh,
		kRefreshPeriod);

	_UpdateTitle();
	_UpdateStatus();
}


MainWindow::~MainWindow()
{
	delete fTicker;
	delete fRefresher;
	delete fMessageClearer;
	_ReleaseHardware();
	delete fOpenPanel;
	delete fSavePanel;
}


//! The GPIO order layout's menu item says the board's order.
void
MainWindow::_UpdateLayoutLabels()
{
	BString label;
	label.SetToFormat("%s pin layout", GpioOrderName());
	fLayoutItems[1]->SetLabel(label);
}


void
MainWindow::_BuildMenu(BMenuBar* menuBar)
{
	BMenu* file = new BMenu("File");
	file->AddItem(new BMenuItem("Open configuration" B_UTF8_ELLIPSIS,
		new BMessage(kMsgOpen), 'O'));
	fSaveItem = new BMenuItem("Save configuration", new BMessage(kMsgSave),
		'S');
	file->AddItem(fSaveItem);
	file->AddItem(new BMenuItem("Save configuration as" B_UTF8_ELLIPSIS,
		new BMessage(kMsgSaveAs), 'S', B_SHIFT_KEY));
	file->AddSeparatorItem();
	file->AddItem(new BMenuItem("Set all pins to Unused",
		new BMessage(kMsgResetPins)));
	file->AddSeparatorItem();
	file->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q'));
	menuBar->AddItem(file);

	BMenu* view = new BMenu("View");
	const char* const kLayoutNames[] = {
		"Board pin layout", "BCM pin layout", "Compact layout"
	};
	for (int i = 0; i < 3; i++) {
		BMessage* message = new BMessage(kMsgSetLayout);
		message->AddInt32("layout", i);
		fLayoutItems[i] = new BMenuItem(kLayoutNames[i], message, '1' + i);
		view->AddItem(fLayoutItems[i]);
	}
	view->AddSeparatorItem();
	BMenu* spans = new BMenu("Waveform time span");
	for (int i = 0; i < 4; i++) {
		BMessage* message = new BMessage(kMsgSetSpan);
		message->AddInt32("seconds", kSpans[i]);
		BString label;
		label.SetToFormat("%d seconds", kSpans[i]);
		fSpanItems[i] = new BMenuItem(label, message);
		spans->AddItem(fSpanItems[i]);
	}
	spans->SetRadioMode(true);
	view->AddItem(spans);
	menuBar->AddItem(view);

	BMenu* device = new BMenu("Device");
	device->AddItem(new BMenuItem("Device details" B_UTF8_ELLIPSIS,
		new BMessage(kMsgDeviceDetails), 'I'));
	device->AddSeparatorItem();
	fSimulatedItem = new BMenuItem("Use simulated pins",
		new BMessage(kMsgUseSimulated));
	device->AddItem(fSimulatedItem);
	menuBar->AddItem(device);

	BMenu* help = new BMenu("Help");
	help->AddItem(new BMenuItem("pigg, the original" B_UTF8_ELLIPSIS,
		new BMessage(kMsgVisitPigg)));
	help->AddSeparatorItem();
	BMenuItem* about = new BMenuItem("About AirPins" B_UTF8_ELLIPSIS,
		new BMessage(B_ABOUT_REQUESTED));
	about->SetTarget(be_app);
	help->AddItem(about);
	menuBar->AddItem(help);
}


bool
MainWindow::QuitRequested()
{
	if (!_AskToSave("quitting"))
		return false;

	BMessage settings;
	StoreSettings(settings);
	SaveSettings(settings);
	return true;
}


void
MainWindow::StoreSettings(BMessage& settings) const
{
	settings.MakeEmpty();
	settings.AddRect("frame", Frame());
	settings.AddInt32("layout", (int32)fPinsView->Mode());
	settings.AddInt32("span", fSpanSeconds);
}


void
MainWindow::MenusBeginning()
{
	fSimulatedItem->SetMarked(fSimulated);
	for (int i = 0; i < 3; i++)
		fLayoutItems[i]->SetMarked((int)fPinsView->Mode() == i);
	for (int i = 0; i < 4; i++)
		fSpanItems[i]->SetMarked(kSpans[i] == fSpanSeconds);
	BWindow::MenusBeginning();
}


void
MainWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgPinEvents:
			_HandleEvents(message);
			break;

		case kMsgSetPinMode:
		case kMsgSetPinPull:
		case kMsgSetPinFunction:
		{
			int32 gpio;
			if (message->FindInt32("pin", &gpio) != B_OK || !IsConfigurable(gpio))
				break;
			PinSetting setting = fPins[gpio].setting;
			int32 value;
			if (message->what != kMsgSetPinPull
				&& message->FindInt32("mode", &value) == B_OK) {
				PinMode mode = (PinMode)value;
				if (mode == PinMode::Input && setting.mode != PinMode::Input) {
					// the pull the pin has after reset, as pigg does
					setting.pull = DefaultPull(gpio);
				}
				if (mode == PinMode::Output && setting.mode != PinMode::Output)
					setting.level = 0;
				setting.mode = mode;
			}
			if (message->what != kMsgSetPinMode
				&& message->FindInt32("pull", &value) == B_OK
				&& setting.mode == PinMode::Input) {
				setting.pull = (Pull)value;
			}
			_SetPin(gpio, setting, true);
			break;
		}

		case kMsgToggleOutput:
		{
			int32 gpio;
			if (message->FindInt32("pin", &gpio) == B_OK)
				_ToggleOutput(gpio);
			break;
		}

		case kMsgHoldOutput:
		{
			int32 gpio;
			bool pressed;
			if (message->FindInt32("pin", &gpio) == B_OK
				&& message->FindBool("pressed", &pressed) == B_OK) {
				_HoldOutput(gpio, pressed);
			}
			break;
		}

		case kMsgTick:
			_Tick();
			break;

		case kMsgRefreshState:
			_RefreshState();
			break;

		case kMsgClearMessage:
			delete fMessageClearer;
			fMessageClearer = NULL;
			fMessageView->SetText("");
			_UpdateStatus();
			break;

		case kMsgSetLayout:
		{
			int32 layout;
			if (message->FindInt32("layout", &layout) == B_OK && layout >= 0
				&& layout <= 2) {
				_SetLayoutMode((LayoutMode)layout);
				_FitToContent();
			}
			break;
		}

		case kMsgSetSpan:
		{
			int32 seconds;
			if (message->FindInt32("seconds", &seconds) == B_OK)
				_SetSpan(seconds);
			break;
		}

		case kMsgOpen:
			_Open();
			break;

		case B_REFS_RECEIVED:
		{
			entry_ref ref;
			if (message->FindRef("refs", &ref) == B_OK) {
				// from the open panel the question was asked already
				if (message->GetBool("ask", false)
					&& !_AskToSave("opening another configuration")) {
					break;
				}
				OpenFile(ref);
			}
			break;
		}

		case kMsgSave:
			_Save(false);
			break;

		case kMsgSaveAs:
			_Save(true);
			break;

		case B_SAVE_REQUESTED:
		{
			entry_ref directory;
			const char* name;
			if (message->FindRef("directory", &directory) == B_OK
				&& message->FindString("name", &name) == B_OK) {
				if (_WriteFile(directory, name) == B_OK && fQuitAfterSave)
					PostMessage(B_QUIT_REQUESTED);
			}
			fQuitAfterSave = false;
			break;
		}

		case B_CANCEL:
			fQuitAfterSave = false;
			break;

		case kMsgResetPins:
			_ResetPins();
			break;

		case kMsgDeviceDetails:
			_ShowDeviceDetails();
			break;

		case kMsgUseSimulated:
			_UseHardware(!fSimulated);
			break;

		case kMsgVisitPigg:
			BUrl(kPiggUrl, false).OpenWithPreferredApplication();
			break;

		default:
			BWindow::MessageReceived(message);
	}
}


//	#pragma mark - hardware


void
MainWindow::_UseHardware(bool simulated)
{
	Configuration configuration = _CurrentConfiguration();
	_ReleaseHardware();

	BString error;
	GpioHardware* hardware = simulated ? NULL : CreateLocalGpio(&error);
	if (hardware == NULL) {
		if (!simulated)
			_ShowMessage("No GPIO hardware found: the pins are simulated");
		// the pins of the board last seen, the Pi's at first
		hardware = CreateSimulatedGpio(CurrentBoard());
		simulated = true;
	}
	fHardware = hardware;
	fSimulated = simulated;
	fHardwareName = fHardware->Details().name;

	// another board's pins are other pins: nothing carries over
	Board board = fHardware->Details().board;
	if (board != CurrentBoard()) {
		SetBoard(board);
		configuration = Configuration();
		fLayoutButtons[0]->SetToolTip(BoardLayoutTip());
		fLayoutButtons[1]->SetToolTip(OrderLayoutTip());
		_UpdateLayoutLabels();
	}

	for (PinEntry& entry : fPins) {
		entry.setting = PinSetting();
		entry.history.Clear();
		entry.holding = false;
	}
	_RefreshState();

	fPump = new EventPump(fHardware, BMessenger(this));
	fPump->Start();

	_ApplyConfiguration(configuration, true);
	_UpdateStatus();
}


void
MainWindow::_ReleaseHardware()
{
	if (fPump != NULL) {
		fPump->Stop();
		delete fPump;
		fPump = NULL;
	}
	// the driver gives the pins back
	delete fHardware;
	fHardware = NULL;
}


bool
MainWindow::_SetPin(int gpio, const PinSetting& setting, bool ask)
{
	if (!IsConfigurable(gpio) || fHardware == NULL)
		return false;
	PinEntry& entry = fPins[gpio];
	if (entry.Configured() && setting == entry.setting) {
		fPinsView->UpdatePin(gpio);
		return true;
	}

	if (setting.mode != PinMode::Unused && !entry.Configured()) {
		if (entry.hardware.claimedElsewhere) {
			BString text;
			text.SetToFormat("%s is in use by another program.",
				Label(gpio).String());
			_ShowMessage(text);
			fPinsView->UpdatePin(gpio);
			return false;
		}
		if (ask && AltIndex(entry.hardware.function) >= 0
			&& !_ConfirmTakeOver(std::vector<int>(1, gpio))) {
			fPinsView->UpdatePin(gpio);
			return false;
		}
	}

	entry.holding = false;
	status_t status = fHardware->Configure(gpio, setting);
	if (status != B_OK) {
		BString text;
		text.SetToFormat("%s: %s", Label(gpio).String(), strerror(status));
		_ShowMessage(text);
		fPinsView->UpdatePin(gpio);
		return false;
	}

	bool wasConfigured = entry.Configured();
	if (entry.setting.mode != setting.mode)
		entry.history.Clear();
	entry.setting = setting;

	if (!fApplying) {
		_RefreshState();
		if (fPinsView->Mode() == LayoutMode::Compact
			&& wasConfigured != entry.Configured()) {
			// as pigg does: the window follows the compact layout's size
			fPinsView->Rebuild();
			_FitToContent();
		} else
			fPinsView->UpdatePin(gpio);
		_UpdateStatus();
	}
	return true;
}


void
MainWindow::_ApplyConfiguration(const Configuration& config, bool ask)
{
	// Pins that serve another function now are taken only when asked.
	std::vector<int> takeOver;
	for (const auto& entry : config.pins) {
		const PinEntry& pin = fPins[entry.first];
		if (entry.second.mode != PinMode::Unused && !pin.Configured()
			&& AltIndex(pin.hardware.function) >= 0) {
			takeOver.push_back(entry.first);
		}
	}
	bool useAlternates = takeOver.empty() || !ask
		|| _ConfirmTakeOver(takeOver);

	fApplying = true;
	std::vector<int> failed;
	for (int gpio = 0; gpio < kMaxGpioPins; gpio++) {
		if (!IsConfigurable(gpio))
			continue;
		auto found = config.pins.find(gpio);
		PinSetting setting;
		if (found != config.pins.end())
			setting = found->second;
		if (!useAlternates
			&& std::find(takeOver.begin(), takeOver.end(), gpio)
				!= takeOver.end()) {
			setting = PinSetting();
		}
		if (setting.mode == PinMode::Unused && !fPins[gpio].Configured())
			continue;
		if (!_SetPin(gpio, setting, false))
			failed.push_back(gpio);
	}
	fApplying = false;

	_RefreshState();
	fPinsView->Rebuild();
	_UpdateStatus();
	if (!failed.empty()) {
		BString text = "Not configured:";
		for (int gpio : failed)
			text << " " << Label(gpio);
		_ShowMessage(text);
	}
}


Configuration
MainWindow::_CurrentConfiguration() const
{
	Configuration config;
	for (const PinEntry& entry : fPins) {
		if (entry.Configured())
			config.pins[entry.gpio] = entry.setting;
	}
	return config;
}


bool
MainWindow::_Confirm(const BString& text, const char* proceed)
{
	BAlert* alert = new BAlert(kAppName, text, "Cancel", proceed, NULL,
		B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	alert->SetShortcut(0, B_ESCAPE);
	return alert->Go() == 1;
}


bool
MainWindow::_ConfirmTakeOver(const std::vector<int>& pins)
{
	BString text;
	for (int gpio : pins) {
		const PinHardwareState& hardware = fPins[gpio].hardware;
		const char* signal = AltSignal(gpio, AltIndex(hardware.function));
		std::string group = SignalGroup(signal);
		BString line;
		line.SetToFormat("%s is %s%s%s%s.\n", Label(gpio).String(),
			signal != NULL ? signal : FunctionSelectName(hardware.function),
			!group.empty() ? " of " : "", group.c_str(),
			IsConsole(gpio, hardware.function)
				? ", the system's serial console" : "");
		text << line;
	}
	text << "\n";
	if (pins.size() == 1) {
		text << "Use the pin anyway? It gets its function back when you set"
			" it to Unused, and when AirPins quits.";
	} else {
		text << "Use these pins anyway? They get their functions back when"
			" you set them to Unused, and when AirPins quits.";
	}
	return _Confirm(text, pins.size() == 1 ? "Use pin" : "Use pins");
}


void
MainWindow::_HandleEvents(BMessage* message)
{
	const GpioEvent* events;
	ssize_t size;
	if (message->FindData("events", B_RAW_TYPE, (const void**)&events, &size)
			!= B_OK) {
		return;
	}

	uint64 changed = 0;
	for (size_t i = 0; i < size / sizeof(GpioEvent); i++) {
		const GpioEvent& event = events[i];
		if (event.pin >= kMaxGpioPins)
			continue;
		PinEntry& entry = fPins[event.pin];
		if (!entry.Configured())
			continue;
		entry.history.Add(event.time, event.level);
		changed |= 1ull << event.pin;
	}
	for (int gpio = 0; changed != 0; gpio++, changed >>= 1) {
		if ((changed & 1) != 0)
			fPinsView->UpdateLevel(gpio);
	}

	int32 lost;
	if (message->FindInt32("lost", &lost) == B_OK && lost > 0) {
		BString text;
		text.SetToFormat("%" B_PRId32 " level change%s came too fast to show",
			lost, lost == 1 ? "" : "s");
		_ShowMessage(text);
	}
}


void
MainWindow::_ToggleOutput(int gpio)
{
	PinEntry& entry = fPins[gpio];
	if (entry.setting.mode != PinMode::Output || entry.holding)
		return;

	bool level = !entry.StableLevel();
	status_t status = fHardware->Write(gpio, level);
	if (status != B_OK) {
		BString text;
		text.SetToFormat("%s: %s", Label(gpio).String(), strerror(status));
		_ShowMessage(text);
		return;
	}
	entry.setting.level = level ? 1 : 0;
	fPinsView->UpdatePin(gpio);
	_UpdateStatus();
}


void
MainWindow::_HoldOutput(int gpio, bool pressed)
{
	PinEntry& entry = fPins[gpio];
	if (entry.setting.mode != PinMode::Output || pressed == entry.holding)
		return;

	if (pressed) {
		entry.holdLevel = entry.StableLevel();
		entry.holding = true;
		fHardware->Write(gpio, !entry.holdLevel);
	} else {
		entry.holding = false;
		fHardware->Write(gpio, entry.holdLevel);
	}
	fPinsView->UpdatePin(gpio);
}


void
MainWindow::_RefreshState()
{
	if (fHardware == NULL)
		return;
	HardwareState state;
	if (fHardware->GetState(state) != B_OK)
		return;

	for (int gpio = 0; gpio < kMaxGpioPins; gpio++) {
		PinEntry& entry = fPins[gpio];
		const PinHardwareState& now = state.pins[gpio];
		PinHardwareState& before = entry.hardware;
		bool changed = now.function != before.function
			|| now.claimedElsewhere != before.claimedElsewhere
			|| (!entry.Configured() && (now.pull != before.pull
				|| now.level != before.level));
		before = now;
		if (changed && !fApplying)
			fPinsView->UpdatePin(gpio);

		if (entry.Configured())
			entry.history.Trim(state.time - kHistoryKept);
	}
}


void
MainWindow::_Tick()
{
	fPinsView->Tick(system_time());
}


//	#pragma mark - view


void
MainWindow::_SetLayoutMode(LayoutMode mode)
{
	fPinsView->SetLayoutMode(mode);
	for (int i = 0; i < 3; i++)
		fLayoutButtons[i]->SetValue((int)mode == i ? B_CONTROL_ON : B_CONTROL_OFF);
}


void
MainWindow::_SetSpan(int seconds)
{
	if (seconds < 1 || seconds > 600)
		seconds = 16;
	fSpanSeconds = seconds;
	fPinsView->SetSpan(seconds * 1000000LL);
}


void
MainWindow::_FitToContent()
{
	// The window's own layout answers with a stale height when the pins
	// were rebuilt; its items (a column without spacing) know their sizes.
	BLayout* layout = GetLayout();
	BSize preferred(0, 0);
	for (int32 i = 0; i < layout->CountItems(); i++) {
		BLayoutItem* item = layout->ItemAt(i);
		if (!item->IsVisible())
			continue;
		BSize size = item->PreferredSize();
		preferred.width = std::max(preferred.width, size.width);
		preferred.height += size.height + 1;
	}

	BRect screen = BScreen(this).Frame().InsetByCopy(20, 20);
	float width = std::min(preferred.width, screen.Width() - 20);
	float height = std::min(preferred.height, screen.Height() - 40);
	ResizeTo(width, height);

	BRect frame = Frame();
	BPoint where = frame.LeftTop();
	if (frame.right > screen.right)
		where.x = std::max(screen.left, screen.right - frame.Width());
	if (frame.bottom > screen.bottom)
		where.y = std::max(screen.top + 20, screen.bottom - frame.Height());
	MoveTo(where);
}


void
MainWindow::_UpdateTitle()
{
	BString title = kAppName;
	if (fHasFile)
		title << ": " << fFileRef.name;
	SetTitle(title);
}


void
MainWindow::_UpdateStatus()
{
	BString status = fHardwareName;
	if (fHardware != NULL) {
		HardwareDetails details = fHardware->Details();
		BoardInfo board = DescribeBoard(details.board, details.revision);
		if (board.known && !details.simulated)
			status << " \xc2\xb7 " << board.model.c_str();
	}
	int configured = (int)_CurrentConfiguration().pins.size();
	status << " \xc2\xb7 " << configured << (configured == 1 ? " pin" : " pins")
		<< " configured";
	fStatusView->SetText(status);

	if (fMessageClearer == NULL) {
		bool dirty = _CurrentConfiguration() != fSavedConfiguration;
		fMessageView->SetText(dirty ? "Unsaved changes" : "");
	}
}


void
MainWindow::_ShowMessage(const BString& message)
{
	fMessageView->SetText(message);
	delete fMessageClearer;
	BMessage clear(kMsgClearMessage);
	fMessageClearer = new BMessageRunner(BMessenger(this), &clear,
		kMessageTime, 1);
}


void
MainWindow::_ShowDeviceDetails()
{
	if (fHardware == NULL)
		return;
	(new DeviceWindow(this, fHardware->Details()))->Show();
}


//	#pragma mark - files


void
MainWindow::_Open()
{
	if (!_AskToSave("opening another configuration"))
		return;
	if (fOpenPanel == NULL) {
		BMessenger target(this);
		fOpenPanel = new BFilePanel(B_OPEN_PANEL, &target, NULL, B_FILE_NODE,
			false);
		fOpenPanel->Window()->SetTitle("AirPins: Open configuration");
	}
	if (fHasFile) {
		BEntry entry(&fFileRef);
		BEntry parent;
		entry_ref directory;
		if (entry.GetParent(&parent) == B_OK
			&& parent.GetRef(&directory) == B_OK) {
			fOpenPanel->SetPanelDirectory(&directory);
		}
	}
	fOpenPanel->Show();
}


void
MainWindow::OpenFile(const entry_ref& ref)
{
	BFile file(&ref, B_READ_ONLY);
	off_t size = 0;
	status_t status = file.InitCheck();
	if (status == B_OK)
		status = file.GetSize(&size);
	if (status == B_OK && size > kMaxFileSize)
		status = B_FILE_TOO_LARGE;
	std::string text;
	if (status == B_OK) {
		text.resize(size);
		ssize_t read = file.Read(&text[0], size);
		if (read != size)
			status = read < 0 ? (status_t)read : B_IO_ERROR;
	}

	Configuration config;
	std::string error;
	std::vector<int> skipped;
	if (status != B_OK)
		error = strerror(status);
	else if (!ParseConfiguration(text, config, &error, &skipped))
		status = B_BAD_DATA;
	if (status != B_OK) {
		BString message;
		message.SetToFormat("Could not open \"%s\":\n\n%s", ref.name,
			error.c_str());
		BAlert* alert = new BAlert(kAppName, message, "OK", NULL, NULL,
			B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->Go(NULL);
		return;
	}

	_ApplyConfiguration(config, true);
	fSavedConfiguration = config;
	fFileRef = ref;
	fHasFile = true;
	be_roster->AddToRecentDocuments(&ref, kAppSignature);
	_UpdateTitle();
	_UpdateStatus();

	BString message;
	message.SetToFormat("Opened %s", ref.name);
	if (!skipped.empty()) {
		message << "; left out pins the header does not offer:";
		for (int gpio : skipped)
			message << " " << gpio;
	}
	_ShowMessage(message);
}


bool
MainWindow::_Save(bool askName)
{
	if (fHasFile && !askName) {
		BEntry entry(&fFileRef);
		BEntry parent;
		entry_ref directory;
		if (entry.GetParent(&parent) == B_OK
			&& parent.GetRef(&directory) == B_OK) {
			return _WriteFile(directory, fFileRef.name) == B_OK;
		}
	}

	if (fSavePanel == NULL) {
		BMessenger target(this);
		fSavePanel = new BFilePanel(B_SAVE_PANEL, &target, NULL, B_FILE_NODE,
			false);
		fSavePanel->Window()->SetTitle("AirPins: Save configuration");
	}
	fSavePanel->SetSaveText(fHasFile ? fFileRef.name : "pins.pigg");
	fSavePanel->Show();
	return false;
}


status_t
MainWindow::_WriteFile(const entry_ref& directoryRef, const char* fileName)
{
	// a copy: the name may be fFileRef's, which is replaced below
	BString name(fileName);
	Configuration config = _CurrentConfiguration();
	std::string text = FormatConfiguration(config);

	BDirectory directory(&directoryRef);
	BFile file;
	status_t status = directory.InitCheck();
	if (status == B_OK) {
		status = file.SetTo(&directory, name,
			B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	}
	if (status == B_OK) {
		ssize_t written = file.Write(text.data(), text.size());
		if (written != (ssize_t)text.size())
			status = written < 0 ? (status_t)written : B_IO_ERROR;
	}
	if (status == B_OK) {
		BNodeInfo info(&file);
		info.SetType(kConfigMimeType);
		BEntry entry(&directory, name);
		status = entry.GetRef(&fFileRef);
	}
	if (status != B_OK) {
		BString message;
		message.SetToFormat("Could not save \"%s\":\n\n%s", name.String(),
			strerror(status));
		BAlert* alert = new BAlert(kAppName, message, "OK", NULL, NULL,
			B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->Go(NULL);
		return status;
	}

	fHasFile = true;
	fSavedConfiguration = config;
	be_roster->AddToRecentDocuments(&fFileRef, kAppSignature);
	_UpdateTitle();
	BString message;
	message.SetToFormat("Saved %s", name.String());
	_ShowMessage(message);
	_UpdateStatus();
	return B_OK;
}


bool
MainWindow::_AskToSave(const char* action)
{
	if (_CurrentConfiguration() == fSavedConfiguration)
		return true;

	BString text;
	text.SetToFormat("Save the changes to the configuration before %s?",
		action);
	BAlert* alert = new BAlert(kAppName, text, "Cancel", "Don't save", "Save",
		B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
	alert->SetShortcut(0, B_ESCAPE);
	switch (alert->Go()) {
		case 0:
			return false;
		case 1:
			return true;
		default:
			if (_Save(false))
				return true;
			// the save panel is open: quitting waits for it
			fQuitAfterSave = strcmp(action, "quitting") == 0;
			return false;
	}
}


void
MainWindow::_ResetPins()
{
	_ApplyConfiguration(Configuration(), false);
	_ShowMessage("All pins are unused");
}

}	// namespace airpins
