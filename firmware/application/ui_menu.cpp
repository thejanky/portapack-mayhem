/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "ui_menu.hpp"
#include "time.hpp"

namespace ui {

/* MenuItemView **********************************************************/

void MenuItemView::select() {
	if( item.on_select ) {
		item.on_select();
	}
}

void MenuItemView::highlight() {
	set_highlighted(true);
	set_dirty();
}

void MenuItemView::unhighlight() {
	set_highlighted(false);
	set_dirty();
}

void MenuItemView::paint(Painter& painter) {
	const auto r = screen_rect();

	const auto paint_style = (highlighted() && parent()->has_focus()) ? style().invert() : style();

	const auto font_height = paint_style.font.line_height();

	painter.fill_rectangle(
		r,
		paint_style.background
	);
	
	ui::Color final_item_color = (highlighted() && parent()->has_focus()) ? ui::Color::black() : item.color;

	if (final_item_color.v == paint_style.background.v) final_item_color = paint_style.foreground;

	Style text_style {
		.font = paint_style.font,
		.background = paint_style.background,
		.foreground = final_item_color
	};

	painter.draw_string(
		{ r.pos.x + 8, r.pos.y + (r.size.h - font_height) / 2 },
		text_style,
		item.text
	);
}

/* MenuView **************************************************************/

MenuView::MenuView() {
	set_focusable(true);
	
	add_child(&arrow_more);
	signal_token_tick_second = time::signal_tick_second += [this]() {
		this->on_tick_second();
	};
	
	arrow_more.set_parent_rect( { 216, 320 - 16 - 24, 16, 16 } );
	arrow_more.set_focusable(false);
}

MenuView::~MenuView() {
	time::signal_tick_second -= signal_token_tick_second;
}

void MenuView::on_tick_second() {
	if (more_ && blink_)
		arrow_more.set_foreground(Color::white());
	else
		arrow_more.set_foreground(Color::black());
		
	blink_ = !blink_;
	
	arrow_more.set_dirty();
}

void MenuView::add_item(const MenuItem item) {
	add_child(new MenuItemView { item });
}

void MenuView::set_parent_rect(const Rect new_parent_rect) {
	View::set_parent_rect(new_parent_rect);
	update_items();
}

void MenuView::update_items() {
	constexpr size_t item_height = 24;
	size_t i = 0;
	Coord y_pos;
	
	if (MENU_MAX + offset_ < (children_.size() - 1))
		more_ = true;
	else
		more_ = false;
	
	for (auto child : children_) {
		if (i) {		// Skip arrow widget
			y_pos = (i - 1 - offset_) * item_height;
			child->set_parent_rect({
				{ 0, y_pos },
				{ size().w, item_height }
			});
			if ((y_pos < 0) || (y_pos >= (320 - 16 - 24 - 16)))
				child->hidden(true);
			else
				child->hidden(false);
		}
		i++;
	}
}

MenuItemView* MenuView::item_view(size_t index) const {
	/* TODO: Terrible cast! Take it as a sign I must be doing something
	 * shamefully wrong here, right?
	 */
	return static_cast<MenuItemView*>(children_[index + 1]);	// Skip arrow widget
}

size_t MenuView::highlighted() const {
	return highlighted_;
}

bool MenuView::set_highlighted(const size_t new_value) {
	if( new_value >= (children_.size() - 1) ) {					// Skip arrow widget
		return false;
	}
	
	if ((new_value - offset_ + 1) >= MENU_MAX) {
		// Shift MenuView up
		offset_ = new_value - MENU_MAX + 1;
		update_items();
	} else if (new_value < offset_) {
		// Shift MenuView down
		offset_ = new_value;
		update_items();
	}

	item_view(highlighted())->unhighlight();
	highlighted_ = new_value;
	item_view(highlighted())->highlight();

	return true;
}

void MenuView::on_focus() {
	item_view(highlighted())->highlight();
}

void MenuView::on_blur() {
	item_view(highlighted())->unhighlight();
}

bool MenuView::on_key(const KeyEvent key) {
	switch(key) {
	case KeyEvent::Up:
		return set_highlighted(highlighted() - 1);

	case KeyEvent::Down:
		return set_highlighted(highlighted() + 1);

	case KeyEvent::Select:
	case KeyEvent::Right:
		item_view(highlighted())->select();
		return true;

	case KeyEvent::Left:
		if( on_left ) {
			on_left();
		}
		return true;

	default:
		return false;
	}
}

bool MenuView::on_encoder(const EncoderEvent event) {
	set_highlighted(highlighted() + event);
	return true;
}

/* TODO: This could be handled by default behavior, if the UI stack were to
 * transmit consumable events from the top of the hit-stack down, and each
 * MenuItem could respond to a touch and update its parent MenuView.
 */
/*
bool MenuView::on_touch(const TouchEvent event) {
	size_t i = 0;
	for(const auto child : children_) {
		if( child->screen_rect().contains(event.point) ) {
			return set_highlighted(i);
		}
		i++;
	}

	return false;
}
*/
} /* namespace ui */
