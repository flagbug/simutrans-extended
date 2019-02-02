/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../simdebug.h"
#include "../sys/simsys.h"

#include "scenario_frame.h"
#include "scenario_info.h"
#include "messagebox.h"

#include "simwin.h"
#include "../simworld.h"

#include "../dataobj/environment.h"
#include "../dataobj/scenario.h"
#include "../dataobj/translator.h"

#include "../network/network.h"

#include "../utils/cbuffer_t.h"

scenario_frame_t::scenario_frame_t() : savegame_frame_t(NULL, true, NULL, false)
{
	static cbuffer_t pakset_scenario;
	static cbuffer_t addons_scenario;

	pakset_scenario.clear();
	pakset_scenario.printf("%s%sscenario/", env_t::program_dir, env_t::objfilename.c_str());

	addons_scenario.clear();
	addons_scenario.printf("addons/%sscenario/", env_t::objfilename.c_str());

	if (env_t::default_settings.get_with_private_paks()) {
		this->add_path(addons_scenario);
	}
	this->add_path(pakset_scenario);

	easy_server.init( button_t::square_automatic, "Start this as a server");
	bottom_left_frame.add_component(&easy_server);

	set_name(translator::translate("Load scenario"));
	set_focus(NULL);
}


/**
 * Action, started after button pressing.
 * @author Hansj�rg Malthaner
 */
bool scenario_frame_t::item_action(const char *fullpath)
{
	if(  env_t::server  ) {
		// kill current session
		welt->announce_server(2);
		remove_port_forwarding( env_t::server );
		network_core_shutdown();
		if(  env_t::fps==15  ) {
			env_t::fps = 25;
		}
	}

	scenario_t *scn = new scenario_t(welt);
	const char* err = scn->init(this->get_basename(fullpath).c_str(), this->get_filename(fullpath).c_str(), welt );
	if (err == NULL) {
		// start the game
		welt->set_pause(false);
		// open scenario info window
		destroy_win(magic_scenario_info);
		create_win(new scenario_info_t(), w_info, magic_scenario_info);
	}
	else {
		create_win(new news_img(err), w_info, magic_none);
		delete scn;
	}

	if(  easy_server.pressed  ) {
		// now start a server with defaults
		env_t::networkmode = network_init_server( env_t::server_port );
		if(  env_t::networkmode  ) {
			// query IP and try to open ports on router
			char IP[256];
			if(  prepare_for_server( IP, env_t::server_port )  ) {
				// we have forwarded a port in router, so we can continue
				env_t::server_dns = IP;
				if(  env_t::server_name.empty()  ) {
					env_t::server_name = std::string("Server at ")+IP;
				}
				env_t::server_announce = 1;
				env_t::easy_server = 1;
				if(  env_t::fps>15  ) {
					env_t::fps = 15;
				}
			}
		}
	}

	return true;
}


const char *scenario_frame_t::get_info(const char *filename)
{
	static char info[PATH_MAX];

	sprintf(info,"%s",this->get_filename(filename, false).c_str());

	return info;
}


bool scenario_frame_t::check_file( const char *filename, const char * )
{
	char buf[PATH_MAX];
	sprintf( buf, "%s/scenario.nut", filename );
	if (FILE* const f = dr_fopen(buf, "r")) {
		fclose(f);
		return true;
	}
	return false;
}
