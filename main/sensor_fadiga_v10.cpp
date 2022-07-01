#include <stdio.h>
#include <stdlib.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"

/******************************************************************************
 * Timer
 ******************************************************************************/
void app_timer_isr_void();
#define TIMER_ISR_VOID app_timer_isr_void()
#include "_TIMER/_TIMER_V1.0.c"

// Protótipos das funções.
#include "_camera/cam.c"

static TaskHandle_t taskhandle_app_controller;
esp_err_t sdcard_save_image(camera_fb_t *img);
esp_err_t sdcard_save_image_with_name(camera_fb_t *img, char *filename);
esp_err_t sdcard_save_file_with_name(uint8_t *data, int length, char *filename);

#define LED_STATUS (gpio_num_t) 33
#include "_LED\LED_STATUS.c"
#include "_wifi\wifi.c"
#include "_AP_Server\webserver.c"
#include "distraction_detect.c"

esp_err_t sdcard_save_image(camera_fb_t *img) {
   static uint64_t counter = 0;
   char filename[64];
   esp_err_t ret = ESP_OK;

   counter++;
   sprintf(filename, MOUNT_POINT "/image_%lld.jpg", counter); //, img->timestamp.tv_sec, img->timestamp.tv_usec * 1000);
   ESP_LOGW(TAG, "Saving: %s. ", filename);

   FILE *file = fopen(filename, "w");
   if (file != NULL) {
      fwrite(img->buf, 1, img->len, file);
      ESP_LOGI(TAG, "File saved: %s", filename);
      ret = ESP_OK;
   }
   else {
      ESP_LOGE(TAG, "Could not open file =(");
      ret = ESP_FAIL;
   }
   fclose(file);

   return ret;
}

esp_err_t sdcard_save_file_with_name(uint8_t *data, int length, char *filename) {
   esp_err_t ret = ESP_OK;

   FILE *file = fopen(filename, "w");
   if (file != NULL) {
      fwrite(data, 1, length, file);
      ret = ESP_OK;
   }
   else {
      ret = ESP_FAIL;
   }
   fclose(file);

   return ret;
}

esp_err_t sdcard_save_image_with_name(camera_fb_t *img, char *filename) {
   esp_err_t ret = ESP_OK;

   FILE *file = fopen(filename, "w");
   if (file != NULL) {
      fwrite(img->buf, 1, img->len, file);
      ret = ESP_OK;
   }
   else {
      ret = ESP_FAIL;
   }
   fclose(file);

   return ret;
}

void app_timer_isr_void() {
   if (distraction_timeout) distraction_timeout--;
   if (alert_timeout_ms) alert_timeout_ms--;
   if (erro_timeout_ms < TEMPO_ERRO_MAX_MS) erro_timeout_ms++;
}

void app_controller(void *pv) {
   init_task_led_status_controller();
   int error_count = 0;
   camera_fb_t *image = NULL;

   error_count = 0;

   do {
      if (init_sdcard() != ESP_OK) error_count++;
      else {
         break;
      }
   } while (error_count < 5);

   while (1) {
      loop_detect();
   }

   vTaskDelete(NULL);
}

extern "C" void app_main(void) {
   nvs_flash_init();
   esp_log_level_set(TAG, ESP_LOG_VERBOSE);
   init_timer(1000); // Inicia o timer
   init_wifi_softap(); // Conecta no wifi
   start_webserver(); // inicia serviço web
   xTaskCreatePinnedToCore(app_controller, "app_controller", 15000, NULL, 1, &taskhandle_app_controller, 1);
}