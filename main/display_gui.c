/*
 * display_gui.c
 *
 *  Created on: 21 oct. 2021
 *      Author: romeofernandoromeo
 */

#include "display_gui.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_types.h"
#include "driver/i2c.h"

/**** User variables ****/

#define B2 				35	// Middle button

// Main menu and globals
static lv_group_t* pp_group;
static lv_obj_t *btn_settings;
static lv_obj_t *btn_measure;
static lv_obj_t *btn_record;
static lv_style_t window_bg_style;
static lv_style_t win_btn_style;

// Settings menu
static lv_obj_t* devmode_switch;
static lv_obj_t* settings_menu;
static lv_obj_t *btn_settings_close;
static lv_obj_t* set_devmode_btn;
static lv_obj_t* user1_btn;
static lv_obj_t* user2_btn;
static lv_obj_t* user3_btn;
static lv_obj_t* set_time_btn;
static lv_obj_t* motor_control_btn;
static lv_obj_t* view_graph_btn;
static bool dev_state = false;
static lv_style_t btn_settings_style;

// Measure window
static lv_obj_t *btn_measure_close;
lv_task_t *get_measure_status_task;
measure_stage measure_STATUS = MEASURE_IDLE;
static lv_obj_t * measure_status_label;
static float measured_bpm = 0.;
static float measured_sys = 0.;
static float measured_dias = 0.;

//Results window
static lv_obj_t *btn_results_close;
static lv_obj_t * sys_label ;
static lv_obj_t * dias_label ;
static lv_obj_t * bpm_label ;
static lv_obj_t * par;
static int kk=0;
//lv_task_t *get_measure_status_task;
//measure_stage measure_STATUS = MEASURE_IDLE;
//static lv_obj_t * measure_status_label;
//static float measured_bpm = 0.;

/**** User function prototypes ****/
#if !SIMULATION
int32_t btn_encoder_count;
bool get_encoder_button_data_cb(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);
uint8_t pwm_power;
#endif
static void btn_settings_cb(lv_obj_t * btn, lv_event_t event);

// Settings menu
static void settings_window();
static void btn_devmode_cb(lv_obj_t * btn, lv_event_t event);
static void close_win_settings_cb(lv_obj_t *btn, lv_event_t event);
static void btn_view_graph_cb(lv_obj_t *btn, lv_event_t event);
static void add_dev_settings();
static void btn_motor_control_cb(lv_obj_t *btn, lv_event_t event);

// Measure window
static void measure_window();
static void btn_measure_cb(lv_obj_t * btn, lv_event_t event);
static void close_win_measure_cb(lv_obj_t *btn, lv_event_t event);
static void get_measure_status_cb(lv_task_t *task);
//void write_measured_bpm(float bpm);

// Measure result window
static void measure_result_window();
static void close_win_results_cb(lv_obj_t *btn, lv_event_t event);
extern void write_measured_bpm(float bpm, float sys, float dias);

void pp_ui(void){


	pp_group = lv_group_create();

		/* Initialize input driver(encoder) */
		lv_indev_drv_t enc_drv;
		lv_indev_drv_init(&enc_drv);
		enc_drv.type = LV_INDEV_TYPE_ENCODER;
	#if SIMULATION
		enc_drv.read_cb = mousewheel_read;
	#else
		enc_drv.read_cb = get_encoder_button_data_cb;
	#endif
		lv_indev_t *enc_indev = lv_indev_drv_register(&enc_drv);
		lv_indev_set_group(enc_indev, pp_group);

		/* style win bg */
		lv_style_init(&window_bg_style);
		lv_style_set_bg_color(&window_bg_style, LV_STATE_DEFAULT, lv_color_hex(0xFFFFFF));
		lv_style_set_bg_grad_color(&window_bg_style, LV_STATE_DEFAULT, lv_color_hex(0xdff9fb));
		lv_style_set_bg_grad_dir(&window_bg_style, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);
		lv_style_set_bg_main_stop(&window_bg_style, LV_STATE_DEFAULT, 10);
		lv_style_set_bg_grad_stop(&window_bg_style, LV_STATE_DEFAULT, 200);
		/* Create main window */
		lv_obj_t *win_main = lv_win_create(lv_scr_act(), NULL);
		lv_win_set_title(win_main, "Main menu");
		lv_win_set_header_height(win_main, 40);
		lv_obj_set_style_local_text_color(win_main, LV_WIN_PART_HEADER, LV_STATE_DEFAULT, lv_color_hex(0xffffff));
		lv_obj_set_style_local_bg_color(win_main, LV_WIN_PART_HEADER, LV_STATE_DEFAULT, lv_color_hex(0x686de0)); // f2f9fa
		lv_obj_add_style(win_main, LV_WIN_PART_BG, &window_bg_style);

		/* label main window */
		lv_obj_t *label_win_main = lv_label_create(lv_scr_act(), NULL);
		lv_label_set_text(label_win_main, "22:34");
		lv_obj_set_style_local_text_color(label_win_main, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xFFFFFF));
		lv_obj_align(label_win_main, win_main, LV_ALIGN_IN_TOP_RIGHT, -20, 12);

		/* Add buttons to main window */

		/* button style */
		static lv_style_t main_btn_style;
		lv_style_init(&main_btn_style);
		lv_style_set_radius(&main_btn_style, LV_STATE_DEFAULT, 10);
		lv_style_set_outline_color(&main_btn_style, LV_STATE_DEFAULT, lv_color_hex(0xFFFFFF));
		lv_style_set_border_color(&main_btn_style, LV_STATE_DEFAULT, lv_color_hex(0xFFFFFF));
		lv_style_set_border_opa(&main_btn_style, LV_STATE_DEFAULT, LV_OPA_30);
		lv_style_set_text_color(&main_btn_style, LV_STATE_DEFAULT, lv_color_hex(0xFFFFFF));
		lv_style_set_outline_width(&main_btn_style, LV_STATE_FOCUSED, 7);
		lv_style_set_outline_color(&main_btn_style, LV_STATE_FOCUSED, lv_color_hex(0xb0eaff));
		lv_style_set_outline_opa(&main_btn_style, LV_STATE_FOCUSED, LV_OPA_80);
		lv_style_set_transform_width(&main_btn_style, LV_STATE_FOCUSED, 5);
		lv_style_set_transform_height(&main_btn_style, LV_STATE_FOCUSED, 5);
		/* label style */
		static lv_style_t main_label_style;
		lv_style_init(&main_label_style);
		lv_style_set_text_color(&main_label_style, LV_STATE_DEFAULT, lv_color_hex(0xffffff));
		/* heart icon style */
		static lv_style_t heart_icon_style;
		lv_style_init(&heart_icon_style);
		lv_style_set_text_color(&heart_icon_style, LV_STATE_DEFAULT, lv_color_hex(0xffffff));
		lv_style_set_text_font(&heart_icon_style, LV_STATE_DEFAULT, &heart_44);
		static lv_style_t user_icon_style;
		lv_style_init(&user_icon_style);
		lv_style_set_text_color(&user_icon_style, LV_STATE_DEFAULT, lv_color_hex(0xffffff));
		lv_style_set_text_font(&user_icon_style, LV_STATE_DEFAULT, &user_44);
		/* icon style */
		static lv_style_t main_icon_style;
		lv_style_init(&main_icon_style);
		lv_style_set_text_color(&main_icon_style, LV_STATE_DEFAULT, lv_color_hex(0xffffff));
		lv_style_set_text_font(&main_icon_style, LV_STATE_DEFAULT, &lv_font_montserrat_44);


		const uint8_t padding_btn = 16;
		const uint8_t button_wh = 85;

		/* btn settings */
		btn_settings = lv_btn_create(win_main, NULL);
		lv_obj_set_size(btn_settings, button_wh, button_wh);
		lv_obj_align(btn_settings, NULL, LV_ALIGN_IN_LEFT_MID, padding_btn,	0);
		lv_obj_set_style_local_bg_color(btn_settings, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x95afc0));
		lv_obj_set_style_local_bg_color(btn_settings, LV_BTN_PART_MAIN, LV_STATE_PRESSED, lv_color_hex(0xc3dbeb));
		lv_group_add_obj(pp_group, btn_settings);
		lv_obj_add_style(btn_settings, LV_BTN_PART_MAIN, &main_btn_style);
		lv_obj_set_event_cb(btn_settings, btn_settings_cb);
		/* icon settings */
		lv_obj_t *icon_settings = lv_label_create(btn_settings, NULL);
		lv_label_set_text(icon_settings, "\uf007" );
		lv_obj_set_style_local_pad_left(icon_settings, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 2);
		//lv_obj_set_style_local_pad_top(icon_settings, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 20);
		lv_obj_add_style(icon_settings, LV_LABEL_PART_MAIN, &user_icon_style);
		/* label settings */
		lv_obj_t *label_settings = lv_label_create(btn_settings, NULL);
		lv_label_set_text(label_settings, "User");
		lv_obj_add_style(label_settings, LV_LABEL_PART_MAIN, &main_label_style);

		/* btn measure */
		btn_measure = lv_btn_create(win_main, NULL);
		lv_obj_set_size(btn_measure, button_wh, button_wh);
		lv_obj_align(btn_measure, btn_settings, LV_ALIGN_OUT_RIGHT_MID, padding_btn,	0);
		lv_obj_set_style_local_bg_color(btn_measure, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xeb4d4b));
		lv_obj_set_style_local_bg_color(btn_measure, LV_BTN_PART_MAIN, LV_STATE_PRESSED, lv_color_hex(0xff7979));
		lv_group_add_obj(pp_group, btn_measure);
		lv_obj_add_style(btn_measure, LV_BTN_PART_MAIN, &main_btn_style);
		lv_obj_set_event_cb(btn_measure, btn_measure_cb);
		/* icon measure */
		lv_obj_t *icon_measure = lv_label_create(btn_measure, NULL);
		//lv_label_set_text(icon_measure, LV_SYMBOL_PLAY);
		lv_label_set_text(icon_measure, "\uf004");
		lv_obj_set_style_local_pad_left(icon_measure, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 2);
		//lv_obj_add_style(icon_measure, LV_LABEL_PART_MAIN, &main_icon_style);
		lv_obj_add_style(icon_measure, LV_LABEL_PART_MAIN, &heart_icon_style);
		/* label measure */
		lv_obj_t *label_measure = lv_label_create(btn_measure, NULL);
		lv_label_set_text(label_measure, "Measure");
		lv_obj_add_style(label_measure, LV_LABEL_PART_MAIN, &main_label_style);

		/* btn records */
		btn_record = lv_btn_create(win_main, NULL);
		lv_obj_set_size(btn_record, button_wh, button_wh);
		lv_obj_align(btn_record, btn_measure, LV_ALIGN_OUT_RIGHT_MID, padding_btn, 0);
		lv_obj_set_style_local_bg_color(btn_record, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xf9ca24));
		lv_obj_set_style_local_bg_color(btn_record, LV_BTN_PART_MAIN, LV_STATE_PRESSED, lv_color_hex(0xf6e58d));
		lv_group_add_obj(pp_group, btn_record);
		lv_obj_add_style(btn_record, LV_BTN_PART_MAIN, &main_btn_style);
		/* icon records */
		lv_obj_t *icon_record = lv_label_create(btn_record, NULL);
		lv_label_set_text(icon_record, LV_SYMBOL_DIRECTORY);
		lv_obj_set_style_local_pad_left(icon_record, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 2);
		lv_obj_add_style(icon_record, LV_LABEL_PART_MAIN, &main_icon_style);
		/* label records */
		lv_obj_t *label_record = lv_label_create(btn_record, NULL);
		lv_label_set_text(label_record, "Records");
		lv_obj_add_style(label_record, LV_LABEL_PART_MAIN, &main_label_style);

		/* window buttons style */
		lv_style_init(&win_btn_style);
		lv_style_set_radius(&win_btn_style, LV_STATE_DEFAULT, 10);
		lv_style_set_outline_color(&win_btn_style, LV_STATE_DEFAULT, lv_color_hex(0xFFFFFF));
		lv_style_set_border_color(&win_btn_style, LV_STATE_DEFAULT, lv_color_hex(0xFFFFFF));
		lv_style_set_border_opa(&win_btn_style, LV_STATE_DEFAULT, LV_OPA_30);
		lv_style_set_text_color(&win_btn_style, LV_STATE_DEFAULT, lv_color_hex(0xFFFFFF));
		lv_style_set_outline_width(&win_btn_style, LV_STATE_FOCUSED, 2);
		lv_style_set_outline_color(&win_btn_style, LV_STATE_FOCUSED, lv_color_hex(0xb0eaff));
		lv_style_set_outline_opa(&win_btn_style, LV_STATE_FOCUSED, LV_OPA_80);
		lv_style_set_transform_height(&win_btn_style, LV_STATE_DEFAULT, -10);
		lv_style_set_transform_width(&win_btn_style, LV_STATE_DEFAULT, -10);

}


#if !SIMULATION
bool get_encoder_button_data_cb(lv_indev_drv_t *indev_drv, lv_indev_data_t *data){
  static int32_t last_encoder_diff = 0;
  int32_t encoder_val = btn_encoder_count;
  int32_t encoder_diff = encoder_val - last_encoder_diff;
  data->enc_diff = encoder_diff;
  last_encoder_diff = encoder_val;

  //btn select=B2 (middle)
  if (!gpio_get_level(B2)) data->state = LV_INDEV_STATE_PR;
  else data->state = LV_INDEV_STATE_REL;

  return false;
}
#endif

static void btn_settings_cb(lv_obj_t * btn, lv_event_t event){
	if(event == LV_EVENT_SHORT_CLICKED) settings_window();
}

static void btn_measure_cb(lv_obj_t * btn, lv_event_t event){
	if(event == LV_EVENT_SHORT_CLICKED){
		measure_STATUS = SCHEDULED;
		measure_window();
	}
}

void write_measure_status(measure_stage stage){
	measure_STATUS = stage;
}

measure_stage read_measure_status(){
	return measure_STATUS;
}

static void measure_window(){

	/* Remove items from scrolling group */
	lv_group_remove_all_objs(pp_group);

	/* Create window */
	lv_obj_t *win_measure = lv_win_create(lv_scr_act(), NULL);
	lv_win_set_title(win_measure, "");
	lv_win_set_header_height(win_measure, 40);
	lv_obj_set_style_local_text_color(win_measure, LV_WIN_PART_HEADER, LV_STATE_DEFAULT, lv_color_hex(0xffffff));
	lv_obj_set_style_local_bg_color(win_measure, LV_WIN_PART_HEADER, LV_STATE_DEFAULT, lv_color_hex(0x5a6975));
	//lv_obj_add_style(win_measure, LV_WIN_PART_BG, &window_bg_style);
	lv_obj_set_style_local_bg_color(win_measure, LV_WIN_PART_BG, LV_STATE_DEFAULT, lv_color_hex(0xcedfed)); //0x130f40
	lv_win_set_layout(win_measure, LV_LAYOUT_COLUMN_MID);
	/* Close button */
	btn_measure_close = lv_win_add_btn_right(win_measure, "");
	lv_obj_add_style(btn_measure_close, LV_BTN_PART_MAIN, &win_btn_style);
	lv_obj_set_event_cb(btn_measure_close, close_win_measure_cb);
	lv_obj_t *close_btn_label = lv_label_create(btn_measure_close, NULL);
	lv_label_set_text(close_btn_label, LV_SYMBOL_CLOSE);
	lv_obj_set_style_local_text_color(close_btn_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xffffff));
	lv_group_add_obj(pp_group, btn_measure_close);

	/* Label */
	lv_obj_t * measure_label = lv_label_create(win_measure, NULL);
	lv_label_set_text(measure_label, "\n\n\nPerforming measurement...\n Please stay still.");
	lv_label_set_align(measure_label, LV_LABEL_ALIGN_CENTER);
	//lv_obj_set_style_local_text_color(measure_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xFFFFFF));
	/*
	measure_status_label = lv_label_create(win_measure, NULL);
	lv_label_set_text(measure_status_label, "\n\n\n...");
	lv_label_set_align(measure_status_label, LV_LABEL_ALIGN_CENTER);
	lv_obj_set_style_local_text_color(measure_status_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xFFFFFF));
	*/
	get_measure_status_task = lv_task_create(get_measure_status_cb, 500, LV_TASK_PRIO_MID,NULL);


}

static void measure_result_window(){

	// Remove items from scrolling group
	lv_group_remove_all_objs(pp_group);

	// Create window
	lv_obj_t *win_result = lv_win_create(lv_scr_act(), NULL);
	lv_win_set_title(win_result, "");
	lv_win_set_header_height(win_result, 40);
	lv_obj_set_style_local_text_color(win_result, LV_WIN_PART_HEADER, LV_STATE_DEFAULT, lv_color_hex(0xffffff));
	lv_obj_set_style_local_bg_color(win_result, LV_WIN_PART_HEADER, LV_STATE_DEFAULT, lv_color_hex(0x5a6975));
	//lv_obj_add_style(win_measure, LV_WIN_PART_BG, &window_bg_style);
	lv_obj_set_style_local_bg_color(win_result, LV_WIN_PART_BG, LV_STATE_DEFAULT, lv_color_hex(0xcedfed)); //0x130f40
	//lv_win_set_layout(win_result, LV_LAYOUT_COLUMN_MID);

	// Close button
	btn_results_close = lv_win_add_btn_right(win_result, "");
	lv_obj_add_style(btn_results_close, LV_BTN_PART_MAIN, &win_btn_style);
	lv_obj_set_event_cb(btn_results_close, close_win_results_cb);
	lv_obj_t *close_btn_label = lv_label_create(btn_results_close, NULL);
	lv_label_set_text(close_btn_label, LV_SYMBOL_CLOSE);
	lv_obj_set_style_local_text_color(close_btn_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xffffff));
	lv_group_add_obj(pp_group, btn_results_close);

	/* heart icon style */
	static lv_style_t heart_icon_style;
	lv_style_init(&heart_icon_style);
	//lv_style_set_text_color(&heart_icon_style, LV_STATE_DEFAULT, lv_color_hex(0xffffff));
	lv_style_set_text_font(&heart_icon_style, LV_STATE_DEFAULT, &heart_20);

	/*mini window style*/
	static lv_style_t par_style;
	lv_style_init(&par_style);
	lv_style_set_bg_color(&par_style, LV_STATE_DEFAULT,lv_color_hex(0xa6a6a6));
	lv_style_set_outline_width(&par_style, LV_STATE_DEFAULT, 2);
	lv_style_set_outline_color(&par_style, LV_STATE_DEFAULT, lv_color_hex(0x5a6975));
	lv_style_set_outline_pad(&par_style, LV_STATE_DEFAULT, 8);


	par = lv_obj_create(win_result, NULL); /*Create a parent object on the current screen*/
	lv_obj_set_size(par, 200, 160);
	lv_obj_align(par,NULL, LV_ALIGN_CENTER,0,0);
	lv_obj_add_style(par, LV_LABEL_PART_MAIN, &par_style);
	lv_group_add_obj(pp_group, par);
	//lv_obj_set_color(par, lv_color_hex(0xa6a6a6));

	/*result style*/
	static lv_style_t result_style;
	lv_style_init(&result_style);
	lv_style_set_text_font(&result_style, LV_STATE_DEFAULT, &montserrat_64);
	static lv_style_t bpm_style;
	lv_style_init(&bpm_style);
	lv_style_set_text_font(&bpm_style, LV_STATE_DEFAULT,&lv_font_montserrat_30);

	/* Label SYS*/
	sys_label = lv_label_create(par, NULL);
	lv_label_set_text(sys_label, "106");
	lv_obj_add_style(sys_label, LV_LABEL_PART_MAIN, &result_style);
	lv_obj_align(sys_label, par, LV_ALIGN_CENTER, 40,-30);
	//lv_label_set_align(sys_label, LV_LABEL_ALIGN_CENTER, 60, 60);
	//lv_obj_set_style_local_text_color(sys_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xFFFFFF));

	/* Label DIAS*/
	dias_label = lv_label_create(par, NULL);
	lv_label_set_text(dias_label, "87");
	lv_obj_add_style(dias_label, LV_LABEL_PART_MAIN, &result_style);
	lv_obj_align(dias_label, par, LV_ALIGN_CENTER, 45,40);
	//lv_label_set_align(dias_label, LV_LABEL_ALIGN_RIGHT);
	//lv_obj_set_style_local_text_color(dias_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xFFFFFF));

	/* Label BPM*/
	bpm_label = lv_label_create(par, NULL);
	lv_label_set_text(bpm_label, "75");
	lv_obj_add_style(bpm_label, LV_LABEL_PART_MAIN, &bpm_style);
	lv_obj_align(bpm_label, par, LV_ALIGN_CENTER, -45,45);
	//lv_label_set_align(bpm_label, LV_LABEL_ALIGN_LEFT);
	//lv_obj_set_style_local_text_color(bpm_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xFFFFFF));

	/*Label heart*/
	lv_obj_t * heart_label = lv_label_create(par, NULL);
	lv_obj_add_style(heart_label, LV_LABEL_PART_MAIN, &heart_icon_style);
	lv_label_set_text(heart_label, "\uf004");
	lv_obj_align(heart_label, par, LV_ALIGN_CENTER, -80,45);

	if(measure_STATUS == WRITE_RESULTS){
		char buf1[30];
		sprintf(buf1, "%2d", (int)(measured_bpm));
		char buf2[30];
		sprintf(buf2, "%2d", (int)(measured_sys));
		char buf3[30];
		sprintf(buf3, "%2d", (int)(measured_dias));
		lv_label_set_text(bpm_label, buf1);
		lv_label_set_text(sys_label, buf2);
		lv_label_set_text(dias_label, buf3);
		kk=1;
		if (!gpio_get_level(B2)){
			uint32_t btn_id = 0;
			lv_event_send(btn_results_close, LV_EVENT_RELEASED, &btn_id);
			//lv_btn_set_state(btn_results_close, LV_BTN_STATE_PRESSED);
			//lv_btn_set_state(btn_results_close, LV_BTN_STATE_RELEASED);
			lv_win_close_event_cb(btn_results_close, LV_EVENT_RELEASED);
			//lv_obj_set_event_cb(btn_measure_close, close_win_measure_cb);

			/* Re-add settings group items */
			lv_group_add_obj(pp_group, btn_settings);
			lv_group_add_obj(pp_group, btn_measure);
			lv_group_add_obj(pp_group, btn_record);
			measure_STATUS = MEASURE_IDLE;

		}
	}

	/*
	measure_status_label = lv_label_create(win_result, NULL);
	lv_label_set_text(measure_status_label, "\n\n\n...");
	lv_label_set_align(measure_status_label, LV_LABEL_ALIGN_CENTER);
	lv_obj_set_style_local_text_color(measure_status_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xFFFFFF));
	*/
	//get_measure_status_task = lv_task_create(get_measure_status_cb, 1000, LV_TASK_PRIO_MID,NULL);


}
static void close_win_results_cb(lv_obj_t *btn, lv_event_t event){
	if(event == LV_EVENT_RELEASED){

			lv_win_close_event_cb(btn, event);
			//lv_obj_set_event_cb(btn_measure_close, close_win_measure_cb);
			/* Re-add settings group items */
			lv_group_add_obj(pp_group, btn_settings);
			lv_group_add_obj(pp_group, btn_measure);
			lv_group_add_obj(pp_group, btn_record);
			measure_STATUS = MEASURE_IDLE;
		}
}
static void close_win_measure_cb(lv_obj_t *btn, lv_event_t event){
	if(event == LV_EVENT_RELEASED){
			lv_win_close_event_cb(btn, event);
			/* Re-add settings group items */
			lv_group_add_obj(pp_group, btn_settings);
			lv_group_add_obj(pp_group, btn_measure);
			lv_group_add_obj(pp_group, btn_record);
		}
}

static void get_measure_status_cb(lv_task_t *task){

	#if SIMULATION
		static uint8_t i = 5;
		if (i== 0){
			measure_STATUS = DONE;
			write_measured_bpm(77, 114, 90);
		}
		i--;
	#endif
/*
	if (measure_STATUS == DONE){
		char buf[50];
		sprintf(buf, "\nBPM: %2d, SYS: NaN, DIA: NaN\nDone.", (int)(measured_bpm));
		lv_label_set_text(measure_status_label, buf);
	}
*/
	if(measure_STATUS == WRITE_RESULTS){

		measure_result_window();
	}
	#if SIMULATION
	if(kk==1) measure_STATUS = MEASURE_IDLE;
	#endif
}

void write_measured_bpm(float bpm, float sys, float dias){
	measured_bpm = bpm;
	measured_sys = sys;
	measured_dias = dias;
}

static void settings_window(){

	/* Remove items from scrolling group */
	lv_group_remove_all_objs(pp_group);

	/* Create window */
	lv_obj_t *win_settings = lv_win_create(lv_scr_act(), NULL);
	lv_win_set_title(win_settings, "Settings");
	lv_win_set_header_height(win_settings, 40);
	lv_obj_set_style_local_text_color(win_settings, LV_WIN_PART_HEADER, LV_STATE_DEFAULT, lv_color_hex(0xffffff));
	lv_obj_set_style_local_bg_color(win_settings, LV_WIN_PART_HEADER, LV_STATE_DEFAULT, lv_color_hex(0x686de0)); // f2f9fa
	lv_obj_add_style(win_settings, LV_WIN_PART_BG, &window_bg_style);
	/* Close button */
	btn_settings_close = lv_win_add_btn_right(win_settings, "");
	lv_obj_add_style(btn_settings_close, LV_BTN_PART_MAIN, &win_btn_style);
	lv_obj_set_event_cb(btn_settings_close, close_win_settings_cb);
	lv_obj_t *close_btn_label = lv_label_create(btn_settings_close, NULL);
	lv_label_set_text(close_btn_label, LV_SYMBOL_CLOSE);
	lv_obj_set_style_local_text_color(close_btn_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xffffff));
	lv_group_add_obj(pp_group, btn_settings_close);


	/* Create settings menu */
	settings_menu = lv_list_create(win_settings, NULL);
	lv_obj_set_size(settings_menu, lv_obj_get_width_fit(lv_scr_act())-30, lv_obj_get_height_fit(lv_scr_act())-70);

	/* Button style */
	lv_style_init(&btn_settings_style);
	lv_style_set_outline_color(&btn_settings_style, LV_STATE_FOCUSED, lv_color_hex(0x22a6b3));
	lv_style_set_radius(&btn_settings_style, LV_STATE_FOCUSED, 10);

	/* User select Buttons */
	set_devmode_btn = lv_list_add_btn(settings_menu,NULL,"User 1");
	lv_obj_add_style(set_devmode_btn, LV_BTN_PART_MAIN, &btn_settings_style);
	devmode_switch = lv_switch_create(win_settings, NULL);
	lv_obj_align(devmode_switch, devmode_switch, LV_ALIGN_IN_TOP_LEFT, 220, 6);
	lv_obj_set_event_cb(set_devmode_btn,btn_devmode_cb);


	set_time_btn = lv_list_add_btn(settings_menu,LV_SYMBOL_EDIT,"Set time");
	lv_obj_add_style(set_time_btn, LV_BTN_PART_MAIN, &btn_settings_style);

	lv_group_add_obj(pp_group, set_devmode_btn);
	lv_group_add_obj(pp_group, set_time_btn);
	if(dev_state){
		add_dev_settings();
		lv_switch_on(devmode_switch, LV_ANIM_OFF);
	}
}

static void btn_devmode_cb(lv_obj_t * btn, lv_event_t event){
	if(event == LV_EVENT_SHORT_CLICKED) {
		lv_switch_toggle(devmode_switch, LV_ANIM_ON);

		if (dev_state){
			dev_state = false;
			lv_list_remove(settings_menu, lv_list_get_size(settings_menu)-1);
			lv_list_remove(settings_menu, lv_list_get_size(settings_menu)-1);

		} else{
			dev_state = true;
			add_dev_settings();
		}

	}
}

static void add_dev_settings(){
	/*
	view_graph_btn = lv_list_add_btn(settings_menu,NULL,"Live graph");
	lv_obj_add_style(view_graph_btn, LV_BTN_PART_MAIN, &btn_settings_style);
	lv_obj_set_event_cb(view_graph_btn, btn_view_graph_cb);

	motor_control_btn = lv_list_add_btn(settings_menu,NULL,"Motor control");
	lv_obj_add_style(motor_control_btn, LV_BTN_PART_MAIN, &btn_settings_style);
	lv_obj_set_event_cb(motor_control_btn, btn_motor_control_cb);

	lv_group_add_obj(pp_group, view_graph_btn);
	lv_group_add_obj(pp_group, motor_control_btn);
	*/
}


static void close_win_settings_cb(lv_obj_t *btn, lv_event_t event){
	if(event == LV_EVENT_RELEASED){
			lv_win_close_event_cb(btn, event);
			/* Re-add settings group items */
			lv_group_add_obj(pp_group, btn_settings);
			lv_group_add_obj(pp_group, btn_measure);
			lv_group_add_obj(pp_group, btn_record);
		}
}
