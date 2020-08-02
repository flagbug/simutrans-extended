/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string.h>
#include <cmath>

#include "gui_chart.h"
#include "../gui_frame.h"
#include "../simwin.h"
#include "../../simcolor.h"
#include "../../utils/simstring.h"
#include "../../dataobj/environment.h"
#include "../../display/simgraph.h"
#include "../gui_theme.h"

static char tooltip[64];

/**
 * Set background color. -1 means no background
 * @author Hj. Malthaner
 */
void gui_chart_t::set_background(int i)
{
	background = i;
}


gui_chart_t::gui_chart_t() : gui_component_t()
{
	// no toolstips at the start
	tooltipcoord = scr_coord::invalid;

	seed = 0;
	show_x_axis = true;
	show_y_axis = true;
	ltr = false;
	x_elements = 0;
	x_label_span = 1;
	x_axis_span = 1;

	// Hajo: transparent by default
	background = -1;
}


int gui_chart_t::add_curve(int color, const sint64 *values, int size, int offset, int elements, int type, bool show, bool show_value, int precision, convert_proc proc, chart_marker_t marker_type)
{
	curve_t new_curve;
	new_curve.color = color;
	new_curve.values = values;
	new_curve.size = size;
	new_curve.offset = offset;
	new_curve.elements = elements;
	new_curve.show = show;
	new_curve.show_value = show_value;
	new_curve.type = type;
	new_curve.precision = precision;
	new_curve.convert = proc;
	new_curve.marker_type = marker_type;
	curves.append(new_curve);
	return curves.get_count();
}


uint32 gui_chart_t::add_line(int color, const sint64 *value, int times, bool show, bool show_value, int precision, convert_proc proc, chart_marker_t marker_type)
{
	line_t new_line;
	new_line.color = color;
	new_line.value = value;
	new_line.times = times;
	new_line.show = show;
	new_line.show_value = show_value;
	new_line.precision = precision;
	new_line.convert = proc;
	new_line.marker_type = marker_type;
	lines.append(new_line);
	return lines.get_count();
}


void gui_chart_t::hide_curve(unsigned int id)
{
	if (id < curves.get_count()) {
		curves.at(id).show = false;
	}
}


void gui_chart_t::show_curve(unsigned int id)
{
	if (id < curves.get_count()) {
		curves.at(id).show = true;
	}
}


void gui_chart_t::show_line(uint32 id)
{
	if(  id<lines.get_count()  ) {
		lines.at(id).show = true;
	}
}


void gui_chart_t::hide_line(uint32 id)
{
	if(  id<lines.get_count()  ) {
		lines.at(id).show = false;
	}
}


void gui_chart_t::draw(scr_coord offset)
{
	offset += pos;

	sint64 last_year=0, tmp=0;
	char cmin[128] = "0", cmax[128] = "0", digit[11];

	sint64 baseline = 0;
	sint64* pbaseline = &baseline;

	float scale = 0;
	float* pscale = &scale;

	// calc baseline and scale
	calc_gui_chart_values(pbaseline, pscale, cmin, cmax);

	// Hajo: draw background if desired
	if(background != -1) {
		display_fillbox_wh_clip(offset.x, offset.y, size.w, size.h, background, false);
	}
	int tmpx, factor;
	if(env_t::left_to_right_graphs) {
		tmpx = offset.x + size.w - size.w % (x_elements - 1);
		factor = -1;
	}
	else {
		tmpx = offset.x;
		factor = 1;
	}

	// draw zero line
	display_direct_line(offset.x+1, offset.y+(scr_coord_val)baseline, offset.x+size.w-2, offset.y+(scr_coord_val)baseline, SYSCOL_CHART_LINES_ZERO);

	if (show_y_axis) {

		// draw zero number only, if it will not disturb any other printed values!
		if ((baseline > 18) && (baseline < size.h -18)) {
			display_proportional_clip(offset.x - 4, offset.y+(scr_coord_val)baseline-3, "0", ALIGN_RIGHT, SYSCOL_TEXT_HIGHLIGHT, true );
		}

		// display min/max money values
		display_proportional_clip(offset.x - 4, offset.y-5, cmax, ALIGN_RIGHT, SYSCOL_TEXT_HIGHLIGHT, true );
		display_proportional_clip(offset.x - 4, offset.y+size.h-5, cmin, ALIGN_RIGHT, SYSCOL_TEXT_HIGHLIGHT, true );
	}

	// draw chart frame
	display_ddd_box_clip(offset.x, offset.y, size.w, size.h, SYSCOL_SHADOW, SYSCOL_HIGHLIGHT);

	// draw chart lines
	scr_coord_val x_last = 0;  // remember last digit position to avoid overwriting by next label
	for(  int i = 0;  i < x_elements;  i++  ) {
		const int j = env_t::left_to_right_graphs ? x_elements - 1 - i : i;
		const scr_coord_val x0 = tmpx + factor * (size.w / (x_elements - 1) ) * j;
		const COLOR_VAL line_color = (i%2) ? SYSCOL_CHART_LINES_ODD : SYSCOL_CHART_LINES_EVEN;
		if(  show_x_axis  ) {
			// display x-axis
			sprintf(digit, j % x_label_span == 0 ? "%i" : "", abs(seed - (j*x_axis_span)));
			scr_coord_val x =  x0 - (seed != j ? (int)(2 * log( (double)abs(seed - j) )) : 0);
			if(  x > x_last  ) {
				x_last = x + display_proportional_clip( x, offset.y + size.h + 6, digit, ALIGN_LEFT, line_color, true );
			}
		}
		// year's vertical lines
		display_vline_wh_clip( x0, offset.y + 1, size.h - 2, line_color, false );
	}

	// display current value?
	int tooltip_n=-1;
	if(tooltipcoord!=scr_coord::invalid) {
		if(env_t::left_to_right_graphs) {
			tooltip_n = x_elements-1-(tooltipcoord.x*x_elements+4)/(size.w|1);
		}
		else {
			tooltip_n = (tooltipcoord.x*x_elements+4)/(size.w|1);
		}
	}

	// draw chart's curves
	FOR(slist_tpl<curve_t>, const& c, curves) {
		if (c.show) {
			double display_tmp;
			int start = abort_display_x ? (env_t::left_to_right_graphs ? c.elements - abort_display_x : 0) : 0;
			int end   = abort_display_x ? (env_t::left_to_right_graphs ? c.elements : abort_display_x) : c.elements;
			// for each curve iterate through all elements and display curve
			for (int i=0; i<end; i++) {
				//tmp=c.values[year*c.size+c.offset];
				tmp = c.values[i*c.size+c.offset];
				// Knightly : convert value where necessary
				if(  c.convert  ) {
					tmp = c.convert(tmp);
					display_tmp = tmp;
				}
				else if(  c.type!=0  ) {
					display_tmp = tmp*0.01;
					tmp /= 100;
				}
				else {
					display_tmp = tmp;
				}

				// display marker(box) for financial value
				if (i >= start && i < end) {
					scr_coord_val x = tmpx + factor * (size.w / (x_elements - 1))*i - 2;
					scr_coord_val y = (scr_coord_val)(offset.y + baseline - (long)(tmp / scale) - 2);
					switch (c.marker_type)
					{
						case cross:
							display_direct_line(x, y, x+4, y+4, c.color);
							display_direct_line(x+4, y, x, y+4, c.color);
							break;
						case diamond:
							for (int j = 0; j < 5; j++) {
								display_vline_wh_clip(x+j, y + abs(2 - j), 5-2*abs(2-j), c.color, false);
							}
							break;
						case none:
							// display nothing
							break;
						case square:
						default:
							display_fillbox_wh_clip(x, y, 5, 5, c.color, true);
							break;
					}
				}

				// display tooltip?
				if(i==tooltip_n  &&  abs((int)(baseline-(int)(tmp/scale)-tooltipcoord.y))<10) {
					number_to_string(tooltip, display_tmp, c.precision);
					win_set_tooltip( get_mouse_x()+8, get_mouse_y()-12, tooltip );
				}

				// draw line between two financial markers; this is only possible from the second value on
				if (i>start && i < end) {
					display_direct_line(tmpx+factor*(size.w / (x_elements - 1))*(i-1),
						(scr_coord_val)( offset.y+baseline-(int)(last_year/scale) ),
						tmpx+factor*(size.w / (x_elements - 1))*(i),
						(scr_coord_val)( offset.y+baseline-(int)(tmp/scale) ),
						c.color);
				}
				else if (i == 0 && !abort_display_x || abort_display_x && i == start) {
					// for the first element print the current value (optionally)
					// only print value if not too narrow to min/max/zero
					if(  c.show_value  ) {
						if(  env_t::left_to_right_graphs  ) {
							number_to_string(cmin, display_tmp, c.precision);
							const sint16 width = proportional_string_width(cmin)+7;
							display_ddd_proportional( tmpx + 8, (scr_coord_val)(offset.y+baseline-(int)(tmp/scale)-4), width, 0, COL_GREY4, c.color, cmin, true);
						}
						else if(  (baseline-tmp/scale-8) > 0  &&  (baseline-tmp/scale+8) < size.h  &&  abs((int)(tmp/scale)) > 9  ) {
							number_to_string(cmin, display_tmp, c.precision);
							display_proportional_clip(tmpx - 4, (scr_coord_val)(offset.y+baseline-(int)(tmp/scale)-4), cmin, ALIGN_RIGHT, c.color, true );
						}
					}
				}
				last_year=tmp;
			}
		}
		last_year=tmp=0;
	}

	// draw chart's lines
	FOR(slist_tpl<line_t>, const& line, lines) {
		if(  line.show  ) {
			tmp = ( line.convert ? line.convert(*(line.value)) : *(line.value) );
			for(  int t=0;  t<line.times;  ++t  ) {
				// display marker(box) for financial value
				display_fillbox_wh_clip(tmpx+factor*(size.w / (x_elements - 1))*t-2, (scr_coord_val)(offset.y+baseline- (int)(tmp/scale)-2), 5, 5, line.color, true);

				// display tooltip?
				if(  t==tooltip_n  &&  abs((int)(baseline-(int)(tmp/scale)-tooltipcoord.y))<10  ) {
					number_to_string(tooltip, (double)tmp, line.precision);
					win_set_tooltip( get_mouse_x()+8, get_mouse_y()-12, tooltip );
				}
				// for the first element print the current value (optionally)
				// only print value if not too close to min/max/zero
				if(  t==0  &&  line.show_value  ) {
					if(  env_t::left_to_right_graphs  ) {
						number_to_string(cmin, (double)tmp, line.precision);
						const sint16 width = proportional_string_width(cmin)+7;
						display_ddd_proportional( tmpx + 8, (scr_coord_val)(offset.y+baseline-(int)(tmp/scale)-4), width, 0, COL_GREY4, line.color, cmin, true);
					}
					else if(  (baseline-tmp/scale-8) > 0  &&  (baseline-tmp/scale+8) < size.h  &&  abs((int)(tmp/scale)) > 9  ) {
						number_to_string(cmin, (double)tmp, line.precision);
						display_proportional_clip(tmpx - 4, (scr_coord_val)(offset.y+baseline-(int)(tmp/scale)-4), cmin, ALIGN_RIGHT, line.color, true );
					}
				}
			}
			// display horizontal line that passes through all markers
			const int y_offset = (int)( offset.y + baseline - (sint64)(tmp/scale) );
			display_fillbox_wh(tmpx, y_offset, factor*(size.w / (x_elements - 1))*(line.times-1), 1, line.color, true);
		}
	}
}


void gui_chart_t::calc_gui_chart_values(sint64 *baseline, float *scale, char *cmin, char *cmax) const
{
	sint64 tmp=0;
	double min = 0, max = 0;
	int precision = 0;

	// first, check curves
	FOR(slist_tpl<curve_t>, const& c, curves) {
		if(  c.show  ) {
			for(  int i=0;  i<c.elements;  i++  ) {
				tmp = c.values[i*c.size+c.offset];
				// Knightly : convert value where necessary
				if(  c.convert  ) {
					tmp = c.convert(tmp);
				}
				else if(  c.type!=0  ) {
					tmp /= 100;
					precision = 0;
				}
				if (min > tmp) {
					min = tmp ;
					precision = c.precision;
				}
				if (max < tmp) {
					max = tmp;
					precision = c.precision;
				}
			}
		}
	}

	// second, check lines
	FOR(slist_tpl<line_t>, const& line, lines) {
		if(  line.show  ) {
			tmp = ( line.convert ? line.convert(*(line.value)) : *(line.value) );
			if(  min>tmp  ) {
				min = tmp;
				precision = line.precision;
			}
			if(  max<tmp  ) {
				max = tmp;
				precision = line.precision;
			}
		}
	}

	// max happend due to rounding errors
	if (precision == 0 && min == max && min != 0.0) {
		max += 1;
	}

	number_to_string(cmin, min, precision);
	number_to_string(cmax, max, precision);

	// scale: factor to calculate money with, to get y-pos offset
	*scale = (double)(max - min) / (size.h-2);
	if(*scale==0.0) {
		*scale = 1.0;
	}

	// baseline: y-pos for the "zero" line in the chart
	*baseline = (sint64)(size.h - labs((sint64)(min / *scale)));
}


/**
 * Events werden hiermit an die GUI-components
 * gemeldet
 * @author Hj. Malthaner
 */
bool gui_chart_t::infowin_event(const event_t *ev)
{
	if(IS_LEFTREPEAT(ev)  ||  IS_LEFTCLICK(ev)) {
		// tooptip to show?
		tooltipcoord = scr_coord(ev->mx,ev->my);
	}
	else {
		tooltipcoord = scr_coord::invalid;
		tooltip[0] = 0;
	}
	return true;
}
