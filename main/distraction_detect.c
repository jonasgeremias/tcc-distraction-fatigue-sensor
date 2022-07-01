
/******************************************************************************
 * Importações de bibliotecas
 *****************************************************************************/
#include <stdio.h>
#include "human_face_detect_msr01.hpp"
#include "human_face_detect_mnp01.hpp"
#include "dl_tool.hpp"
#include "dl_image.hpp"

// Tag de log.
const char *LOG_TEST = "Detect";

/******************************************************************************
 * Instancia o modelo do Tensor Flow
 *****************************************************************************/
dl::tool::Latency latency;
HumanFaceDetectMSR01 s1(0.1F, 0.5F, 10, 0.2F);
HumanFaceDetectMNP01 s2(0.5F, 0.3F, 5);

typedef struct pointer_t {
   uint32_t eye_l[2];
   uint32_t eye_r[2];
   uint32_t nose[2];
   uint32_t mouth_l[2];
   uint32_t mouth_r[2];
   uint32_t box[4];
   float score;
} pointer_t;

/******************************************************************************
 * Variáveis de controle da imagem
 *****************************************************************************/
camera_fb_t *image = NULL;
esp_err_t err = ESP_OK;
size_t _jpeg_buf_len = 0;
uint8_t *_jpeg_buf = NULL;

/******************************************************************************
 * Variáveis do cartão de memória
 *****************************************************************************/
static volatile bool save_image = 0;
char filename[16];
char sdfile_filename[64];
char text[100];

/******************************************************************************
 * Definições de controle da aplicação
 *****************************************************************************/
#define TEMPO_ALERTA_MS    1000
#define TEMPO_DISTRACAO_MS 3000
#define TEMPO_ERRO_MS      10000
#define TEMPO_ERRO_MAX_MS  600000

/******************************************************************************
 * Variáveis de controle da aplicação
 *****************************************************************************/
pointer_t data_predict;
volatile bool detect_anterior = 1;
volatile int rosto_ok = 0;
volatile int rosto_anterior = 0;
volatile int8_t qty_faces_detected = 0;
volatile int8_t camera_error_count = 0;
volatile bool camera_iniciou = 0;
volatile bool release_erro = 0;
IRAM_ATTR int distraction_timeout = TEMPO_DISTRACAO_MS * 2;
IRAM_ATTR int erro_timeout_ms = TEMPO_ERRO_MAX_MS; // Espera uma detecção ok
IRAM_ATTR int alert_timeout_ms = 0;

/******************************************************************************
 * Função de controle de inicio da câmera
 *****************************************************************************/
void camera_init_test() {
   if (!camera_iniciou) {
      do {
         if (init_camera() != ESP_OK) camera_error_count++;
         else {
            camera_error_count = 0;
            vTaskDelay(50 / portTICK_RATE_MS);
            image = esp_camera_fb_get();
            esp_camera_fb_return(image);
            camera_iniciou = 1;
            break;
         }
      } while (camera_error_count < 5);
   }
}

void salvar_data_in_sd() {
   bool jpeg_converted = frame2jpg(image, 80, &_jpeg_buf, &_jpeg_buf_len);
   if (image != NULL) esp_camera_fb_return(image);
   if (jpeg_converted) {
      sprintf((char *) filename, "i%05lld", esp_timer_get_time()); // Nome do arquivo

      // Salvando a imagem
      sprintf((char *) sdfile_filename, MOUNT_POINT "/%s.jpg", filename);
      sdcard_save_file_with_name((uint8_t *) _jpeg_buf, (int) _jpeg_buf_len, (char *) &sdfile_filename);

      // Salvando o arquivo com as informações
      sprintf((char *) sdfile_filename, MOUNT_POINT "/%s.txt", filename);
      char *p = (char *) &text;
      char *p_ini = p;

      // score, box, eye_l, eye_r, nose, mouth_l, mouth_r
      p += sprintf((char*) &text, "{\"score\":%.5f, \"box\": [%d, %d, %d, %d],", data_predict.score, data_predict.box[0], data_predict.box[1], data_predict.box[2], data_predict.box[3]);
      p += sprintf(p, "\"pt\": [%d, %d, %d, %d, %d, %d, %d, %d, %d, %d]}",
                   data_predict.eye_l[0], data_predict.eye_l[1],
                   data_predict.eye_r[0], data_predict.eye_r[1],
                   data_predict.nose[0], data_predict.nose[1],
                   data_predict.mouth_l[0], data_predict.mouth_l[1],
                   data_predict.mouth_r[0], data_predict.mouth_r[1]);
      sdcard_save_file_with_name((uint8_t *) &text, p - p_ini, (char *) &sdfile_filename);
      ESP_LOGW(LOG_TEST, "%s", text);
   }
   else {
      ESP_LOGE(LOG_TEST, "jpeg_converted error");
   }

   if (_jpeg_buf != NULL) {
      free(_jpeg_buf);
   }
}

/******************************************************************************
 * @audit Esta não foi utilizada 
 *****************************************************************************/
int distance_between(int p1_x, int p1_y, int p2_x, int p2_y) {
   double dx = p2_x - p1_x;
   double dy = p2_y - p1_y;
   return sqrt((dx * dx) + (dy * dy));
}

/******************************************************************************
 * @@ Analise de angulo entre dois pontos
 *****************************************************************************/
float angle_between(int p1_x, int p1_y, int p2_x, int p2_y) {
   double dx = p2_x - p1_x;
   double dy = p2_y - p1_y;

   double angle = 0;
   if (p2_x != p1_x) {
      angle = ((double) atan2(dx, dy) * 180) / 3.14159265;
      if (angle < 0) angle *= -1;
   }
   else
      angle = 90.0;

   return angle;
}

/******************************************************************************
 * @@ Analise de angulo para identificar distração
 *****************************************************************************/
int distraction_detect(double angle_nose_mouth_l, double angle_nose_mouth_r, double angle_eye_l_mouth_l) {
   if ((angle_nose_mouth_l < 25) || (angle_nose_mouth_r < 25)) return false;
   else if ((angle_nose_mouth_l < 30) && (angle_nose_mouth_r < 30))
      return false;
   else if ((angle_eye_l_mouth_l > 10) && (angle_eye_l_mouth_l < 80))
      return false;
   else
      return true;
}

/******************************************************************************
 * Loop de detecção
 *****************************************************************************/
void loop_detect() {
   rosto_ok = 0;
   qty_faces_detected = 0;
   save_image = 0;

   /***************************************************************************
    * @@ => 1 - Lendo a imagem da câmera
    **************************************************************************/
   camera_init_test();
   image = esp_camera_fb_get();
   if (image != NULL) {
      camera_error_count = 0;

      /************************************************************************
       * @@ => 2 - Rodando o modelo de machine learning
       ***********************************************************************/
      latency.start(); // inference
      std::list<dl::detect::result_t> &candidates = s1.infer((uint16_t *) image->buf, {(int) image->height, (int) image->width, 3});
      std::list<dl::detect::result_t> &dt_results = s2.infer((uint16_t *) image->buf, {(uint16_t) image->height, (uint16_t) image->width, 3}, candidates);
      latency.end();

      for (std::list<dl::detect::result_t>::iterator prediction = dt_results.begin(); prediction != dt_results.end(); prediction++) {
         data_predict.box[0] = prediction->box[0];
         data_predict.box[1] = prediction->box[1];
         data_predict.box[2] = prediction->box[2];
         data_predict.box[3] = prediction->box[3];
         data_predict.score = prediction->score;
         data_predict.eye_l[0] = prediction->keypoint[0];
         data_predict.eye_l[1] = prediction->keypoint[1];
         data_predict.eye_r[0] = prediction->keypoint[6];
         data_predict.eye_r[1] = prediction->keypoint[7];
         data_predict.nose[0] = prediction->keypoint[4];
         data_predict.nose[1] = prediction->keypoint[5];
         data_predict.mouth_l[0] = prediction->keypoint[2];
         data_predict.mouth_l[1] = prediction->keypoint[3];
         data_predict.mouth_r[0] = prediction->keypoint[8];
         data_predict.mouth_r[1] = prediction->keypoint[9];

         /*********************************************************************
          * @@ => 3 - Algoritmo de detecção por inclinação do rosto.
          ********************************************************************/
         double angle_nose_mouth_l = angle_between(data_predict.nose[0], data_predict.nose[1], data_predict.mouth_l[0], data_predict.mouth_l[1]);
         double angle_nose_mouth_r = angle_between(data_predict.nose[0], data_predict.nose[1], data_predict.mouth_r[0], data_predict.mouth_r[1]);
         double angle_eye_l_mouth_l = angle_between(data_predict.eye_l[0], data_predict.eye_l[1], data_predict.mouth_l[0], data_predict.mouth_l[1]);
         rosto_ok = distraction_detect(angle_nose_mouth_l, angle_nose_mouth_r, angle_eye_l_mouth_l);
         qty_faces_detected = 1;
         break;
      }

      if (qty_faces_detected) ESP_LOGI(LOG_TEST, "Detectou!");
      else
         ESP_LOGW(LOG_TEST, "Não Detectou!");
   }
   else {
      ESP_LOGE(LOG_TEST, "Camera capture failed");
      rosto_ok = 0;
      if (++camera_error_count > 10) {
         camera_iniciou = 0;
         camera_error_count = 0;
      }
   }

   // Limpa o tempo se detectar um rosto ok.
   if (rosto_ok) {
      distraction_timeout = TEMPO_DISTRACAO_MS;
      release_erro = 0;
   }

   /***************************************************************************
    * @@ => 4 - Controle de alerta da alicação
    **************************************************************************/
   // Se passou os tempo configurado, verifica se nâo está em erro.
   if (!distraction_timeout) {
      alert_timeout_ms = TEMPO_ALERTA_MS;
      if (!release_erro) {
         erro_timeout_ms = 0;
         release_erro = 1;
         save_image = 1;
      }
   }
   if ((alert_timeout_ms) && (erro_timeout_ms < TEMPO_ERRO_MS)) {
      modo_led_status((modo_led_t) LED_STATUS_PISCA_RAPIDO);
      ESP_LOGE(LOG_TEST, "Distracao! >D:%d A:%d e:%d", distraction_timeout, alert_timeout_ms, erro_timeout_ms);
   }
   else {
      modo_led_status((modo_led_t) LED_STATUS_LIGADO);
      ESP_LOGI(LOG_TEST, "Normal! >D:%d A:%d  e:%d", distraction_timeout, alert_timeout_ms, erro_timeout_ms);
   }

   /***************************************************************************
    * @@ => 5 - Rotina de salvamento da imagem
    **************************************************************************/
   if (save_image == 1) {
      save_image = 0;
      salvar_data_in_sd();
   }

   if (image != NULL) esp_camera_fb_return(image);
}