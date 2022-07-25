/*
 * display_gui.h
 *
 *  Created on: 21 oct. 2021
 *      Author: romeofernandoromeo
 */

#ifndef PRESSURE_PULSE_SENSOR_DISPLAY_GUI_H_
#define PRESSURE_PULSE_SENSOR_DISPLAY_GUI_H_

#define SIMULATION 0

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "driver/gpio.h"
#if SIMULATION
#include "../lvgl/lvgl.h"
#include "lv_drivers/indev/mousewheel.h"
#else
#include "lvgl/lvgl.h"
//Include fonts
/*
#include "heart_20.c"
#include "heart_44.c"
#include "heart_30.c"
#include "user_44.c"
#include "montserrat_44.c"
#include "montserrat_64.c"
#include "montserrat_30.c"
#include "monterrat_56.c"
*/
//#include "main.h"
#endif

/* User typedef */
typedef enum{MEASURE_IDLE, SCHEDULED, INFLATING, DEFLATING, DONE, WRITE_RESULTS} measure_stage;

/* Shared global variables */
extern lv_obj_t *pp_live_graph;
extern lv_chart_series_t *live_data_series;
//extern int16_t graph_ymin; // DEBUG
//extern int16_t graph_ymax; //DEBUG

/*icon fonts*/
LV_FONT_DECLARE(heart_20);
LV_FONT_DECLARE(heart_44);
LV_FONT_DECLARE(heart_30);
LV_FONT_DECLARE(user_44);
LV_FONT_DECLARE(montserrat_44);
LV_FONT_DECLARE(montserrat_64);
LV_FONT_DECLARE(montserrat_30);
LV_FONT_DECLARE(monterrat_56);

/* Function prototypes */
void pp_ui();
void write_measure_status(measure_stage stage);
measure_stage read_measure_status();
extern void write_measured_bpm(float bpm, float sys, float dias);

#endif /* PRESSURE_PULSE_SENSOR_DISPLAY_GUI_H_ */
