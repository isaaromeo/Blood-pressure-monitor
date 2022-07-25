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
#include "fifo_queue.h"
#include "driver/ledc.h"
#include "findPeaks.h"
#include "fft.h"
#include "display_gui.h"
#include "lvgl/lvgl.h"
#include "lvgl_helpers.h"


#define B1 				34	// Aux button
#define B2 				35	// Middle button
#define B3 				27	// Right button
#define B4 				14	// Left button
#define EOC_PS 			17	// End of Conversion prs sensor
#define RST_PS 			16	// Reset prs sensor
#define SDA_PS 			21	// End of Conversion prs sensor
#define SCL_PS 			5	// End of Conversion prs sensor
#define MPRL_ADDRESS    0x18//I2C adress
#define I2C_MASTER_FREQ_HZ 100000//SCK maste freq 100KHz

//Display
#define LCD_CS 			14
#define LCD_RST			25
#define LCD_DC  		26
#define LCD_MOSI		32
#define LCD_SCK			33

// i2c
#define ACK_CHECK_EN 0x1                        /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0                       /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                             /*!< I2C ack value */
#define NACK_VAL 0x1                            /*!< I2C nack value */

// Pressure related
#define COUNTS_224 (16777216L)      ///< Constant: 2^24
#define PSI_to_HPA (68.947572932)   ///< Constant: PSI to HPA conversion factor
#define PSI_to_ATM (0.068046)
#define PSI_to_mmHg (51.715)   ///< Constant: PSI to mmHg conversion factor
#define MPRLS_OUTPUT_min (uint32_t)((float)COUNTS_224 * (10 / 100.0) + 0.5)
#define MPRLS_OUTPUT_max (uint32_t)((float)COUNTS_224 * (90 / 100.0) + 0.5)
#define MPRLS_PSI_min 0
#define MPRLS_PSI_max 25

//typedef enum{MEASURE_IDLE, SCHEDULED, INFLATING, DEFLATING, DONE} measure_stage;

#define VALVE_PIN 		22
#define FWD_PIN			19
//#define BWD_PIN			18
#define PUMP_CHANNEL_A 	LEDC_CHANNEL_1
//#define PUMP_CHANNEL_B 	LEDC_CHANNEL_2
#define TMR_RES			LEDC_TIMER_12_BIT
//display
#define LV_TICK_PERIOD_MS		1


//measure_stage measure_STATUS;


/* User global variables */

static TaskHandle_t button_task;
static QueueHandle_t button_queue;

static TaskHandle_t pressure_task;
static QueueHandle_t pressure_queue;

static TaskHandle_t pwm_task;
//static QueueHandle_t pressure_queue;

static TaskHandle_t gui_task;
static SemaphoreHandle_t xGuiSemaphore;
int32_t btn_encoder_count = 0;

bool valve_state = false;
bool store = false;
struct fifo_queue oscil_queue;
struct fifo_queue press_queue;

struct envelope env;

float first_pressure = 0;





uint32_t button_count;
static const char* TAG= "ButtonInfo";



/* Task declarations */

static void button_pressed_task(void *params);
static void read_pressure_task(void *params);
static void lv_tick_task(void *arg);
static void gui_manager_task(void *pvParameter);
static void IRAM_ATTR gpio_isr_handler(void *args){
	uint32_t pin_number = (uint32_t)args;
	xQueueSendFromISR(button_queue, &pin_number, NULL);
}
void pressure_control_handler(void *params);

void app_main(void){
	/*Button config*/
	gpio_config_t config;
	config.intr_type = GPIO_INTR_NEGEDGE;
	config.mode = GPIO_MODE_INPUT;
	config.pull_down_en = false;
	config.pull_up_en = true;
	config.pin_bit_mask = ((1ULL<<B1) | (1ULL<<B2) | (1ULL<<B3) | (1ULL<<B4));

	gpio_config(&config);
	button_queue = xQueueCreate(4,sizeof(uint32_t));
	xTaskCreate(button_pressed_task, "button pushed", 2048, NULL, 3, &button_task);

	gpio_install_isr_service(0);
	gpio_isr_handler_add(B1, gpio_isr_handler, (void *)B1);
	gpio_isr_handler_add(B2, gpio_isr_handler, (void *)B2);
	gpio_isr_handler_add(B3, gpio_isr_handler, (void *)B3);
	gpio_isr_handler_add(B4, gpio_isr_handler, (void *)B4);
	//gpio_isr_handler_add(PIN_SWITCH, gpio_isr_handler, (void *)PIN_SWITCH);

	/*I2C config*/

	gpio_pad_select_gpio(RST_PS);
	gpio_set_direction(RST_PS, GPIO_MODE_OUTPUT);
	gpio_set_level(RST_PS, 1);

	gpio_pad_select_gpio(EOC_PS);
	gpio_set_direction(EOC_PS, GPIO_MODE_INPUT);


	i2c_config_t i2c_config = {
	        .mode = I2C_MODE_MASTER,
	        .sda_io_num = SDA_PS,
	        .scl_io_num = SCL_PS,
	        .sda_pullup_en = GPIO_PULLUP_ENABLE,
	        .scl_pullup_en = GPIO_PULLUP_ENABLE,
	        .master.clk_speed = I2C_MASTER_FREQ_HZ};
	i2c_param_config(I2C_NUM_0, &i2c_config);
	i2c_set_timeout(I2C_NUM_0, 1048575);
	i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
	xTaskCreate(read_pressure_task, "read pressure", 4096, NULL, 5, &pressure_task);

	ESP_LOGI(TAG, "Setup done");

	// Configure PWM
	ledc_timer_config_t pwm_timer_config = {
		  .speed_mode = LEDC_LOW_SPEED_MODE,
		  .duty_resolution = TMR_RES,
		  .timer_num = LEDC_TIMER_1,
		  .freq_hz = 15000,
		  .clk_cfg = LEDC_AUTO_CLK};
	ledc_timer_config(&pwm_timer_config);

	ledc_channel_config_t pump_channel_config = {
		  .gpio_num = FWD_PIN,
		  .speed_mode = LEDC_LOW_SPEED_MODE,
		  .channel = PUMP_CHANNEL_A,
		  .timer_sel = LEDC_TIMER_1,
		  .duty = 0,
		  .hpoint = 0};
	ledc_channel_config(&pump_channel_config);

	xTaskCreate(pressure_control_handler, "pwm", 14*2048, NULL, 5, &pwm_task);

	// Configure screen
	xTaskCreate(gui_manager_task, "gui", 4096*2, NULL, 1, &gui_task);
	// see gui_manager_task() and pp_ui() for the layout
	//xTaskCreatePinnedToCore(gui_manager_task, "gui", 4096*2, NULL, 0, &gui_task, 1);

	// Configure valve
	gpio_config_t valve_config = {
			.mode = GPIO_MODE_OUTPUT,
			.pull_up_en = GPIO_PULLUP_DISABLE,
			.pull_down_en = GPIO_PULLDOWN_DISABLE,
			.pin_bit_mask = (1ULL<<VALVE_PIN),
			.intr_type = GPIO_INTR_DISABLE
	};
	ESP_ERROR_CHECK(gpio_config(&valve_config));






}

static void button_pressed_task(void *params)
{

	uint32_t pin_number=0;

	while(1)
	{
		if (xQueueReceive(button_queue, &pin_number, portMAX_DELAY))
		{

			// disable the interrupt
			gpio_isr_handler_remove(pin_number);

			// wait some time while we check for the button to be released
			do
			{
				vTaskDelay(20 / portTICK_PERIOD_MS);
			} while(gpio_get_level(pin_number) == 0);

			//do some work
			switch(pin_number)
			{
			case B1: // Aux button
            	printf("Boton auxiliar ya le daremos algun uso");
            	ESP_LOGI(TAG, "auxiliar");
            	break;

            case B2: // Middle button
            	ESP_LOGI(TAG, "middle");
				break;

            case B3: // Right button
            	//measure_STATUS = DEFLATING;
            	btn_encoder_count++;
            	ESP_LOGI(TAG, "encoder++, value : %d \n", btn_encoder_count);
				break;

            case B4: // Left button
            	//measure_STATUS = SCHEDULED;
            	btn_encoder_count--;
            	ESP_LOGI(TAG, "encoder++, value : %d \n",btn_encoder_count);
				break;
            }

			// re-enable the interrupt
			 gpio_isr_handler_add(pin_number, gpio_isr_handler, (void *)pin_number);



		}
	}
}


uint8_t read_MPRLS_status(){

	uint8_t status_data;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (MPRL_ADDRESS << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
	i2c_master_read_byte(cmd, &status_data, NACK_VAL);
	i2c_master_stop(cmd);
	ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS));
	i2c_cmd_link_delete(cmd);
	return status_data;

}

uint32_t read_MPRLS_data(){

	uint8_t query_command[3] = {0xAA, 0x00, 0x00};

	// Ask for data
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (MPRL_ADDRESS << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
	i2c_master_write(cmd, query_command, 3, ACK_CHECK_EN);
	i2c_master_stop(cmd);
	ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS));
	i2c_cmd_link_delete(cmd);

	// Wait for data
	TickType_t t= xTaskGetTickCount();
	uint8_t last_status;
	while ((last_status = read_MPRLS_status()) & 0x20) { // Device busy flag

		//printf("status: 0x%X, \r\n", last_status);
		float ellapsed_wait_time_ms = (xTaskGetTickCount() - t) / portTICK_RATE_MS;
		if (ellapsed_wait_time_ms > 20){
				//printf("TIMEOUT BUSY FLAG \r\n");
		        return 0xFFFFFFFF; // timeout
		}
	}


	// Read data
	//ESP_LOGI(TAG, "Reading data");
	const size_t N_PRESSURE_BYTES = 4;
	uint8_t pressure_raw_data[N_PRESSURE_BYTES];

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (MPRL_ADDRESS << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
	i2c_master_read(cmd, pressure_raw_data, N_PRESSURE_BYTES - 1, ACK_VAL);
	i2c_master_read_byte(cmd, pressure_raw_data + N_PRESSURE_BYTES - 1, NACK_VAL);
	i2c_master_stop(cmd);
	ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS));
	i2c_cmd_link_delete(cmd);

	//printf("READ status: 0x%X, \r\n", pressure_raw_data[0]);
	uint32_t pressure_raw = (pressure_raw_data[1] << 16) | (pressure_raw_data[2] << 8) | (pressure_raw_data[3]);
	return pressure_raw;
}

float read_MPRLS_pressure(){

	uint32_t raw_psi = read_MPRLS_data();
	float psi = (raw_psi - MPRLS_OUTPUT_min) * (MPRLS_PSI_max - MPRLS_PSI_min);
	psi /= (float)(MPRLS_OUTPUT_max - MPRLS_OUTPUT_min);
	psi += MPRLS_PSI_min;

	return psi;


}


static void read_pressure_task(void *params){

	// Initialization
		gpio_set_level(RST_PS, 0);
		vTaskDelay(10/ portTICK_RATE_MS);
		gpio_set_level(RST_PS, 1);
		vTaskDelay(10/ portTICK_RATE_MS); // Startup timing
	//init pressure queue
		init_fifo_queue( &press_queue, 3000);
		init_fifo_queue( &oscil_queue, 3000);
	// Bandpass pass
		//const float b0 = 0.1453, b2 = -0.2906, b2 = 0.2929;
		//const float a2 = 0.1716; // a0=1, a1 = 0
		const float float b0 = 0.1453, b1 =0, b2 = -0.2906,b3 = 0, b4 = 0.1453;
		const float a4= 1, a3 = -2.521, a2 = 2.3844, a1 = -1.1096, a0=0.2523;
		float xk1=0, xk2=0, xk3=0, xk4=0, yk1=0, yk2=0, yk2=0, yk3=0, yk4=0;
		float xk, yk;

		TickType_t last_wake_time = xTaskGetTickCount();
		while(1){

				//uint32_t pressure_raw = read_MPRLS_data();
				float psi = read_MPRLS_pressure();
				float mmHg = psi * PSI_to_mmHg;
				// Band pass filter
				//xk = mmHg;
				//yk = b0*xk + b1*xk1 + b2*xk2 + b3*x3 + b4*x4 + 
					(a0*yk + a1*yk1 + a2*yk2 + a3*yk3 + a4*yk4);

				//yk4 = yk3; yk3 = yk2; yk2 = yk1; yk1 = yk;
				//xk4 = xk3; xk3= xk2; xk2 = xk1; xk1 = xk;

				xk = mmHg;
				yk = b0*xk + b1*xk1 + b2*xk2 - a1*yk1 - a2*yk2;

				yk2 = yk1; yk1 = yk;
				xk2 = xk1; xk1 = xk;

				if(store == true){
					enqueue(&oscil_queue, yk);
					enqueue(&press_queue, mmHg);
					//for data aquisition
					//printf("{TIMEPLOT|data|RawPressure|T|%d}\n", (uint32_t)((yk - 740)*1000)); // DEBUG
					//printf("{TIMEPLOT|data|OscilAmplitude|T|%d}\n", (uint32_t)((yk_h*1000)));
				}
				
				vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(25));
		}


}


void pressure_control_handler(void *params){

	TickType_t last_wake_time = xTaskGetTickCount();

	// Array for bpm measurement
	const float Ts = 0.025;
	measure_stage measure_STATUS;
	const uint32_t N = 512;
	int offset = 500;
	float max_harmonic=0;
	measure_STATUS = MEASURE_IDLE;
	float sys_r = 0;
	float dias_r = 0;


	while(1){

		measure_STATUS = read_measure_status();
		switch(measure_STATUS){

		case MEASURE_IDLE:;
			// PWM Control
			ESP_LOGI(TAG, "Ready for measurement");
			//float ref_pressure = 0;
			// Valve control
			gpio_set_level(VALVE_PIN, 0);//valveopen
			break;

		case SCHEDULED:
			ESP_LOGI(TAG, "Scheduled measurement");
			ledc_fade_func_install(0);
			measure_STATUS = INFLATING;
			ESP_LOGI(TAG, "Proceeding to inflate");
			gpio_set_level(VALVE_PIN, 1);//valve closed
			//Comenzamos a guardar los datos
			store = true;
			//ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, PUMP_CHANNEL_A, (1 << (TMR_RES - 1)) -1, 3000, LEDC_FADE_WAIT_DONE);
			ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, PUMP_CHANNEL_A, 2800, 1500, LEDC_FADE_WAIT_DONE);

			first_pressure = last_queue_value(&press_queue);
			break;

		case INFLATING:;

			float current_pressure = last_queue_value(&press_queue);
			printf("Pressure measured %f \r\n", current_pressure);
			if (current_pressure > 890.){
				measure_STATUS = DEFLATING;
				//ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, PUMP_CHANNEL_A, 2000, 500, LEDC_FADE_WAIT_DONE);

			}

			break;

		case DEFLATING:;
			//semaforo = true;
			gpio_set_level(VALVE_PIN, 0);
			ESP_LOGI(TAG, "Proceeding to deflate");
			ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, PUMP_CHANNEL_A, 0, 35000, LEDC_FADE_WAIT_DONE);
			//vTaskDelay(pdMS_TO_TICKS(500));

			store = false;
			measure_STATUS = DONE;
			break;

		case DONE:
			gpio_set_level(VALVE_PIN, 0);
			ESP_LOGI(TAG, "Measurement done");
			vTaskDelay(pdMS_TO_TICKS(500));

			//BUSCAMOOS LOS MAXIMOS LOCALES CON ENVPEAKS
			int minD=30;
			init_envelope(&env, minD, 2800);
			findPeaks(&env, &oscil_queue);
			int sys= env.As_idx;
			int dias= env.Ad_idx;
			sys_r = (press_queue.arr[sys])-740;
			dias_r = (press_queue.arr[dias])-740;
			/*
			printf("sys Pressure: %f \n", (press_queue.arr[sys])-740);
			printf("dias Pressure: %f \n", (press_queue.arr[dias])-740);
			printf("first Press %f: \n", first_pressure);
			printf("Press: \n");

			 */

			//Calculamos BPM
			complex *ordered_bpm_arr = (complex *) malloc(N*sizeof(complex));
			complex *temp = (complex *) malloc(N*sizeof(complex));

			for (size_t j = 0; j < N; j++){
				ordered_bpm_arr[j].Im = 0.;
				ordered_bpm_arr[j].Re = oscil_queue.arr_norm[offset + j];

			}

			fft(ordered_bpm_arr, N, temp); // Use unordered array as scratch
			//print_magnitude_vector("bpm", ordered_bpm_arr, N);
			max_harmonic = get_highest_harmonic(ordered_bpm_arr, N, 1/Ts);
			//write_measured_bpm(60*max_harmonic, (press_queue.arr[sys])-740,(press_queue.arr[dias])-740); // send data to GUI


			free(ordered_bpm_arr);
			free(temp);


			//ESP_LOGI(TAG, "DEFLATING");
			printf("Highest harmonic is %4.4f \n", max_harmonic);
			printf("BPM: %f \n", max_harmonic*60);


			free_envelope(&env);
			free_queue(&press_queue);
			free_queue(&oscil_queue);
			measure_STATUS = WRITE_RESULTS;
			break;
		case WRITE_RESULTS:

			printf("sys Pressure: %f \n", sys_r);
			printf("dias Pressure: %f \n", dias_r);
			printf("first Press %f: \n", first_pressure);
			printf("Highest harmonic is %4.4f \n", max_harmonic);
			printf("BPM: %f \n", max_harmonic*60);
			write_measured_bpm(60*max_harmonic, sys_r,dias_r);
			ESP_LOGI(TAG, "write_result_status");



			break;
		default:
			break;

		}
		write_measure_status(measure_STATUS);
		vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(50));
	}

}


static void gui_manager_task(void *pvParameter) {

    (void) pvParameter;
    //xGuiSemaphore = xSemaphoreCreateMutex();
    TickType_t last_wake_time = xTaskGetTickCount();
    lv_init();
    lvgl_driver_init();

    // Use double buffered when not working with monochrome displays
    static lv_color_t buf1[DISP_BUF_SIZE];
    static lv_color_t buf2[DISP_BUF_SIZE];
    static lv_disp_buf_t disp_buf;
    uint32_t size_in_px = DISP_BUF_SIZE;
    lv_disp_buf_init(&disp_buf, buf1, buf2, size_in_px);

    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = disp_driver_flush;
    disp_drv.buffer = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    // Create and start a periodic timer interrupt to call lv_tick_inc
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "periodic_gui"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));

    // Create the demo application
    pp_ui();

    while (1) {
        // Delay 1 tick (assumes FreeRTOS tick is 10ms
        vTaskDelay(pdMS_TO_TICKS(10));

        // Try to take the semaphore, call lvgl related function on success
        //if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)) {
            lv_task_handler();
           // xSemaphoreGive(xGuiSemaphore);
      // }
    }
    //vTaskDelete(NULL);
    vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(100));
}

static void lv_tick_task(void *arg) {
    (void) arg;
    lv_tick_inc(LV_TICK_PERIOD_MS);
}

