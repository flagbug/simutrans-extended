/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string.h>

#include "../simworld.h"
#include "simobj.h"
#include "../simmem.h"
#include "../display/simimg.h"

#include "../bauer/brueckenbauer.h"

#include "../descriptor/bridge_desc.h"

#include "../boden/grund.h"

#include "../dataobj/loadsave.h"
#include "pillar.h"
#include "bruecke.h"
#include "../dataobj/environment.h"


pillar_t::pillar_t(loadsave_t *file) :
#ifdef INLINE_OBJ_TYPE
	obj_t(obj_t::pillar)
#else
	obj_t()
#endif
{
	desc = NULL;
	asymmetric = false;
	rdwr(file);
}


pillar_t::pillar_t( koord3d pos, player_t *player, const bridge_desc_t *desc, bridge_desc_t::img_t img, int height ) :
#ifdef INLINE_OBJ_TYPE
	obj_t(obj_t::pillar, pos)
#else
	obj_t(pos)
#endif
{
	this->desc = desc;
	this->dir = (uint8)img;
	set_yoff( -height );
	set_owner( player );
	asymmetric = desc->has_pillar_asymmetric();
	calc_image();
}


void pillar_t::calc_image()
{
	bool hide = false;
	int height = get_yoff();
	if(  desc->has_pillar_asymmetric()  ) {
		if(  grund_t *gr = welt->lookup(get_pos())  ) {
			slope_t::type slope = gr->get_grund_hang();
			if(  dir == bridge_desc_t::NS_Pillar  ) {
				height += min( corner_sw(slope), corner_se(slope) ) * TILE_HEIGHT_STEP;
			}
			else {
				height += min( corner_se(slope), corner_ne(slope) ) * TILE_HEIGHT_STEP;
			}
			if(  height > 0  ) {
				hide = true;
			}
		}

	}
	image = hide ? IMG_EMPTY : desc->get_background( (bridge_desc_t::img_t)dir, get_pos().z-height/TILE_HEIGHT_STEP >= welt->get_snowline()  ||  welt->get_climate( get_pos().get_2d() ) == arctic_climate );
}


/**
 * @return Einen Beschreibungsstring fuer das Objekt, der z.B. in einem
 * Beobachtungsfenster angezeigt wird.
 */
void pillar_t::show_info()
{
	planquadrat_t *plan=welt->access(get_pos().get_2d());
	for(unsigned i=0;  i<plan->get_boden_count();  i++  ) {
		grund_t *bd=plan->get_boden_bei(i);
		if(bd->ist_bruecke()) {
			bruecke_t* br = bd->find<bruecke_t>();
			if(br  &&  br->get_desc()==desc) {
				br->show_info();
			}
		}
	}
}


void pillar_t::rdwr(loadsave_t *file)
{
	xml_tag_t p( file, "pillar_t" );

	obj_t::rdwr(file);

	if(file->is_saving()) {
		const char *s = desc->get_name();
		file->rdwr_str(s);
		file->rdwr_byte(dir);
	}
	else {
		char s[256];
		file->rdwr_str(s, lengthof(s));
		file->rdwr_byte(dir);

		desc = bridge_builder_t::get_desc(s);
		if(desc==0) {
			if(strstr(s,"ail")) {
				desc = bridge_builder_t::get_desc("ClassicRail");
				dbg->warning("pillar_t::rdwr()","Unknown bridge %s replaced by ClassicRail",s);
			}
			else if(strstr(s,"oad")) {
				desc = bridge_builder_t::get_desc("ClassicRoad");
				dbg->warning("pillar_t::rdwr()","Unknown bridge %s replaced by ClassicRoad",s);
			}
		}
		asymmetric = desc && desc->has_pillar_asymmetric();

		if(  file->is_version_less(112, 7) && env_t::pak_height_conversion_factor==2  ) {
			switch(dir) {
				case bridge_desc_t::OW_Pillar:  dir = bridge_desc_t::OW_Pillar2;  break;
				case bridge_desc_t::NS_Pillar:  dir = bridge_desc_t::NS_Pillar2;  break;
			}
		}
	}
}


void pillar_t::rotate90()
{
	// since we may have a "3D" offset from the slope, we must remove it beofer rotation
	sint8 hoff = get_yoff();
	set_yoff(0);
	obj_t::rotate90();
	set_yoff(hoff);

	// may need to hide/show asymmetric pillars
	// this is done now in calc_image, which is called after karte_t::rotate anyway
	// we cannot decide this here, since welt->lookup(get_pos())->get_grund_hang() cannot be called
	// since we are in the middle of the rotation process

	// the rotated image parameter is just one in front/back
	switch(dir) {
		case bridge_desc_t::NS_Pillar:  dir=bridge_desc_t::OW_Pillar  ; break;
		case bridge_desc_t::OW_Pillar:  dir=bridge_desc_t::NS_Pillar  ; break;
		case bridge_desc_t::NS_Pillar2: dir=bridge_desc_t::OW_Pillar2 ; break;
		case bridge_desc_t::OW_Pillar2: dir=bridge_desc_t::NS_Pillar2 ; break;
	}
}
