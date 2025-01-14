/*
 * Copyright (C) 2006-2012 David Robillard <d@drobilla.net>
 * Copyright (C) 2006-2017 Paul Davis <paul@linuxaudiosystems.com>
 * Copyright (C) 2009-2010 Carl Hetherington <carl@carlh.net>
 * Copyright (C) 2013-2017 John Emmas <john@creativepost.co.uk>
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

#ifndef ardour_control_protocols_h
#define ardour_control_protocols_h

#include <list>
#include <memory>
#include <string>
#include <vector>

#include "pbd/signals.h"
#include "pbd/stateful.h"
#include "pbd/glib_event_source.h"

#include "control_protocol/basic_ui.h"
#include "control_protocol/types.h"
#include "control_protocol/visibility.h"

namespace ARDOUR {

class Route;
class Session;
class Bundle;
class Stripable;
class PluginInsert;

class LIBCONTROLCP_API ControlProtocol : public PBD::Stateful, public PBD::ScopedConnectionList, public BasicUI
{
public:
	ControlProtocol (Session&, std::string name);
	virtual ~ControlProtocol ();

	virtual std::string name () const { return _name; }

	virtual int set_active (bool yn);
	virtual bool active () const { return _active; }

	virtual int set_feedback (bool /*yn*/) { return 0; }
	virtual bool get_feedback () const { return false; }

	virtual void midi_connectivity_established (bool) {}

	virtual void stripable_selection_changed () = 0;

	PBD::Signal<void()> ActiveChanged;

	/* signals that a control protocol can emit and other (presumably graphical)
	 * user interfaces can respond to
	 */

	static PBD::Signal<void()> ZoomToSession;
	static PBD::Signal<void()> ZoomIn;
	static PBD::Signal<void()> ZoomOut;
	static PBD::Signal<void()> Enter;
	static PBD::Signal<void()> Undo;
	static PBD::Signal<void()> Redo;
	static PBD::Signal<void(float)> ScrollTimeline;
	static PBD::Signal<void(uint32_t)> GotoView;
	static PBD::Signal<void()> CloseDialog;
	static PBD::Signal<void()> VerticalZoomInAll;
	static PBD::Signal<void()> VerticalZoomOutAll;
	static PBD::Signal<void()> VerticalZoomInSelected;
	static PBD::Signal<void()> VerticalZoomOutSelected;
	static PBD::Signal<void()> StepTracksDown;
	static PBD::Signal<void()> StepTracksUp;
	static PBD::Signal<void(std::weak_ptr<ARDOUR::PluginInsert> )> PluginSelected;

	void add_stripable_to_selection (std::shared_ptr<ARDOUR::Stripable>);
	void set_stripable_selection (std::shared_ptr<ARDOUR::Stripable>);
	void toggle_stripable_selection (std::shared_ptr<ARDOUR::Stripable>);
	void remove_stripable_from_selection (std::shared_ptr<ARDOUR::Stripable>);
	void clear_stripable_selection ();

	virtual void add_rid_to_selection (int rid);
	virtual void set_rid_selection (int rid);
	virtual void toggle_rid_selection (int rid);
	virtual void remove_rid_from_selection (int rid);

	std::shared_ptr<ARDOUR::Stripable> first_selected_stripable () const;

	/* the model here is as follows:

	   we imagine most control surfaces being able to control
	   from 1 to N tracks at a time, with a session that may
	   contain 1 to M tracks, where M may be smaller, larger or
	   equal to N.

	   the control surface has a fixed set of physical controllers
	   which can potentially be mapped onto different tracks/busses
	   via some mechanism.

	   therefore, the control protocol object maintains
	   a table that reflects the current mapping between
	   the controls and route object.
	*/

	void set_route_table_size (uint32_t size);
	void set_route_table (uint32_t table_index, std::shared_ptr<ARDOUR::Route>);
	bool set_route_table (uint32_t table_index, uint32_t remote_control_id);

	void route_set_rec_enable (uint32_t table_index, bool yn);
	bool route_get_rec_enable (uint32_t table_index);

	float route_get_gain (uint32_t table_index);
	void  route_set_gain (uint32_t table_index, float);
	float route_get_effective_gain (uint32_t table_index);

	float route_get_peak_input_power (uint32_t table_index, uint32_t which_input);

	bool route_get_muted (uint32_t table_index);
	void route_set_muted (uint32_t table_index, bool);

	bool route_get_soloed (uint32_t table_index);
	void route_set_soloed (uint32_t table_index, bool);

	std::string route_get_name (uint32_t table_index);

	virtual std::list<std::shared_ptr<ARDOUR::Bundle> > bundles ();

	virtual bool has_editor () const { return false; }
	virtual void* get_gui () const { return 0; }
	virtual void tear_down_gui () {}

	XMLNode& get_state () const;
	int set_state (XMLNode const&, int version);

	static const std::string state_node_name;

	static StripableNotificationList const& last_selected () { return _last_selected; }
	static void notify_stripable_selection_changed (StripableNotificationListPtr);

protected:
	void next_track (uint32_t initial_id);
	void prev_track (uint32_t initial_id);

	std::vector<std::shared_ptr<ARDOUR::Route> > route_table;
	std::string _name;
	GlibEventLoopCallback glib_event_callback;
	virtual void event_loop_precall ();
	void install_precall_handler (Glib::RefPtr<Glib::MainContext>);

private:
	LIBCONTROLCP_LOCAL ControlProtocol (const ControlProtocol&); /* noncopyable */

	bool _active;

	static StripableNotificationList          _last_selected;
	static PBD::ScopedConnection              selection_connection;
	static bool                               selection_connected;
};

extern "C" {
class ControlProtocolDescriptor
{
public:
	const char* name;                       /* descriptive */
	const char* id;                         /* unique and version-specific */
	void*       module;                     /* not for public access */
	bool (*available) ();                   /* called directly after loading module */
	bool (*probe_port) ();                  /* called when ports change (PortRegisteredOrUnregistered) */
	bool (*match_usb) (uint16_t, uint16_t); /* called when USB devices are hotplugged (libusb) */
	ControlProtocol* (*initialize) (Session*);
	void (*destroy) (ControlProtocol*);
};
}
}

/* this is where the strange inheritance pattern hits the wall. A control
   protocol thread/event loop is inherited from AbstractUI, but the precall
   handler is inherited from ControlProtocol. When the AbstractUI sets up the
   event loop, it will call attach_request_source() which will in turn pass a
   Glib::MainContext to maybe_install_precall_handler(). We override the
   definition of that method here to make it actuall install the
   ControlProtocol's handler.
*/

#define CONTROL_PROTOCOL_THREADS_NEED_TEMPO_MAP_DECL() \
	void maybe_install_precall_handler (Glib::RefPtr<Glib::MainContext> ctxt) { install_precall_handler (ctxt); }


#endif // ardour_control_protocols_h
