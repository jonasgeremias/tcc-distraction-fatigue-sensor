
#include "wifi.h"
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "string.h"
#include "mdns.h"
#include "lwip/dns.h"

static const char *LOG_WIFI = "wifi";
#define APP_CONFIG_WIFI_SSID     "sensor-fadiga"
#define APP_CONFIG_WIFI_PASSWORD "12345678"
#define APP_CONFIG_MAX_STA_CONN  2

// Controle da camada de redes
esp_netif_t *app_sta_netif;
esp_netif_t *app_ap_netif;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
   if (event_id == WIFI_EVENT_AP_STACONNECTED) {
      wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *) event_data;
      ESP_LOGI(LOG_WIFI, "station " MACSTR " join, AID=%d",
               MAC2STR(event->mac), event->aid);
   }
   else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
      wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *) event_data;
      ESP_LOGI(LOG_WIFI, "station " MACSTR " leave, AID=%d",
               MAC2STR(event->mac), event->aid);
   }

   else if (event_id == WIFI_EVENT_STA_START) {
      esp_wifi_connect();
   }
   else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
      esp_wifi_connect();
   }
}

static void add_mdns_services() {
   // Configurando o hostname da aplicação
   char hostname[60];
   sprintf((char *) hostname, "%s", "sensor_fadiga");

   // initialize mDNS service
   esp_err_t err = mdns_init();
   if (err != ESP_OK) {
      return;
   }

   // Seta o hostname
   esp_netif_set_hostname(app_sta_netif, hostname);
   esp_netif_set_hostname(app_ap_netif, hostname);
   mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
   mdns_service_instance_name_set("_http", "_tcp", "Tecnnic Connect Web Server");
}

static void config_tcpip_ap_fix() {
   // Configura de IP fixo.

   esp_netif_ip_info_t net;
   esp_netif_dns_info_t dns1;
   esp_netif_dns_info_t dns2;
   esp_netif_set_ip4_addr(&net.ip, 192, 168, 100, 1);
   esp_netif_set_ip4_addr(&net.gw, 192, 168, 100, 1);
   esp_netif_set_ip4_addr(&net.netmask, 255, 255, 255, 0);
   esp_netif_set_ip4_addr(&dns1.ip.u_addr.ip4, 192, 168, 100, 1);
   esp_netif_set_ip4_addr(&dns2.ip.u_addr.ip4, 8, 8, 8, 8);

   esp_netif_dhcps_stop(app_ap_netif);
   esp_netif_set_ip_info(app_ap_netif, &net);
   esp_netif_set_dns_info(app_ap_netif, ESP_NETIF_DNS_MAIN, &dns1);
   esp_netif_set_dns_info(app_ap_netif, ESP_NETIF_DNS_BACKUP, &dns2);

   esp_netif_dhcps_start(app_ap_netif);

   dns_init();
}

static void app_netif_init() {
   ESP_ERROR_CHECK(esp_netif_init());
   ESP_ERROR_CHECK(esp_event_loop_create_default());

   // Criar a camada física
   app_sta_netif = esp_netif_create_default_wifi_sta();
   app_ap_netif = esp_netif_create_default_wifi_ap();
   assert(app_sta_netif);
   assert(app_ap_netif);

   ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
   ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

   // Inicializa driver TCP/IP
   config_tcpip_ap_fix();

   // Configuração de hostname e serviços.
   add_mdns_services();
}

void init_wifi_softap() {
   app_netif_init();
   wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
   cfg.nvs_enable = 0;

   ESP_ERROR_CHECK(esp_wifi_init(&cfg));
   ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

   wifi_config_t wifi_config_ap = {};
   wifi_config_ap.ap.ssid_len = sprintf((char *) wifi_config_ap.ap.ssid, "%s", (char *) APP_CONFIG_WIFI_SSID);
   sprintf((char *) wifi_config_ap.ap.password, "%s", (char *) APP_CONFIG_WIFI_PASSWORD);
   wifi_config_ap.ap.max_connection = APP_CONFIG_MAX_STA_CONN;
   wifi_config_ap.ap.beacon_interval = 100;
   wifi_config_ap.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

   wifi_config_t wifi_config_sta = {};
   sprintf((char *) wifi_config_sta.sta.ssid, "%s", (char *) "Jesus_te_ama");
   sprintf((char *) wifi_config_sta.sta.password, "%s", (char *) "quesenha");
   wifi_config_sta.sta.scan_method = WIFI_FAST_SCAN;
   wifi_config_sta.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
   wifi_config_sta.sta.listen_interval = WIFI_PS_NONE;

   if (strlen(APP_CONFIG_WIFI_PASSWORD) == 0) {
      wifi_config_ap.ap.authmode = WIFI_AUTH_OPEN;
   }

   ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
   ESP_ERROR_CHECK(esp_wifi_set_config((wifi_interface_t) ESP_IF_WIFI_AP, &wifi_config_ap));
   ESP_ERROR_CHECK(esp_wifi_set_config((wifi_interface_t) ESP_IF_WIFI_STA, &wifi_config_sta));
   ESP_ERROR_CHECK(esp_wifi_start());

   ESP_LOGI(LOG_WIFI, "wifi_init_softap finished. SSID:%s password:%s",
            APP_CONFIG_WIFI_SSID, APP_CONFIG_WIFI_PASSWORD);
}
