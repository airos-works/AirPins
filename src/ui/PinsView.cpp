/*
 * Copyright 2026, air/OS contributors.
 * Distributed under the terms of the MIT License.
 */

#include "PinsView.h"

#include <CardLayout.h>
#include <GridLayout.h>
#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <ScrollBar.h>
#include <SpaceLayoutItem.h>
#include <StringView.h>
#include <Window.h>

#include <algorithm>
#include <math.h>

#include "IconButton.h"
#include "LedView.h"
#include "PinButton.h"
#include "WaveformView.h"


namespace airpins {

namespace {

BMessage*
PinMessage(uint32 what, int gpio)
{
	BMessage* message = new BMessage(what);
	message->AddInt32("pin", gpio);
	return message;
}


//! Menu fields as wide as their widest choice: the columns stay put.
void
FixWidth(BMenuField* field, const char* widest)
{
	float width = ceilf(be_plain_font->StringWidth(widest)
		+ be_plain_font->Size() * 3.6f);
	field->SetExplicitMinSize(BSize(width, B_SIZE_UNSET));
	field->SetExplicitMaxSize(BSize(width, B_SIZE_UNSET));
}


BMenuField*
FunctionField(int gpio, BHandler* target)
{
	BPopUpMenu* menu = new BPopUpMenu("function");
	const struct { const char* label; PinMode mode; } kModes[] = {
		{ "Unused", PinMode::Unused },
		{ "Input", PinMode::Input },
		{ "Output", PinMode::Output },
	};
	for (const auto& mode : kModes) {
		BMessage* message = PinMessage(kMsgSetPinMode, gpio);
		message->AddInt32("mode", (int32)mode.mode);
		BMenuItem* item = new BMenuItem(mode.label, message);
		item->SetTarget(target);
		menu->AddItem(item);
	}
	BMenuField* field = new BMenuField("function", NULL, menu);
	field->SetToolTip("The pin's function");
	FixWidth(field, "Unused");
	return field;
}


BMenuField*
PullField(int gpio, BHandler* target)
{
	BPopUpMenu* menu = new BPopUpMenu("pull");
	const Pull kPulls[] = { Pull::Up, Pull::Down, Pull::None };
	for (Pull pull : kPulls) {
		BMessage* message = PinMessage(kMsgSetPinPull, gpio);
		message->AddInt32("pull", (int32)pull);
		BMenuItem* item = new BMenuItem(PullName(pull), message);
		item->SetTarget(target);
		menu->AddItem(item);
	}
	BMenuField* field = new BMenuField("pull", NULL, menu);
	field->SetToolTip("The input's pull resistor");
	FixWidth(field, "Pull down");
	return field;
}


void
MarkItem(BMenuField* field, int index)
{
	BMenu* menu = field->Menu();
	BMenuItem* item = menu->ItemAt(index);
	if (item != NULL && !item->IsMarked())
		item->SetMarked(true);
}

}	// namespace


PinsView::PinsView(PinEntry* pins, BHandler* target)
	:
	BView("pins", B_WILL_DRAW | B_FRAME_EVENTS),
	fPins(pins),
	fTarget(target),
	fMode(LayoutMode::Board),
	fSpan(16000000),
	fContent(NULL)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
}


void
PinsView::AttachedToWindow()
{
	BView::AttachedToWindow();
	if (fContent == NULL)
		Rebuild();
}


void
PinsView::FrameResized(float width, float height)
{
	BView::FrameResized(width, height);
	_UpdateContentSize();
}


BSize
PinsView::MinSize()
{
	// the window may be smaller than the pins: they scroll
	return BSize(be_plain_font->Size() * 16, be_plain_font->Size() * 8);
}


BSize
PinsView::MaxSize()
{
	return BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
}


BSize
PinsView::PreferredSize()
{
	return ContentSize();
}


void
PinsView::SetLayoutMode(LayoutMode mode)
{
	if (mode == fMode && fContent != NULL)
		return;
	fMode = mode;
	if (Window() != NULL)
		Rebuild();
}


void
PinsView::Rebuild()
{
	if (fContent != NULL) {
		fContent->RemoveSelf();
		delete fContent;
		fContent = NULL;
	}
	fWidgets.clear();
	fDock.clear();

	fContent = new BView("content", 0);
	fContent->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	BGroupLayout* column = new BGroupLayout(B_VERTICAL, B_USE_DEFAULT_SPACING);
	fContent->SetLayout(column);
	column->SetInsets(B_USE_WINDOW_SPACING);

	// A layout takes items only once it belongs to a view: every layout is
	// added to the column before anything goes into it.
	switch (fMode) {
		case LayoutMode::Board:
		{
			// each connector of the board: two columns of pins as on the
			// header (odd pins on the left), or one
			for (int i = 0; i < ConnectorCount(); i++) {
				const Connector& connector = ConnectorAt(i);
				if (connector.name != NULL) {
					BString title;
					title.SetToFormat("%s (%d pins)", connector.name,
						connector.pinCount);
					BStringView* label = new BStringView("connector", title);
					BFont font(be_bold_font);
					label->SetFont(&font);
					column->AddView(label);
				}
				BGridLayout* grid = new BGridLayout(B_USE_SMALL_SPACING,
					B_USE_SMALL_SPACING);
				column->AddItem(grid);
				if (connector.columns == 2) {
					for (int row = 0; row < connector.pinCount / 2; row++) {
						_AddPin(grid, row, 0,
							HeaderPinAt(connector.first + row * 2), true);
						_AddPin(grid, row, 7,
							HeaderPinAt(connector.first + row * 2 + 1), false);
					}
				} else {
					for (int row = 0; row < connector.pinCount; row++) {
						_AddPin(grid, row, 0,
							HeaderPinAt(connector.first + row), false);
					}
				}
			}
			break;
		}

		case LayoutMode::Bcm:
		case LayoutMode::Compact:
		{
			bool compact = fMode == LayoutMode::Compact;
			if (compact) {
				BGroupLayout* dock = new BGroupLayout(B_HORIZONTAL,
					B_USE_HALF_ITEM_SPACING);
				column->AddItem(dock);
				BStringView* label = new BStringView("dock label",
					"Unused pins:");
				dock->AddView(label);
				for (int i = 0; i < HeaderPinCount(); i++) {
					const HeaderPin& pin = HeaderPinAt(i);
					if (pin.kind != PinKind::Gpio
						|| fPins[pin.gpio].Configured()) {
						continue;
					}
					PinButton* button = new PinButton(pin, &fPins[pin.gpio],
						fTarget, true);
					fDock.push_back(button);
					dock->AddView(button);
				}
				dock->AddItem(BSpaceLayoutItem::CreateGlue());
				if (fDock.empty())
					label->SetText("All pins are in use.");
			}

			std::vector<int> pins;
			for (int gpio : GpioOrder()) {
				if (!compact || fPins[gpio].Configured())
					pins.push_back(gpio);
			}
			if (pins.empty()) {
				BStringView* empty = new BStringView("empty",
					"Click a pin above to configure it.");
				empty->SetHighUIColor(B_PANEL_TEXT_COLOR,
					B_DISABLED_LABEL_TINT);
				column->AddView(empty);
				break;
			}
			BGridLayout* grid = new BGridLayout(B_USE_SMALL_SPACING,
				B_USE_SMALL_SPACING);
			column->AddItem(grid);
			int row = 0;
			for (int gpio : pins)
				_AddPin(grid, row++, 0, *HeaderPinForGpio(gpio), false);
			break;
		}
	}

	column->AddItem(BSpaceLayoutItem::CreateGlue());

	AddChild(fContent);
	UpdateAll();
	_UpdateContentSize();
	// the scroll view and window learn the new preferred size
	InvalidateLayout();
}


void
PinsView::_AddPin(BGridLayout* grid, int row, int firstColumn,
	const HeaderPin& pin, bool mirrored)
{
	// columns from the pin outwards: disc, name, hint, function, options,
	// LED, waveform; mirrored for the header's left column
	auto column = [&](int index) {
		return mirrored ? firstColumn + 6 - index : firstColumn + index;
	};

	PinEntry* entry = pin.kind == PinKind::Gpio ? &fPins[pin.gpio] : NULL;
	PinButton* button = new PinButton(pin, entry, fTarget);
	grid->AddView(button, column(0), row);

	BStringView* name = new BStringView("name", pin.name);
	name->SetAlignment(mirrored ? B_ALIGN_RIGHT : B_ALIGN_LEFT);
	grid->AddView(name, column(1), row);
	if (pin.kind == PinKind::Reserved || pin.kind == PinKind::Ground
		|| pin.kind == PinKind::Power3V3 || pin.kind == PinKind::Power5V) {
		name->SetHighUIColor(B_PANEL_TEXT_COLOR, B_DISABLED_LABEL_TINT);
	}
	if (entry == NULL)
		return;

	Widgets widgets = {};
	widgets.gpio = pin.gpio;
	widgets.button = button;
	widgets.name = name;

	widgets.hint = new BStringView("hint", "");
	BFont font(be_plain_font);
	font.SetSize(ceilf(font.Size() * 0.85f));
	widgets.hint->SetFont(&font);
	widgets.hint->SetHighUIColor(B_PANEL_TEXT_COLOR, B_DISABLED_LABEL_TINT);
	widgets.hint->SetAlignment(mirrored ? B_ALIGN_RIGHT : B_ALIGN_LEFT);
	grid->AddView(widgets.hint, column(2), row);

	widgets.function = FunctionField(pin.gpio, fTarget);
	grid->AddView(widgets.function, column(3), row);

	BView* options = new BView("options", 0);
	options->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	widgets.options = new BCardLayout();
	options->SetLayout(widgets.options);
	widgets.options->AddView(new BView("none", 0));
	widgets.pull = PullField(pin.gpio, fTarget);
	BView* pullHolder = new BView("pull holder", 0);
	BLayoutBuilder::Group<>(pullHolder, B_HORIZONTAL, 0)
		.Add(widgets.pull);
	widgets.options->AddView(pullHolder);
	widgets.toggle = new IconButton("level", "High", kIconToggleOn,
		PinMessage(kMsgToggleOutput, pin.gpio));
	widgets.toggle->SetTarget(fTarget);
	widgets.toggle->SetToolTip("Click to toggle the output's level");
	BSize toggleSize = widgets.toggle->PreferredSize();
	widgets.toggle->SetExplicitSize(toggleSize);
	widgets.options->AddView(widgets.toggle);
	widgets.options->SetVisibleItem((int32)0);
	grid->AddView(options, column(4), row);

	widgets.led = new LedView(entry, fTarget);
	grid->AddView(widgets.led, column(5), row);

	widgets.wave = new WaveformView(entry);
	widgets.wave->SetSpan(fSpan);
	grid->AddView(widgets.wave, column(6), row);

	fWidgets.push_back(widgets);
}


void
PinsView::_Update(Widgets& widgets)
{
	const PinEntry& entry = fPins[widgets.gpio];
	const PinHardwareState& hardware = entry.hardware;
	bool busy = hardware.claimedElsewhere;

	widgets.button->Update();

	BString hint;
	int alt = AltIndex(hardware.function);
	if (busy)
		hint = "in use";
	else if (!entry.Configured() && alt >= 0) {
		const char* signal = AltSignal(widgets.gpio, alt);
		hint = signal != NULL ? signal : FunctionSelectName(hardware.function);
	}
	if (hint != widgets.hint->Text())
		widgets.hint->SetText(hint);

	MarkItem(widgets.function, (int)entry.setting.mode);
	widgets.function->SetEnabled(!busy);

	switch (entry.setting.mode) {
		case PinMode::Unused:
			widgets.options->SetVisibleItem((int32)0);
			break;
		case PinMode::Input:
			MarkItem(widgets.pull, entry.setting.pull == Pull::Up ? 0
				: entry.setting.pull == Pull::Down ? 1 : 2);
			widgets.options->SetVisibleItem((int32)1);
			break;
		case PinMode::Output:
		{
			bool high = entry.holding ? entry.holdLevel : entry.StableLevel();
			widgets.toggle->SetLabel(high ? "High" : "Low");
			widgets.toggle->SetIconId(high ? kIconToggleOn : kIconToggleOff);
			widgets.options->SetVisibleItem((int32)2);
			break;
		}
	}

	widgets.led->Update();
	widgets.wave->Invalidate();
}


void
PinsView::UpdatePin(int gpio)
{
	for (Widgets& widgets : fWidgets) {
		if (widgets.gpio == gpio)
			_Update(widgets);
	}
	for (PinButton* button : fDock)
		button->Update();
}


void
PinsView::UpdateAll()
{
	for (Widgets& widgets : fWidgets)
		_Update(widgets);
	for (PinButton* button : fDock)
		button->Update();
}


void
PinsView::UpdateLevel(int gpio)
{
	for (Widgets& widgets : fWidgets) {
		if (widgets.gpio != gpio)
			continue;
		widgets.led->Update();
		const PinEntry& entry = fPins[gpio];
		if (entry.setting.mode == PinMode::Output && entry.setting.level < 0)
			_Update(widgets);
	}
}


void
PinsView::Tick(bigtime_t now)
{
	for (Widgets& widgets : fWidgets)
		widgets.wave->Tick(now);
}


void
PinsView::SetSpan(bigtime_t span)
{
	fSpan = span;
	for (Widgets& widgets : fWidgets)
		widgets.wave->SetSpan(span);
}


BSize
PinsView::ContentSize()
{
	if (fContent == NULL)
		return BSize(0, 0);
	return fContent->PreferredSize();
}


void
PinsView::_UpdateContentSize()
{
	if (fContent == NULL)
		return;

	BRect bounds = Bounds();
	BSize preferred = fContent->PreferredSize();
	float width = std::max(bounds.Width(), preferred.width);
	float height = std::max(bounds.Height(), preferred.height);
	fContent->MoveTo(0, 0);
	fContent->ResizeTo(width, height);
	// nothing above lays the content out: it is a layout root
	fContent->Layout(true);

	float step = ceilf(be_plain_font->Size() * 3);
	BScrollBar* scrollBar = ScrollBar(B_HORIZONTAL);
	if (scrollBar != NULL) {
		float range = std::max(0.0f, preferred.width - bounds.Width());
		scrollBar->SetRange(0, range);
		scrollBar->SetProportion(preferred.width > 0
			? std::min(1.0f, (bounds.Width() + 1) / (preferred.width + 1)) : 1);
		scrollBar->SetSteps(step, std::max(step, bounds.Width() * 0.9f));
	}
	scrollBar = ScrollBar(B_VERTICAL);
	if (scrollBar != NULL) {
		float range = std::max(0.0f, preferred.height - bounds.Height());
		scrollBar->SetRange(0, range);
		scrollBar->SetProportion(preferred.height > 0
			? std::min(1.0f, (bounds.Height() + 1) / (preferred.height + 1)) : 1);
		scrollBar->SetSteps(step, std::max(step, bounds.Height() * 0.9f));
	}
}

}	// namespace airpins
