
#ifdef DEBUG_WIFI
const char *LOG_WIFI = "Wifi";
#endif

#ifdef DEBUG_HTTP
const char *LOG_HTTP = "HTTP";
#endif

esp_netif_t *app_sta_netif;
esp_netif_t *app_ap_netif;
#include "mdns.h"
#include "lwip/dns.h"

// Definições da aplicação ----------------------------------------------------
#define QTD_LOGIN_WIFI                6
#define ID_NULL                       -1
#define WIFI_TEMPO_SCAN_AP_OFF        300
#define WIFI_TEMPO_CONECTAR_AO_AP     10000
#define WIFI_TEMPO_AGUARDANDO_IP      10000
#define ESP_WIFI_SERVER_MAX_CON       4
#define ESP_WIFI_SERVER_CHANNEL       5
#define TEMPO_ERRO_LINK_WIFI_WEB      45000
#define WIFI_QUANTIDADE_DE_PROCURAS   5
#define WIFI_TEMPO_RETORNO_DE_PROCURA 15000 // 15 s
#define WIFI_RETRI_SCAN_TIMEOUT_AP_3G 30000 // 30 s
#define WIFI_RETRI_SCAN_TIMEOUT_AP    2500

// Variáveis ------------------------------------------------------------------
typedef enum {
   DESCONECTADO,
   PROCURANDO_WIFI,
   CONECTANDO,
   AGUARDANDO_IP,
   CONECTADO,
   CONECTADO_E_PROCURANDO_WIFI,
   DESABILITADO
} app_wifi_sta_status_t;

static TaskHandle_t taskhandle_wifi_app_control;
static SemaphoreHandle_t mutex_pesquisa_wifi;
uint8_t mac_address[6] = {0, 0, 0, 0, 0, 0};

static volatile struct {
   app_wifi_sta_status_t status;
   int id_scan;
   int id_connected;
   int retri_scan_timeout;
   int tempo_scan;
   bool link_wifi_web;
   uint32_t link_wifi_web_timeout;
   uint32_t tempo_envio_timeout;
   // uint32_t tempo_erro_envio_timeout;
} app_wifi_sta = {
   .status = DESABILITADO,
   .id_scan = ID_NULL,
   .id_connected = ID_NULL,
   .retri_scan_timeout = WIFI_RETRI_SCAN_TIMEOUT_AP,
   .tempo_scan = WIFI_TEMPO_SCAN_AP_OFF,
   .link_wifi_web = 0,
   .link_wifi_web_timeout = TEMPO_ERRO_LINK_WIFI_WEB,
   .tempo_envio_timeout = 0
   //,.tempo_erro_envio_timeout = 0
   };

static volatile int wifi_aguardando_ip_timeout = WIFI_TEMPO_AGUARDANDO_IP;
static volatile int wifi_conectar_ao_ap_timeout = WIFI_TEMPO_CONECTAR_AO_AP;
static volatile int wifi_retri_scan_timeout_3G = WIFI_RETRI_SCAN_TIMEOUT_AP_3G;
static volatile bool esp_restart_now = 0; // Para reset do ESP32, setar para 1 e adicionar tempo
static volatile uint16_t esp_restart_timeout = 0;

typedef struct {
   uint32_t ip;
   uint32_t netmask;
   uint32_t gw;
   uint32_t dns1;
   uint32_t dns2;
} net_ipv4;

typedef struct {
   char ssid[32];
   char pass[64];
   uint8_t bssid[6];
   bool bssid_set;
   bool falhou_conectar;
   bool status_procura;
   net_ipv4 net_ip4;
   bool ip4_fix;
} login_wifi_t;
login_wifi_t login_wifi[QTD_LOGIN_WIFI];

// Variavel para criação do ponto de acesso Wi-Fi
struct {
   char ssid[32];
   char pass[64];
   esp_netif_ip_info_t net;
   esp_netif_dns_info_t dns;
} wifi_ap = {
   .ssid = "TECNNIC-000000",
   .pass = "tecnnicnet",
};

static wifi_ap_record_t lista_ap[16]; // Lista de Aps na procura de Wifi
static bool sta_conectando_wifi = 0;

// Protótipos de funções ------------------------------------------------------

/******************************************************************************
 * Trata Os eventos do wifi.
 *****************************************************************************/
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data);
//static esp_err_t event_handler(void *ctx, system_event_t *event);

/******************************************************************************
 * Configura o ponto de acesso.
 *****************************************************************************/
static esp_err_t wifi_ap_config();

/******************************************************************************
 * Configura o IP do ponto de acesso como fixo.
 *****************************************************************************/
static void config_tcpip_ap_fix();

/******************************************************************************
 * Configura o login e senha para conectar o ESP (STA) em um Wi-Fi disponivel.
 *****************************************************************************/
static esp_err_t wifi_sta_config(int id);

/******************************************************************************
 * Faz a procura de Wifis e compara se existe um Wi-Fi conhecido para conectar.
 *****************************************************************************/
static int wifi_app_start_scan();

/******************************************************************************
 * Inicia o modulo Wi-Fi do ESP no modo AP-STA.
 *****************************************************************************/
static void wifi_init();

/******************************************************************************
 * Função para desconectar do Wi-Fi e desabilitar a reconexão.
 *****************************************************************************/
static void desconectar_do_wifi();

/******************************************************************************
 * Função deve ser adicionada ao um timer de 1ms para contar corretamente.
 *****************************************************************************/
static void IRAM_ATTR wifi_timer_ms();

/******************************************************************************
 * Função que verifique periodicamente os status do wifi e tarfas relacionadas.
 *****************************************************************************/
static void wifi_app_control(void *pvParameters);

/******************************************************************************
 * Inicia tarefa de cntole do Wi-Fi .
 *****************************************************************************/
static void wifi_app_init();

// Pesquisa de Wifi com mutex
static int wifi_scan(uint16_t *qtd_ap);


static int gera_json_configwifi(char *p);
static int ap_gera_json_scanwifi(char *p);