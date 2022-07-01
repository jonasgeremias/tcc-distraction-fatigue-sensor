#include "webserver.h"
#include "freertos/FreeRTOS.h"
// #include "esp_err.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
// #include "main.h"
// #include "camera.h"
#include "esp_log.h"
// #include "light.h"
// #include "esp_wifi.h"
#include "esp_event.h"
// #include "test.c"

// #include "..\_camera\detect.cpp"
#define PART_BOUNDARY "123456789000000000000987654321"

static const char *LOG_HTTPS = "http_server";
static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";
// static struct {
//    uint8_t frame[100000];
//    int length;
// } fb_buffer_app;

// void copy_image(int _jpeg_buf_len, uint8_t *_jpeg_buf) {
//    xSemaphoreTake(imgMutex, portMAX_DELAY);
//    memcpy((uint8_t *) &fb_buffer_app.frame, (uint8_t *) _jpeg_buf, _jpeg_buf_len);
//    fb_buffer_app.length = _jpeg_buf_len;
//    xSemaphoreGive(imgMutex);
// }

typedef struct {
   httpd_req_t *req;
   size_t len;
} jpg_chunking_t;

static size_t jpg_encode_stream(void *arg, size_t index, const void *data, size_t len) {
   jpg_chunking_t *j = (jpg_chunking_t *) arg;
   if (!index) {
      j->len = 0;
   }
   if (httpd_resp_send_chunk(j->req, (const char *) data, len) != ESP_OK) {
      return 0;
   }
   j->len += len;
   return len;
}

// @audit-info criado por mim para envio separado do loop
esp_err_t send_frame_data(httpd_req_t *req, uint8_t *_jpeg_buf, size_t _jpeg_buf_len) {
   char *part_buf[64];
   int res = ESP_OK;

   size_t hlen = snprintf((char *) part_buf, 64, _STREAM_PART, _jpeg_buf_len);
   res = httpd_resp_send_chunk(req, (const char *) part_buf, hlen);

   if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, (const char *) _jpeg_buf, _jpeg_buf_len);
   }
   if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
   }

   return res;
}

esp_err_t stream_jpeg_handler(httpd_req_t *req) {
   camera_fb_t *fb = NULL;
   esp_err_t res = ESP_OK;
   size_t _jpeg_buf_len = 0;
   uint8_t *_jpeg_buf = NULL;
   static int64_t last_frame = 0;

   res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
   if (res != ESP_OK) {
      return res;
   }

   if (!last_frame) {
      last_frame = esp_timer_get_time();
   }

   while (true) {
      /******************************************************************
       * 1 - Leitura da imagem
       *****************************************************************/
      fb = esp_camera_fb_get();
      if (!fb) {
         ESP_LOGE(LOG_HTTPS, "camera capture failed");
         res = ESP_FAIL;
      }
      else {
         if (fb->format != PIXFORMAT_JPEG) {
            bool jpeg_converted = frame2jpg(fb, 30, &_jpeg_buf, &_jpeg_buf_len);
            if (!jpeg_converted) {
               ESP_LOGE(LOG_HTTPS, "jpeg compression failed");
               esp_camera_fb_return(fb);
               res = ESP_FAIL;
            }
         }
         else {
            _jpeg_buf_len = fb->len;
            _jpeg_buf = fb->buf;
         }
      }

      /******************************************************************
       * 2 - Detecção
       *****************************************************************/
      // if (res == ESP_OK) res = frame_detect(fb, _jpeg_buf, _jpeg_buf_len);

      /******************************************************************
       * 3 - Enviando imagem para a WEB
       *****************************************************************/
      // if (res == ESP_OK)
      res = send_frame_data(req, _jpeg_buf, _jpeg_buf_len);

      /******************************************************************
       * 4 - Liberando os recursos
       *****************************************************************/
      esp_camera_fb_return(fb);

      if (fb->format != PIXFORMAT_JPEG) {
         free(_jpeg_buf);
      }

      if (res != ESP_OK) {
         break;
      }

      /******************************************************************
       * 5 - imprimiendo logs
       *****************************************************************/
      int64_t fr_end = esp_timer_get_time();
      int64_t frame_time = fr_end - last_frame;
      last_frame = fr_end;
      frame_time /= 1000;
      ESP_LOGI(LOG_HTTPS, "mjpeg: %uKB %ums (%.1ffps)", (uint32_t) (_jpeg_buf_len / 1024), (uint32_t) frame_time, 1000.0 / (uint32_t) frame_time);
   }

   last_frame = 0;
   return res;
}

esp_err_t capture_bmp_handler(httpd_req_t *req) {
   camera_fb_t *fb = NULL;
   esp_err_t res = ESP_OK;
   int64_t fr_start = esp_timer_get_time();

   fb = esp_camera_fb_get();
   if (!fb) {
      ESP_LOGE(LOG_HTTPS, "Camera capture failed");
      httpd_resp_send_500(req);
      return ESP_FAIL;
   }

   uint8_t *buf = NULL;
   size_t buf_len = 0;
   bool converted = frame2bmp(fb, &buf, &buf_len);
   esp_camera_fb_return(fb);
   if (!converted) {
      ESP_LOGE(LOG_HTTPS, "BMP conversion failed");
      httpd_resp_send_500(req);
      return ESP_FAIL;
   }

   res = httpd_resp_set_type(req, "image/x-windows-bmp") ||
         httpd_resp_set_hdr(req, "Content-Disposition",
                            "inline; filename=capture.bmp") ||
         httpd_resp_send(req, (const char *) buf, buf_len);
   free(buf);
   int64_t fr_end = esp_timer_get_time();
   ESP_LOGI(LOG_HTTPS, "BMP: %uKB %ums",
            (uint32_t) (buf_len / 1024), (uint32_t) ((fr_end - fr_start) / 1000));
   return res;
}

esp_err_t capture_jpeg_handler(httpd_req_t *req) {
   camera_fb_t *fb = NULL;
   esp_err_t res = ESP_OK;
   size_t fb_len = 0;
   int64_t fr_start = esp_timer_get_time();

   fb = esp_camera_fb_get();
   if (!fb) {
      ESP_LOGE(LOG_HTTPS, "Camera capture failed");
      httpd_resp_send_500(req);
      return ESP_FAIL;
   }
   res = httpd_resp_set_type(req, "image/jpeg");
   if (res == ESP_OK) {
      res = httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
   }

   if (res == ESP_OK) {
      if (fb->format == PIXFORMAT_JPEG) {
         fb_len = fb->len;
         res = httpd_resp_send(req, (const char *) fb->buf, fb->len);
      }
      else {
         jpg_chunking_t jchunk = {req, 0};
         res = frame2jpg_cb(fb, 80, jpg_encode_stream, &jchunk) ? ESP_OK : ESP_FAIL;
         httpd_resp_send_chunk(req, NULL, 0);
         fb_len = jchunk.len;
      }
   }
   esp_camera_fb_return(fb);
   int64_t fr_end = esp_timer_get_time();
   ESP_LOGI(LOG_HTTPS, "JPG: %uKB %ums", (uint32_t) (fb_len / 1024), (uint32_t) ((fr_end - fr_start) / 1000));
   return res;
}

esp_err_t settings_handler(httpd_req_t *req) {
   esp_err_t res = ESP_OK;
   size_t buf_len = httpd_req_get_url_query_len(req) + 1;
   if (buf_len > 1) {
      char *buf = (char *) malloc(buf_len);

      if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
         ESP_LOGI(LOG_HTTPS, "found url query => %s", buf);
         char param[32];
         char *end;
         sensor_t *sensor = esp_camera_sensor_get();
         // TODO: error handling/strtol
         if (httpd_query_key_value(buf, "framesize", param, sizeof(param)) == ESP_OK) {
            ESP_LOGI(LOG_HTTPS, "found url query parameter => framesize=%s", param);
            if (strcmp(param, "QQVGA") == 0) {
               sensor->set_framesize(sensor, FRAMESIZE_QQVGA);
            }
            else if (strcmp(param, "QVGA") == 0) {
               sensor->set_framesize(sensor, FRAMESIZE_QVGA);
            }
            else if (strcmp(param, "SVGA") == 0) {
               sensor->set_framesize(sensor, FRAMESIZE_SVGA);
            }
            else if (strcmp(param, "UXGA") == 0) {
               sensor->set_framesize(sensor, FRAMESIZE_UXGA);
            }
         }
         if (httpd_query_key_value(buf, "quality", param, sizeof(param)) == ESP_OK) {
            ESP_LOGI(LOG_HTTPS, "found url query parameter => quality=%s", param);
            int quality = strtol(param, &end, 10);
            sensor->set_quality(sensor, quality);
         }
         if (httpd_query_key_value(buf, "contrast", param, sizeof(param)) == ESP_OK) {
            ESP_LOGI(LOG_HTTPS, "found url query parameter => contrast=%s", param);
            int contrast = strtol(param, &end, 10);
            sensor->set_contrast(sensor, contrast);
         }
         if (httpd_query_key_value(buf, "brightness", param, sizeof(param)) == ESP_OK) {
            ESP_LOGI(LOG_HTTPS, "found url query parameter => brightness=%s", param);
            int brightness = strtol(param, &end, 10);
            sensor->set_brightness(sensor, brightness);
         }
         if (httpd_query_key_value(buf, "saturation", param, sizeof(param)) == ESP_OK) {
            ESP_LOGI(LOG_HTTPS, "found url query parameter => saturation=%s", param);
            int saturation = strtol(param, &end, 10);
            sensor->set_saturation(sensor, saturation);
         }
         if (httpd_query_key_value(buf, "automatic-exposure-level", param, sizeof(param)) == ESP_OK) {
            ESP_LOGI(LOG_HTTPS, "found url query parameter => automatic-exposure-level=%s", param);
            int automatic_exposure_level = strtol(param, &end, 10);
            sensor->set_ae_level(sensor, automatic_exposure_level);
         }
         if (httpd_query_key_value(buf, "automatic-gain-celing", param, sizeof(param)) == ESP_OK) {
            ESP_LOGI(LOG_HTTPS, "found url query parameter => automatic-gain-ceiling=%s", param);
            int automatic_gain_ceiling = strtol(param, &end, 10);
            sensor->set_agc_gain(sensor, automatic_gain_ceiling);
         }
      }
   }
   httpd_resp_send_chunk(req, NULL, 0);
   return res;
}

esp_err_t toggle_light_handler(httpd_req_t *req) {
   esp_err_t res = ESP_OK;
   // toggle_light(); @pending add se necessário
   httpd_resp_send_chunk(req, NULL, 0);
   return res;
}

const httpd_uri_t stream_jpeg_uri = {
   .uri = "/stream/jpeg",
   .method = HTTP_GET,
   .handler = stream_jpeg_handler,
   .user_ctx = NULL};

const httpd_uri_t capture_bmp_uri = {
   .uri = "/capture/bmp",
   .method = HTTP_GET,
   .handler = capture_bmp_handler,
   .user_ctx = NULL};

const httpd_uri_t capture_jpeg_uri = {
   .uri = "/capture/jpeg",
   .method = HTTP_GET,
   .handler = capture_jpeg_handler,
   .user_ctx = NULL};

const httpd_uri_t settings_uri = {
   .uri = "/settings",
   .method = HTTP_GET,
   .handler = settings_handler,
   .user_ctx = NULL};

const httpd_uri_t toggle_light_uri = {
   .uri = "/toggle_light",
   .method = HTTP_GET,
   .handler = toggle_light_handler,
   .user_ctx = NULL};

// void connect_event_handler(void *arg, esp_event_base_t event_base,
//                            int32_t event_id, void *event_data) {
//    httpd_handle_t *server = (httpd_handle_t *) arg;
//    if (*server == NULL) {
//       ESP_LOGI(LOG_HTTPS, "Starting webserver");
//       *server = start_webserver();
//    }
// }

// void disconnect_event_handler(void *arg, esp_event_base_t event_base,
//                               int32_t event_id, void *event_data) {
//    httpd_handle_t *server = (httpd_handle_t *) arg;
//    if (*server) {
//       ESP_LOGI(LOG_HTTPS, "Stopping webserver");
//       stop_webserver((httpd_handle_t) *server);
//       *server = NULL;
//    }
// }

httpd_handle_t start_webserver() {
   led_bicolor_configura_pinos();
   init_task_led_status_controller();

   httpd_handle_t server = NULL;
   httpd_config_t config = HTTPD_DEFAULT_CONFIG();

   ESP_LOGI("HEAP", LOG_BOLD(LOG_COLOR_BLUE) "%lld;\t%d", esp_timer_get_time(), esp_get_free_heap_size());

   config.stack_size = (10000);
   config.core_id = 0;
   config.uri_match_fn = httpd_uri_match_wildcard;
   config.max_resp_headers = 50;
   config.task_priority = 6;
   config.server_port = 80;

   ESP_LOGI(LOG_HTTPS, "starting server on port %d", config.server_port);
   if (httpd_start(&server, &config) == ESP_OK) {
      // ESP_ERROR_CHECK(esp_event_handler_register(
      //     IP_EVENT, IP_EVENT_STA_GOT_IP,
      //     &connect_event_handler, &server));
      // ESP_ERROR_CHECK(esp_event_handler_register(
      //     WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,
      //     &disconnect_event_handler, &server));
      // ESP_LOGI(LOG_HTTPS, "registering uri handlers");
      httpd_register_uri_handler(server, &capture_bmp_uri);
      httpd_register_uri_handler(server, &capture_jpeg_uri);
      httpd_register_uri_handler(server, &settings_uri);
      httpd_register_uri_handler(server, &toggle_light_uri);
      httpd_register_uri_handler(server, &stream_jpeg_uri);
      return server;
   } else {
      ESP_LOGI(LOG_HTTPS, "error starting server!");
   }

   
   return NULL;
}

void stop_webserver(httpd_handle_t server) {
   httpd_stop(server);
}
