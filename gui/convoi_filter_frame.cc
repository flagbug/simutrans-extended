/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "convoi_filter_frame.h"
#include "convoi_frame.h"
#include "components/gui_convoiinfo.h"
#include "../simcolor.h"

#include "../descriptor/goods_desc.h"
#include "../bauer/goods_manager.h"
#include "../dataobj/translator.h"

#include "components/gui_image.h"
#include "../simskin.h"

const char *convoi_filter_frame_t::filter_buttons_text[FILTER_BUTTONS] = {
	"clf_chk_name_filter",
	"clf_chk_waren",
	"clf_chk_type_filter",
	"clf_chk_cars",
	"clf_chk_trains",
	"clf_chk_ships",
	"clf_chk_aircrafts",
	"clf_chk_monorail",
	"clf_chk_maglev",
	"clf_chk_narrowgauge",
	"clf_chk_trams",
	"clf_chk_spezial_filter",
	"clf_chk_noroute",
	"clf_chk_stucked",
	"clf_chk_noincome",
	"clf_chk_indepot",
	"clf_chk_noschedule",
	"clf_chk_noline",
	"clf_chk_obsolete"
};

convoi_filter_frame_t::filter_flag_t convoi_filter_frame_t::filter_buttons_types[FILTER_BUTTONS] = {
	name_filter,
	ware_filter,
	typ_filter,
	lkws_filter,
	zuege_filter,
	schiffe_filter,
	aircraft_filter,
	monorail_filter,
	maglev_filter,
	narrowgauge_filter,
	tram_filter,
	special_filter,
	noroute_filter,
	stucked_filter,
	noincome_filter,
	indepot_filter,
	nofpl_filter,
	noline_filter,
	obsolete_filter
};

slist_tpl<const goods_desc_t *>convoi_filter_frame_t::active_ware;
char convoi_filter_frame_t::name_filter_text[] = "";
uint32 convoi_filter_frame_t::filter_flags = 0;

convoi_filter_frame_t::convoi_filter_frame_t(player_t *player, convoi_frame_t *m) :
	gui_frame_t( translator::translate("clf_title"), player),
	main_frame(m),
	ware_scrolly(&ware_cont)
{
	for(  int i=0; i < FILTER_BUTTONS; i++  ) {
		filter_buttons[i].init(button_t::square_state, filter_buttons_text[i]);
		filter_buttons[i].add_listener(this);
		if(filter_buttons_types[i] < sub_filter) {
			filter_buttons[i].background_color = color_idx_to_rgb(COL_WHITE);
		}
		filter_buttons[i].pressed = get_filter(filter_buttons_types[i]);
	}

	set_table_layout(2,0);
	set_alignment(ALIGN_TOP);

	// first column
	add_table(1,0);
	{
		// name filter
		add_component(filter_buttons + 0);

		name_filter_input.set_text( name_filter_text, lengthof(name_filter_text) );
		name_filter_input.add_listener(this);
		add_component(&name_filter_input);

		add_table(2, 0);
		{
			// type and special buttons
			for(  int i=2;  i<FILTER_BUTTONS;  i++  ) {
				if (i==2 || i==11) {
					if (i==11) {
						new_component<gui_margin_t>();
						new_component<gui_empty_t>();
					}
					add_component(filter_buttons + i, 2);
				}
				else {
					new_component<gui_margin_t>(D_CHECKBOX_WIDTH);
					add_component(filter_buttons + i);
				}
			}
		}
		end_table();
	}
	end_table();

	// second columns: accepted cargo types
	add_table(1,0);
	{
		add_component(filter_buttons + 1);
		add_table(3,0);
		{
			ware_alle.init(button_t::roundbox, "clf_btn_alle");
			ware_alle.set_size(scr_size(D_BUTTON_WIDTH * 3/5, D_BUTTON_HEIGHT));
			ware_alle.add_listener(this);
			add_component(&ware_alle);
			ware_keine.init(button_t::roundbox, "clf_btn_keine");
			ware_keine.set_size(scr_size(D_BUTTON_WIDTH * 3/5, D_BUTTON_HEIGHT));
			ware_keine.add_listener(this);
			add_component(&ware_keine);
			ware_invers.init(button_t::roundbox, "clf_btn_invers");
			ware_invers.set_size(scr_size(D_BUTTON_WIDTH * 3/5, D_BUTTON_HEIGHT));
			ware_invers.add_listener(this);
			add_component(&ware_invers);
		}
		end_table();

		ware_scrolly.set_scroll_amount_y(D_BUTTON_HEIGHT);
		add_component(&ware_scrolly);

		ware_cont.set_table_layout(3,0);
		all_ware.clear();
		for(  int i=0;  i < goods_manager_t::get_count();  i++  ) {
			const goods_desc_t *ware = goods_manager_t::get_info(i);
			if(  ware == goods_manager_t::none  ) {
				continue;
			}
			if(  ware->get_catg() == 0  ) {
				// Special freight: Each good is special
				ware_cont.new_component<gui_image_t>()->set_image(ware->get_catg_symbol(), true);
				ware_item_t *item = ware_cont.new_component<ware_item_t>(this, ware);
				item->init(button_t::square_state, translator::translate(ware->get_name()));
				item->pressed = active_ware.is_contained(ware);
				all_ware.append(item);
				ware_cont.new_component<gui_fill_t>();
			}
		}
		// now add other good categories
		for(  int i=1;  i < goods_manager_t::get_max_catg_index();  i++  ) {
			const goods_desc_t *ware = goods_manager_t::get_info_catg(i);
			if(  ware->get_catg() != 0  ) {
				ware_cont.new_component<gui_image_t>()->set_image(ware->get_catg_symbol(), true);
				ware_item_t *item = ware_cont.new_component<ware_item_t>(this, ware);
				item->init(button_t::square_state, translator::translate(ware->get_catg_name()));
				item->pressed = active_ware.is_contained(ware);
				all_ware.append(item);
				ware_cont.new_component<gui_fill_t>();
			}
		}
	}
	end_table();

	set_resizemode(diagonal_resize);
	reset_min_windowsize();
}


void convoi_filter_frame_t::init(uint32 filter_flags, const slist_tpl<const goods_desc_t*>* wares)
{
	for (int i = 0; i < FILTER_BUTTONS; i++) {
		set_filter(filter_buttons_types[i], filter_flags & filter_buttons_types[i]);
		filter_buttons[i].pressed = get_filter(filter_buttons_types[i]);
	}
	if (&active_ware != wares) {
		active_ware.clear();
		if (wares) {
			FOR(slist_tpl<ware_item_t*>, wi, all_ware) {
				wi->pressed = wares->is_contained(wi->ware);
				if (wi->pressed) {
					active_ware.append(wi->ware);
				}
			}
		}
	}
}


/**
 * This method is called if an action is triggered
 */
bool convoi_filter_frame_t::action_triggered( gui_action_creator_t *comp,value_t /* */)
{
	for( int i = 0; i < FILTER_BUTTONS; i++) {
		if(comp == filter_buttons + i) {
			filter_buttons[i].pressed ^= 1;
			set_filter( filter_buttons_types[i], !get_filter(filter_buttons_types[i]) );
			sort_list();
			return true;
		}
	}
	if(comp == &ware_alle) {
		FOR( slist_tpl<ware_item_t *>, wi, all_ware ) {
			wi->pressed = true;
		}
		sort_list();
		return true;
	}
	if(comp == &ware_keine) {
		FOR( slist_tpl<ware_item_t *>, wi, all_ware ) {
			wi->pressed = false;
		}
		sort_list();
		return true;
	}
	if(  comp == &ware_invers  ) {
		FOR( slist_tpl<ware_item_t *>, wi, all_ware ) {
			wi->pressed ^= true;
		}
		sort_list();
		return true;
	}
	if(  comp == &name_filter_input  &&  filter_flags&name_filter  ) {
		sort_list();
		return true;
	}
	return false;
}


void convoi_filter_frame_t::sort_list()
{
	active_ware.clear();
	FOR( slist_tpl<ware_item_t *>, wi, all_ware ) {
		if(  wi->pressed  ) {
/*
			uint8 catg = wi->ware->get_catg();
			if(  catg  ) {
				// now all goods of this category
				for(  int i=1;  i<goods_manager_t::get_max_catg_index();  i++   ) {
					if(  goods_manager_t::get_info(i)->get_catg()==catg  ) {
						active_ware.append( goods_manager_t::get_info(i) );
					}
				}
			}
			else {
				active_ware.append( wi->ware );
			}
*/
			active_ware.append( wi->ware );
		}
	}
	main_frame->sort_list( filter_flags&name_filter ? name_filter_text : NULL, filter_flags, &active_ware );
}
