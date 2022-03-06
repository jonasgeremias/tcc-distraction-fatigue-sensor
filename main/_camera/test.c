
#include "detect.cpp"

pixformat_t array_pixel_format[] = {
   PIXFORMAT_RGB565, // 2BPP/RGB565
   // PIXFORMAT_YUV422, // 2BPP/YUV422
   // PIXFORMAT_GRAYSCALE, // 1BPP/GRAYSCALE
   // PIXFORMAT_JPEG, // JPEG/COMPRESSED
   // PIXFORMAT_RGB888, // 3BPP/RGB888
   // PIXFORMAT_RAW, // RAW
   // PIXFORMAT_RGB444, // 3BP2P/RGB444
   // PIXFORMAT_RGB555 // 3BP2P/RGB555
};

framesize_t array_frame_size[] = {
   // FRAMESIZE_96X96, // 96x96
   // FRAMESIZE_QQVGA, // 160x120
   // FRAMESIZE_QCIF,  // 176x144
   // FRAMESIZE_HQVGA, // 240x176
   // FRAMESIZE_240X240, // 240x240
   // FRAMESIZE_QVGA, // 320x240
   FRAMESIZE_CIF, // 400x296
   // FRAMESIZE_HVGA, // 480x320
   FRAMESIZE_VGA, // 640x480
   FRAMESIZE_SVGA, // 800x600
   FRAMESIZE_XGA, // 1024x768
   // FRAMESIZE_HD, // 1280x720
   // FRAMESIZE_SXGA, // 1280x1024
   // FRAMESIZE_UXGA // 1600x1200
};

const char *LOG_TEST = "test_images";

void test_images() {
   camera_config_t cam_cfg;
   memcpy(&cam_cfg, &camera_config, sizeof(camera_config_t));

   camera_fb_t *image = NULL;
   esp_err_t err = ESP_OK;
   char filename[64];
   char log[500];
   char *p = NULL;

   size_t _jpeg_buf_len = 0;
   uint8_t *_jpeg_buf = NULL;

   esp_log_level_set(LOG_TEST, ESP_LOG_VERBOSE);

   ESP_LOGE(LOG_TEST, "------------------------- Inicio do teste ---------------------------");
   esp_camera_deinit();

   vTaskDelay(1000 / portTICK_RATE_MS);
   for (int pixel_ = 0; pixel_ < ARRAY_SIZE_OF(array_pixel_format); pixel_++) { // Loop array_pixel_format
      for (int frame_size = 0; frame_size < ARRAY_SIZE_OF(array_frame_size); frame_size++) { // Loop array_frame_size
         for (int count = 0; count < 5; count++) { // 10 vezes cada configuração.

            /***********************************************************************
             * 0 - Criando o nome do arquivo
             ***********************************************************************/
            sprintf(filename, MOUNT_POINT "/p%d_%d_%d.jpg", pixel_, frame_size, count);
            // ESP_LOGI(LOG_TEST, "Testando->'%s'", filename);

            /***********************************************************************
             * 1 - Configurar camera e inicar
             ***********************************************************************/
            uint64_t time_1 = esp_timer_get_time();
            cam_cfg.pixel_format = array_pixel_format[pixel_];
            cam_cfg.frame_size = array_frame_size[frame_size];
            err = esp_camera_init(&cam_cfg);
            // sensor_t * s = esp_camera_sensor_get();
            // s->set_special_effect(s, 2); // grayscale

            if (err != ESP_OK) {
               ESP_LOGE(LOG_TEST, "erro [%d] esp_camera_init : pixel_format: %d, frame_size: %d.", err, pixel_, frame_size);
               continue;
            }

            /***********************************************************************
             * 2 - Tirar foto
             ***********************************************************************/
            uint64_t time_2 = esp_timer_get_time();
            image = esp_camera_fb_get();

            /***********************************************************************
             * 3 - Converter em JPEG - @pending ver necessidade de converter
             ***********************************************************************/
            uint64_t time_3 = esp_timer_get_time();
            if (cam_cfg.pixel_format == PIXFORMAT_RGB565) {
               bool jpeg_converted = frame2jpg(image, 80, &_jpeg_buf, &_jpeg_buf_len);
               if (!jpeg_converted) {
                  ESP_LOGE(LOG_TEST, "jpeg_converted erro");
                  // esp_camera_fb_return(fb);
               }
            }

            /***********************************************************************
             * 4 - Detect
             **********************************************************************/
            uint64_t time_4 = esp_timer_get_time();
            int res = frame_detect(image, _jpeg_buf, _jpeg_buf_len);

            /***********************************************************************
             * 5 - Salvar no cartao foto
             ***********************************************************************/
            uint64_t time_5 = esp_timer_get_time();
            // err = sdcard_save_image_with_name(image, (char *) &filename);
            err = sdcard_save_file_with_name((uint8_t *) _jpeg_buf, (int) _jpeg_buf_len, (char *) &filename);

            /***********************************************************************
             * 6 - Liberar recursos
             ***********************************************************************/
            uint64_t time_6 = esp_timer_get_time();
            esp_camera_fb_return(image);
            esp_camera_deinit();

            /***********************************************************************
             * 7 - LOG
             ***********************************************************************/
            uint64_t time_7 = esp_timer_get_time(); // Fim

            p = (char *) &log;
            p += sprintf(p, "{\"%s\":{", filename);
            p += sprintf(p, "\"frameSize\":%d,", array_frame_size[frame_size]);
            p += sprintf(p, "\"configCam\":%lld,", time_2 - time_1);
            p += sprintf(p, "\"getImage\":%lld,", time_3 - time_2);
            p += sprintf(p, "\"frame2jpg\":%lld,", time_4 - time_3);
            p += sprintf(p, "\"Detect\":%lld,", time_5 - time_4);
            p += sprintf(p, "\"saveFile\":%lld,\"saveStatus\":%d,", time_6 - time_5, err);
            p += sprintf(p, "\"release\":%lld,", time_7 - time_6);
            p += sprintf(p, "\"end\":%lld,", esp_timer_get_time() - time_7);
            p += sprintf(p, "\"totalTime\":%lld}}", time_7 - time_1);
            printf("%s\n", log);
            vTaskDelay(50 / portTICK_RATE_MS);
         }
      }
   }

   ESP_LOGE(LOG_TEST, "------------------------- Fim do teste ---------------------------");
}