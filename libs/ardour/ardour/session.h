/*
 * Copyright (C) 2000-2019 Paul Davis <paul@linuxaudiosystems.com>
 * Copyright (C) 2005-2009 Taybin Rutkin <taybin@taybin.com>
 * Copyright (C) 2006-2007 Jesse Chappell <jesse@essej.net>
 * Copyright (C) 2006-2009 Sampo Savolainen <v2@iki.fi>
 * Copyright (C) 2006-2015 David Robillard <d@drobilla.net>
 * Copyright (C) 2007-2012 Carl Hetherington <carl@carlh.net>
 * Copyright (C) 2007-2015 Tim Mayberry <mojofunk@gmail.com>
 * Copyright (C) 2008-2009 Hans Baier <hansfbaier@googlemail.com>
 * Copyright (C) 2008-2009 Sakari Bergen <sakari.bergen@beatwaves.net>
 * Copyright (C) 2012-2019 Robin Gareus <robin@gareus.org>
 * Copyright (C) 2013-2017 Nick Mainsbridge <mainsbridge@gmail.com>
 * Copyright (C) 2013-2018 John Emmas <john@creativepost.co.uk>
 * Copyright (C) 2014-2018 Ben Loftis <ben@harrisonconsoles.com>
 * Copyright (C) 2015-2018 Len Ovens <len@ovenwerks.net>
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

#ifdef WAF_BUILD
#include "libardour-config.h"
#endif

#include <atomic>
#include <exception>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <queue>
#include <stdint.h>

#include <boost/dynamic_bitset.hpp>
#include <glibmm/threads.h>

#include <ltc.h>

#include "pbd/atomic.h"
#include "pbd/error.h"
#include "pbd/event_loop.h"
#include "pbd/file_archive.h"
#include "pbd/history_owner.h"
#include "pbd/rcu.h"
#include "pbd/statefuldestructible.h"
#include "pbd/signals.h"
#include "pbd/undo.h"

#ifdef USE_TLSF
#  include "pbd/tlsf.h"
#else
#  include "pbd/reallocpool.h"
#endif
#include "lua/luastate.h"

#include "temporal/range.h"
#include "temporal/domain_provider.h"

#include "midi++/types.h"
#include "midi++/mmc.h"

#include "temporal/time.h"

#include "ardour/ardour.h"
#include "ardour/chan_count.h"
#include "ardour/delivery.h"
#include "ardour/interthread_info.h"
#include "ardour/luascripting.h"
#include "ardour/location.h"
#include "ardour/monitor_processor.h"
#include "ardour/presentation_info.h"
#include "ardour/rc_configuration.h"
#include "ardour/session_configuration.h"
#include "ardour/session_event.h"
#include "ardour/plugin.h"
#include "ardour/presentation_info.h"
#include "ardour/route.h"
#include "ardour/graph_edges.h"
#include "ardour/transport_api.h"
#include "ardour/triggerbox.h"

class XMLTree;
class XMLNode;
struct _AEffect;
typedef struct _AEffect AEffect;

class PTFFormat;

namespace MIDI {
class Port;
class MachineControl;
class Parser;
}

namespace PBD {
class Controllable;
class Progress;
class Command;
}

namespace luabridge {
	class LuaRef;
}

namespace Evoral {
class Curve;
}

namespace Temporal {
class TempoMap;
}

namespace ARDOUR {

class Amp;
class AsyncMIDIPort;
class AudioEngine;
class AudioFileSource;
class AudioRegion;
class AudioSource;
class AudioTrack;
class Auditioner;
class AutomationList;
class AuxInput;
class BufferSet;
class Bundle;
class Butler;
class Click;
class CoreSelection;
class ExportHandler;
class ExportStatus;
class Graph;
struct GraphChain;
class IO;
class IOPlug;
class IOProcessor;
class IOTaskList;
class ImportStatus;
class MidiClockTicker;
class MidiControlUI;
class MidiPortManager;
class MidiPort;
class MidiRegion;
class MidiSource;
class MidiTrack;
class MixerScene;
class Playlist;
class PluginInsert;
class PluginInfo;
class Port;
class PortInsert;
class PortManager;
class ProcessThread;
class Processor;
class Region;
class Return;
class Route;
class RouteGroup;
class RTTaskList;
class SMFSource;
class Send;
class SceneChanger;
class SessionDirectory;
class SessionMetadata;
class SessionPlaylists;
class SoloMuteRelease;
class Source;
class Speakers;
class TransportMaster;
struct TransportFSM;
class Track;
class UI_TransportMaster;
class VCAManager;
class WindowsVSTPlugin;

extern void setup_enum_writer ();

class LIBARDOUR_API SessionException: public std::exception {
public:
	explicit SessionException(const std::string msg) : _message(msg) {}
	virtual ~SessionException() throw() {}

	virtual const char* what() const throw() { return _message.c_str(); }

private:
	std::string _message;
};

/** Ardour Session */
class LIBARDOUR_API Session : public PBD::HistoryOwner,
                              public PBD::StatefulDestructible,
                              public PBD::ScopedConnectionList,
                              public SessionEventManager,
                              public TransportAPI,
                              public Temporal::TimeDomainProvider
{
public:
	/* a new session might have non-empty mix_template, an existing session should always have an empty one.
	   the bus profile can be null if no master out bus is required.
	*/

	Session (AudioEngine&,
	         const std::string& fullpath,
	         const std::string& snapshot_name,
	         BusProfile const * bus_profile = 0,
	         std::string mix_template = "",
	         bool unnamed = false,
	         samplecnt_t samplerate = 0);

	virtual ~Session ();

	static int get_info_from_path (const std::string& xmlpath, float& sample_rate, SampleFormat& data_format, std::string& program_version, XMLNode* engine_hints = 0);
	static std::string get_snapshot_from_instant (const std::string& session_dir);

	/** a monotonic counter used for naming user-visible things uniquely
	 * (curently the sidechain port).
	 * Use sparingly to keep the numbers low, prefer PBD::ID for all
	 * internal, not user-visible IDs */
	static unsigned int next_name_id ();

	std::string path() const { return _path; }
	std::string name() const { return _name; }
	std::string snap_name() const { return _current_snapshot_name; }
	std::string raid_path () const;
	bool path_is_within_session (const std::string&);

	bool writable() const { return _writable; }
	void set_clean ();   // == Clean and emit DirtyChanged IFF session was Dirty
	void set_dirty ();   // |= Dirty and emit DirtyChanged (unless already dirty or Loading, Deletion)
	void unset_dirty (bool emit_dirty_changed = false); // &= ~Dirty
	void set_deletion_in_progress ();   // |= Deletion
	void clear_deletion_in_progress (); // &= ~Deletion

	bool reconnection_in_progress () const         { return _reconnecting_routes_in_progress; }
	bool routes_deletion_in_progress () const      { return _route_deletion_in_progress; }
	bool dirty () const                            { return _state_of_the_state & Dirty; }
	bool deletion_in_progress () const             { return _state_of_the_state & Deletion; }
	bool peaks_cleanup_in_progres () const         { return _state_of_the_state & PeakCleanup; }
	bool loading () const                          { return _state_of_the_state & Loading; }
	bool cannot_save () const                      { return _state_of_the_state & CannotSave; }
	bool in_cleanup () const                       { return _state_of_the_state & InCleanup; }
	bool inital_connect_or_deletion_in_progress () const { return _state_of_the_state & (InitialConnecting | Deletion); }
	bool have_external_connections_for_current_backend (bool tracks_only = true) const;
	bool unnamed() const;

	void end_unnamed_status () const;

	PBD::Signal<void()> DirtyChanged;

	const SessionDirectory& session_directory () const { return *(_session_dir.get()); }

	static PBD::Signal<void(std::string)> Dialog;

	PBD::Signal<void()> BatchUpdateStart;
	PBD::Signal<void()> BatchUpdateEnd;

	int ensure_subdirs ();

	std::string automation_dir () const;  ///< Automation data
	std::string analysis_dir () const;    ///< Analysis data
	std::string plugins_dir () const;     ///< Plugin state
	std::string externals_dir () const;   ///< Links to external files

	std::string construct_peak_filepath (const std::string& audio_path, const bool in_session = false, const bool old_peak_name = false) const;

	bool audio_source_name_is_unique (const std::string& name);
	std::string format_audio_source_name (const std::string& legalized_base, uint32_t nchan, uint32_t chan, bool take_required, uint32_t cnt, bool related_exists);
	std::string new_audio_source_path_for_embedded (const std::string& existing_path);
	std::string new_audio_source_path (const std::string&, uint32_t nchans, uint32_t chan, bool take_required);
	std::string new_midi_source_path (const std::string&, bool need_source_lock = true);

	/** create a new track or bus from a template (XML path)
	 * @param how_many how many tracks or busses to create
	 * @param template_path path to xml template file
	 * @param insert_at position where to add new track, use PresentationInfo::max_order to append at the end.
	 * @param name name (prefix) of the route to create
	 * @param pd Playlist disposition
	 * @return list of newly created routes
	 */
	RouteList new_route_from_template (uint32_t how_many, PresentationInfo::order_t insert_at, const std::string& template_path, const std::string& name, PlaylistDisposition pd = NewPlaylist);
	RouteList new_route_from_template (uint32_t how_many, PresentationInfo::order_t insert_at, XMLNode&, const std::string& name, PlaylistDisposition pd = NewPlaylist);
	std::vector<std::string> get_paths_for_new_sources (bool allow_replacing, const std::string& import_file_path,
	                                                    uint32_t channels, std::vector<std::string> const & smf_track_names, bool use_smf_file_names);

	int bring_all_sources_into_session (std::function<void(uint32_t,uint32_t,std::string)> callback);

	void process (pframes_t nframes);

	void send_ltc_for_cycle (samplepos_t, samplepos_t, pframes_t);
	void send_mclk_for_cycle (samplepos_t, samplepos_t, pframes_t, samplecnt_t);

	BufferSet& get_silent_buffers (ChanCount count = ChanCount::ZERO);
	BufferSet& get_noinplace_buffers (ChanCount count = ChanCount::ZERO);
	BufferSet& get_scratch_buffers (ChanCount count = ChanCount::ZERO, bool silence = true);
	BufferSet& get_route_buffers (ChanCount count = ChanCount::ZERO, bool silence = true);
	BufferSet& get_mix_buffers (ChanCount count = ChanCount::ZERO);

	void ensure_buffers_unlocked (ChanCount howmany);

	bool have_rec_enabled_track () const;
	bool have_rec_disabled_track () const;

	bool have_captured() const { return _have_captured; }

	samplecnt_t capture_duration () const { return _capture_duration; }
	unsigned int capture_xruns () const { return _capture_xruns; }
	unsigned int export_xruns () const { return _export_xruns; }

	void refill_all_track_buffers ();
	Butler* butler() { return _butler; }
	void butler_transport_work (bool have_process_lock = false);

	void refresh_disk_space ();

	int load_routes (const XMLNode&, int);
	std::shared_ptr<RouteList const> get_routes() const {
		return routes.reader ();
	}

	std::shared_ptr<RTTaskList> rt_tasklist () { return _rt_tasklist; }
	std::shared_ptr<IOTaskList> io_tasklist () { return _io_tasklist; }

	RouteList get_routelist (bool mixer_order = false, PresentationInfo::Flag fl = PresentationInfo::MixerRoutes) const;

	CoreSelection& selection () const { return *_selection; }

	/* because the set of Stripables consists of objects managed
	 * independently, in multiple containers within the Session (or objects
	 * owned by the session), we fill out a list in-place rather than
	 * return a pointer to a copy of the (RCU) managed list, as happens
	 * with get_routes()
	 */

	void get_stripables (StripableList&, PresentationInfo::Flag fl = PresentationInfo::MixerStripables) const;
	StripableList get_stripables () const;
	std::shared_ptr<RouteList> get_tracks() const;
	std::shared_ptr<RouteList> get_routes_with_internal_returns() const;
	std::shared_ptr<RouteList> get_routes_with_regions_at (timepos_t const &) const;

	uint32_t nstripables (bool with_monitor = false) const;
	uint32_t nroutes() const { return routes.reader()->size(); }
	uint32_t ntracks () const;
	uint32_t naudiotracks () const;
	uint32_t nbusses () const;

	bool plot_process_graph (std::string const& file_name) const;

	std::shared_ptr<BundleList const> bundles () {
		return _bundles.reader ();
	}

	void notify_presentation_info_change (PBD::PropertyChange const&);

	template<class T> void foreach_route (T *obj, void (T::*func)(Route&), bool sort = true);
	template<class T> void foreach_route (T *obj, void (T::*func)(std::shared_ptr<Route>), bool sort = true);
	template<class T, class A> void foreach_route (T *obj, void (T::*func)(Route&, A), A arg, bool sort = true);

	template<class A>            void foreach_track (void (Track::*method)(A), A arg);
	template<class A1, class A2> void foreach_track (void (Track::*method)(A1, A2), A1 arg1, A2 arg2);

	void foreach_route (void (Route::*method)());
	template<class A>            void foreach_route (void (Route::*method)(A), A arg);
	template<class A1, class A2> void foreach_route (void (Route::*method)(A1, A2), A1 arg1, A2 arg2);

	static const std::string session_name_is_legal (const std::string&);

	bool io_name_is_legal (const std::string&) const;
	std::shared_ptr<Route> route_by_name (std::string) const;
	std::shared_ptr<Route> route_by_id (PBD::ID) const;
	std::shared_ptr<Stripable> stripable_by_name (std::string) const;
	std::shared_ptr<Stripable> stripable_by_id (PBD::ID) const;
	std::shared_ptr<Stripable> get_remote_nth_stripable (PresentationInfo::order_t n, PresentationInfo::Flag) const;
	std::shared_ptr<Route> get_remote_nth_route (PresentationInfo::order_t n) const;
	std::shared_ptr<Route> route_by_selected_count (uint32_t cnt) const;
	void routes_using_input_from (const std::string& str, RouteList& rl);

	bool route_name_unique (std::string) const;
	bool route_name_internal (std::string) const;

	uint32_t track_number_decimals () const {
		return _track_number_decimals;
	}

	bool get_record_enabled() const {
		return (record_status () >= Enabled);
	}

	RecordState record_status() const {
		return _record_status.load();
	}

	bool actively_recording () const {
		return record_status() == Recording;
	}

	bool record_enabling_legal () const;
	void maybe_enable_record (bool rt_context = false);
	void disable_record (bool rt_context, bool force = false);
	void step_back_from_record ();

	void set_all_tracks_record_enabled(bool);

	void maybe_write_autosave ();

	PBD::Signal<void(int)> SurroundObjectCountChanged;

	/* Emitted when all i/o connections are complete */

	PBD::Signal<void()> IOConnectionsComplete;

	/* Record status signals */

	PBD::Signal<void()> RecordStateChanged; /* signals changes in recording state (i.e. are we recording) */
	/* XXX may 2015: paul says: it isn't clear to me that this has semantics that cannot be inferrred
	   from the previous signal and session state.
	*/
	PBD::Signal<void()> RecordArmStateChanged; /* signals changes in recording arming */

	PBD::Signal<void()> UpdateRouteRecordState; /* signals potential change in route recording arming */

	PBD::Signal<void()> RecordPassCompleted;
	PBD::Signal<void()> ClearedLastCaptureSources;

	/* Emited when session is loaded */
	PBD::Signal<void()> SessionLoaded;

	/* Transport mechanism signals */

	/** Emitted on the following changes in transport state:
	 *  - stop (from the butler thread)
	 *  - change in whether or not we are looping (from the process thread)
	 *  - change in the play range (from the process thread)
	 *  - start (from the process thread)
	 *  - engine halted
	 */
	PBD::Signal<void()> TransportStateChange;

	PBD::Signal<void(samplepos_t)> PositionChanged; /* sent after any non-sequential motion */
	PBD::Signal<void(samplepos_t)> Xrun;
	PBD::Signal<void()> TransportLooped;

	/** emitted when a locate has occurred */
	PBD::Signal<void()> Located;

	PBD::Signal<void(RouteList&)> RouteAdded;
	/** Emitted when a property of one of our route groups changes.
	 *  The parameter is the RouteGroup that has changed.
	 */
	PBD::Signal<void(RouteGroup *)> RouteGroupPropertyChanged;
	/** Emitted when a route is added to one of our route groups.
	 *  First parameter is the RouteGroup, second is the route.
	 */
	PBD::Signal<void(RouteGroup *, std::weak_ptr<Route> )> RouteAddedToRouteGroup;
	/** Emitted when a route is removed from one of our route groups.
	 *  First parameter is the RouteGroup, second is the route.
	 */
	PBD::Signal<void(RouteGroup *, std::weak_ptr<Route> )> RouteRemovedFromRouteGroup;

	/** Emitted when a foldback send is created or deleted
	 */
	PBD::Signal<void()> FBSendsChanged;

	/* Step Editing status changed */
	PBD::Signal<void(bool)> StepEditStatusChange;

	/* Timecode state signals */
	PBD::Signal<void()> MtcOrLtcInputPortChanged;

	void queue_event (SessionEvent*);

	void request_roll_at_and_return (samplepos_t start, samplepos_t return_to);
	void request_bounded_roll (samplepos_t start, samplepos_t end);
	void request_roll (TransportRequestSource origin = TRS_UI);
	void request_stop (bool abort = false, bool clear_state = false, TransportRequestSource origin = TRS_UI);
	void request_locate (samplepos_t sample, bool force = false, LocateTransportDisposition ltd = RollIfAppropriate, TransportRequestSource origin = TRS_UI);

	void request_play_loop (bool yn, bool leave_rolling = false);
	bool get_play_loop () const { return play_loop; }

	samplepos_t last_transport_start () const { return _last_roll_location; }
	void goto_end ();
	void goto_start (bool and_roll = false);
	void use_rf_shuttle_speed ();
	void allow_auto_play (bool yn);

	double default_play_speed ();
	void reset_transport_speed (TransportRequestSource origin = TRS_UI);

	void trigger_stop_all (bool now = true);

	void request_transport_speed (double speed, TransportRequestSource origin = TRS_UI);
	void request_default_play_speed (double speed, TransportRequestSource origin = TRS_UI);
	void request_transport_speed_nonzero (double, TransportRequestSource origin = TRS_UI);
	void request_overwrite_buffer (std::shared_ptr<Track>, OverwriteReason);
	void adjust_playback_buffering();
	void adjust_capture_buffering();

	bool global_locate_pending() const { return _global_locate_pending; }
	bool locate_pending() const;
	bool locate_initiated() const;
	bool declick_in_progress () const;
	bool transport_locked () const;

	bool had_destructive_tracks () const;
	void set_had_destructive_tracks (bool yn);

	int wipe ();

	timepos_t current_end () const;
	timepos_t current_start () const;
	samplepos_t current_end_sample () const;
	samplepos_t current_start_sample () const;
	/** "actual" sample rate of session, set by current audioengine rate, pullup/down etc. */
	samplecnt_t sample_rate () const { return _current_sample_rate; }
	/** "native" sample rate of session, regardless of current audioengine rate, pullup/down etc */
	samplecnt_t nominal_sample_rate () const { return _base_sample_rate; }
	samplecnt_t frames_per_hour () const { return _frames_per_hour; }

	double samples_per_timecode_frame() const { return _samples_per_timecode_frame; }
	samplecnt_t timecode_frames_per_hour() const { return _timecode_frames_per_hour; }

	MIDI::byte get_mtc_timecode_bits() const {
		return mtc_timecode_bits;   /* encoding of SMTPE type for MTC */
	}

	double timecode_frames_per_second() const;
	bool timecode_drop_frames() const;

	/* Locations */

	Locations *locations() { return _locations; }

	PBD::Signal<void(Location*)>    auto_loop_location_changed;
	PBD::Signal<void(Location*)>    auto_punch_location_changed;
	PBD::Signal<void()>              locations_modified;

	void set_auto_punch_location (Location *);
	void set_auto_loop_location (Location *);
	void set_session_extents (timepos_t const & start, timepos_t const & end);
	bool session_range_is_free () const { return _session_range_is_free; }
	void set_session_range_is_free (bool);

	pframes_t get_block_size () const         { return current_block_size; }
	samplecnt_t worst_output_latency () const { return _worst_output_latency; }
	samplecnt_t worst_input_latency () const  { return _worst_input_latency; }
	samplecnt_t worst_route_latency () const  { return _worst_route_latency; }
	samplecnt_t io_latency () const           { return _io_latency; }
	samplecnt_t worst_latency_preroll () const;
	samplecnt_t worst_latency_preroll_buffer_size_ceil () const;

	PBD::Signal<void(bool)> LatencyUpdated;

	struct SaveAs {
		std::string new_parent_folder;  /* parent folder where new session folder will be created */
		std::string new_name;           /* name of newly saved session */
		bool        switch_to;     /* true if we should be working on newly saved session after save-as; false otherwise */
		bool        include_media; /* true if the newly saved session should contain references to media */
		bool        copy_media;    /* true if media files (audio, media, etc) should be copied into newly saved session; false otherwise */
		bool        copy_external; /* true if external media should be consolidated into the newly saved session; false otherwise */

		std::string final_session_folder_name; /* filled in by * Session::save_as(), provides full path to newly saved session */

		/* emitted as we make progress. 3 arguments passed to signal
		 * handler:
		 *
		 *  1: percentage complete measured as a fraction (0-1.0) of
		 *     total data copying done.
		 *  2: number of files copied so far
		 *  3: total number of files to copy
		 *
		 * Handler should return true for save-as to continue, or false
		 * to stop (and remove all evidence of partial save-as).
		 */
		PBD::Signal<bool(float,int64_t,int64_t)> Progress;

		/* if save_as() returns non-zero, this string will indicate the reason why.
		 */
		std::string failure_message;
	};

	int save_as (SaveAs&);

	/** save session
	 * @param snapshot_name name of the session (use an empty string for the current name)
	 * @param pending save a 'recovery', not full state (default: false)
	 * @param switch_to_snapshot switch to given snapshot after saving (default: false)
	 * @param template_only save a session template (default: false)
	 * @param for_archive save only data relevant for session-archive
	 * @param only_used_assets skip Sources that are not used, mainly useful with \p for_archive
	 * @return zero on success
	 */
	int save_state (std::string snapshot_name = "",
	                bool pending = false,
	                bool switch_to_snapshot = false,
	                bool template_only = false,
	                bool for_archive = false,
	                bool only_used_assets = false);

	enum ArchiveEncode {
		NO_ENCODE,
		FLAC_16BIT,
		FLAC_24BIT
	};

	int archive_session (const std::string&, const std::string&,
	                     ArchiveEncode compress_audio = FLAC_16BIT,
	                     PBD::FileArchive::CompressionLevel compression_level = PBD::FileArchive::CompressGood,
	                     bool only_used_sources = false,
	                     PBD::Progress* p = 0);

	int restore_state (std::string snapshot_name);
	int save_template (const std::string& template_name, const std::string& description = "", bool replace_existing = false);
	int save_history (std::string snapshot_name = "");
	int restore_history (std::string snapshot_name);
	void remove_state (std::string snapshot_name);
	void rename_state (std::string old_name, std::string new_name);
	void remove_pending_capture_state ();
	int rename (const std::string&);
	bool get_nsm_state () const { return _under_nsm_control; }
	void set_nsm_state (bool state) { _under_nsm_control = state; }
	bool save_default_options ();

	PBD::Signal<void(std::string)> StateSaved;
	PBD::Signal<void()> StateReady;

	/* emitted when session needs to be saved due to some internal
	 * event or condition (i.e. not in response to a user request).
	 *
	 * Only one object should
	 * connect to this signal and take responsibility.
	 *
	 * Argument is the snapshot name to use when saving.
	 */
	PBD::Signal<void(std::string)> SaveSessionRequested;

	/* emitted during a session save to allow other entities to add state, via
	 * extra XML, to the session state
	 */
	PBD::Signal<void()> SessionSaveUnderway;

	std::vector<std::string> possible_states() const;
	static std::vector<std::string> possible_states (std::string path);

	bool export_track_state (std::shared_ptr<RouteList> rl, const std::string& path);

	/// The instant xml file is written to the session directory
	void add_instant_xml (XMLNode&, bool write_to_config = true);
	XMLNode* instant_xml (const std::string& str);

	enum StateOfTheState {
		Clean = 0x0,
		Dirty = 0x1,
		CannotSave = 0x2,
		Deletion = 0x4,
		InitialConnecting = 0x8,
		Loading = 0x10,
		InCleanup = 0x20,
		PeakCleanup = 0x40
	};

	class StateProtector {
		public:
			StateProtector (Session* s) : _session (s) {
				PBD::atomic_inc (s->_suspend_save);
			}
			~StateProtector () {
				if (PBD::atomic_dec_and_test (_session->_suspend_save)) {
					while (_session->_save_queued) {
						_session->_save_queued = false;
						_session->save_state ("");
					}
					while (_session->_save_queued_pending) {
						_session->_save_queued_pending = false;
						_session->save_state ("", true);
					}
				}
			}
		private:
			Session * _session;
	};

	class LIBARDOUR_API ProcessorChangeBlocker {
	public:
		ProcessorChangeBlocker (Session* s, bool rc = true);
		~ProcessorChangeBlocker ();

	private:
		Session* _session;
		bool _reconfigure_on_delete;
	};

	RouteGroup* new_route_group (const std::string&);
	void add_route_group (RouteGroup *);
	void remove_route_group (RouteGroup* rg) { if (rg) remove_route_group (*rg); }
	void remove_route_group (RouteGroup&);
	void reorder_route_groups (std::list<RouteGroup*>);

	RouteGroup* route_group_by_name (std::string);
	RouteGroup& all_route_group() const;

	PBD::Signal<void(RouteGroup*)> route_group_added;
	PBD::Signal<void()>             route_group_removed;
	PBD::Signal<void()>             route_groups_reordered;

	void foreach_route_group (std::function<void(RouteGroup*)> f) {
		for (std::list<RouteGroup *>::iterator i = _route_groups.begin(); i != _route_groups.end(); ++i) {
			f (*i);
		}
	}

	std::list<RouteGroup*> const & route_groups () const {
		return _route_groups;
	}

	/* fundamental operations. duh. */

	std::list<std::shared_ptr<AudioTrack> > new_audio_track (
		int input_channels,
		int output_channels,
		RouteGroup* route_group,
		uint32_t how_many,
		std::string name_template,
		PresentationInfo::order_t order,
		TrackMode mode = Normal,
		bool input_auto_connect = true,
		bool trigger_visibility = false
		);

	std::list<std::shared_ptr<MidiTrack> > new_midi_track (
		const ChanCount& input, const ChanCount& output, bool strict_io,
		std::shared_ptr<PluginInfo> instrument,
		Plugin::PresetRecord* pset,
		RouteGroup* route_group, uint32_t how_many, std::string name_template,
		PresentationInfo::order_t,
		TrackMode mode,
		bool input_auto_connect,
		bool trigger_visibility = false
		);

	RouteList new_audio_route (int input_channels, int output_channels, RouteGroup* route_group, uint32_t how_many, std::string name_template, PresentationInfo::Flag, PresentationInfo::order_t);
	RouteList new_midi_route (RouteGroup* route_group, uint32_t how_many, std::string name_template, bool strict_io, std::shared_ptr<PluginInfo> instrument, Plugin::PresetRecord*, PresentationInfo::Flag, PresentationInfo::order_t);

	void remove_routes (std::shared_ptr<RouteList>);
	void remove_route (std::shared_ptr<Route>);

	void resort_routes ();

	AudioEngine & engine() { return _engine; }
	AudioEngine const & engine () const { return _engine; }

	static std::string default_track_name_pattern (DataType);

	/* Time */

	samplepos_t transport_sample () const { return _transport_sample; }
	samplepos_t record_location () const { return _last_record_location; }
	samplepos_t audible_sample (bool* latent_locate = NULL) const;
	samplepos_t requested_return_sample() const { return _requested_return_sample; }
	void set_requested_return_sample(samplepos_t return_to);
	std::optional<samplepos_t> const & nominal_jack_transport_sample() { return _nominal_jack_transport_sample; }

	bool compute_audible_delta (samplepos_t& pos_and_delta) const;
	samplecnt_t remaining_latency_preroll () const { return _remaining_latency_preroll; }

	enum PullupFormat {
		pullup_Plus4Plus1,
		pullup_Plus4,
		pullup_Plus4Minus1,
		pullup_Plus1,
		pullup_None,
		pullup_Minus1,
		pullup_Minus4Plus1,
		pullup_Minus4,
		pullup_Minus4Minus1
	};

	void sync_time_vars();

	void timecode_to_sample(Timecode::Time& timecode, samplepos_t& sample, bool use_offset, bool use_subframes) const;
	void sample_to_timecode(samplepos_t sample, Timecode::Time& timecode, bool use_offset, bool use_subframes) const;
	void timecode_time (Timecode::Time &);
	void timecode_time (samplepos_t when, Timecode::Time&);
	void timecode_time_subframes (samplepos_t when, Timecode::Time&);

	void timecode_duration (samplecnt_t, Timecode::Time&) const;
	void timecode_duration_string (char *, size_t len, samplecnt_t) const;

	samplecnt_t convert_to_samples (AnyTime const & position);
	samplecnt_t any_duration_to_samples (samplepos_t position, AnyTime const & duration);

	static PBD::Signal<void(samplepos_t)> StartTimeChanged;
	static PBD::Signal<void(samplepos_t)> EndTimeChanged;

	void   request_sync_source (std::shared_ptr<TransportMaster>);
	bool   synced_to_engine() const;

	double engine_speed() const { return _engine_speed; }
	double actual_speed() const;
	double transport_speed (bool incl_preroll = false) const;
	/** @return true if the transport state (TFSM) is stopped */
	bool   transport_stopped() const;
	/** @return true if the transport state (TFSM) is stopped or stopping */
	bool   transport_stopped_or_stopping() const;
	/** @return true if the transport state (TFSM) is rolling.
	 *  Note: the transport may not yet move if pre-roll or count-in in ongoing.
	 */
	bool   transport_state_rolling() const;
	/** @return true if the the transport is actively (audible) rolling.
	 *  playback speed is not zero, and count-in as well as latency-preroll is complete,
	 *  and _transport_sample changes every process cycle.
	 */
	bool transport_rolling() const;
	bool transport_will_roll_forwards() const;
	bool transport_locating() const;

	bool silent () { return _silent; }

	bool punch_is_possible () const;
	bool loop_is_possible () const;
	PBD::Signal<void()> PunchLoopConstraintChange;

	void maybe_update_tempo_from_midiclock_tempo (float bpm);

	unsigned int    get_xrun_count () const {return _xrun_count; }
	void            reset_xrun_count ();

	/* region info  */

	std::shared_ptr<Region> find_whole_file_parent (std::shared_ptr<Region const>) const;

	std::shared_ptr<Region>      XMLRegionFactory (const XMLNode&, bool full);
	std::shared_ptr<AudioRegion> XMLAudioRegionFactory (const XMLNode&, bool full);
	std::shared_ptr<MidiRegion>  XMLMidiRegionFactory (const XMLNode&, bool full);

	void deinterlace_midi_region ( std::shared_ptr<MidiRegion> mr );

	/* source management */

	void import_files (ImportStatus&);
	bool sample_rate_convert (ImportStatus&, std::string infile, std::string& outfile);
	std::string build_tmp_convert_name (std::string file);

	std::shared_ptr<ExportHandler> get_export_handler ();
	std::shared_ptr<ExportStatus> get_export_status ();

	int start_audio_export (samplepos_t position, bool realtime = false, bool region_export = false);

	PBD::Signal<int(samplecnt_t)> ProcessExport;
	static PBD::Signal<void(std::string, std::string, bool, samplepos_t)> Exported;

	void add_source (std::shared_ptr<Source>);
	void remove_source (std::weak_ptr<Source>, bool drop_references = true);

	void cleanup_regions();
	bool can_cleanup_peakfiles () const;
	int  cleanup_peakfiles ();
	int  cleanup_sources (CleanupReport&);
	int  cleanup_trash_sources (CleanupReport&);

	int destroy_sources (std::list<std::shared_ptr<Source> > const&);

	int remove_last_capture ();
	bool have_last_capture_sources () const;
	void last_capture_sources (std::list<std::shared_ptr<Source> >&) const;
	void reset_last_capture_sources ();

	/** handlers should return -1 for "stop cleanup",
	    0 for "yes, delete this playlist",
	    1 for "no, don't delete this playlist".
	*/
	static PBD::Signal<int(std::shared_ptr<Playlist> )>  AskAboutPlaylistDeletion;

	/** handlers should return 0 for "ignore the rate mismatch",
	    !0 for "do not use this session"
	*/
	static PBD::Signal<int(samplecnt_t, samplecnt_t)> AskAboutSampleRateMismatch;

	/** non interactive message */
	static PBD::Signal<void(samplecnt_t, samplecnt_t)> NotifyAboutSampleRateMismatch;

	/** handlers should return !0 for use pending state, 0 for ignore it.
	 */
	static PBD::Signal<int()> AskAboutPendingState;

	/** after loading a session, once all ports have been created and connected
	 * signal is emitted to let objects that need to do some housekeeping
	 * post-connect.
	 */

	static PBD::Signal<void()> AfterConnect;

	std::shared_ptr<AudioFileSource> create_audio_source_for_session (
		size_t, std::string const &, uint32_t);

	std::shared_ptr<MidiSource> create_midi_source_for_session (std::string const &);
	std::shared_ptr<MidiSource> create_midi_source_by_stealing_name (std::shared_ptr<Track>);

	std::shared_ptr<Source> source_by_id (const PBD::ID&);
	std::shared_ptr<AudioFileSource> audio_source_by_path_and_channel (const std::string&, uint16_t) const;
	std::shared_ptr<MidiSource> midi_source_by_path (const std::string&, bool need_source_lock) const;
	uint32_t count_sources_by_origin (const std::string&);

	void add_playlist (std::shared_ptr<Playlist>);

	/* Curves and AutomationLists (TODO when they go away) */
	void add_automation_list(AutomationList*);

	/* auditioning */

	std::shared_ptr<Auditioner> the_auditioner() { return auditioner; }
	void audition_playlist ();
	void audition_region (std::shared_ptr<Region>);
	void cancel_audition ();
	bool is_auditioning () const;

	PBD::Signal<void(bool)> AuditionActive;

	/* session script */
	void register_lua_function (const std::string&, const std::string&, const LuaScriptParamList&);
	void unregister_lua_function (const std::string& name);
	std::vector<std::string> registered_lua_functions ();
	uint32_t registered_lua_function_count () const { return _n_lua_scripts; }
	void scripts_changed (); // called from lua, updates _n_lua_scripts

	PBD::Signal<void()> LuaScriptsChanged;

	/* I/O Plugin */
	PBD::Signal<void()> IOPluginsChanged;

	void load_io_plugin (std::shared_ptr<IOPlug>);
	bool unload_io_plugin (std::shared_ptr<IOPlug>);

	std::shared_ptr<IOPlug> nth_io_plug (uint32_t n) {
		std::shared_ptr<IOPlugList const> iop (_io_plugins.reader ());
		if (n < iop->size ()) {
			return (*iop)[n];
		}
		return std::shared_ptr<IOPlug> ();
	}

	std::shared_ptr<IOPlugList const> io_plugs () const {
		return _io_plugins.reader ();
	}

	/* flattening stuff */

	std::shared_ptr<Region> write_one_track (Track&, samplepos_t start, samplepos_t end,
	                                         bool overwrite, std::vector<std::shared_ptr<Source> >&, InterThreadInfo& wot,
	                                         std::shared_ptr<Processor> endpoint,
	                                         bool include_endpoint, bool for_export, bool for_freeze,
	                                         std::string const& source_name = "", std::string const& region_name = "");
	int freeze_all (InterThreadInfo&);

	/* session-wide solo/mute/rec-enable */

	bool muted () const;
	std::vector<std::weak_ptr<AutomationControl> > cancel_all_mute ();

	bool soloing () const { return _non_soloed_outs_muted; }
	bool listening () const;
	bool solo_isolated () const { return _solo_isolated_cnt > 0; }
	void cancel_all_solo ();

	bool solo_selection_active ();
	void solo_selection (StripableList&, bool);

	void clear_all_solo_state (std::shared_ptr<RouteList const>);
	void prepare_momentary_solo (SoloMuteRelease* smr = NULL, bool exclusive = false, std::shared_ptr<Route> route = std::shared_ptr<Route> ());

	static const SessionEvent::RTeventCallback rt_cleanup;

	/* Control-based methods */

	void set_controls (std::shared_ptr<AutomationControlList>, double val, PBD::Controllable::GroupControlDisposition);
	void set_control (std::shared_ptr<AutomationControl>, double val, PBD::Controllable::GroupControlDisposition);

	void set_exclusive_input_active (std::shared_ptr<RouteList> rt, bool onoff, bool flip_others = false);

	PBD::Signal<void(bool)> SoloActive;
	PBD::Signal<void()> SoloChanged;
	PBD::Signal<void()> MuteChanged;
	PBD::Signal<void()> IsolatedChanged;
	PBD::Signal<void()> MonitorChanged;
	PBD::Signal<void()> MonitorBusAddedOrRemoved;
	PBD::Signal<void()> SurroundMasterAddedOrRemoved;

	PBD::Signal<void()> session_routes_reconnected;

	/* monitor/master out */
	int add_master_bus (ChanCount const&);

	void reset_monitor_section ();
	bool monitor_active() const { return (_monitor_out && _monitor_out->monitor_control () && _monitor_out->monitor_control ()->monitor_active()); }

	std::shared_ptr<Route> monitor_out() const { return _monitor_out; }
	std::shared_ptr<Route> master_out() const { return _master_out; }
	std::shared_ptr<GainControl>  master_volume () const;
	std::shared_ptr<Route> surround_master() const { return _surround_master; }

	PresentationInfo::order_t master_order_key () const { return _master_out ? _master_out->presentation_info ().order () : -1; }
	bool ensure_stripable_sort_order ();

	void globally_add_internal_sends (std::shared_ptr<Route> dest, Placement p, bool);
	void globally_set_send_gains_from_track (std::shared_ptr<Route> dest);
	void globally_set_send_gains_to_zero (std::shared_ptr<Route> dest);
	void globally_set_send_gains_to_unity (std::shared_ptr<Route> dest);
	void add_internal_sends (std::shared_ptr<Route> dest, Placement p, std::shared_ptr<RouteList> senders);
	void add_internal_send (std::shared_ptr<Route>, int, std::shared_ptr<Route>);
	void add_internal_send (std::shared_ptr<Route>, std::shared_ptr<Processor>, std::shared_ptr<Route>);

	static void set_disable_all_loaded_plugins (bool yn) {
		_disable_all_loaded_plugins = yn;
	}
	static bool get_disable_all_loaded_plugins() {
		return _disable_all_loaded_plugins;
	}
	static void set_bypass_all_loaded_plugins (bool yn) {
		_bypass_all_loaded_plugins = yn;
	}
	static bool get_bypass_all_loaded_plugins() {
		return _bypass_all_loaded_plugins;
	}

	uint32_t next_send_id();
	uint32_t next_surround_send_id();
	uint32_t next_aux_send_id();
	uint32_t next_return_id();
	uint32_t next_insert_id();
	void mark_send_id (uint32_t);
	void mark_surround_send_id (uint32_t);
	void mark_aux_send_id (uint32_t);
	void mark_return_id (uint32_t);
	void mark_insert_id (uint32_t);
	void unmark_send_id (uint32_t);
	void unmark_surround_send_id (uint32_t);
	void unmark_aux_send_id (uint32_t);
	void unmark_return_id (uint32_t);
	void unmark_insert_id (uint32_t);

	bool vapor_barrier ();
	bool vapor_export_barrier ();

	/* s/w "RAID" management */

	std::optional<samplecnt_t> available_capture_duration();

	/* I/O bundles */

	void add_bundle (std::shared_ptr<Bundle>, bool emit_signal = true);
	void remove_bundle (std::shared_ptr<Bundle>);
	std::shared_ptr<Bundle> bundle_by_name (std::string) const;

	PBD::Signal<void()> BundleAddedOrRemoved;

	void midi_panic ();

	/* History (for editors, mixers, UIs etc.) */

	/** Undo some transactions.
	 * @param n Number of transactions to undo.
	 */
	void undo (uint32_t n);
	/** Redo some transactions.
	 * @param n Number of transactions to undo.
	 */
	void redo (uint32_t n);

	PBD::UndoHistory& history() { return _history; }

	uint32_t undo_depth() const { return _history.undo_depth(); }
	uint32_t redo_depth() const { return _history.redo_depth(); }
	std::string next_undo() const { return _history.next_undo(); }
	std::string next_redo() const { return _history.next_redo(); }


	std::map<PBD::ID,PBD::StatefulDestructible*> registry;

	// these commands are implemented in libs/ardour/session_command.cc
	PBD::Command* memento_command_factory(XMLNode* n);
	PBD::Command* stateful_diff_command_factory (XMLNode *);
	void register_with_memento_command_factory(PBD::ID, PBD::StatefulDestructible*);

	/* clicking */

	std::shared_ptr<IO> click_io() { return _click_io; }
	std::shared_ptr<Amp> click_gain() { return _click_gain; }

	/* disk, buffer loads */

	uint32_t playback_load ();
	uint32_t capture_load ();

	/* ranges */

	void request_play_range (std::list<TimelineRange>*, bool leave_rolling = false);
	void request_cancel_play_range ();
	bool get_play_range () const { return _play_range; }

	void maybe_update_session_range (timepos_t const &, timepos_t const &);

	/* preroll */
	samplecnt_t preroll_samples (samplepos_t) const;

	void request_preroll_record_trim (samplepos_t start, samplecnt_t preroll);
	void request_count_in_record ();

	samplecnt_t preroll_record_trim_len () const { return _preroll_record_trim_len; }

	/* temporary hacks to allow selection to be pushed from GUI into backend.
	   Whenever we move the selection object into libardour, these will go away.
	 */
	void set_range_selection (Temporal::timepos_t const & start, Temporal::timepos_t const & end);
	void set_object_selection (Temporal::timepos_t const & start, Temporal::timepos_t const & end);
	void clear_range_selection ();
	void clear_object_selection ();

	void cut_copy_section (Temporal::timepos_t const& start, Temporal::timepos_t const& end, Temporal::timepos_t const& to, SectionOperation const op);

	/* buffers for gain and pan */

	gain_t* gain_automation_buffer () const;
	gain_t* trim_automation_buffer () const;
	gain_t* send_gain_automation_buffer () const;
	gain_t* scratch_automation_buffer () const;
	pan_t** pan_automation_buffer () const;

	/* VST support */

	static int  vst_current_loading_id;
	static const char* vst_can_do_strings[];
	static const int vst_can_do_string_count;

	static intptr_t vst_callback (
		AEffect* effect,
		int32_t opcode,
		int32_t index,
		intptr_t value,
		void* ptr,
		float opt
		);

	static PBD::Signal<void()> SendFeedback;

	/* Speakers */

	std::shared_ptr<Speakers> get_speakers ();

	/* Controllables */

	std::shared_ptr<ARDOUR::Trigger> trigger_by_id (PBD::ID) const;

	std::shared_ptr<Processor> processor_by_id (PBD::ID) const;

	std::shared_ptr<PBD::Controllable> controllable_by_id (const PBD::ID&);
	std::shared_ptr<AutomationControl> automation_control_by_id (const PBD::ID&);

	void add_controllable (std::shared_ptr<PBD::Controllable>);

	std::shared_ptr<PBD::Controllable> solo_cut_control() const;
	std::shared_ptr<PBD::Controllable> recently_touched_controllable () const;

	bool apply_nth_mixer_scene (size_t);
	bool apply_nth_mixer_scene (size_t, RouteList const&);
	void store_nth_mixer_scene (size_t);
	bool nth_mixer_scene_valid (size_t) const;
	size_t last_touched_mixer_scene_idx () {return _last_touched_mixer_scene_idx;}

	std::shared_ptr<MixerScene>              nth_mixer_scene (size_t, bool create_if_missing = false);
	std::vector<std::shared_ptr<MixerScene>> mixer_scenes () const;

	SessionConfiguration config;

	SessionConfiguration* cfg () { return &config; }

	bool exporting () const {
		return _exporting;
	}

	bool realtime_export() const {
		return _realtime_export;
	}

	bool bounce_processing() const {
		return _bounce_processing_active;
	}

	/* this is a private enum, but setup_enum_writer() needs it,
	   and i can't find a way to give that function
	   friend access. sigh.
	*/

	enum PostTransportWork {
		PostTransportStop               = 0x1,
		PostTransportLocate             = 0x2,
		PostTransportAbort              = 0x8,
		PostTransportOverWrite          = 0x10,
		PostTransportAudition           = 0x20,
		PostTransportReverse            = 0x40,
		PostTransportClearSubstate      = 0x80,
		PostTransportAdjustPlaybackBuffering  = 0x100,
		PostTransportAdjustCaptureBuffering   = 0x200,
		PostTransportLoopChanged        = 0x400
	};

	std::shared_ptr<SessionPlaylists> playlists () const { return _playlists; }

	void send_mmc_locate (samplepos_t);
	void queue_full_time_code () { _send_timecode_update = true; }
	void queue_song_position_pointer () { /* currently does nothing */ }

	bool step_editing() const { return (_step_editors > 0); }

	void request_suspend_timecode_transmission ();
	void request_resume_timecode_transmission ();
	bool timecode_transmission_suspended () const;

	std::vector<std::string> source_search_path(DataType) const;
	void ensure_search_path_includes (const std::string& path, DataType type);
	void remove_dir_from_search_path (const std::string& path, DataType type);

	std::list<std::string> unknown_processors () const;

	std::list<std::string> missing_filesources (DataType) const;

	/** Emitted when a feedback cycle has been detected within Ardour's signal
	    processing path.  Until it is fixed (by the user) some (unspecified)
	    routes will not be run.
	*/
	static PBD::Signal<void()> FeedbackDetected;

	/** Emitted when a graph sort has successfully completed, which means
	    that it has no feedback cycles.
	*/
	static PBD::Signal<void()> SuccessfulGraphSort;

	/* handlers can return an integer value:
	   0: config.set_audio_search_path() or config.set_midi_search_path() was used
	   to modify the search path and we should try to find it again.
	   1: quit entire session load
	   2: as 0, but don't ask about other missing files
	   3: don't ask about other missing files, and just mark this one missing
	   -1: just mark this one missing
	   any other value: as -1
	*/
	static PBD::Signal<int(Session*,std::string,DataType)> MissingFile;

	void set_missing_file_replacement (const std::string& mfr) {
		_missing_file_replacement = mfr;
	}

	/** Emitted when the session wants Ardour to quit */
	static PBD::Signal<void()> Quit;

	/** Emitted when Ardour is asked to load a session in an older session
	 * format, and makes a backup copy.
	 */
	static PBD::Signal<void(std::string,std::string)> VersionMismatch;

	SceneChanger* scene_changer() const { return _scene_changer; }

	/* asynchronous MIDI control ports */

	std::shared_ptr<Port> mmc_output_port () const;
	std::shared_ptr<Port> mmc_input_port () const;
	std::shared_ptr<Port> trigger_input_port () const;
	std::shared_ptr<Port> scene_input_port () const;
	std::shared_ptr<Port> scene_output_port () const;

	std::shared_ptr<AsyncMIDIPort> vkbd_output_port () const;

	/* synchronous MIDI ports used for synchronization */

	std::shared_ptr<MidiPort> midi_clock_output_port () const;
	std::shared_ptr<MidiPort> mtc_output_port () const;
	std::shared_ptr<Port> ltc_output_port() const { return _ltc_output_port; }

	MIDI::MachineControl& mmc() { return *_mmc; }

	void reconnect_midi_scene_ports (bool);
	void reconnect_mmc_ports (bool);

	void reconnect_ltc_output ();

	VCAManager& vca_manager() { return *_vca_manager; }
	VCAManager* vca_manager_ptr() { return _vca_manager; }

	void auto_connect_thread_wakeup ();

	double compute_speed_from_master (pframes_t nframes);
	bool   transport_master_is_external() const;
	bool   transport_master_no_external_or_using_engine() const;
	std::shared_ptr<TransportMaster> transport_master() const;

	void import_pt_sources (PTFFormat& ptf, ImportStatus& status);
	void import_pt_rest (PTFFormat& ptf);
	bool import_sndfile_as_region (std::string path, SrcQuality quality, timepos_t& pos, SourceList& sources, ImportStatus& status, uint32_t current, uint32_t total);

	struct ptflookup {
		uint16_t index1;
		uint16_t index2;
		PBD::ID  id;

		bool operator ==(const struct ptflookup& other) {
			return (this->index1 == other.index1);
		}
	};
	std::vector<struct ptflookup> ptfwavpair;
	SourceList pt_imported_sources;

	enum TimingTypes {
		OverallProcess = 0,
		ProcessFunction = 1,
		NoRoll = 2,
		Roll = 3,
		/* end */
		NTT = 4
	};

	PBD::TimingStats dsp_stats[NTT];

	int32_t first_cue_within (samplepos_t s, samplepos_t e, bool& was_recorded);
	void trigger_cue_row (int32_t);
	CueEvents const & cue_events() const { return _cue_events; }

	int num_triggerboxes () const;
	std::shared_ptr<TriggerBox> triggerbox_at (int32_t route_index) const;
	TriggerPtr trigger_at (int32_t route_index, int32_t row_index) const;
	bool bang_trigger_at(int32_t route_index, int32_t row_index, float velocity = 1.0);
	bool unbang_trigger_at(int32_t route_index, int32_t row_index);
	void clear_cue (int row_index);
	std::shared_ptr<TriggerBox> armed_triggerbox () const;

	void start_domain_bounce (Temporal::DomainBounceInfo&);
	void finish_domain_bounce (Temporal::DomainBounceInfo&);

	AnyTime global_quantization() const { return _global_quantization; }
	void set_global_quantization (AnyTime const &);
	PBD::Signal<void()> QuantizationChanged;

protected:
	friend class AudioEngine;
	void set_block_size (pframes_t nframes);
	void set_sample_rate (samplecnt_t nframes);

	friend class Route;
	void update_latency_compensation (bool force, bool called_from_backend);

	/* transport API */

	void locate (samplepos_t, bool for_loop_end=false, bool force=false, bool with_mmc=true);
	void stop_transport (bool abort = false, bool clear_state = false);
	void start_transport (bool after_loop);
	void butler_completed_transport_work ();
	void post_locate ();
	void schedule_butler_for_transport_work ();
	bool should_roll_after_locate () const;
	bool user_roll_after_locate () const;
	bool should_stop_before_locate () const;
	samplepos_t position() const { return _transport_sample; }
	void set_transport_speed (double speed);
	void set_default_play_speed (double spd);
	bool need_declick_before_locate () const;
	void tempo_map_changed ();

private:
	int  create (const std::string& mix_template, BusProfile const *, bool unnamed);
	void destroy ();

	static std::atomic<unsigned int> _name_id_counter;
	static void init_name_id_counter (unsigned int n);
	static unsigned int name_id_counter ();

	std::shared_ptr<SessionPlaylists> _playlists;

	/* stuff used in process() should be close together to
	   maximise cache hits
	*/

	typedef void (Session::*process_function_type)(pframes_t);

	AudioEngine&            _engine;
	process_function_type    process_function;
	process_function_type    last_process_function;
	bool                    _bounce_processing_active;
	bool                     waiting_for_sync_offset;
	samplecnt_t             _base_sample_rate;     // sample-rate of the session at creation time, "native" SR
	samplecnt_t             _current_sample_rate;  // this includes video pullup offset
	samplepos_t             _transport_sample;
	std::atomic<int>       _seek_counter;
	std::atomic<int>       _butler_seek_counter;
	Location*               _session_range_location; ///< session range, or 0 if there is nothing in the session yet
	bool                    _session_range_is_free;
	bool                    _silent;
	samplecnt_t             _remaining_latency_preroll;
	size_t                  _last_touched_mixer_scene_idx;

	// varispeed playback -- TODO: move out of session to backend.
	double                  _engine_speed;
	double                  _signalled_varispeed;

	bool                     auto_play_legal;
	samplepos_t             _requested_return_sample;
	pframes_t                current_block_size;
	samplecnt_t             _worst_output_latency;
	samplecnt_t             _worst_input_latency;
	samplecnt_t             _worst_route_latency;
	samplecnt_t             _io_latency;
	uint32_t                _send_latency_changes;
	bool                    _update_send_delaylines;
	bool                    _have_captured;
	samplecnt_t             _capture_duration;
	unsigned int            _capture_xruns;
	unsigned int            _export_xruns;
	bool                    _non_soloed_outs_muted;
	bool                    _listening;
	uint32_t                _listen_cnt;
	uint32_t                _solo_isolated_cnt;
	bool                    _writable;
	bool                    _was_seamless;
	bool                    _under_nsm_control;
	unsigned int            _xrun_count;

	std::string             _missing_file_replacement;

	mutable std::atomic<int> _processing_prohibited;
	mutable std::atomic<RecordState> _record_status;

	void add_monitor_section ();
	void remove_monitor_section ();

	void add_surround_master ();
	void remove_surround_master ();

	std::optional<bool> _vapor_available;
	std::optional<bool> _vapor_exportable;

	void update_latency (bool playback);
	void set_owned_port_public_latency (bool playback);
	bool update_route_latency (bool reverse, bool apply_to_delayline, bool* delayline_update_needed);
	void initialize_latencies ();
	void set_worst_output_latency ();
	void set_worst_input_latency ();

	void send_latency_compensation_change ();
	void update_send_delaylines ();

	void setup_engine_resampling ();

	void ensure_buffers (ChanCount howmany = ChanCount::ZERO);
	ChanCount _required_thread_buffers;
	size_t    _required_thread_buffersize;

	void process_without_events (pframes_t);
	void process_with_events    (pframes_t);
	void process_audition       (pframes_t);
	void process_export         (pframes_t);
	void process_export_fw      (pframes_t);

	samplecnt_t calc_preroll_subcycle (samplecnt_t) const;

	void block_processing();
	void unblock_processing() { _processing_prohibited.store (0); }
	bool processing_blocked() const { return _processing_prohibited.load (); }

	static const samplecnt_t bounce_chunk_size;

	/* Transport master DLL */

	enum TransportMasterState {
		Stopped, /* no incoming or invalid signal/data for master to run with */
		Waiting, /* waiting to get full lock on incoming signal/data */
		Running  /* lock achieved, master is generating meaningful speed & position */
	};

	samplepos_t master_wait_end;

	enum TransportMasterAction {
		TransportMasterRelax,
		TransportMasterNoRoll,
		TransportMasterLocate,
		TransportMasterStart,
		TransportMasterStop,
		TransportMasterWait,
	};

	struct TransportMasterStrategy {
		TransportMasterAction action;
		samplepos_t target;
		LocateTransportDisposition roll_disposition;
		double catch_speed;

		TransportMasterStrategy ()
			: action (TransportMasterRelax)
			, target (0)
			, roll_disposition (MustStop)
			, catch_speed (0.) {}
	};

	TransportMasterStrategy transport_master_strategy;
	double plan_master_strategy (pframes_t nframes, double master_speed, samplepos_t master_transport_sample, double catch_speed);
	double plan_master_strategy_engine (pframes_t nframes, double master_speed, samplepos_t master_transport_sample, double catch_speed);
	bool implement_master_strategy ();

	bool follow_transport_master (pframes_t nframes);

	void sync_source_changed (SyncSource, samplepos_t pos, pframes_t cycle_nframes);

	bool post_export_sync;
	samplepos_t post_export_position;

	bool _exporting;
	bool _export_rolling;
	bool _realtime_export;
	bool _region_export;
	samplepos_t _export_preroll;

	std::shared_ptr<ExportHandler> export_handler;
	std::shared_ptr<ExportStatus>  export_status;

	int  pre_export ();
	int  stop_audio_export ();
	void finalize_audio_export (TransportRequestSource trs);
	void finalize_export_internal (bool stop_freewheel);
	bool _pre_export_mmc_enabled;

	PBD::ScopedConnection export_freewheel_connection;

	void get_track_statistics ();
	int  process_routes (pframes_t, bool& need_butler);
	int  silent_process_routes (pframes_t, bool& need_butler);

	/** @return 1 if there is a pending declick fade-in,
	    -1 if there is a pending declick fade-out,
	    0 if there is no pending declick.
	*/
	bool maybe_stop (samplepos_t limit);
	bool maybe_sync_start (pframes_t &);

	std::string             _path;
	std::string             _name;
	bool                    _is_new;
	bool                    _send_qf_mtc;
	/** Number of process samples since the last MTC output (when sending MTC); used to
	 *  know when to send full MTC messages every so often.
	 */
	pframes_t               _pframes_since_last_mtc;
	std::atomic<bool>        play_loop;
	bool                     loop_changing;
	samplepos_t              last_loopend;

	const std::unique_ptr<SessionDirectory> _session_dir;

	void hookup_io ();
	void graph_reordered (bool called_from_backend);

	/** current snapshot name, without the .ardour suffix */
	void set_snapshot_name (const std::string &);
	void save_snapshot_name (const std::string &);
	std::string _current_snapshot_name;

	XMLTree*         state_tree;
	StateOfTheState _state_of_the_state;

	friend class     StateProtector;
	std::atomic<int> _suspend_save;
	volatile bool    _save_queued;
	volatile bool    _save_queued_pending;
	bool             _no_save_signal;

	Glib::Threads::Mutex save_state_lock;
	Glib::Threads::Mutex save_source_lock;
	Glib::Threads::Mutex peak_cleanup_lock;

	int        load_options (const XMLNode&);
	int        load_state (std::string snapshot_name, bool from_template = false);
	static int parse_stateful_loading_version (const std::string&);

	samplepos_t _last_roll_location;
	/** the session sample time at which we last rolled, located, or changed transport direction */
	samplepos_t _last_roll_or_reversal_location;
	samplepos_t _last_record_location;

	bool              pending_abort;
	bool              pending_auto_loop;

#ifdef USE_TLSF
	PBD::TLSF _mempool;
#else
	PBD::ReallocPool _mempool;
#endif
	LuaState lua;
	mutable Glib::Threads::Mutex lua_lock;
	luabridge::LuaRef * _lua_run;
	luabridge::LuaRef * _lua_add;
	luabridge::LuaRef * _lua_del;
	luabridge::LuaRef * _lua_list;
	luabridge::LuaRef * _lua_load;
	luabridge::LuaRef * _lua_save;
	luabridge::LuaRef * _lua_cleanup;
	uint32_t            _n_lua_scripts;

	void setup_lua ();
	void luabindings_session_rt (lua_State*);
	void try_run_lua (pframes_t);

	SerializedRCUManager<IOPlugList> _io_plugins;

	std::vector<std::shared_ptr<MixerScene>> _mixer_scenes;
	mutable Glib::Threads::RWLock              _mixer_scenes_lock;

	Butler* _butler;

	TransportFSM* _transport_fsm;

	static const PostTransportWork ProcessCannotProceedMask = PostTransportWork (PostTransportAudition);

	std::atomic<PostTransportWork> _post_transport_work; /* accessed only atomic ops */
	PostTransportWork post_transport_work() const        { return _post_transport_work.load(); }
	void set_post_transport_work (PostTransportWork ptw) { _post_transport_work.store (ptw); }
	void add_post_transport_work (PostTransportWork ptw);

	void schedule_playback_buffering_adjustment ();
	void schedule_capture_buffering_adjustment ();

	Locations*       _locations;
	void location_added (Location*);
	void location_removed (Location*);
	void locations_changed ();
	void _locations_changed (const Locations::LocationList&);

	void update_skips (Location*, bool consolidate);
	void update_marks (Location* loc);
	void consolidate_skips (Location*);
	void sync_locations_to_skips ();
	void _sync_locations_to_skips ();

	PBD::ScopedConnectionList skip_update_connections;
	bool _ignore_skips_updates;

	PBD::ScopedConnectionList punch_connections;
	void             auto_punch_start_changed (Location *);
	void             auto_punch_end_changed (Location *);
	void             auto_punch_changed (Location *);

	PBD::ScopedConnectionList loop_connections;
	void             auto_loop_changed (Location *);

	void pre_engine_init (std::string path);
	int  post_engine_init ();
	int  immediately_post_engine ();
	void remove_empty_sounds ();

	void session_loaded ();

	void setup_midi_control ();
	int  midi_read (MIDI::Port *);

	void enable_record ();

	void increment_transport_position (samplecnt_t val) {
		if (max_samplepos - val < _transport_sample) {
			_transport_sample = max_samplepos;
		} else {
			_transport_sample += val;
		}
	}

	void decrement_transport_position (samplecnt_t val) {
		if (val < _transport_sample) {
			_transport_sample -= val;
		} else {
			_transport_sample = 0;
		}
	}

	void post_transport_motion ();
	static void *session_loader_thread (void *arg);

	void *do_work();

	/* Signal Forwarding */
	void emit_route_signals ();
	void emit_thread_run ();
	static void *emit_thread (void *);
	void emit_thread_start ();
	void emit_thread_terminate ();

	pthread_t       _rt_emit_thread;
	bool            _rt_thread_active;

	pthread_mutex_t _rt_emit_mutex;
	pthread_cond_t  _rt_emit_cond;
	bool            _rt_emit_pending;

	/* Auto Connect Thread */
	static void *auto_connect_thread (void *);
	void auto_connect_thread_run ();
	void auto_connect_thread_start ();
	void auto_connect_thread_terminate ();

	pthread_t        _auto_connect_thread;
	std::atomic<int> _ac_thread_active;
	pthread_mutex_t  _auto_connect_mutex;
	pthread_cond_t   _auto_connect_cond;

	struct AutoConnectRequest {
		public:
		AutoConnectRequest (std::shared_ptr <Route> r,
				bool ci, bool co,
				const ChanCount& is,
				const ChanCount& os,
				const ChanCount& io,
				const ChanCount& oo)
			: route (std::weak_ptr<Route> (r))
			, connect_inputs (ci)
			, connect_outputs (co)
			, input_start (is)
			, output_start (os)
			, input_offset (io)
			, output_offset (oo)
		{}

		std::weak_ptr <Route> route;
		bool connect_inputs;
		bool connect_outputs;
		ChanCount input_start;
		ChanCount output_start;
		ChanCount input_offset;
		ChanCount output_offset;
	};

	Glib::Threads::Mutex  _update_latency_lock;

	typedef std::queue<AutoConnectRequest> AutoConnectQueue;
	Glib::Threads::Mutex _auto_connect_queue_lock;
	AutoConnectQueue     _auto_connect_queue;
	std::atomic<unsigned int>   _latency_recompute_pending;

	void get_physical_ports (std::vector<std::string>& inputs, std::vector<std::string>& outputs, DataType type,
	                         MidiPortFlags include = MidiPortFlags (0),
	                         MidiPortFlags exclude = MidiPortFlags (0));

	void auto_connect (const AutoConnectRequest&);
	void queue_latency_recompute ();

	/* SessionEventManager interface */

	void process_event (SessionEvent*);
	void set_next_event ();
	void cleanup_event (SessionEvent*,int);

	/* MIDI Machine Control */

	void spp_start ();
	void spp_continue ();
	void spp_stop ();

	void mmc_deferred_play (MIDI::MachineControl &);
	void mmc_stop (MIDI::MachineControl &);
	void mmc_step (MIDI::MachineControl &, int);
	void mmc_pause (MIDI::MachineControl &);
	void mmc_record_pause (MIDI::MachineControl &);
	void mmc_record_strobe (MIDI::MachineControl &);
	void mmc_record_exit (MIDI::MachineControl &);
	void mmc_track_record_status (MIDI::MachineControl &, uint32_t track, bool enabled);
	void mmc_fast_forward (MIDI::MachineControl &);
	void mmc_rewind (MIDI::MachineControl &);
	void mmc_locate (MIDI::MachineControl &, const MIDI::byte *);
	void mmc_shuttle (MIDI::MachineControl &mmc, float speed, bool forw);
	void mmc_record_enable (MIDI::MachineControl &mmc, size_t track, bool enabled);

	int64_t _last_mmc_step;
	double  step_speed;

	typedef std::function<bool()> MidiTimeoutCallback;
	typedef std::list<MidiTimeoutCallback> MidiTimeoutList;

	MidiTimeoutList midi_timeouts;
	bool mmc_step_timeout ();
	void send_immediate_mmc (MIDI::MachineControlCommand);

	MIDI::byte mtc_msg[16];
	MIDI::byte mtc_timecode_bits;   /* encoding of SMTPE type for MTC */
	MIDI::byte midi_msg[16];
	double outbound_mtc_timecode_frame;
	Timecode::Time transmitting_timecode_time;
	int next_quarter_frame_to_send;

	double _samples_per_timecode_frame; /* has to be floating point because of drop sample */
	samplecnt_t _frames_per_hour;
	samplecnt_t _timecode_frames_per_hour;

	/* cache the most-recently requested time conversions. This helps when we
	 * have multiple clocks showing the same time (e.g. the transport sample) */
	bool last_timecode_valid;
	samplepos_t last_timecode_when;
	Timecode::Time last_timecode;

	bool _send_timecode_update; ///< Flag to send a full sample (Timecode) MTC message this cycle

	int send_midi_time_code_for_cycle (samplepos_t, samplepos_t, pframes_t nframes);

	LTCEncoder*       ltc_encoder;
	ltcsnd_sample_t*  ltc_enc_buf;

	Timecode::TimecodeFormat ltc_enc_tcformat;
	int32_t           ltc_buf_off;
	int32_t           ltc_buf_len;

	double            ltc_speed;
	int32_t           ltc_enc_byte;
	samplepos_t       ltc_enc_pos;
	double            ltc_enc_cnt;
	samplepos_t       ltc_enc_off;
	bool              restarting;
	samplepos_t       ltc_prev_cycle;

	samplepos_t       ltc_timecode_offset;
	bool              ltc_timecode_negative_offset;

	LatencyRange      ltc_out_latency;
	LatencyRange      mtc_out_latency;

	void ltc_tx_initialize();
	void ltc_tx_cleanup();
	void ltc_tx_reset();
	void ltc_tx_resync_latency (bool);
	void ltc_tx_recalculate_position();
	void ltc_tx_parse_offset();
	PBD::ScopedConnectionList ltc_tx_connections;

	void reset_record_status ();

	int no_roll (pframes_t nframes);
	int fail_roll (pframes_t nframes);

	bool non_realtime_work_pending() const { return static_cast<bool>(post_transport_work()); }
	bool process_can_proceed() const { return !(post_transport_work() & ProcessCannotProceedMask); }

	MidiControlUI* midi_control_ui;

	int           start_midi_thread ();

	bool should_ignore_transport_request (TransportRequestSource, TransportRequestType);

	void set_play_loop (bool yn, bool change_transport_state);
	void unset_play_loop (bool change_transport_state = false);
	void overwrite_some_buffers (std::shared_ptr<Route>, OverwriteReason);
	void flush_all_inserts ();
	int  micro_locate (samplecnt_t distance);

	enum PunchLoopLock {
		NoConstraint,
		OnlyPunch,
		OnlyLoop,
	};

	std::atomic<PunchLoopLock> _punch_or_loop;
	std::atomic<int> _current_usecs_per_track;

	bool punch_active () const;
	void unset_punch ();
	void reset_punch_loop_constraint ();
	bool maybe_allow_only_loop (bool play_loop = false);
	bool maybe_allow_only_punch ();

	void force_locate (samplepos_t sample, LocateTransportDisposition);
	void realtime_stop (bool abort, bool clear_state);
	void realtime_locate (bool);
	void non_realtime_set_speed ();
	void non_realtime_locate ();
	void non_realtime_stop (bool abort, int entry_request_count, bool& finished, bool will_locate);
	void non_realtime_overwrite (int entry_request_count, bool& finished, bool reset_loop_declicks);
	void engine_halted ();
	void engine_running ();
	void xrun_recovery ();
	void set_track_loop (bool);
	bool select_playhead_priority_target (samplepos_t&);
	void follow_playhead_priority ();
	void flush_cue_recording ();

	/* These are synchronous and so can only be called from within the process
	 * cycle
	 */

	void mtc_tx_resync_latency (bool);
	int  send_full_time_code (samplepos_t, pframes_t nframes);
	void send_song_position_pointer (samplepos_t);

	/* edit/mix groups */

	int load_route_groups (const XMLNode&, int);

	std::list<RouteGroup *> _route_groups;
	RouteGroup*             _all_route_group;

	/* routes stuff */

	SerializedRCUManager<RouteList>  routes;

	void add_routes (RouteList&, bool input_auto_connect, bool output_auto_connect, PresentationInfo::order_t);
	void add_routes_inner (RouteList&, bool input_auto_connect, bool output_auto_connect, PresentationInfo::order_t);
	bool _adding_routes_in_progress;
	bool _reconnecting_routes_in_progress;
	bool _route_deletion_in_progress;
	bool _route_reorder_in_progress;

	void load_and_connect_instruments (RouteList&,
			bool strict_io,
			std::shared_ptr<PluginInfo> instrument,
			Plugin::PresetRecord* pset,
			ChanCount& existing_outputs
			);

	std::shared_ptr<Route> XMLRouteFactory (const XMLNode&, int);
	std::shared_ptr<Route> XMLRouteFactory_2X (const XMLNode&, int);
	std::shared_ptr<Route> XMLRouteFactory_3X (const XMLNode&, int);

	void route_processors_changed (RouteProcessorChange);

	bool find_route_name (std::string const &, uint32_t& id, std::string& name, bool);
	void count_existing_track_channels (ChanCount& in, ChanCount& out);
	void auto_connect_route (std::shared_ptr<Route>, bool, bool, const ChanCount&, const ChanCount&, const ChanCount& io = ChanCount(), const ChanCount& oo = ChanCount());
	void midi_output_change_handler (IOChange change, void* /*src*/, std::weak_ptr<Route> midi_track);

	/* track numbering */

	void reassign_track_numbers ();
	uint32_t _track_number_decimals;

	/* solo/mute/notifications */

	void route_listen_changed (PBD::Controllable::GroupControlDisposition, std::weak_ptr<Route>);
	void route_mute_changed ();
	void route_solo_changed (bool self_solo_change, PBD::Controllable::GroupControlDisposition group_override, std::weak_ptr<Route>);
	void route_solo_isolated_changed (std::weak_ptr<Route>);

	void update_route_solo_state (std::shared_ptr<RouteList const> r = std::shared_ptr<RouteList const>());

	void listen_position_changed ();
	void solo_control_mode_changed ();

	/* REGION MANAGEMENT */

	mutable Glib::Threads::Mutex region_lock;

	int load_regions (const XMLNode& node);
	int load_compounds (const XMLNode& node);

	void route_added_to_route_group (RouteGroup *, std::weak_ptr<Route>);
	void route_removed_from_route_group (RouteGroup *, std::weak_ptr<Route>);
	void route_group_property_changed (RouteGroup *);

	/* SOURCES */

	mutable Glib::Threads::Mutex source_lock;

public:

	/* Emited when a new source is added to the session */
	PBD::Signal< void(std::weak_ptr<Source> )> SourceAdded;
	PBD::Signal< void(std::weak_ptr<Source> )> SourceRemoved;

	typedef std::map<PBD::ID,std::shared_ptr<Source> > SourceMap;

	void foreach_source (std::function<void( std::shared_ptr<Source> )> f) {
		Glib::Threads::Mutex::Lock ls (source_lock);
		for (SourceMap::iterator i = sources.begin(); i != sources.end(); ++i) {
			f ( (*i).second );
		}
	}

	bool playlist_is_active( std::shared_ptr<Playlist>);
	ARDOUR::CueMarkers pending_source_markers; // source markers created while recording

private:
	void reset_write_sources (bool mark_write_complete);
	SourceMap sources;

	int load_sources (const XMLNode& node);
	XMLNode& get_sources_as_xml ();

	std::shared_ptr<Source> XMLSourceFactory (const XMLNode&);

	/* PLAYLISTS */

	void remove_playlist (std::weak_ptr<Playlist>);
	void track_playlist_changed (std::weak_ptr<Track>);
	void playlist_region_added (std::weak_ptr<Region>);
	void playlist_ranges_moved (std::list<Temporal::RangeMove> const &);
	void playlist_regions_extended (std::list<Temporal::Range> const &);

	/* CURVES and AUTOMATION LISTS */
	std::map<PBD::ID, AutomationList*> automation_lists;

	/** load 2.X Sessions. Diskstream-ID to playlist-name mapping */
	std::map<PBD::ID, std::string> _diskstreams_2X;

	/* DEFAULT FADE CURVES */

	float default_fade_steepness;
	float default_fade_msecs;

	/* AUDITIONING */

	std::shared_ptr<Auditioner> auditioner;
	void set_audition (std::shared_ptr<Region>);
	void non_realtime_set_audition ();
	std::shared_ptr<Region> pending_audition_region;

	/* EXPORT */

	/* FLATTEN */

	int flatten_one_track (AudioTrack&, samplepos_t start, samplecnt_t cnt);

	/* INSERT AND SEND MANAGEMENT */

	boost::dynamic_bitset<uint32_t> send_bitset;
	boost::dynamic_bitset<uint32_t> surround_send_bitset;
	boost::dynamic_bitset<uint32_t> aux_send_bitset;
	boost::dynamic_bitset<uint32_t> return_bitset;
	boost::dynamic_bitset<uint32_t> insert_bitset;

	/* S/W RAID */

	struct space_and_path {
		uint32_t blocks;     ///< 4kB blocks
		bool blocks_unknown; ///< true if blocks is unknown
		std::string path;

		space_and_path ()
			: blocks (0)
			, blocks_unknown (true)
		{}
	};

	struct space_and_path_ascending_cmp {
		bool operator() (space_and_path a, space_and_path b) {
			if (a.blocks_unknown != b.blocks_unknown) {
				return !a.blocks_unknown;
			}
			return a.blocks > b.blocks;
		}
	};

	void setup_raid_path (std::string path);

	std::vector<space_and_path> session_dirs;
	std::vector<space_and_path>::iterator last_rr_session_dir;
	uint64_t _total_free_4k_blocks;
	/** If this is true, _total_free_4k_blocks is not definite,
	    as one or more of the session directories' filesystems
	    could not report free space.
	*/
	bool _total_free_4k_blocks_uncertain;
	Glib::Threads::Mutex space_lock;

	bool no_questions_about_missing_files;

	std::string get_best_session_directory_for_new_audio ();

	mutable std::atomic<int> _playback_load;
	mutable std::atomic<int> _capture_load;

	/* I/O bundles */

	SerializedRCUManager<BundleList> _bundles;
	XMLNode* _bundle_xml_node;
	int load_bundles (XMLNode const &);

	int  backend_sync_callback (TransportState, samplepos_t);

	void process_rtop (SessionEvent*);

	enum snapshot_t {
		NormalSave,
		SnapshotKeep,
		SwitchToSnapshot
	};

	XMLNode& state (bool save_template,
	                snapshot_t snapshot_type = NormalSave,
	                bool for_archive = false,
	                bool only_used_assets = false) const;

	XMLNode& get_state () const;
	int      set_state (const XMLNode& node, int version); // not idempotent
	XMLNode& get_template ();

	bool maybe_copy_midifile (snapshot_t, std::shared_ptr<Source> src, XMLNode*);

	void collect_sources_of_this_snapshot (std::set<std::shared_ptr<Source>>&, bool incl_unused = true) const;

	/* click track */
	typedef std::list<Click*>     Clicks;
	Clicks                        clicks;
	bool                         _clicking;
	bool                         _click_rec_only;
	std::shared_ptr<IO>          _click_io;
	std::shared_ptr<Amp>         _click_gain;
	Sample*                       click_data;
	Sample*                       click_emphasis_data;
	samplecnt_t                   click_length;
	samplecnt_t                   click_emphasis_length;
	mutable Glib::Threads::RWLock click_lock;
	samplecnt_t                  _click_io_latency;
	PBD::ScopedConnection        _click_io_connection;
	Temporal::GridIterator       _click_iterator;

	static const Sample     default_click[];
	static const samplecnt_t default_click_length;
	static const Sample     default_click_emphasis[];
	static const samplecnt_t default_click_emphasis_length;

	Click* get_click();
	samplepos_t _clicks_cleared;
	samplecnt_t _count_in_samples;
	void setup_click_sounds (int which);
	void setup_click_sounds (Sample**, Sample const *, samplecnt_t*, samplecnt_t, std::string const &);
	void clear_clicks ();
	void click (samplepos_t start, samplecnt_t nframes);
	void run_click (samplepos_t start, samplecnt_t nframes);
	void add_click (samplepos_t pos, bool emphasis);
	void click_io_resync_latency (bool);

	/* range playback */

	std::list<TimelineRange> current_audio_range;
	bool _play_range;
	void set_play_range (std::list<TimelineRange>&, bool leave_rolling);
	void unset_play_range ();

	/* temporary hacks to allow selection to be pushed from GUI into backend
	   Whenever we move the selection object into libardour, these will go away.
	*/
	Temporal::Range _range_selection;
	Temporal::Range _object_selection;

	void unset_preroll_record_trim ();

	samplecnt_t _preroll_record_trim_len;
	bool _count_in_once;

	/* main outs */
	uint32_t main_outs;

	std::shared_ptr<Route> _master_out;
	std::shared_ptr<Route> _monitor_out;
	std::shared_ptr<Route> _surround_master;

	friend class PortManager;
	void auto_connect_master_bus ();
	void auto_connect_monitor_bus ();
	void auto_connect_surround_master ();
	void auto_connect_io (std::shared_ptr<IO>);

	void setup_route_monitor_sends (bool enable, bool need_process_lock);
	void setup_route_surround_sends (bool enable, bool need_process_lock);

	int find_all_sources (std::string path, std::set<std::string>& result);
	int find_all_sources_across_snapshots (std::set<std::string>& result, bool exclude_this_snapshot);

	typedef std::set<std::shared_ptr<PBD::Controllable> > Controllables;
	Glib::Threads::Mutex controllables_lock;
	Controllables controllables;

	std::shared_ptr<PBD::Controllable> _solo_cut_control;

	void controllable_touched (std::weak_ptr<PBD::Controllable>);
	std::weak_ptr<PBD::Controllable> _recently_touched_controllable;

	void reset_native_file_format();
	bool first_file_data_format_reset;
	bool first_file_header_format_reset;

	void config_changed (std::string, bool);

	XMLNode& get_control_protocol_state () const;

	void set_history_depth (uint32_t depth);

	static bool _disable_all_loaded_plugins;
	static bool _bypass_all_loaded_plugins;

	mutable bool have_looped; ///< Used in \ref audible_sample

	bool roll_started_loop;

	void update_route_record_state ();
	std::atomic<int> _have_rec_enabled_track;
	std::atomic<int> _have_rec_disabled_track;

	static int ask_about_playlist_deletion (std::shared_ptr<Playlist>);

	/* realtime "apply to set of routes" operations */
	template<typename T> SessionEvent*
		get_rt_event (std::shared_ptr<RouteList const> rl, T targ, SessionEvent::RTeventCallback after, PBD::Controllable::GroupControlDisposition group_override,
		              void (Session::*method) (std::shared_ptr<RouteList const>, T, PBD::Controllable::GroupControlDisposition)) {
		SessionEvent* ev = new SessionEvent (SessionEvent::RealTimeOperation, SessionEvent::Add, SessionEvent::Immediate, 0, 0.0);
		ev->rt_slot = std::bind (method, this, rl, targ, group_override);
		ev->rt_return = after;
		ev->event_loop = PBD::EventLoop::get_event_loop_for_thread ();

		return ev;
	}

	/* specialized version realtime "apply to set of routes" operations */
	template<typename T1, typename T2> SessionEvent*
		get_rt_event (std::shared_ptr<RouteList const> rl, T1 t1arg, T2 t2arg, SessionEvent::RTeventCallback after, PBD::Controllable::GroupControlDisposition group_override,
		              void (Session::*method) (std::shared_ptr<RouteList const>, T1, T2, PBD::Controllable::GroupControlDisposition)) {
		SessionEvent* ev = new SessionEvent (SessionEvent::RealTimeOperation, SessionEvent::Add, SessionEvent::Immediate, 0, 0.0);
		ev->rt_slot = std::bind (method, this, rl, t1arg, t2arg, group_override);
		ev->rt_return = after;
		ev->event_loop = PBD::EventLoop::get_event_loop_for_thread ();

		return ev;
	}

	/* specialized version realtime "apply to set of controls" operations */
	SessionEvent* get_rt_event (std::shared_ptr<WeakAutomationControlList> cl, double arg, PBD::Controllable::GroupControlDisposition group_override) {
		SessionEvent* ev = new SessionEvent (SessionEvent::RealTimeOperation, SessionEvent::Add, SessionEvent::Immediate, 0, 0.0);
		ev->rt_slot = std::bind (&Session::rt_set_controls, this, cl, arg, group_override);
		ev->rt_return = Session::rt_cleanup;
		ev->event_loop = PBD::EventLoop::get_event_loop_for_thread ();

		return ev;
	}

	void rt_set_controls (std::shared_ptr<WeakAutomationControlList>, double val, PBD::Controllable::GroupControlDisposition group_override);
	void rt_clear_all_solo_state (std::shared_ptr<RouteList const>, bool yn, PBD::Controllable::GroupControlDisposition group_override);

	void setup_midi_machine_control ();

	void step_edit_status_change (bool);
	uint32_t _step_editors;

	/** true if timecode transmission by the transport is suspended, otherwise false */
	mutable std::atomic<int> _suspend_timecode_transmission;

	void start_time_changed (samplepos_t);
	void end_time_changed (samplepos_t);

	void set_track_monitor_input_status (bool);
	samplepos_t compute_stop_limit () const;

	std::shared_ptr<Speakers> _speakers;
	void load_nested_sources (const XMLNode& node);

	/** The directed graph of routes that is currently being used for audio processing
	 * and solo/mute computations.
	 */
	GraphEdges _current_route_graph;

	friend class IOPlug;
	std::shared_ptr<Graph>      _process_graph;
	std::shared_ptr<GraphChain> _graph_chain;
	std::shared_ptr<GraphChain> _io_graph_chain[2];

	void resort_routes_using (std::shared_ptr<RouteList>);
	void resort_io_plugs ();

	bool rechain_process_graph (GraphNodeList&);
	bool rechain_ioplug_graph (bool);

	void ensure_route_presentation_info_gap (PresentationInfo::order_t, uint32_t gap_size);

	friend class ProcessorChangeBlocker;
	std::atomic<int> _ignore_route_processor_changes;
	std::atomic<int> _ignored_a_processor_change;

	MidiClockTicker* midi_clock;

	std::shared_ptr<Port>  _ltc_output_port;

	std::shared_ptr<RTTaskList> _rt_tasklist;
	std::shared_ptr<IOTaskList> _io_tasklist;

	/* Scene Changing */
	SceneChanger* _scene_changer;

	/* persistent, non-track related MIDI ports */
	MidiPortManager* _midi_ports;
	MIDI::MachineControl* _mmc;

	void setup_ltc ();
	void setup_click ();
	void setup_click_state (const XMLNode*);
	void setup_bundles ();

	void port_registry_changed ();
	void probe_ctrl_surfaces ();

	void save_as_bring_callback (uint32_t, uint32_t, std::string);

	static const uint32_t session_end_shift;

	std::string _template_state_dir;

	VCAManager* _vca_manager;

	std::shared_ptr<Route> get_midi_nth_route_by_id (PresentationInfo::order_t n) const;

	std::string created_with;
	mutable std::string modified_with;

	void midi_track_presentation_info_changed (PBD::PropertyChange const &, std::weak_ptr<MidiTrack>);
	void rewire_selected_midi (std::shared_ptr<MidiTrack>);
	void rewire_midi_selection_ports ();
	void disconnect_port_for_rewire (std::string const& port) const;
	std::weak_ptr<MidiTrack> current_midi_target;

	StripableList _soloSelection;  //the items that are soloe'd during a solo-selection operation; need to unsolo after the roll

	CoreSelection* _selection;

	bool _global_locate_pending;
	std::optional<samplepos_t> _nominal_jack_transport_sample;

	bool _had_destructive_tracks;

	std::string unnamed_file_name () const;

	std::atomic<int> _update_pretty_names;

	void setup_thread_local_variables ();
	void cue_marker_change (Location*);

	struct CueEventTimeComparator {
		bool operator() (CueEvent const & c, samplepos_t s) {
			return c.time < s;
		}
	};

	CueEvents _cue_events;
	void sync_cues ();
	void sync_cues_from_list (Locations::LocationList const &);

	std::atomic<int32_t> _pending_cue;
	std::atomic<int32_t> _active_cue;
	void maybe_find_pending_cue ();
	void clear_active_cue ();

	int tb_with_filled_slots;
	void handle_slots_empty_status (std::weak_ptr<Route> const &);

	void time_domain_changed ();

	AnyTime _global_quantization;
};


} // namespace ARDOUR

