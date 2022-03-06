#include "nvs_flash.h"
#include "nvs.h"
#define memoria "nvs"
#define report  "report"

#ifdef DEBUG_NVS
const char *LOG_NVS = "NVS";
#endif

// Definição la no main.h
#ifndef QUANTIDADE_MAX_EVENTOS
   #define QUANTIDADE_MAX_EVENTOS 600 
#endif

// @audit corrigir - Configuração de eventos
const char *ev_header = "[\"Posição\",\"Dia\",\"Mês\",\"Ano\",\"Hora\",\"Min\",\"Seg\",\"UTC\",\"Tipo\",\"Valor\",\"Long.\",\"Lat.\",\"Vel.\",\"GPS Sats.\",\"CSQ3G\",\"Links\"]";
eventos_config_t eventos_config;

typedef uint32_t nvs_handle_t;
static SemaphoreHandle_t mutex_nvs;

// Protótipos NVS -------------------------------------------------------------
// static esp_err_t nvs_app_read_overflow();
static esp_err_t nvs_app_init_config_events();
static esp_err_t app_nvs_init();
static esp_err_t nvs_app_read_config_wifi();
static esp_err_t nvs_app_write_config_module();
static esp_err_t nvs_app_write_config_http_client();
static esp_err_t nvs_app_write_config_wifi();
static esp_err_t carrega_configuracao_default(uint32_t versao);
static esp_err_t nvs_app_write_config_http_client();
static esp_err_t nvs_app_read_event(event_t *ev, int end);
static esp_err_t nvs_app_write_event(event_t *ev);
static esp_err_t nvs_app_clear_report();
static esp_err_t nvs_app_read_config_events(eventos_config_t *ev_config);
static esp_err_t nvs_app_write_config_block_device();
static esp_err_t nvs_app_read_module();