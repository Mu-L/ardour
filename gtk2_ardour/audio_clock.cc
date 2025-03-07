/*
 * Copyright (C) 2005-2006 Taybin Rutkin <taybin@taybin.com>
 * Copyright (C) 2005-2018 Paul Davis <paul@linuxaudiosystems.com>
 * Copyright (C) 2006-2012 David Robillard <d@drobilla.net>
 * Copyright (C) 2007 Doug McLain <doug@nostar.net>
 * Copyright (C) 2009-2012 Carl Hetherington <carl@carlh.net>
 * Copyright (C) 2012-2019 Robin Gareus <robin@gareus.org>
 * Copyright (C) 2013-2017 John Emmas <john@creativepost.co.uk>
 * Copyright (C) 2015-2016 Tim Mayberry <mojofunk@gmail.com>
 * Copyright (C) 2015 Ben Loftis <ben@harrisonconsoles.com>
 * Copyright (C) 2015 Colin Fletcher <colin.m.fletcher@googlemail.com>
 * Copyright (C) 2016-2017 Nick Mainsbridge <mainsbridge@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <cstdio> // for sprintf
#include <cmath>

#include "pbd/convert.h"
#include "pbd/enumwriter.h"

#include <ytkmm/style.h>
#include <sigc++/bind.h>

#include "gtkmm2ext/utils.h"
#include "gtkmm2ext/rgb_macros.h"

#include "widgets/tooltips.h"

#include "ardour/profile.h"
#include "ardour/lmath.h"
#include "ardour/session.h"
#include "ardour/transport_master.h"
#include "ardour/tempo.h"
#include "ardour/transport_master_manager.h"
#include "ardour/types.h"

#include "ardour_ui.h"
#include "audio_clock.h"
#include "enums_convert.h"
#include "gui_thread.h"
#include "keyboard.h"
#include "ui_config.h"
#include "utils.h"

#include "pbd/i18n.h"

using namespace ARDOUR;
using namespace ARDOUR_UI_UTILS;
using namespace ArdourWidgets;
using namespace PBD;
using namespace Gtk;
using namespace std;
using namespace Temporal;

using Gtkmm2ext::Keyboard;

vector<AudioClock*> AudioClock::clocks;

#define BBT_BAR_CHAR "|"
#define BBT_SCANF_FORMAT "%" PRIu32 "%*c%" PRIu32 "%*c%" PRIu32

AudioClock::AudioClock (const string& clock_name, bool transient, const string& widget_name,
			bool allow_edit, bool follows_playhead, bool duration, bool with_info,
			bool accept_on_focus_out)
	: ops_menu (0)
	, _name (clock_name)
	, is_duration (duration)
	, editable (allow_edit)
	, _follows_playhead (follows_playhead)
	, _accept_on_focus_out (accept_on_focus_out)
	, _off (false)
	, _negative_allowed (false)
	, edit_is_negative (false)
	, _limit_pos (timepos_t::max (Temporal::AudioTime))
	, _with_info (with_info)
	, editing_attr (0)
	, foreground_attr (0)
	, first_height (0)
	, first_width (0)
	, style_resets_first (true)
	, layout_height (0)
	, layout_width (0)
	, corner_radius (4)
	, font_size (10240)
	, editing (false)
	, dragging (false)
	, drag_field (Field (0))
	, xscale (1.0)
	, yscale (1.0)
	, _hovering (false)
{
	if (editable) {
		set_can_focus ();
	}

	_layout = Pango::Layout::create (get_pango_context());
	_layout->set_attributes (normal_attributes);

	set_widget_name (widget_name);

	_mode = BBT; /* lie to force mode switch */
	set_mode (Timecode);

	if (!transient) {
		clocks.push_back (this);
	}

	_left_btn.set_name ("transport option button");
	_right_btn.set_name ("transport option button");

	_left_btn.set_sizing_text (_("0000000000000"));
	_right_btn.set_sizing_text (_("0000000000000"));

	_left_btn.set_layout_font (UIConfiguration::instance().get_ArdourSmallFont());
	_right_btn.set_layout_font (UIConfiguration::instance().get_ArdourSmallFont());

	UIConfiguration::instance().ColorsChanged.connect (sigc::mem_fun (*this, &AudioClock::set_colors));
	UIConfiguration::instance().DPIReset.connect (sigc::mem_fun (*this, &AudioClock::dpi_reset));

	TempoMap::MapChanged.connect (tempo_map_connection, invalidator (*this), std::bind (&AudioClock::tempo_map_changed, this), gui_context());
}

AudioClock::~AudioClock ()
{
	delete ops_menu;
	delete foreground_attr;
	delete editing_attr;
}

void
AudioClock::set_widget_name (const string& str)
{
	if (str.empty()) {
		set_name ("clock");
	} else {
		set_name (str + " clock");
	}

	if (get_realized()) {
		set_colors ();
	}
}


void
AudioClock::on_realize ()
{
	Gtk::Requisition req;

	CairoWidget::on_realize ();

	set_clock_dimensions (req);

	first_width = req.width;
	first_height = req.height;

	set_colors ();
}

void
AudioClock::set_active_state (Gtkmm2ext::ActiveState s)
{
	CairoWidget::set_active_state (s);
	set_colors ();
}

void
AudioClock::set_colors ()
{
	int r, g, b, a;

	uint32_t bg_color;
	uint32_t text_color;
	uint32_t editing_color;
	uint32_t cursor_color;

	if (active_state()) {
		bg_color = UIConfiguration::instance().color (string_compose ("%1 active: background", get_name()));
		text_color = UIConfiguration::instance().color (string_compose ("%1 active: text", get_name()));
		editing_color = UIConfiguration::instance().color (string_compose ("%1 active: edited text", get_name()));
		cursor_color = UIConfiguration::instance().color (string_compose ("%1 active: cursor", get_name()));
	} else {
		bg_color = UIConfiguration::instance().color (string_compose ("%1: background", get_name()));
		text_color = UIConfiguration::instance().color (string_compose ("%1: text", get_name()));
		editing_color = UIConfiguration::instance().color (string_compose ("%1: edited text", get_name()));
		cursor_color = UIConfiguration::instance().color (string_compose ("%1: cursor", get_name()));
	}

	/* store for bg and cursor in render() */

	UINT_TO_RGBA (bg_color, &r, &g, &b, &a);

	bg_r = r/255.0;
	bg_g = g/255.0;
	bg_b = b/255.0;
	bg_a = a/255.0;

	UINT_TO_RGBA (cursor_color, &r, &g, &b, &a);

	cursor_r = r/255.0;
	cursor_g = g/255.0;
	cursor_b = b/255.0;
	cursor_a = a/255.0;

	/* rescale for Pango colors ... sigh */

	r = lrint (r * 65535.0);
	g = lrint (g * 65535.0);
	b = lrint (b * 65535.0);

	UINT_TO_RGBA (text_color, &r, &g, &b, &a);
	r = lrint ((r/255.0) * 65535.0);
	g = lrint ((g/255.0) * 65535.0);
	b = lrint ((b/255.0) * 65535.0);
	delete foreground_attr;
	foreground_attr = new Pango::AttrColor (Pango::Attribute::create_attr_foreground (r, g, b));

	UINT_TO_RGBA (editing_color, &r, &g, &b, &a);
	r = lrint ((r/255.0) * 65535.0);
	g = lrint ((g/255.0) * 65535.0);
	b = lrint ((b/255.0) * 65535.0);
	delete editing_attr;
	editing_attr = new Pango::AttrColor (Pango::Attribute::create_attr_foreground (r, g, b));

	normal_attributes.change (*foreground_attr);
	editing_attributes.change (*foreground_attr);
	editing_attributes.change (*editing_attr);

	if (!editing) {
		_layout->set_attributes (normal_attributes);
	} else {
		_layout->set_attributes (editing_attributes);
	}

	queue_draw ();
}

void
AudioClock::set_scale (double x, double y)
{
	xscale = x;
	yscale = y;

	queue_draw ();
}

void
AudioClock::render (Cairo::RefPtr<Cairo::Context> const& ctx, cairo_rectangle_t*)
{
	cairo_t* cr = ctx->cobj();
	/* main layout: rounded rect, plus the text */

	if (_need_bg) {
		cairo_set_source_rgba (cr, bg_r, bg_g, bg_b, bg_a);
		if (corner_radius) {
			Gtkmm2ext::rounded_rectangle (cr, 0, 0, get_width(), get_height(), corner_radius);
		} else {
			cairo_rectangle (cr, 0, 0, get_width(), get_height());
		}
		cairo_fill (cr);
	}

	double lw = layout_width * xscale;
	double lh = layout_height * yscale;

	cairo_move_to (cr, (get_width() - lw) / 2.0, (get_height() - lh) / 2.0);

	if (xscale != 1.0 || yscale != 1.0) {
		cairo_save (cr);
		cairo_scale (cr, xscale, yscale);
	}

	pango_cairo_show_layout (cr, _layout->gobj());

	if (xscale != 1.0 || yscale != 1.0) {
		cairo_restore (cr);
	}

	if (editing) {
		if (!insert_map.empty()) {

			int xcenter = (get_width() - layout_width) /2;

			if (input_string.length() < insert_map.size()) {
				Pango::Rectangle cursor;

				if (input_string.empty()) {
					/* nothing entered yet, put cursor at the end
					   of string
					*/
					cursor = _layout->get_cursor_strong_pos (edit_string.length());
				} else {
					cursor = _layout->get_cursor_strong_pos (1 + insert_map[input_string.length()]);
				}

				cairo_set_source_rgba (cr, cursor_r, cursor_g, cursor_b, cursor_a);
				cairo_rectangle (cr,
				                 min (get_width() - 2.0,
				                   (double) xcenter + cursor.get_x()/PANGO_SCALE),
				                 (get_height() - layout_height)/2.0,
				                 2.0, cursor.get_height()/PANGO_SCALE);
				cairo_fill (cr);
			} else {
				/* we've entered all possible digits, no cursor */
			}

		} else {
			if (input_string.empty()) {
				cairo_set_source_rgba (cr, cursor_r, cursor_g, cursor_b, cursor_a);
				cairo_rectangle (cr,
						 (get_width()/2.0),
						 (get_height() - layout_height)/2.0,
						 2.0, get_height());
				cairo_fill (cr);
			}
		}
	}

	if (UIConfigurationBase::instance().get_widget_prelight() && (_hovering || editing)) {
		if (corner_radius) {
			Gtkmm2ext::rounded_rectangle (cr, 0, 0, get_width(), get_height(), corner_radius);
		} else {
			cairo_rectangle (cr, 0, 0, get_width(), get_height());
		}
		cairo_set_source_rgba (cr, 0.905, 0.917, 0.925, 0.12);
		cairo_fill (cr);
	}
}

void
AudioClock::set_clock_dimensions (Gtk::Requisition& req)
{
	Glib::RefPtr<Pango::Layout> tmp;
	Glib::RefPtr<Gtk::Style> style = get_style ();
	Pango::FontDescription font;

	tmp = Pango::Layout::create (get_pango_context());

	if (!get_realized()) {
		font = get_font_for_style (get_name());
	} else {
		font = style->get_font();
	}

	tmp->set_font_description (font);

	/* this string is the longest thing we will ever display */
	if (_mode == MinSec)
		tmp->set_text (" 88:88:88,888 ");
	else
		tmp->set_text (" 88:88:88,88 ");
	tmp->get_pixel_size (req.width, req.height);

	layout_height = req.height;
	layout_width = req.width;
}

void
AudioClock::on_size_request (Gtk::Requisition* req)
{
	/* even for non fixed width clocks, the size we *ask* for never changes,
	   even though the size we receive might. so once we've computed it,
	   just return it.
	*/

	if (first_width) {
		req->width = first_width;
		req->height = first_height;
		return;
	}

	set_clock_dimensions (*req);

	/* now tackle height, for which we need to know the height of the lower
	 * layout
	 */
}

void
AudioClock::show_edit_status (int length)
{
	editing_attr->set_start_index (edit_string.length() - length);
	editing_attr->set_end_index (edit_string.length());

	editing_attributes.change (*foreground_attr);
	editing_attributes.change (*editing_attr);

	_layout->set_attributes (editing_attributes);
}

void
AudioClock::start_edit (Field f)
{
	if (!editing) {
		pre_edit_string = _layout->get_text ();
		if (!insert_map.empty()) {
			edit_string = pre_edit_string;
		} else {
			edit_string.clear ();
			_layout->set_text ("");
		}

		input_string.clear ();
		editing = true;
		edit_is_negative = false;

		if (f) {
			input_string = get_field (f);
			show_edit_status (merge_input_and_edit_string ());
			_layout->set_text (edit_string);
		}

		queue_draw ();

		Keyboard::magic_widget_grab_focus ();
		grab_focus ();
	}
}

string
AudioClock::get_field (Field f)
{
	switch (f) {
	case Timecode_Hours:
		return edit_string.substr (1, 2);
		break;
	case Timecode_Minutes:
		return edit_string.substr (4, 2);
		break;
	case Timecode_Seconds:
		return edit_string.substr (7, 2);
		break;
	case Timecode_frames:
		return edit_string.substr (10, 2);
		break;
	case MS_Hours:
		return edit_string.substr (1, 2);
		break;
	case MS_Minutes:
		return edit_string.substr (4, 2);
		break;
	case MS_Seconds:
		return edit_string.substr (7, 2);
		break;
	case MS_Milliseconds:
		return edit_string.substr (10, 3);
		break;
	case Bars:
		return edit_string.substr (1, 3);
		break;
	case Beats:
		return edit_string.substr (5, 2);
		break;
	case Ticks:
		return edit_string.substr (8, 4);
		break;
	case SS_Seconds:
		return edit_string.substr (0, 8);
	case SS_Deciseconds:
		return edit_string.substr (9, 1);
	case S_Samples:
		return edit_string;
		break;
	}
	return "";
}

void
AudioClock::end_edit (bool modify)
{
	if (modify) {

		bool ok = true;

		switch (_mode) {
		case Timecode:
			ok = timecode_validate_edit (edit_string);
			break;

		case BBT:
			ok = bbt_validate_edit (edit_string);
			break;

		case MinSec:
			ok = minsec_validate_edit (edit_string);
			break;

		case Seconds:
			/* fallthrough */
		case Samples:
			if (edit_string.length() < 1) {
				edit_string = pre_edit_string;
			}
			break;
		}

		if (!ok) {
			edit_string = pre_edit_string;
			input_string.clear ();
			_layout->set_text (edit_string);
			show_edit_status (0);
			/* edit attributes remain in use */
		} else {

			editing = false;
			samplepos_t samples = 0; /* stupid gcc */

			switch (_mode) {
			case Timecode:
				samples = samples_from_timecode_string (edit_string);
				break;

			case BBT:
				if (is_duration) {
					samples = sample_duration_from_bbt_string (last_when(), edit_string);
				} else {
					samples = samples_from_bbt_string (timepos_t(), edit_string);
				}
				break;

			case MinSec:
				samples = samples_from_minsec_string (edit_string);
				break;

			case Seconds:
				samples = samples_from_seconds_string (edit_string);
				break;

			case Samples:
				samples = samples_from_audiosamples_string (edit_string);
				break;
			}

			if (is_duration) {
				AudioClock::set_duration (timecnt_t (samples), true);
			} else {
				AudioClock::set (timepos_t (samples), true);
			}

			_layout->set_attributes (normal_attributes);
			ValueChanged(); /* EMIT_SIGNAL */
		}

	} else {

		editing = false;
		edit_is_negative = false;
		_layout->set_attributes (normal_attributes);
		_layout->set_text (pre_edit_string);
	}

	queue_draw ();

	if (!editing) {
		drop_focus ();
	}
}

void
AudioClock::drop_focus ()
{
	Keyboard::magic_widget_drop_focus ();

	if (has_focus()) {
		/* move focus back to the default widget in the top level window */
		ARDOUR_UI::instance()->reset_focus (this);
	}
}

timecnt_t
AudioClock::parse_as_seconds_distance (const std::string& str)
{
	float f;

	if (sscanf (str.c_str(), "%f", &f) == 1) {
		return timecnt_t (f * _session->sample_rate());

	}

	return timecnt_t (samplecnt_t (0));
}

timecnt_t
AudioClock::parse_as_samples_distance (const std::string& str)
{
	samplecnt_t samples = 0;

	if (sscanf (str.c_str(), "%" PRId64, &samples) == 1) {
		return timecnt_t (samples);
	}

	return timecnt_t (samples);
}

timecnt_t
AudioClock::parse_as_minsec_distance (const std::string& str)
{
	samplecnt_t sr = _session->sample_rate();
	int msecs;
	int secs;
	int mins;
	int hrs;
	samplecnt_t samples = 0;

	switch (str.length()) {
	case 0:
		break;
	case 1:
	case 2:
	case 3:
	case 4:
		sscanf (str.c_str(), "%" PRId32, &msecs);
		samples = msecs * (sr / 1000);
		break;

	case 5:
		sscanf (str.c_str(), "%1" PRId32 "%" PRId32, &secs, &msecs);
		samples = (secs * sr) + (msecs * (sr/1000));
		break;

	case 6:
		sscanf (str.c_str(), "%2" PRId32 "%" PRId32, &secs, &msecs);
		samples = (secs * sr) + (msecs * (sr/1000));
		break;

	case 7:
		sscanf (str.c_str(), "%1" PRId32 "%2" PRId32 "%" PRId32, &mins, &secs, &msecs);
		samples = (mins * 60 * sr) + (secs * sr) + (msecs * (sr/1000));
		break;

	case 8:
		sscanf (str.c_str(), "%2" PRId32 "%2" PRId32 "%" PRId32, &mins, &secs, &msecs);
		samples = (mins * 60 * sr) + (secs * sr) + (msecs * (sr/1000));
		break;

	case 9:
		sscanf (str.c_str(), "%1" PRId32 "%2" PRId32 "%2" PRId32 "%" PRId32, &hrs, &mins, &secs, &msecs);
		samples = (hrs * 3600 * sr) + (mins * 60 * sr) + (secs * sr) + (msecs * (sr/1000));
		break;

	case 10:
		sscanf (str.c_str(), "%1" PRId32 "%2" PRId32 "%2" PRId32 "%" PRId32, &hrs, &mins, &secs, &msecs);
		samples = (hrs * 3600 * sr) + (mins * 60 * sr) + (secs * sr) + (msecs * (sr/1000));
		break;

	default:
		break;
	}

	return timecnt_t (samples);
}

timecnt_t
AudioClock::parse_as_timecode_distance (const std::string& str)
{
	double fps = _session->timecode_frames_per_second();
	samplecnt_t sr = _session->sample_rate();
	int samples;
	int secs;
	int mins;
	int hrs;
	samplecnt_t ret = 0;

	switch (str.length()) {
	case 0:
		break;
	case 1:
	case 2:
		sscanf (str.c_str(), "%" PRId32, &samples);
		ret = llrint ((samples/(float)fps) * sr);
		break;

	case 3:
		sscanf (str.c_str(), "%1" PRId32 "%" PRId32, &secs, &samples);
		ret = (secs * sr) + llrint ((samples/(float)fps) * sr);
		break;

	case 4:
		sscanf (str.c_str(), "%2" PRId32 "%" PRId32, &secs, &samples);
		ret = (secs * sr) + llrint ((samples/(float)fps) * sr);
		break;

	case 5:
		sscanf (str.c_str(), "%1" PRId32 "%2" PRId32 "%" PRId32, &mins, &secs, &samples);
		ret = (mins * 60 * sr) + (secs * sr) + llrint ((samples/(float)fps) * sr);
		break;

	case 6:
		sscanf (str.c_str(), "%2" PRId32 "%2" PRId32 "%" PRId32, &mins, &secs, &samples);
		ret = (mins * 60 * sr) + (secs * sr) + llrint ((samples/(float)fps) * sr);
		break;

	case 7:
		sscanf (str.c_str(), "%1" PRId32 "%2" PRId32 "%2" PRId32 "%" PRId32, &hrs, &mins, &secs, &samples);
		ret = (hrs * 3600 * sr) + (mins * 60 * sr) + (secs * sr) + llrint ((samples/(float)fps) * sr);
		break;

	case 8:
		sscanf (str.c_str(), "%2" PRId32 "%2" PRId32 "%2" PRId32 "%" PRId32, &hrs, &mins, &secs, &samples);
		ret = (hrs * 3600 * sr) + (mins * 60 * sr) + (secs * sr) + llrint ((samples/(float)fps) * sr);
		break;

	default:
		break;
	}

	return timecnt_t (ret);
}

timecnt_t
AudioClock::parse_as_bbt_distance (const std::string&)
{
	return timecnt_t ();
}

timecnt_t
AudioClock::parse_as_distance (const std::string& instr)
{
	switch (_mode) {
	case Timecode:
		return parse_as_timecode_distance (instr);
		break;
	case Samples:
		return parse_as_samples_distance (instr);
		break;
	case BBT:
		return parse_as_bbt_distance (instr);
		break;
	case MinSec:
		return parse_as_minsec_distance (instr);
		break;
	case Seconds:
		return parse_as_seconds_distance (instr);
		break;
	}
	return timecnt_t();
}

void
AudioClock::end_edit_relative (bool add)
{
	bool ok = true;

	switch (_mode) {
	case Timecode:
		ok = timecode_validate_edit (edit_string);
		break;

	case BBT:
		ok = bbt_validate_edit (edit_string);
		break;

	case MinSec:
		ok = minsec_validate_edit (edit_string);
		break;

	case Seconds:
		break;

	case Samples:
		break;
	}

	if (!ok) {
		edit_string = pre_edit_string;
		input_string.clear ();
		_layout->set_text (edit_string);
		show_edit_status (0);
		/* edit attributes remain in use */
		queue_draw ();
		return;
	}

	timecnt_t distance = parse_as_distance (input_string);

	editing = false;

	editing = false;
	_layout->set_attributes (normal_attributes);

	if (!distance.is_zero ()) {
		if (add) {
			AudioClock::set (last_when() + timepos_t (distance), true);
		} else {
			timepos_t c = last_when();

			if (c > timepos_t (distance)|| _negative_allowed) {
				AudioClock::set (c.earlier (distance), true);
			} else {
				AudioClock::set (timepos_t(), true);
			}
		}
		ValueChanged (); /* EMIT SIGNAL */
	}

	input_string.clear ();
	queue_draw ();
	drop_focus ();
}

void
AudioClock::tempo_map_changed ()
{
	if (is_duration) {
		set_duration (current_duration(), true);
	} else {
		AudioClock::set (last_when(), true);
	}
}

void
AudioClock::session_configuration_changed (std::string p)
{
	if (_negative_allowed) {
		/* session option editor clock */
		return;
	}

	if (p == "sync-source" || p == "external-sync") {
		if (is_duration) {
			set_duration (current_duration(), true);
		} else {
			AudioClock::set (last_when(), true);
		}
		return;
	}

	if (p == "timecode-offset" && p == "timecode-offset-negative") {
		switch (_mode) {
		case Timecode:
			if (is_duration) {
				set_duration (current_duration(), true);
			} else {
				AudioClock::set (last_when(), true);
			}
			break;
		default:
			break;
		}
	}
}

void
AudioClock::set_duration (Temporal::timecnt_t const & dur, bool force)
{
	is_duration = true;

	if ((!force && !get_visible()) || !_session) {
		return;
	}

	timepos_t when;
	timecnt_t d;

	/* durations for timecode display should use extent, not length
	 *
	 * reminder:
	 *     length = end - start
	 *     extent = end - start - 1
	 *
	 * and thus:
	 *     start + length = end
	 *     start + extent = last_point_inside duration
	 */

	switch (_mode) {
	case Timecode:
		d = dur.is_zero() ? dur : dur.decrement ();
		break;
	default:
		d = dur;
		break;
	}

	/* Converting a negative duration to a position throws */

	if (d.is_negative()) {
		when = timepos_t (d);
	} else {
		when = -timepos_t (-d);
	}

	if (when == last_when() && !force) {
		return;
	}

	bool btn_en = false;

	if (!editing) {

		switch (_mode) {
		case Timecode:
			set_timecode (when);
			break;

		case BBT:
			set_bbt (dur);
			btn_en = true;
			break;

		case MinSec:
			set_minsec (when);
			break;

		case Seconds:
			set_seconds (when);
			break;

		case Samples:
			set_samples (when);
			break;
		}
	}

	finish_set (when, btn_en);
	last_time = dur;
}

void
AudioClock::set (timepos_t const & w, bool force, bool round_to_beat)
{
	is_duration = false;

	timepos_t when (w);

	if ((!force && !get_visible()) || !_session) {
		return;
	}

	if (when > _limit_pos) {
		when = _limit_pos;
	} else if (when < -_limit_pos) {
		when = -_limit_pos;
	}

	if (when == last_when() && !force) {
		return;
	}

	bool btn_en = false;

	if (!editing) {

		switch (_mode) {
		case Timecode:
			set_timecode (when);
			break;

		case BBT:
			if (round_to_beat) {
				TempoMap::SharedPtr tmap (TempoMap::use());
				Temporal::BBT_Argument BBT = tmap->round_to_bar (tmap->bbt_at (when));
				when = Temporal::timepos_t::from_superclock (tmap->superclock_at(BBT));
			}
			set_bbt (timecnt_t (when));
			btn_en = true;
			break;

		case MinSec:
			set_minsec (when);
			break;

		case Seconds:
			set_seconds (when);
			break;

		case Samples:
			set_samples (when);
			break;
		}
	}

	finish_set (when, btn_en);

	last_time = timecnt_t (last_time.distance(), when);
}

void
AudioClock::finish_set (Temporal::timepos_t const & when, bool btn_en)
{
	if (_with_info) {
		_left_btn.set_sensitive (btn_en);
		_right_btn.set_sensitive (btn_en);
		_left_btn.set_visual_state (Gtkmm2ext::NoVisualState);
		_right_btn.set_visual_state (Gtkmm2ext::NoVisualState);
		if (btn_en) {
			_left_btn.set_elements (ArdourButton::Element(ArdourButton::Edge|ArdourButton::Body|ArdourButton::Text));
			_right_btn.set_elements (ArdourButton::Element(ArdourButton::Edge|ArdourButton::Body|ArdourButton::Text));
			_left_btn.set_alignment (.5, .5);
			_right_btn.set_alignment (.5, .5);
			set_tooltip (_left_btn, _("Change current tempo"));
			set_tooltip (_right_btn, _("Change current time signature"));
		} else {
			_left_btn.set_elements (ArdourButton::Text);
			_right_btn.set_elements (ArdourButton::Text);
			_left_btn.set_alignment (0, .5);
			_right_btn.set_alignment (1, .5);
			set_tooltip (_left_btn, (""));
			set_tooltip (_right_btn, (""));
		}
	}

	queue_draw ();
}

void
AudioClock::set_slave_info ()
{
	if (!_with_info || !_session) {
		return;
	}

	std::shared_ptr<TransportMaster> tm = TransportMasterManager::instance().current();

	if (_session->transport_master_is_external()) {

		switch (tm->type()) {
		case Engine:
			_left_btn.set_text (tm->name(), true);
			_right_btn.set_text ("", true);
			break;
		case MIDIClock:
			if (tm) {
				_left_btn.set_text (tm->display_name(), true);
				_right_btn.set_text (tm->delta_string (), true);
			} else {
				_left_btn.set_text (_("--pending--"), true);
				_right_btn.set_text ("", true);
			}
			break;
		case LTC:
		case MTC:
			if (tm) {
				std::shared_ptr<TimecodeTransportMaster> tcmaster;
				if ((tcmaster = std::dynamic_pointer_cast<TimecodeTransportMaster>(tm)) != 0) {
					//TODO: _left_btn.set_name () //  tcmaster->apparent_timecode_format() != _session->config.get_timecode_format();
					_left_btn.set_text (string_compose ("%1%2", tm->display_name()[0], tcmaster->position_string ()), false);
					_right_btn.set_text (tm->delta_string (), false);
				}
			} else {
				_left_btn.set_text (_("--pending--"), false);
				_right_btn.set_text ("", false);
			}
			break;
		}
	} else {
		_left_btn.set_text (string_compose ("%1/%2", _("INT"), tm->display_name()), false);
		_right_btn.set_text ("", false);
	}
}

void
AudioClock::set_out_of_bounds (bool negative)
{
	if (is_duration) {
		if (negative) {
			_layout->set_text (" >>> -- <<< ");
		} else {
			_layout->set_text (" >>> ++ <<< ");
		}
	} else {
		if (negative) {
			_layout->set_text (" <<<<<<<<<< ");
		} else {
			_layout->set_text (" >>>>>>>>>> ");
		}
	}
}

void
AudioClock::set_samples (timepos_t const & w)
{
	timepos_t when (w);
	char buf[32];
	bool negative = false;

	if (_off) {
		_layout->set_text (" ----------");
		_left_btn.set_text ("", false);
		_right_btn.set_text ("", false);
		return;
	}

	if (when.is_negative ()) {
		when = -when;
		negative = true;
	}

	if (when >= _limit_pos) {
		set_out_of_bounds (negative);
	} else if (negative) {
		snprintf (buf, sizeof (buf), "-%10" PRId64, when.samples());
		_layout->set_text (buf);
	} else {
		snprintf (buf, sizeof (buf), " %10" PRId64, when.samples());
		_layout->set_text (buf);
	}

	if (_with_info) {
		samplecnt_t rate = _session->sample_rate();

		if (fmod (rate, 100.0) == 0.0) {
			sprintf (buf, "%.1fkHz", rate/1000.0);
		} else {
			sprintf (buf, "%" PRId64 "Hz", rate);
		}

		_left_btn.set_text (string_compose ("%1 %2", _("SR"), buf), false);

		float vid_pullup = _session->config.get_video_pullup();

		if (vid_pullup == 0.0) {
			_right_btn.set_text ("", false);
		} else {
			sprintf (buf, _("%+.4f%%"), vid_pullup);
			_right_btn.set_text (string_compose ("%1 %2", _("Pull"), buf), false);
		}
	}
}

void
AudioClock::set_seconds (timepos_t const & when)
{
	char buf[32];

	if (_off) {
		_layout->set_text (" ----------");
		_left_btn.set_text ("", false);
		_right_btn.set_text ("", false);
		return;
	}

	if (when >= _limit_pos || when <= -_limit_pos) {
		set_out_of_bounds (when.is_negative ());
	} else {
		if (when.is_negative ()) {
			snprintf (buf, sizeof (buf), "%12.1f", when.samples() / (float)_session->sample_rate());
		} else {
			snprintf (buf, sizeof (buf), " %11.1f", when.samples() / (float)_session->sample_rate());
		}
		_layout->set_text (buf);
	}

	set_slave_info();
}

void
AudioClock::print_minsec (samplepos_t when, char* buf, size_t bufsize, float sample_rate, int decimals)
{
	samplecnt_t left;
	int hrs;
	int mins;
	int secs;
	int millisecs;
	bool negative;

	if (when < 0) {
		when = -when;
		negative = true;
	} else {
		negative = false;
	}

	float dmult;
	switch (decimals) {
		case 1:
			dmult = 10.f;
			break;
		case 2:
			dmult = 100.f;
			break;
		default:
		case 3:
			dmult = 1000.f;
			break;
	}

	left = when;
	hrs = (int) floor (left / (sample_rate * 60.0f * 60.0f));
	left -= (samplecnt_t) floor (hrs * sample_rate * 60.0f * 60.0f);
	mins = (int) floor (left / (sample_rate * 60.0f));
	left -= (samplecnt_t) floor (mins * sample_rate * 60.0f);
	secs = (int) floor (left / (float) sample_rate);
	left -= (samplecnt_t) floor ((double)(secs * sample_rate));
	millisecs = floor (left * dmult / (float) sample_rate);

	switch (decimals) {
		case 0:
			snprintf (buf, bufsize, "%c%02" PRId32 ":%02" PRId32 ":%02" PRId32, negative ? '-' : ' ', hrs, mins, secs);
			break;
		case 1:
			snprintf (buf, bufsize, "%c%02" PRId32 ":%02" PRId32 ":%02" PRId32 ".%01" PRId32, negative ? '-' : ' ', hrs, mins, secs, millisecs);
			break;
		case 2:
			snprintf (buf, bufsize, "%c%02" PRId32 ":%02" PRId32 ":%02" PRId32 ".%02" PRId32, negative ? '-' : ' ', hrs, mins, secs, millisecs);
			break;
		default:
		case 3:
			snprintf (buf, bufsize, "%c%02" PRId32 ":%02" PRId32 ":%02" PRId32 ".%03" PRId32, negative ? '-' : ' ', hrs, mins, secs, millisecs);
			break;
	}
}

void
AudioClock::set_minsec (timepos_t const & when)
{
	char buf[32];

	if (_off) {
		_layout->set_text (" --:--:--.---");
		_left_btn.set_text ("", false);
		_right_btn.set_text ("", false);

		return;
	}

	if (when >= _limit_pos || when <= -_limit_pos) {
		set_out_of_bounds (when.is_negative ());
	} else {
		print_minsec (when.samples(), buf, sizeof (buf), _session->sample_rate());
		_layout->set_text (buf);
	}

	set_slave_info();
}

void
AudioClock::set_timecode (timepos_t const & w)
{
	timepos_t when (w);
	Timecode::Time TC;
	bool negative = false;

	if (_off) {
		_layout->set_text (" --:--:--:--");
		_left_btn.set_text ("", false);
		_right_btn.set_text ("", false);
		return;
	}

	if (when.is_negative ()) {
		when = -when;
		negative = true;
	}
	if (when >= _limit_pos) {
		set_out_of_bounds (negative);
		set_slave_info();
		return;
	}

	if (is_duration) {
		_session->timecode_duration (when.samples(), TC);
	} else {
		_session->timecode_time (when.samples(), TC);
	}

	TC.negative = TC.negative || negative;

	_layout->set_text (Timecode::timecode_format_time(TC));

	set_slave_info();
}

void
AudioClock::set_bbt (timecnt_t const & w)
{
	timepos_t pos (w);
	char buf[64];
	Temporal::BBT_Time BBT;
	bool negative = false;

	if (_off || pos >= _limit_pos || pos < -_limit_pos) {
		_layout->set_text (" ---|--|----");
		_left_btn.set_text ("", false);
		_right_btn.set_text ("", false);
		return;
	}

	if (pos.is_negative ()) {
		pos = -pos;
		negative = true;
	}

	/* handle a common case */

	TempoMap::SharedPtr tmap (TempoMap::use());

	if (is_duration) {
		if (pos.is_zero ()) {
			BBT.bars = 0;
			BBT.beats = 0;
			BBT.ticks = 0;
		} else {

			const int divisions = tmap->meter_at (w.position()).divisions_per_bar();
			Temporal::BBT_Time sub_bbt;

			if (negative) {
				BBT = tmap->bbt_at (timepos_t (tmap->quarters_at (w.position())));
				sub_bbt = tmap->bbt_at (last_when().earlier (w.position()));
			} else {
				BBT = tmap->bbt_at (timepos_t (tmap->quarters_at (w.end())));
				sub_bbt = tmap->bbt_at (w.position());
			}

			BBT.bars -= sub_bbt.bars;

			if (BBT.ticks < sub_bbt.ticks) {
				if (BBT.beats == 1) {
					BBT.bars--;
					BBT.beats = divisions;
				} else {
					BBT.beats--;
				}
				BBT.ticks = Temporal::ticks_per_beat - (sub_bbt.ticks - BBT.ticks);
			} else {
				BBT.ticks -= sub_bbt.ticks;
			}

			if (BBT.beats < sub_bbt.beats) {
				BBT.bars--;
				BBT.beats = divisions - (sub_bbt.beats - BBT.beats);
			} else {
				BBT.beats -= sub_bbt.beats;
			}
		}
	} else {
		BBT = tmap->bbt_at (pos);
	}

	if (negative) {
		snprintf (buf, sizeof (buf), "-%03" PRIu32 BBT_BAR_CHAR "%02" PRIu32 BBT_BAR_CHAR "%04" PRIu32,
			  BBT.bars, BBT.beats, BBT.ticks);
	} else {
		snprintf (buf, sizeof (buf), " %03" PRIu32 BBT_BAR_CHAR "%02" PRIu32 BBT_BAR_CHAR "%04" PRIu32,
			  BBT.bars, BBT.beats, BBT.ticks);
	}

	_layout->set_text (buf);

	if (_with_info) {

		TempoMetric m (tmap->metric_at (pos));

		if (m.tempo().note_type() == 4) {
			snprintf (buf, sizeof(buf), u8"\u2669 = %.3f", m.tempo().note_types_per_minute_at_DOUBLE (pos));
			_left_btn.set_text (string_compose ("%1", buf), false);
		} else if (m.tempo().note_type() == 8) {
			snprintf (buf, sizeof(buf), u8"\u266a = %.3f", m.tempo().note_types_per_minute_at_DOUBLE (pos));
			_left_btn.set_text (string_compose ("%1", buf), false);
		} else {
			snprintf (buf, sizeof(buf), "1/%d = %.3f",m.tempo().note_type(), m.tempo().note_types_per_minute_at_DOUBLE (pos));
			_left_btn.set_text (buf, false);
		}

		snprintf (buf, sizeof(buf), "%d/%d", m.meter().divisions_per_bar(), m.meter().note_value());
		_right_btn.set_text (string_compose ("%1: %2", S_("TimeSignature|TS"), buf), false);
	}
}

void
AudioClock::set_session (Session *s)
{
	SessionHandlePtr::set_session (s);

	if (_session) {

		int64_t limit_sec = UIConfiguration::instance().get_clock_display_limit ();

		if (limit_sec > 0) {
			_limit_pos = timepos_t (limit_sec * _session->sample_rate());
		}

		Config->ParameterChanged.connect (_session_connections, invalidator (*this), std::bind (&AudioClock::session_configuration_changed, this, _1), gui_context());
		_session->config.ParameterChanged.connect (_session_connections, invalidator (*this), std::bind (&AudioClock::session_configuration_changed, this, _1), gui_context());

		/* try load v6 style settings (session file) */
		XMLNode* node = _session->extra_xml (X_("ClockModes"));
		if (!node) {
			/* get new v7+ settings from instant_xml */
			node = ARDOUR_UI::instance ()->clock_mode_settings ();
		}

		for (XMLNodeList::const_iterator i = node->children().begin(); i != node->children().end(); ++i) {
			std::string name;
			if ((*i)->get_property (X_("name"), name) && name == _name) {

				AudioClock::Mode amode;
				if ((*i)->get_property (X_("mode"), amode)) {
					set_mode (amode);
				}
				bool on;
				if ((*i)->get_property (X_("on"), on)) {
					set_off (!on);
				}
				break;
			}
		}

		if (is_duration) {
			AudioClock::set_duration (current_duration (), true);
		} else {
			AudioClock::set (last_when(), true);
		}
	}
}

bool
AudioClock::on_key_press_event (GdkEventKey* ev)
{
	if (!editing) {
		return false;
	}

	string new_text;
	char new_char = 0;
	int highlight_length;
	samplepos_t pos;

	switch (ev->keyval) {
	case GDK_0:
	case GDK_KP_0:
		new_char = '0';
		break;
	case GDK_1:
	case GDK_KP_1:
		new_char = '1';
		break;
	case GDK_2:
	case GDK_KP_2:
		new_char = '2';
		break;
	case GDK_3:
	case GDK_KP_3:
		new_char = '3';
		break;
	case GDK_4:
	case GDK_KP_4:
		new_char = '4';
		break;
	case GDK_5:
	case GDK_KP_5:
		new_char = '5';
		break;
	case GDK_6:
	case GDK_KP_6:
		new_char = '6';
		break;
	case GDK_7:
	case GDK_KP_7:
		new_char = '7';
		break;
	case GDK_8:
	case GDK_KP_8:
		new_char = '8';
		break;
	case GDK_9:
	case GDK_KP_9:
		new_char = '9';
		break;

	case GDK_minus:
	case GDK_KP_Subtract:
		if (_negative_allowed && input_string.empty()) {
				edit_is_negative = true;
				edit_string.replace(0,1,"-");
				_layout->set_text (edit_string);
				queue_draw ();
		} else {
			end_edit_relative (false);
		}
		return true;
		break;

	case GDK_plus:
		end_edit_relative (true);
		return true;
		break;

	case GDK_Tab:
	case GDK_Return:
	case GDK_KP_Enter:
		end_edit (true);
		return true;
		break;

	case GDK_Escape:
		end_edit (false);
		return true;

	case GDK_Delete:
	case GDK_BackSpace:
		if (!input_string.empty()) {
			/* delete the last key entered
			*/
			input_string = input_string.substr (0, input_string.length() - 1);
		}
		goto use_input_string;

	default:
		/* do not allow other keys to passthru to the rest of the GUI
		   when editing.
		*/
		return true;
	}

	if (!insert_map.empty() && (input_string.length() >= insert_map.size())) {
		/* too many digits: eat the key event, but do nothing with it */
		return true;
	}

	input_string.push_back (new_char);

  use_input_string:

	switch (_mode) {
	case Samples:
		/* get this one in the right order, and to the right width */
		if (ev->keyval == GDK_Delete || ev->keyval == GDK_BackSpace) {
			edit_string = edit_string.substr (0, edit_string.length() - 1);
		} else {
			edit_string.push_back (new_char);
		}
		if (!edit_string.empty()) {
			char buf[32];
			sscanf (edit_string.c_str(), "%" PRId64, &pos);
			snprintf (buf, sizeof (buf), " %10" PRId64, pos);
			edit_string = buf;
		}
		/* highlight the whole thing */
		highlight_length = edit_string.length();
		break;

	default:
		highlight_length = merge_input_and_edit_string ();
	}

	if (edit_is_negative) {
		edit_string.replace(0,1,"-");
	} else {
		if (!pre_edit_string.empty() && (pre_edit_string.at(0) == '-')) {
			edit_string.replace(0,1,"_");
		} else {
			edit_string.replace(0,1," ");
		}
	}

	show_edit_status (highlight_length);
	_layout->set_text (edit_string);
	queue_draw ();

	return true;
}

int
AudioClock::merge_input_and_edit_string ()
{
	/* merge with pre-edit-string into edit string */

	edit_string = pre_edit_string;

	if (input_string.empty()) {
		return 0;
	}

	string::size_type target;
	for (string::size_type i = 0; i < input_string.length(); ++i) {
		target = insert_map[input_string.length() - 1 - i];
		edit_string[target] = input_string[i];
	}
	/* highlight from end to wherever the last character was added */
	return edit_string.length() - insert_map[input_string.length()-1];
}


bool
AudioClock::on_key_release_event (GdkEventKey *ev)
{
	if (!editing) {
		return false;
	}

	/* return true for keys that we used on press
	   so that they cannot possibly do double-duty
	*/
	switch (ev->keyval) {
	case GDK_0:
	case GDK_KP_0:
	case GDK_1:
	case GDK_KP_1:
	case GDK_2:
	case GDK_KP_2:
	case GDK_3:
	case GDK_KP_3:
	case GDK_4:
	case GDK_KP_4:
	case GDK_5:
	case GDK_KP_5:
	case GDK_6:
	case GDK_KP_6:
	case GDK_7:
	case GDK_KP_7:
	case GDK_8:
	case GDK_KP_8:
	case GDK_9:
	case GDK_KP_9:
	case GDK_period:
	case GDK_comma:
	case GDK_KP_Decimal:
	case GDK_Tab:
	case GDK_Return:
	case GDK_KP_Enter:
	case GDK_Escape:
	case GDK_minus:
	case GDK_plus:
	case GDK_KP_Add:
	case GDK_KP_Subtract:
		return true;
	default:
		return false;
	}
}

AudioClock::Field
AudioClock::index_to_field (int index) const
{
	switch (_mode) {
	case Timecode:
		if (index < 4) {
			return Timecode_Hours;
		} else if (index < 7) {
			return Timecode_Minutes;
		} else if (index < 10) {
			return Timecode_Seconds;
		} else {
			return Timecode_frames;
		}
		break;
	case BBT:
		if (index < 5) {
			return Bars;
		} else if (index < 7) {
			return Beats;
		} else {
			return Ticks;
		}
		break;
	case MinSec:
		if (index < 3) {
			return Timecode_Hours;
		} else if (index < 6) {
			return MS_Minutes;
		} else if (index < 9) {
			return MS_Seconds;
		} else {
			return MS_Milliseconds;
		}
		break;
	case Seconds:
		if (index < 10) {
			return SS_Seconds;
		} else {
			return SS_Deciseconds;
		}
		break;
	case Samples:
		return S_Samples;
		break;
	}

	return Field (0);
}

bool
AudioClock::on_button_press_event (GdkEventButton *ev)
{
	if (!_session || _session->actively_recording()) {
		/* swallow event, do nothing */
		return true;
	}

	switch (ev->button) {
	case 1:
		if (editable && !_off) {
			int index;
			int trailing;
			int y;
			int x;

			/* the text has been centered vertically, so adjust
			 * x and y.
			 */
			int xcenter = (get_width() - layout_width) /2;

			y = ev->y - ((get_height() - layout_height)/2);
			x = ev->x - xcenter;

			if (!_layout->xy_to_index (x * PANGO_SCALE, y * PANGO_SCALE, index, trailing)) {
				/* pretend it is a character on the far right */
				index = 99;
			}
			drag_field = index_to_field (index);
			dragging = true;
			/* make absolutely sure that the pointer is grabbed */
			gdk_pointer_grab(ev->window,false ,
					 GdkEventMask( Gdk::POINTER_MOTION_MASK | Gdk::BUTTON_PRESS_MASK |Gdk::BUTTON_RELEASE_MASK),
					 NULL,NULL,ev->time);
			drag_accum = 0;
			drag_start_y = ev->y;
			drag_y = ev->y;
		}
		break;

	default:
		return false;
		break;
	}

	return true;
}

bool
AudioClock::on_button_release_event (GdkEventButton *ev)
{
	if (!_session || _session->actively_recording()) {
		/* swallow event, do nothing */
		return true;
	}

	if (editable && !_off) {
		if (dragging) {
			gdk_pointer_ungrab (GDK_CURRENT_TIME);
			dragging = false;
			if (ev->y > drag_start_y+1 || ev->y < drag_start_y-1 || Keyboard::modifier_state_equals (ev->state, Keyboard::TertiaryModifier)){
				// we actually dragged so return without
				// setting editing focus, or we shift clicked
				return true;
			} else {
				if (ev->button == 1) {
					start_edit ();
				}
			}
		}
	}

	if (Keyboard::is_context_menu_event (ev)) {
		if (ops_menu == 0) {
			build_ops_menu ();
		}
		ops_menu->popup (ev->button, ev->time);
		return true;
	}

	return false;
}

bool
AudioClock::on_enter_notify_event (GdkEventCrossing* ev)
{
	if (UIConfigurationBase::instance().get_widget_prelight() && editable && !_off) {
		_hovering = true;
		CairoWidget::set_dirty ();
	}
	return CairoWidget::on_enter_notify_event (ev);
}

bool
AudioClock::on_leave_notify_event (GdkEventCrossing* ev)
{
	_hovering = false;

	if (UIConfigurationBase::instance().get_widget_prelight()) {
		CairoWidget::set_dirty ();
  }

	return CairoWidget::on_leave_notify_event (ev);
}

bool
AudioClock::on_focus_out_event (GdkEventFocus* ev)
{
	bool ret = CairoWidget::on_focus_out_event (ev);

	if (editing) {
		end_edit (_accept_on_focus_out);
	}

	return ret;
}

bool
AudioClock::on_scroll_event (GdkEventScroll *ev)
{
	int index;
	int trailing;

	if (editing || _session == 0 || !editable || _off || _session->actively_recording())  {
		return false;
	}

	int y;
	int x;

	/* the text has been centered vertically, so adjust
	 * x and y.
	 */

	int xcenter = (get_width() - layout_width) /2;
	y = ev->y - ((get_height() - layout_height)/2);
	x = ev->x - xcenter;

	if (!_layout->xy_to_index (x * PANGO_SCALE, y * PANGO_SCALE, index, trailing)) {
		/* not in the main layout */
		return false;
	}

	Field f = index_to_field (index);
	timepos_t step;
	timecnt_t beat = timecnt_t (Temporal::Beats (1, 0));

	switch (ev->direction) {

	case GDK_SCROLL_UP:
		step = get_incremental_step (f, last_when());
		if (!step.is_zero ()) {
			if (Keyboard::modifier_state_equals (ev->state, Keyboard::PrimaryModifier)) {
				step *= 10;
			}
			if (is_duration) {
				AudioClock::set_duration (current_duration() + step, true); // XXX step is too small ?!
			} else {
				AudioClock::set (last_when() + step, true, step > beat);
			}
			ValueChanged (); /* EMIT_SIGNAL */
		}
		break;

	case GDK_SCROLL_DOWN:
		step = get_incremental_step (f, last_when());
		if (!step.is_zero ()) {
			if (Keyboard::modifier_state_equals (ev->state, Keyboard::PrimaryModifier)) {
				step *= 10;
			}
			if (is_duration) {
				if (!(current_duration () - step).is_negative()) {
					AudioClock::set_duration (current_duration () - step, true);
				}
			} else if (!_negative_allowed && last_when() < step) {
				AudioClock::set (timepos_t (), true);
			} else {
				AudioClock::set (last_when().earlier (step), true, step > beat);
			}

			ValueChanged (); /* EMIT_SIGNAL */
		}
		break;

	default:
		return false;
		break;
	}

	return true;
}

bool
AudioClock::on_motion_notify_event (GdkEventMotion *ev)
{
	if (editing || _session == 0 || !dragging || _session->actively_recording()) {
		return false;
	}

	const double y_delta = ev->y - drag_y;

	drag_accum += y_delta;
	drag_y = ev->y;

	if (drag_accum) {


		timepos_t pos = last_when ();
		timepos_t step = get_incremental_step (drag_field, pos);
		timecnt_t beat = timecnt_t (Temporal::Beats (1, 0));

		step *= fabs (drag_accum);

		if (drag_accum > 0) { /* positive, so downward motion ... decrement clock */

			if (is_duration) {
				if (!step.is_zero () && (current_duration () >= timecnt_t(step))) {
					AudioClock::set_duration (current_duration () - step, false);
				}
			} else if (!step.is_zero () && (step < pos)) {
				AudioClock::set (pos.earlier (step), false, step > beat);
			} else {
				AudioClock::set (timepos_t (), false);
			}

		} else if (is_duration) {
		/* negative so upward motion .. increment clock */
			AudioClock::set_duration (current_duration () + step, false);
		} else {
			/* negative so upward motion .. increment clock */
			AudioClock::set (pos + step, false, step > beat);
		}


		drag_accum = 0;
		ValueChanged();	 /* EMIT_SIGNAL */

	}

	return true;
}

timepos_t
AudioClock::get_incremental_step (Field field, timepos_t const & pos)
{
	timepos_t f;
	Temporal::BBT_Time BBT;

	switch (field) {
	case Timecode_Hours:
		f = timepos_t (floor (3600.0 * _session->sample_rate()));
		break;
	case Timecode_Minutes:
		f = timepos_t (floor (60.0 * _session->sample_rate()));
		break;
	case Timecode_Seconds:
		f = timepos_t (_session->sample_rate());
		break;
	case Timecode_frames:
		f = timepos_t (floor (_session->sample_rate() / _session->timecode_frames_per_second()));
		break;

	case S_Samples:
		f = timepos_t (1);
		break;

	case SS_Seconds:
		f = timepos_t (_session->sample_rate());
		break;
	case SS_Deciseconds:
		f = timepos_t (_session->sample_rate() / 10.f);
		break;

	case MS_Hours:
		f = timepos_t (floor (3600.0 * _session->sample_rate()));
		break;
	case MS_Minutes:
		f = timepos_t (floor (60.0 * _session->sample_rate()));
		break;
	case MS_Seconds:
		f = timepos_t (_session->sample_rate());
		break;
	case MS_Milliseconds:
		f = timepos_t (floor (_session->sample_rate() / 1000.0));
		break;

	case Bars:
		BBT.bars = 1;
		BBT.beats = 0;
		BBT.ticks = 0;
		f = TempoMap::use()->bbt_duration_at (pos,BBT);
		break;
	case Beats:
		f = timepos_t (Temporal::Beats (1, 0));
		break;
	case Ticks:
		f = timepos_t (Temporal::Beats (0, 1));
		break;
	default:
		error << string_compose (_("programming error: %1"), "attempt to get samples from non-text field!") << endmsg;
		f = timepos_t (0);
		break;
	}

	return f;
}

timecnt_t
AudioClock::current_duration (timepos_t pos) const
{
	assert (is_duration);

	timecnt_t ret;

	switch (_mode) {
	case BBT:
		ret = timecnt_t (sample_duration_from_bbt_string (pos, _layout->get_text()));
		break;

	case Timecode:
	case MinSec:
	case Seconds:
	case Samples:
		ret = last_time;
		break;
	}

	return ret;
}

bool
AudioClock::bbt_validate_edit (string & str)
{
	BBT_Time bbt;

	if (sscanf (str.c_str(), BBT_SCANF_FORMAT, &bbt.bars, &bbt.beats, &bbt.ticks) != 3) {
		return false;
	}

	if (bbt.ticks > Temporal::ticks_per_beat) {
		return false;
	}

	if (!is_duration && bbt.bars == 0) {
		return false;
	}

	if (!is_duration && bbt.beats == 0) {
		/* user could not have mean zero beats because for a
		 * non-duration clock that's impossible. Assume that they
		 * mis-entered things and meant Bar|1|ticks
		 */

		char buf[128];
		snprintf (buf, sizeof (buf), "%" PRIu32 "|%" PRIu32 "|%" PRIu32, bbt.bars, 1, bbt.ticks);
		str = buf;
	}

	return true;
}

bool
AudioClock::timecode_validate_edit (const string& str)
{
	Timecode::Time TC;
	int hours;

	if (sscanf (str.c_str(), "%*[- _]%" PRId32 ":%" PRId32 ":%" PRId32 "%*[:;]%" PRId32,
	            &hours, &TC.minutes, &TC.seconds, &TC.frames) != 4) {
		return false;
	}

	if (hours < 0) {
		TC.hours = hours * -1;
		TC.negative = true;
	} else {
		TC.hours = hours;
		TC.negative = false;
	}

	if (TC.negative && !_negative_allowed) {
		return false;
	}

	if (TC.hours > 23U || TC.minutes > 59U || TC.seconds > 59U) {
		return false;
	}

	if (TC.frames > (uint32_t) rint (_session->timecode_frames_per_second()) - 1) {
		return false;
	}

	if (_session->timecode_drop_frames()) {
		if (TC.minutes % 10 && TC.seconds == 0U && TC.frames < 2U) {
			return false;
		}
	}

	return true;
}

bool
AudioClock::minsec_validate_edit (const string& str)
{
	int hrs, mins, secs, millisecs;

	if (sscanf (str.c_str(), "%d:%d:%d.%d", &hrs, &mins, &secs, &millisecs) != 4) {
		return false;
	}

	if (hrs > 23 || mins > 59 || secs > 59 || millisecs > 999) {
		return false;
	}

	return true;
}

samplepos_t
AudioClock::samples_from_timecode_string (const string& str) const
{
	if (_session == 0) {
		return 0;
	}

	Timecode::Time TC;
	samplepos_t sample;
	int hours;

	if (sscanf (str.c_str(), "%*[- _]%d:%d:%d%*[:;]%d", &hours, &TC.minutes, &TC.seconds, &TC.frames) != 4) {
		error << string_compose (_("programming error: %1 %2"), "badly formatted timecode clock string", str) << endmsg;
		return 0;
	}
	TC.hours = abs(hours);
	TC.rate = _session->timecode_frames_per_second();
	TC.drop= _session->timecode_drop_frames();

	_session->timecode_to_sample (TC, sample, false /* use_offset */, false /* use_subframes */ );

	// timecode_tester ();
	if (edit_is_negative) {
		sample = - sample;
	}

	return sample;
}

samplepos_t
AudioClock::samples_from_minsec_string (const string& str) const
{
	if (_session == 0) {
		return 0;
	}

	int hrs, mins, secs, millisecs;
	samplecnt_t sr = _session->sample_rate();

	if (sscanf (str.c_str(), "%d:%d:%d.%d", &hrs, &mins, &secs, &millisecs) != 4) {
		error << string_compose (_("programming error: %1 %2"), "badly formatted minsec clock string", str) << endmsg;
		return 0;
	}

	return (samplepos_t) floor ((hrs * 60.0f * 60.0f * sr) + (mins * 60.0f * sr) + (secs * sr) + (millisecs * sr / 1000.0));
}

samplepos_t
AudioClock::samples_from_bbt_string (timepos_t const & pos, const string& str) const
{
	if (_session == 0) {
#ifndef NDEBUG
		cout << "AudioClock::samples_from_bbt_string() called with BBT mode but without session!\n";
#endif
		return 0;
	}

	BBT_Argument bbt;

	if (sscanf (str.c_str(), BBT_SCANF_FORMAT, &bbt.bars, &bbt.beats, &bbt.ticks) != 3) {
		return 0;
	}

	if (is_duration) {
		bbt.bars++;
		bbt.beats++;
		return TempoMap::use()->bbt_duration_at (pos, bbt).samples();
	} else {
		return TempoMap::use()->sample_at (bbt);
	}
}


samplepos_t
AudioClock::sample_duration_from_bbt_string (timepos_t const & pos, const string& str) const
{
	if (_session == 0) {
#ifndef NDEBUG
		cout << "AudioClock::sample_duration_from_bbt_string() called with BBT mode but without session!\n";
#endif
		return 0;
	}

	Temporal::BBT_Time bbt;

	if (sscanf (str.c_str(), BBT_SCANF_FORMAT, &bbt.bars, &bbt.beats, &bbt.ticks) != 3) {
		return 0;
	}

	return TempoMap::use()->bbt_duration_at(pos,bbt).samples();
}

samplepos_t
AudioClock::samples_from_seconds_string (const string& str) const
{
	float f;
	sscanf (str.c_str(), "%f", &f);
	return f * _session->sample_rate();
}

samplepos_t
AudioClock::samples_from_audiosamples_string (const string& str) const
{
	samplepos_t f;
	sscanf (str.c_str(), "%" PRId64, &f);
	return f;
}

void
AudioClock::copy_text_to_clipboard () const
{
	string val;
	if (editing) {
		val = pre_edit_string;
	} else {
		val = _layout->get_text ();
	}
	const size_t trim = val.find_first_not_of(" ");
	if (trim == string::npos) {
		assert(0); // empty clock, can't be right.
		return;
	}
	Glib::RefPtr<Clipboard> cl = Gtk::Clipboard::get();
	cl->set_text (val.substr(trim));
}

void
AudioClock::build_ops_menu ()
{
	using namespace Menu_Helpers;
	ops_menu = new Menu;
	MenuList& ops_items = ops_menu->items();
	ops_menu->set_name ("ArdourContextMenu");

	ops_items.push_back (MenuElem (_("Timecode"), sigc::bind (sigc::mem_fun(*this, &AudioClock::set_mode), Timecode)));
	ops_items.push_back (MenuElem (_("Bars:Beats"), sigc::bind (sigc::mem_fun(*this, &AudioClock::set_mode), BBT)));
	ops_items.push_back (MenuElem (_("Minutes:Seconds"), sigc::bind (sigc::mem_fun(*this, &AudioClock::set_mode), MinSec)));
	ops_items.push_back (MenuElem (_("Seconds"), sigc::bind (sigc::mem_fun(*this, &AudioClock::set_mode), Seconds)));
	ops_items.push_back (MenuElem (_("Samples"), sigc::bind (sigc::mem_fun(*this, &AudioClock::set_mode), Samples)));

	if (editable && !_off && !is_duration && !_follows_playhead) {
		ops_items.push_back (SeparatorElem());
		ops_items.push_back (MenuElem (_("Set from Playhead"), sigc::mem_fun(*this, &AudioClock::set_from_playhead)));
		ops_items.push_back (MenuElem (_("Locate to This Time"), sigc::mem_fun(*this, &AudioClock::locate)));
	}
	ops_items.push_back (SeparatorElem());
	ops_items.push_back (MenuElem (_("Copy to clipboard"), sigc::mem_fun(*this, &AudioClock::copy_text_to_clipboard)));
}

void
AudioClock::set_from_playhead ()
{
	if (!_session) {
		return;
	}

	assert (!is_duration);

	AudioClock::set (timepos_t (_session->transport_sample()));
	ValueChanged ();
}

void
AudioClock::locate ()
{
	if (!_session || is_duration) {
		return;
	}
	if (_session->actively_recording()) {
		return;
	}

	_session->request_locate (last_when().samples());
}

void
AudioClock::set_mode (Mode m)
{
	if (_mode == m) {
		return;
	}

	timecnt_t existing_duration;

	if (_session && is_duration) {
		existing_duration = current_duration ();
	}

	_mode = m;

	insert_map.clear();

	Gtk::Requisition req;
	set_clock_dimensions (req);

	switch (_mode) {
	case Timecode:
		insert_map.push_back (11);
		insert_map.push_back (10);
		insert_map.push_back (8);
		insert_map.push_back (7);
		insert_map.push_back (5);
		insert_map.push_back (4);
		insert_map.push_back (2);
		insert_map.push_back (1);
		break;

	case BBT:
		insert_map.push_back (11);
		insert_map.push_back (10);
		insert_map.push_back (9);
		insert_map.push_back (8);
		insert_map.push_back (6);
		insert_map.push_back (5);
		insert_map.push_back (3);
		insert_map.push_back (2);
		insert_map.push_back (1);
		break;

	case MinSec:
		insert_map.push_back (12);
		insert_map.push_back (11);
		insert_map.push_back (10);
		insert_map.push_back (8);
		insert_map.push_back (7);
		insert_map.push_back (5);
		insert_map.push_back (4);
		insert_map.push_back (2);
		insert_map.push_back (1);
		break;

	case Seconds:
		insert_map.push_back (11);
		insert_map.push_back (9);
		insert_map.push_back (8);
		insert_map.push_back (7);
		insert_map.push_back (6);
		insert_map.push_back (5);
		insert_map.push_back (4);
		insert_map.push_back (3);
		insert_map.push_back (2);
		insert_map.push_back (1);
		break;

	case Samples:
		break;
	}

	if (is_duration) {
		AudioClock::set_duration (existing_duration, true);
	} else {
		AudioClock::set (last_when(), true);
	}

	mode_changed (); /* EMIT SIGNAL (the member one) */
}

void
AudioClock::on_style_changed (const Glib::RefPtr<Gtk::Style>& old_style)
{
	CairoWidget::on_style_changed (old_style);

	Glib::RefPtr<Gtk::Style> const& new_style = get_style ();
	if (_layout && (_layout->get_font_description ().gobj () == 0 || _layout->get_font_description () != new_style->get_font ())) {
		_layout->set_font_description (new_style->get_font ());
		queue_resize ();
	} else if (get_realized ()) {
		queue_resize ();
	}

	Gtk::Requisition req;
	set_clock_dimensions (req);

	/* set-colors also sets up font-attributes */
	set_colors ();
}

void
AudioClock::set_editable (bool yn)
{
	editable = yn;
}

void
AudioClock::set_off (bool yn)
{
	if (_off == yn) {
		return;
	}

	_off = yn;

	/* force a redraw. last_time will be preserved, but the clock text will
	 * change
	 */

	if (is_duration) {
		set_duration (last_time, true);
	} else {
		AudioClock::set (last_when(), true);
	}
}

void
AudioClock::focus ()
{
	start_edit (Field (0));
}

void
AudioClock::set_corner_radius (double r)
{
	corner_radius = r;
	first_width = 0;
	first_height = 0;
	queue_resize ();
}

void
AudioClock::dpi_reset ()
{
	/* force recomputation of size even if we are fixed width
	 */
	first_width = 0;
	first_height = 0;

	if (get_realized ()) {
		queue_resize ();
	}
}

void
AudioClock::set_negative_allowed (bool yn)
{
	_negative_allowed = yn;
}
