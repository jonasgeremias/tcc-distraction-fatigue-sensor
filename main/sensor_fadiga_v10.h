// Configurações da aplicação alteradas via OTA -------------------------------

// Controle de logs das bibliotecas criadas -----------------------------------

// Habilitar LOG geral para debug.
// #define DEBUG

// #define DEBUG_HEAP
// #define DEBUG_MAIN
// #define DEBUG_TASK_MONITOR
// #define DEBUG_PIC
// #define DEBUG_AP_SERVER
// #define DEBUG_WIFI
// #define DEBUG_HTTP
// #define DEBUG_HTTP_TRANSPORT
// #define DEBUG_NVS
// #define DEBUG_GPS
// #define DEBUG_EVENTO
// #define DEBUG_OTA
// #define DEBUG_SYS_TIME

#ifdef DEBUG
   #define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#endif

#include "esp_log.h"

// Textos dos Logs ------------------------------------------------------------
#ifdef DEBUG_EVENTO
const char *LOG_EVENTO = "Evento";
#endif

#ifdef DEBUG_MAIN
const char *LOG_MAIN = "Main";
#endif

// Proteção do heap baixo
#define PROTECAO_RESET_HEAP          1
#define PROTECAO_RESET_HEAP_VALOR    30000
#define PROTECAO_RESET_HEAP_TEMPO_MS 15000

#define QTD_LOGIN_WIFI                6
#define TIPO_PRODUTO                  1 // Indica que é Display Inclinômetro G4.1
#define NIVEL_BLOQ_DISPOSITIVO        4
#define QUANTIDADE_MAX_EVENTOS        1000
#define SNTP_UPDATE_DELAY             3600000
#define TEMPO_APP_SYS_TIME_LOOP_MS    10000
#define CONFIG_ADD_TIME_BLOCK_RELEASE 60

// Funçoes MAKE
#define MAKE8(a, i)        ((uint8_t)(a >> (8 * (3 - i)) & 0xff))
#define MAKE32(a, b, c, d) (((uint32_t)((a) &0xff) << 24) | ((uint32_t)((b) &0xff) << 16) | ((uint32_t)((c) &0xff) << 8) | (uint32_t)((d) &0xff))
#define BIT_TEST(a, p)     ((a & ((int) 1 << p)) >> p)
#define BIT_SET(a, b)      (a |= 1 << b)
#define BIT_CLEAR(a, b)    (a &= ~(1 << b))

// Controle e monitoramento das tarefas
static TaskHandle_t taskhandle_app_controller;
static EventGroupHandle_t app_event_group;
#define CONECTADO_NO_WIFI            BIT0
#define REQUISITANDO_NA_SERIAL       BIT1
#define REQUISICAO_SERIAL_FINALIZADA BIT2
#define ENVIANDO_HTTP                BIT3
#define ENVIO_HTTP_FINALIZADO        BIT4
#define ENVIAR_HTTP                  BIT5
#define LINK_COM_PIC                 BIT6

// Tratamento do json recebido pelo servidor
enum {
   JSON_ERROR = 0,
   RECEBEU_CONFIG_HOST,
   NAO_RECEBEU_RESULT,
   RECEBEU_RESULT
};

typedef struct event_pic_t {
   uint8_t seg;
   uint8_t min;
   uint8_t hora;
   uint8_t dia;
   uint8_t mes;
   uint8_t ano;
   int8_t utc_h;
   uint8_t utc_m;
   uint8_t tipo;
   int16_t valor;
} event_pic_t;

typedef struct event_t {
   uint16_t pos;
   event_pic_t pic;
   float latitude; // 5
   float longitude; // 6
   uint8_t vel; // 1
   int8_t sats_in_use; // 1
   int8_t csq3g; // 1
   uint8_t links; // 1
} event_t;

typedef struct eventos_config_t {
   uint16_t index_ev;
   uint16_t qtd_ev;
} eventos_config_t;

static volatile struct {
   uint32_t energizou : 1;
   uint32_t snapshot : 1;
   uint32_t reset_report : 1;
   uint32_t config : 2;
   uint32_t config_wifi : 2;
   uint32_t config_servidor : 2;
   uint32_t falha_link_pic : 1;
   uint32_t falha_link_modulo_3G : 1;
   uint32_t falha_link_GPS : 1;
   uint32_t erro_relatorio : 1;
   uint32_t atualizacao_ota : 1;
   uint32_t block_device : 1;
   uint32_t : 17;
   uint8_t block_device_value;
} registrar_ev = {
   .energizou = 0,
   .snapshot = 0,
   .config = 0,
   .reset_report = 0,
   .config_servidor = 0,
   .falha_link_pic = 0,
   .falha_link_modulo_3G = 0,
   .falha_link_GPS = 0,
   .atualizacao_ota = 0,
   .erro_relatorio = 0,
   .block_device = 0,
   .block_device_value = 0};

typedef struct config_envio_web_t {
   bool hab_envio_3g;
   bool hab_envio_wifi;
   uint16_t tempo_envio_wifi;
   uint16_t tempo_envio_3g;
   uint16_t tempo_envio_wifi_user_on;
   uint16_t tempo_envio_3g_user_on;
} config_envio_web_t;

typedef struct datetime_t {
   uint8_t seg;
   uint8_t min;
   uint8_t hora;
   uint8_t dia;
   uint8_t mes;
   uint8_t ano;
   float utc;
} datetime_t;

static struct {
   char nome[21];
   char numero_serie[10];
   char passwifi[64];
   uint32_t versao_firmware;
   char key[16];
   char web_key[7];
   char passconfig[6];
   char obs[52];
   char enable_sync_time;
} modulo = {
   .nome = "TECNNIC",
   .web_key = "000000",
   .numero_serie = "000000",
   .passwifi = "tecnnicnet",
   .versao_firmware = 0,
   .key = "",
   .obs = "",
   .passconfig = "0200",
   .enable_sync_time = 0};

typedef struct block_device_t {
   volatile int8_t level;
   volatile uint64_t dateRelease;
} block_device_t;

static block_device_t block_device = {
   .level = 0,
   .dateRelease = 0};

event_t ev_temp;
eventos_config_t ev_config_temp;

// Tipos de retornos ao receber um post
typedef enum {
   NAO_ENVIAR_ERRO = 1, // Para desabiitar o envio de erros
   // COMANDO_SENDO_PROCESSADO = 2,
   // COMANDO_SENDO_PROCESSADO_MASTER = 3,
   CONFIG_OK = 0, //"Solicitação bem sucedida."
   CONFIG_ERROR_PASS = -1, //"Nova senha de configuração é inválida."
   CONFIG_ERROR_NAME = -2, //"Nome do módulo é inválido."
   CONFIG_ERROR_NS = -3, //"Número de série inválido."
   CONFIG_ERROR_DATE = -4, //"Data inválida."
   CONFIG_MEMORY_ERROR = -5, //"Erro ao gravar na memória, tente novamente e se o problema persistir contate a Assistência."
   CONFIG_ERROR_DATA_INVALID = -6, //"Erro com formato de dados, verifique se é do tipo JSON."
   CONFIG_ERROR_PASS_IN = -7, //"Senha inválida."
   CONFIG_ERROR_CONFIG_WEB = -8, //"Erro ao receber as configurações de servidor."
   CONFIG_ERROR_CONFIG_TEMPO_REGISTER_MEN = -9, //"Tempo de snapshot é inválido."
   CONFIG_ERROR_CONFIG_WEB_KEY = -10, //"Chave inválida."
   CONFIG_ERROR_SERIAL_PIC = -11, //"Erro de comunicação, tente novamente e se o problema persistir contate a Assistência."
   CONFIG_ERROR_CONFIG_WIFI = -12, //"Erro ao configurar o Wi-fi"
   CONFIG_ERROR_SCAN_WIFI = -13, //"Erro ao procurar os Wi-fi"
   CONFIG_ERROR_REQ_INVALID = -14,
   CONFIG_ERROR_SEND_CONFIG_TIMEOUT = -99
} cod_error_response_t;

// -----------------------------------------------------------------------------
static SemaphoreHandle_t mutex_pacote_serial;
static SemaphoreHandle_t mutex_pic;
static SemaphoreHandle_t mutex_gps;
static SemaphoreHandle_t mutex_pacote_report;
static SemaphoreHandle_t mutex_envio_servidor;

static volatile uint16_t inicia_esp_timeout = 0;
static volatile uint8_t limpar_eventos = 0;
char json_tempo_real[1500] = "{}";
static volatile uint32_t gravar_evento_timeout = 0;

#define TEMPO_LINK_SERVIDOR 60000
// static uint32_t link_wifi_timeout = 0;
// static uint32_t link_wifi = 0;

// ----------------------------------------------------------------------------
// Pacote tempo real do PIC
typedef struct pic_pacote_tempo_real_t {
   // uint8_t BD; // versão
   // uint8_t BB; // Tipo de produto
   datetime_t BE; // Data e hora local
   int16_t BF; // LG
   int16_t BG; // LT
   uint8_t BJ; // PTO
   uint8_t BK; // CX baixa
   uint8_t BL; // Status LT
   uint8_t BM; // Status LG
   uint8_t BP; // Status Geral
   uint8_t CF; // CX Alta
} pic_pacote_tempo_real_t;

typedef struct pic_send_config_t {
   uint32_t BC : 1; // numero de série
   uint32_t DU : 1; // senha config
   uint32_t DV : 1; // senha força
   // uint32_t BB : 1; // Tipo de produto
   uint32_t BD : 1; // versão
   uint32_t EA : 1; // tempo_forca
   uint32_t EH : 1; // auto_reset
   uint32_t EI : 1; // brilho_lcd
   uint32_t EB : 1; // linguagem
   uint32_t BT : 1; // hab_ent_pto
   uint32_t BU : 1; // hab_ent_cx_baixa
   uint32_t CG : 1; // hab_ent_cx_alta
   uint32_t DR : 1; // set_lt_al
   uint32_t DS : 1; // set_lt_bl
   uint32_t DP : 1; // set_lg_al
   uint32_t DQ : 1; // set_lg_bl
   uint32_t DT : 1; // tempo_lambda
   uint32_t EC : 1; // inv_lg_lt
   uint32_t EE : 1; // inv_sent_lt
   uint32_t ED : 1; // inv_sent_lg
   uint32_t BE : 1; // Data
   uint32_t DM : 1; // clear_n_operacoes
   uint32_t DL : 1; // clear_tempo_ligado
   uint32_t DZ : 1; // cal_inc
   uint32_t DN : 1; // valor cal inc LG (x)
   uint32_t DO : 1; // valor cal inc LT (y)
   uint32_t packet : 1;
   uint32_t : 5;
} pic_send_config_t;

typedef struct pic_pacote_config_t {
   char BC[10]; // numero de série
   char DU[4]; // senha config
   char DV[4]; // senha força
   // uint8_t BB; // Tipo de produto
   uint8_t BD; // versão
   int16_t EA; // tempo_forca
   uint8_t EH; // auto_reset
   uint8_t EI; // brilho_lcd
   uint8_t EB; // linguagem
   uint8_t BT; // hab_ent_pto
   uint8_t BU; // hab_ent_cx_baixa
   uint8_t CG; // hab_ent_cx_alta
   int16_t DR; // set_lt_al
   int16_t DS; // set_lt_bl
   int16_t DP; // set_lg_al
   int16_t DQ; // set_lg_bl
   uint8_t DT; // tempo_lambda
   uint8_t EC; // inv_lg_lt
   uint8_t EE; // inv_sent_lt
   uint8_t ED; // inv_sent_lg
   datetime_t BE; // Data
   int16_t DM; // clear_n_operacoes
   float DL; // clear_tempo_ligado
   uint8_t DZ; // cal_inc
   int16_t DN; // valor cal inc LG (x)
   int16_t DO; // valor cal inc LT (y)
   pic_send_config_t send;
} pic_pacote_config_t;

typedef enum {
   ERRO_REQUISICAO = -1,
   TIPO_SEM_REQUISICAO = 0,
   TIPO_REQUISICAO_CONCLUIDA = 1,
   TIPO_GET_CONFIG,
   TIPO_SET_CONFIG
} tipo_pacote_t;

typedef struct config_req_serial_pic_t {
   tipo_pacote_t tipo_pacote;
   pic_pacote_config_t config_pic;
   uint32_t aguarda_timeout;
   uint32_t tempo_aguarda;
   uint8_t pass_tecnnic;
   // uint8_t qtd_tentativas_realizadas;
   uint8_t qtd_tentativas;
} config_req_serial_pic_t;

static pic_pacote_tempo_real_t pic_pacote_tempo_real; // Pacote tempo real

// static pic_pacote_config_t pic_pacote_config_rx; // Este de recebimento de configuração
static config_req_serial_pic_t req_serial_pic;
static config_req_serial_pic_t config_serial_server;
// ----------------------------------------------------------------------------
#define TEMPO_BLOQUEIO_RESET_3G 120000
#define TEMPO_PULSO_RESET       1500

static volatile struct {
   int conectado_sem_rede_timeout;
   int pulso_timeout_ms; // Duração do pulso
   int contagem_erro; // contagem de erro ao tentar conectar
   int bloqueio_reset_3g_timeout_ms; // Tempo que permanece sem realizar pulso
   int habilita_reset; // habilita para realizar pulso de reset
} controle_reset_3g;

int64_t IRAM_ATTR timer_count = 0;

// void print_task_monitor();
void controle_app_led();
void print_heap(char *msg);
static esp_err_t atualizar_evento(event_t *evento, bool add_data);
void app_controller(void *pvParameters);
void inicia_variaveis_aplicacao();
void ap_get_config_modulo(char *p);
void ap_get_realtimejson(char *p); // Usa no front-end
void ap_get_realtime(char *p); // Usa no APP
bool verifica_numero_serie_valido();
esp_err_t testa_senha_dinamica(char *pass, char *dica);

#include "config_ota.h"