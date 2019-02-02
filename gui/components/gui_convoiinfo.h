/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_CONVOIINFO_H
#define GUI_COMPONENTS_GUI_CONVOIINFO_H


#include "gui_aligned_container.h"
#include "gui_label.h"
#include "gui_scrolled_list.h"
#include "gui_speedbar.h"
#include "../../convoihandle_t.h"
#include "gui_convoy_formation.h"
#include "gui_convoy_payloadinfo.h"

/**
 * Convoi info stats, like loading status bar
 * One element of the vehicle list display
 *
 * @author Hj. Malthaner
 */
class gui_convoiinfo_t : public gui_aligned_container_t, public gui_scrolled_list_t::scrollitem_t
{
private:
	/**
	* Handle Convois to be displayed.
	* @author Hj. Malthaner
	*/
	convoihandle_t cnv;

	gui_speedbar_t filled_bar;
	gui_label_buf_t label_name, label_profit, label_line;

	gui_convoy_formation_t formation;
	gui_convoy_payloadinfo_t payload;

	bool show_line_name = true;

	uint8 display_mode = cnvlist_normal;

public:
	/**
	* @param cnv, the handler for the Convoi to be displayed.
	* @author Hj. Malthaner
	*/
	gui_convoiinfo_t(convoihandle_t cnv, bool show_line_name = true);

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	* Draw the component
	* @author Hj. Malthaner
	*/
	void draw(scr_coord offset) OVERRIDE;

	void set_mode(uint8 mode);
	enum cl_display_mode_t { cnvlist_normal = 0, cnvlist_payload, cnvlist_formation, DISPLAY_MODES };
	static const char *cnvlist_mode_button_texts[DISPLAY_MODES];

	void update_label();

	const char* get_text() const OVERRIDE;

	virtual bool is_valid() const OVERRIDE { return cnv.is_bound(); }

	convoihandle_t get_cnv() const { return cnv; }

	using gui_aligned_container_t::get_min_size;
	using gui_aligned_container_t::get_max_size;
};


#endif
