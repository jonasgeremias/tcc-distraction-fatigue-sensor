/*
@pending ver Queue no frame:
https://github.com/espressif/esp-who/blob/master/components/modules/camera/who_camera.c

*/


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
#include "_libs/utils.c"
#include "_camera/cam.c"

// Protótipos das funções.
esp_err_t sdcard_save_image(camera_fb_t *img);
esp_err_t sdcard_save_image_with_name(camera_fb_t *img, char *filename);
esp_err_t sdcard_save_file_with_name(uint8_t *data, int length, char *filename);

#include "_camera/test.c"

static TaskHandle_t taskhandle_app_controller;

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
   printf("file: %s\n", filename);

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

static void app_controller(void *pv) {
   // int err = ESP_OK;
   int error_count = 0;
   camera_fb_t *image = NULL;

   do {
      if (init_camera() != ESP_OK) error_count++;
      else {
         image = esp_camera_fb_get();
         esp_camera_fb_return(image);
         break;
      }

   } while (error_count < 5);

   error_count = 0;

   do {
      if (init_sdcard() != ESP_OK) error_count++;
      else
         break;
   } while (error_count < 5);

   esp_log_level_set(TAG, ESP_LOG_VERBOSE);

   while (1) {
      ESP_LOGI(TAG, "Taking picture...");
      image = esp_camera_fb_get();
      sdcard_save_image(image);
      esp_camera_fb_return(image);
      test_images();

      vTaskDelay(10000 / portTICK_RATE_MS);
   }

   vTaskDelete(NULL);
}

extern "C" void app_main(void) {
   // @audit Desligando todos os logs
   esp_log_level_set("*", ESP_LOG_NONE);
   xTaskCreatePinnedToCore(app_controller, "app_controller", 32000, NULL, 1, &taskhandle_app_controller, 1);
}
