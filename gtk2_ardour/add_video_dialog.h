/*
 * Copyright (C) 2013-2017 Robin Gareus <robin@gareus.org>
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
#pragma once

#include <string>

#ifdef interface
#undef interface
#endif

#include <ytkmm/box.h>
#include <ytkmm/button.h>
#include <ytkmm/cellrendererpixbuf.h>
#include <ytkmm/checkbutton.h>
#include <ytkmm/filechooserwidget.h>
#include <ytkmm/image.h>
#include <ytkmm/label.h>
#include <ytkmm/liststore.h>
#include <ytkmm/notebook.h>
#include <ytkmm/scale.h>
#include <ytkmm/treemodel.h>
#include <ytkmm/treeview.h>

#include "ardour/types.h"
#include "ardour/template_utils.h"
#include <ytkmm/filechooserwidget.h>
#include "ardour_dialog.h"

enum VtlImportOption {
 VTL_IMPORT_NONE = 0,
 VTL_IMPORT_TRANSCODE = 1,
};

class AddVideoDialog : public ArdourDialog
{
public:
	AddVideoDialog (ARDOUR::Session*);
	~AddVideoDialog ();

	std::string file_name (bool &local_file);
	VtlImportOption import_option ();
	bool launch_xjadeo ();
	bool auto_set_session_fps ();

private:
	void on_show ();
	bool page_switch();
	void set_action_ok(bool yn);
	void clear_preview_image();

	/* preview pane related */
	void request_preview(std::string vpath);
	void seek_preview();

	Gtk::Image *preview_image;
	Gtk::HScale seek_slider;
	Glib::RefPtr<Gdk::Pixbuf> imgbuf;
	std::string preview_path;
	Gtk::Label pi_tcin;
	Gtk::Label pi_tcout;
	Gtk::Label pi_aspect;
	Gtk::Label pi_fps;

	/* file chooser related */
	void file_selection_changed ();
	void file_activated ();
	bool on_video_filter (const Gtk::FileFilter::Info& filter_info);

	Gtk::FileChooserWidget chooser;

	Gtk::CheckButton  xjadeo_checkbox;
	Gtk::CheckButton  set_session_fps_checkbox;
	Gtk::Notebook notebook;
	Gtk::Button *ok_button;

	Gtk::VBox server_index_box;
	Gtk::VBox file_chooser_box;
	Gtk::HBox browser_container;

	/* Harvid Browser related */
	class HarvidColumns : public Gtk::TreeModel::ColumnRecord
	{
	public:
		Gtk::TreeModelColumn<std::string> id;
		Gtk::TreeModelColumn<std::string> uri;
		Gtk::TreeModelColumn<std::string> filename;

		HarvidColumns() {
			add(id);
			add(filename);
			add(uri);
		}
	};

	Gtk::CellRendererPixbuf pixBufRenderer;

	void harvid_list_view_selected ();
	void harvid_list_view_activated (const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn*);
	void harvid_request(std::string u);
	void harvid_load_docroot();

	bool harvid_initialized;
	Gtk::Label harvid_path;
	Gtk::Button harvid_reset;
	HarvidColumns harvid_list_columns;
	Glib::RefPtr<Gtk::ListStore> harvid_list;
	Gtk::TreeView harvid_list_view;

	bool show_advanced;
	bool loaded_docroot;
};

