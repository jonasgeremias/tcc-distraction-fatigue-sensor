static int ap_gera_json_scanwifi(char *p) {
   char *p_ini = (char *) p;
   int ret = ESP_FAIL;
   uint16_t qtd_ap = 0;

   // inicia pesquisa se o mutex estiver liberado
   xSemaphoreTake(mutex_pesquisa_wifi, portMAX_DELAY);
   ret = wifi_scan(&qtd_ap);
   if (ret == ESP_OK) {
      p += sprintf(p, "{\"AB\":{"); // Abre estrutura wifi
      for (int i = 0; i < qtd_ap; i++) {
         p += sprintf(p, "\"%d\":{", i);
         p += sprintf(p, "\"ssid\":\"%s\",", lista_ap[i].ssid);
         p += sprintf(p, "\"ch\":%d,", lista_ap[i].primary);
         p += sprintf(p, "\"rssi\":%d,", lista_ap[i].rssi);
         p += sprintf(p, "\"auth\":%d,", lista_ap[i].authmode);
         p += sprintf(p, "\"bssid\":\"%02X:%02X:%02X:%02X:%02X:%02X\"},", lista_ap[i].bssid[0], lista_ap[i].bssid[1], lista_ap[i].bssid[2], lista_ap[i].bssid[3], lista_ap[i].bssid[4], lista_ap[i].bssid[5]);
      }
      if (*(p - 1) == ',') { // Se teve evento, remove a virgula do final
         p -= 1;
      }
      p += sprintf(p, "}"); // Fecha o AB
   }
   xSemaphoreGive(mutex_pesquisa_wifi);

   if (*(p - 1) == ',') { // Se teve evento, remove a virgula do final
      p -= 1;
   }

   // Erro é depois porque precisa realizar a pesquisa com sucesso.
   p += sprintf(p, "}"); // Resposta da request
   return (p - p_ini);
}

static bool bssid_exists(uint8_t *bssid) {
   uint8_t count = 0;

   for (uint8_t i = 0; i < 6; i++) {
      if (*bssid == 0) count++;
   }

   if (count == 6) return 0;

   return 1;
}

static int gera_json_configwifi(char *p) {
   int i = 0;
   p += sprintf(p, "{\"AB\":{"); // Abre estrutura wifi

   char *p_ini = (char *) p;
   // Passando por todos os wi-fi
   for (i = 0; i < QTD_LOGIN_WIFI; i++) {
      p += sprintf(p, "\"%d\":{", i);
      p += sprintf(p, "\"ssid\":\"%s\",", login_wifi[i].ssid);
      p += sprintf(p, "\"pass\":%d,", (strlen(login_wifi[i].pass) > 4));
      p += sprintf(p, "\"con\":%d,", (i == app_wifi_sta.id_connected));
      p += sprintf(p, "\"bssid_set\":%d,", login_wifi[i].bssid_set);

      if (bssid_exists((uint8_t *) &login_wifi[i].bssid)) {
         p += sprintf(p, "\"bssid\":\"%02X:%02X:%02X:%02X:%02X:%02X\",", login_wifi[i].bssid[0], login_wifi[i].bssid[1], login_wifi[i].bssid[2], login_wifi[i].bssid[3], login_wifi[i].bssid[4], login_wifi[i].bssid[5]);
      }
      else {
         p += sprintf(p, "\"bssid\":\"\",");
      }

      p += sprintf(p, "\"ip4_fix\":%d,", login_wifi[i].ip4_fix);

      // O último é do 3G
      if (i == (QTD_LOGIN_WIFI - 1)) {
         p += sprintf(p, "\"cel\":1,");
      }

      if (login_wifi[i].net_ip4.ip != 0) {
         p += sprintf(p, "\"ip4_ip\":\"%d.%d.%d.%d\",",
                      MAKE8(login_wifi[i].net_ip4.ip, 0),
                      MAKE8(login_wifi[i].net_ip4.ip, 1),
                      MAKE8(login_wifi[i].net_ip4.ip, 2),
                      MAKE8(login_wifi[i].net_ip4.ip, 3));
      }
      if (login_wifi[i].net_ip4.gw != 0) {
         p += sprintf(p, "\"ip4_gw\":\"%d.%d.%d.%d\",",
                      MAKE8(login_wifi[i].net_ip4.gw, 0),
                      MAKE8(login_wifi[i].net_ip4.gw, 1),
                      MAKE8(login_wifi[i].net_ip4.gw, 2),
                      MAKE8(login_wifi[i].net_ip4.gw, 3));
      }
      if (login_wifi[i].net_ip4.netmask != 0) {
         p += sprintf(p, "\"ip4_netmask\":\"%d.%d.%d.%d\",",
                      MAKE8(login_wifi[i].net_ip4.netmask, 0),
                      MAKE8(login_wifi[i].net_ip4.netmask, 1),
                      MAKE8(login_wifi[i].net_ip4.netmask, 2),
                      MAKE8(login_wifi[i].net_ip4.netmask, 3));
      }
      if (login_wifi[i].net_ip4.dns1 != 0) {
         p += sprintf(p, "\"ip4_dns1\":\"%d.%d.%d.%d\",",
                      MAKE8(login_wifi[i].net_ip4.dns1, 0),
                      MAKE8(login_wifi[i].net_ip4.dns1, 1),
                      MAKE8(login_wifi[i].net_ip4.dns1, 2),
                      MAKE8(login_wifi[i].net_ip4.dns1, 3));
      }

      if (login_wifi[i].net_ip4.dns2 != 0) {
         p += sprintf(p, "\"ip4_dns2\":\"%d.%d.%d.%d\",",
                      MAKE8(login_wifi[i].net_ip4.dns2, 0),
                      MAKE8(login_wifi[i].net_ip4.dns2, 1),
                      MAKE8(login_wifi[i].net_ip4.dns2, 2),
                      MAKE8(login_wifi[i].net_ip4.dns2, 3));
      }

      if (*(p - 1) == ',') {
         p -= 1;
      }

      p += sprintf(p, "},");
   }

   if (*(p - 1) == ',') { // Se teve evento, remove a virgula do final
      p -= 1;
   }
   p += sprintf(p, "}"); // Fim do AB
   p += sprintf(p, "}"); // Fim json

   return (p - p_ini);
}

/*
static cJSON *gera_json_configwifi() {
   cJSON *json_configwifi = cJSON_CreateObject();

   uint8_t i = 0;
   static char buffer[20];

   // Aqui todos entram
   for (i = 0; i < QTD_LOGIN_WIFI; i++) {
      cJSON *rede_wifi = cJSON_CreateObject();
      cJSON_AddStringToObject(rede_wifi, "ssid", login_wifi[i].ssid);
      if (strlen(login_wifi[i].pass) < 4) cJSON_AddBoolToObject(rede_wifi, "pass", 0);
      else
         cJSON_AddBoolToObject(rede_wifi, "pass", 1);

      cJSON_AddBoolToObject(rede_wifi, "con", (i == app_wifi_sta.id_connected));
      cJSON_AddBoolToObject(rede_wifi, "ip4_fix", login_wifi[i].ip4_fix);

      if (login_wifi[i].net_ip4.ip != 0) {
         sprintf(buffer, "%d.%d.%d.%d",
                 MAKE8(login_wifi[i].net_ip4.ip, 0),
                 MAKE8(login_wifi[i].net_ip4.ip, 1),
                 MAKE8(login_wifi[i].net_ip4.ip, 2),
                 MAKE8(login_wifi[i].net_ip4.ip, 3));
         cJSON_AddStringToObject(rede_wifi, "ip4_ip", buffer);
      }
      if (login_wifi[i].net_ip4.gw != 0) {
         sprintf(buffer, "%d.%d.%d.%d",
                 MAKE8(login_wifi[i].net_ip4.gw, 0),
                 MAKE8(login_wifi[i].net_ip4.gw, 1),
                 MAKE8(login_wifi[i].net_ip4.gw, 2),
                 MAKE8(login_wifi[i].net_ip4.gw, 3));
         cJSON_AddStringToObject(rede_wifi, "ip4_gw", buffer);
      }
      if (login_wifi[i].net_ip4.netmask != 0) {
         sprintf(buffer, "%d.%d.%d.%d",
                 MAKE8(login_wifi[i].net_ip4.netmask, 0),
                 MAKE8(login_wifi[i].net_ip4.netmask, 1),
                 MAKE8(login_wifi[i].net_ip4.netmask, 2),
                 MAKE8(login_wifi[i].net_ip4.netmask, 3));
         cJSON_AddStringToObject(rede_wifi, "ip4_netmask", buffer);
      }
      if (login_wifi[i].net_ip4.dns1 != 0) {
         sprintf(buffer, "%d.%d.%d.%d",
                 MAKE8(login_wifi[i].net_ip4.dns1, 0),
                 MAKE8(login_wifi[i].net_ip4.dns1, 1),
                 MAKE8(login_wifi[i].net_ip4.dns1, 2),
                 MAKE8(login_wifi[i].net_ip4.dns1, 3));
         cJSON_AddStringToObject(rede_wifi, "ip4_dns1", buffer);
      }

      if (login_wifi[i].net_ip4.dns2 != 0) {
         sprintf(buffer, "%d.%d.%d.%d",
                 MAKE8(login_wifi[i].net_ip4.dns2, 0),
                 MAKE8(login_wifi[i].net_ip4.dns2, 1),
                 MAKE8(login_wifi[i].net_ip4.dns2, 2),
                 MAKE8(login_wifi[i].net_ip4.dns2, 3));
         cJSON_AddStringToObject(rede_wifi, "ip4_dns2", buffer);
      }

      sprintf(buffer, "%u", i);
      cJSON_AddItemToObject(json_configwifi, buffer, rede_wifi);
   }

   return json_configwifi;
}
*/

/******************************************************************************
 * Trata Os eventos do wifi.
 *****************************************************************************/
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
   if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
#ifdef DEBUG_WIFI
      ESP_LOGI(LOG_WIFI, "AP Start");
#endif
   }
   else if (event_base == WIFI_EVENT && event_id == SYSTEM_EVENT_AP_STACONNECTED) {
#ifdef DEBUG_WIFI
      ESP_LOGI(LOG_WIFI, "Disp. conectou no ESP");
      wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *) event_data;
      ESP_LOGI(LOG_WIFI, "station " MACSTR " join, AID=%d",
               MAC2STR(event->mac), event->aid);
#endif
   }
   else if (event_base == WIFI_EVENT && event_id == SYSTEM_EVENT_AP_STADISCONNECTED) { // Dispositivo desconectou do ESP
#ifdef DEBUG_WIFI
      wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *) event_data;
      ESP_LOGI(LOG_WIFI, "Disp. desconectou do ESP");
      ESP_LOGI(LOG_WIFI, MACSTR "leave, AID=%d",
               MAC2STR(event->mac), event->aid);
#endif
   }
   else if (event_base == WIFI_EVENT && event_id == SYSTEM_EVENT_STA_CONNECTED) { // Conectou no roteador
#ifdef DEBUG_WIFI
      ESP_LOGI(LOG_WIFI, "Conectou no Rot.");
#endif
      if (login_wifi[app_wifi_sta.id_scan].ip4_fix == 0) {
         app_wifi_sta.status = AGUARDANDO_IP;
         wifi_aguardando_ip_timeout = WIFI_TEMPO_AGUARDANDO_IP;

#ifdef DEBUG_WIFI
         ESP_LOGI(LOG_WIFI, "AGUARDANDO_IP");
#endif
      }
      else {
         app_wifi_sta.id_connected = app_wifi_sta.id_scan;

         // Se esta no 3G seta o timeout para procurar um novo wifi. A manutenção da procura é no loop.
         if (app_wifi_sta.id_connected == (QTD_LOGIN_WIFI - 1)) {
            wifi_retri_scan_timeout_3G = WIFI_RETRI_SCAN_TIMEOUT_AP_3G;
            controle_reset_3g.pulso_timeout_ms = 0;
         }

         app_wifi_sta.status = CONECTADO;
#ifdef DEBUG_WIFI
         ESP_LOGI(LOG_WIFI, "CONECTADO");
#endif
         xEventGroupSetBits(app_event_group, CONECTADO_NO_WIFI);
      }
   }
   else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) { // IPV4 ----------------------

#ifdef DEBUG_WIFI
      ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
      ESP_LOGI(LOG_WIFI, "Recebeu ip:" IPSTR, IP2STR(&event->ip_info.ip));
#endif
      app_wifi_sta.id_connected = app_wifi_sta.id_scan;

      // Se esta no 3G seta o timeout para procurar um novo wifi. A manutenção da procura é no loop.
      if (app_wifi_sta.id_connected == (QTD_LOGIN_WIFI - 1)) {
         wifi_retri_scan_timeout_3G = WIFI_RETRI_SCAN_TIMEOUT_AP_3G;
         controle_reset_3g.contagem_erro = 0;
         controle_reset_3g.pulso_timeout_ms = 0;
      }

      app_wifi_sta.status = CONECTADO;
#ifdef DEBUG_WIFI
      ESP_LOGI(LOG_WIFI, "CONECTADO");
#endif

      // if (app_wifi_sta.id_connected == (QTD_LOGIN_WIFI - 1)) {
      //    wifi_retri_scan_timeout_3G = WIFI_RETRI_SCAN_TIMEOUT_AP_3G;
      // controle_reset_3g.pulso_timeout_ms = 0;
      // }

      xEventGroupSetBits(app_event_group, CONECTADO_NO_WIFI);
   }
   else if (event_base == WIFI_EVENT && event_id == SYSTEM_EVENT_STA_DISCONNECTED) { // Desconectou do Roteador
      app_wifi_sta.status = DESCONECTADO;

      servidor_clear_requests();

      if (app_wifi_sta.id_scan == (QTD_LOGIN_WIFI - 1)) {
         if ((++controle_reset_3g.contagem_erro > 2) && (controle_reset_3g.habilita_reset)) {
            registrar_ev.falha_link_modulo_3G = 1; // Gravar Evento
            controle_reset_3g.bloqueio_reset_3g_timeout_ms = TEMPO_BLOQUEIO_RESET_3G;
            controle_reset_3g.pulso_timeout_ms = TEMPO_PULSO_RESET; // Duração do pulso 1000 ms
            controle_reset_3g.contagem_erro = 0;
         }

#ifdef DEBUG_WIFI
         ESP_LOGE(LOG_WIFI, "Rot. 3G não conectou!");
#endif
      }
      login_wifi[app_wifi_sta.id_scan].falhou_conectar = 1; // Seta que ja tentou conectar
      app_wifi_sta.id_connected = ID_NULL;
      app_wifi_sta.id_scan = ID_NULL;
      sta_conectando_wifi = 0;
      xEventGroupClearBits(app_event_group, CONECTADO_NO_WIFI);
#ifdef DEBUG_WIFI
      ESP_LOGI(LOG_WIFI, "Desconectou do Rot.");
#endif
   }

   else if (event_base == WIFI_EVENT && event_id == SYSTEM_EVENT_AP_STAIPASSIGNED) { // Dispositivo conectou no ESP e recebeu IP
#ifdef DEBUG_WIFI
      ESP_LOGI(LOG_WIFI, "Disp. Conectou.");
#endif
   }
   else if (event_base == WIFI_EVENT && event_id == SYSTEM_EVENT_STA_START) {
#ifdef DEBUG_WIFI
      ESP_LOGI(LOG_WIFI, "Evento STA start.");
#endif
   }
   else if (event_base == WIFI_EVENT && event_id == SYSTEM_EVENT_SCAN_DONE) {
   }
   else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_LOST_IP) {
#ifdef DEBUG_WIFI
      ESP_LOGI(LOG_WIFI, "IP ruim.");
#endif
      esp_wifi_disconnect();
   }
   else {
#ifdef DEBUG_WIFI
      ESP_LOGE(LOG_WIFI, "Evento Inválido ->%d, %d", (int) event_base, event_id);
#endif
   }
}

/******************************************************************************
 * Configura o ponto de acesso.
 *****************************************************************************/
static esp_err_t wifi_ap_config() {
   wifi_config_t wifi_conf = {
      .ap = {
         .max_connection = ESP_WIFI_SERVER_MAX_CON,
         .authmode = WIFI_AUTH_WPA2_PSK,
         //.channel = ESP_WIFI_SERVER_CHANNEL,
         .beacon_interval = 500}};

   // Monta nome do Wi-Fi do servidor
   if (strlen(wifi_ap.ssid) < 32) sprintf((char *) wifi_conf.ap.ssid, "%s", (char *) wifi_ap.ssid);
   else
      sprintf((char *) wifi_conf.ap.ssid, "%s", (char *) "TECNNIC-000000");
   if (strlen(wifi_ap.pass) < 8) strcpy((char *) wifi_conf.ap.password, "tecnnicnet");
   else
      strcpy((char *) wifi_conf.ap.password, wifi_ap.pass);

   int len = strlen((char *) wifi_conf.ap.ssid);
   wifi_conf.ap.ssid_len = len;
   if (len == 0) wifi_conf.ap.authmode = WIFI_AUTH_OPEN;

   return esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_conf);
}

/******************************************************************************
 * Configura o IP do ponto de acesso como fixo.
 *****************************************************************************/
static void config_tcpip_ap_fix() {
   // Configura de IP fixo.
   esp_netif_set_ip4_addr(&wifi_ap.net.ip, 200, 200, 200, 1);
   esp_netif_set_ip4_addr(&wifi_ap.net.gw, 200, 200, 200, 1);
   esp_netif_set_ip4_addr(&wifi_ap.net.netmask, 255, 255, 255, 248);
   esp_netif_set_ip4_addr(&wifi_ap.dns.ip.u_addr.ip4, 200, 200, 200, 1);
   // esp_netif_set_ip4_addr(&wifi_ap.dns.ip.u_addr.ip4.addr, 8, 8, 8, 8);
   wifi_ap.dns.ip.type = IPADDR_TYPE_V4;

   esp_netif_dhcps_stop(app_ap_netif);
   esp_netif_set_ip_info(app_ap_netif, &wifi_ap.net);
   esp_netif_set_dns_info(app_ap_netif, ESP_NETIF_DNS_MAIN, &wifi_ap.dns);
   esp_netif_dhcps_start(app_ap_netif);

   dns_init();

#ifdef DEBUG_WIFI
   esp_netif_get_dns_info(app_ap_netif, ESP_NETIF_DNS_MAIN, &wifi_ap.dns);
   ESP_LOGW(LOG_WIFI, "ESP_NETIF_DNS_MAIN->%d",
            wifi_ap.dns.ip.u_addr.ip4.addr);
#endif
}

/******************************************************************************
 * Configura o login e senha para conectar o ESP (STA) em um Wi-Fi disponivel.
 *****************************************************************************/
static esp_err_t wifi_sta_config(int id) {
   wifi_config_t wifi_conf_sta = {
      .sta = {
         .scan_method = WIFI_FAST_SCAN,
         .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
         .listen_interval = WIFI_PS_NONE}};

#ifdef DEBUG_WIFI
   ESP_LOGI(LOG_WIFI, "wifi_sta_config");
#endif

   app_wifi_sta.id_scan = id;
   app_wifi_sta.id_connected = ID_NULL;

   if (id >= 0) {
      // Verifica se as configurações são válidas e atribui a struct de configuração do wifi.
      if ((strlen(login_wifi[id].ssid) > 0) && (strlen(login_wifi[id].pass) > 7)) {
#ifdef DEBUG_WIFI
         ESP_LOGI(LOG_WIFI, "Config -> %s", login_wifi[id].ssid);
#endif
         sprintf((char *) wifi_conf_sta.sta.ssid, "%s", (char *) login_wifi[id].ssid);
         sprintf((char *) wifi_conf_sta.sta.password, "%s", (char *) login_wifi[id].pass);

         // Atribuindo MAC do roteador
         wifi_conf_sta.sta.bssid[0] = login_wifi[id].bssid[0];
         wifi_conf_sta.sta.bssid[1] = login_wifi[id].bssid[1];
         wifi_conf_sta.sta.bssid[2] = login_wifi[id].bssid[2];
         wifi_conf_sta.sta.bssid[3] = login_wifi[id].bssid[3];
         wifi_conf_sta.sta.bssid[4] = login_wifi[id].bssid[4];
         wifi_conf_sta.sta.bssid[5] = login_wifi[id].bssid[5];
         wifi_conf_sta.sta.bssid_set = login_wifi[id].bssid_set;
      }
      else {
         login_wifi[id].falhou_conectar = 1;
         app_wifi_sta.status = DESCONECTADO;
         app_wifi_sta.id_scan = ID_NULL;
         return ESP_FAIL;
      }
   }

   // Verifica se o Cliente DHCP está ativado.
   esp_netif_dhcp_status_t status = 0;
   esp_netif_dhcpc_get_status(app_sta_netif, &status);
#ifdef DEBUG_WIFI
   ESP_LOGI(LOG_WIFI, "Status DHCPC:%d", status);
#endif
   if ((login_wifi[id].ip4_fix == 1) && (id >= 0)) { // DHCPC
      if (status != ESP_NETIF_DHCP_STOPPED) {
         esp_netif_dhcpc_stop(app_sta_netif);
      }

      esp_netif_ip_info_t ipv4_config;
      ipv4_config.ip.addr = MAKE32(
         MAKE8(login_wifi[id].net_ip4.ip, 3),
         MAKE8(login_wifi[id].net_ip4.ip, 2),
         MAKE8(login_wifi[id].net_ip4.ip, 1),
         MAKE8(login_wifi[id].net_ip4.ip, 0));

      ipv4_config.gw.addr = MAKE32(
         MAKE8(login_wifi[id].net_ip4.gw, 3),
         MAKE8(login_wifi[id].net_ip4.gw, 2),
         MAKE8(login_wifi[id].net_ip4.gw, 1),
         MAKE8(login_wifi[id].net_ip4.gw, 0));

      ipv4_config.netmask.addr = MAKE32(
         MAKE8(login_wifi[id].net_ip4.netmask, 3),
         MAKE8(login_wifi[id].net_ip4.netmask, 2),
         MAKE8(login_wifi[id].net_ip4.netmask, 1),
         MAKE8(login_wifi[id].net_ip4.netmask, 0));

      esp_netif_dns_info_t dns;
      dns.ip.u_addr.ip4.addr = login_wifi[id].net_ip4.dns1;

      ESP_ERROR_CHECK(esp_netif_set_ip_info(app_sta_netif, &ipv4_config));
      ESP_ERROR_CHECK(esp_netif_set_dns_info(app_sta_netif, ESP_NETIF_DNS_MAIN, &dns));
   }
   else { // Se a configuração ? DHCPC
      if (status != ESP_NETIF_DHCP_STARTED) {
         esp_netif_dhcpc_start(app_sta_netif);
      }
   }

   return esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_conf_sta);
}

static int wifi_scan(uint16_t *qtd_ap) {
   // Inicia pesquisa
   wifi_scan_config_t scan_conf = {
      .ssid = NULL,
      .bssid = NULL,
      .show_hidden = true,
      .scan_time.passive = 200,
      .scan_type = WIFI_SCAN_TYPE_PASSIVE};

   // Inicia pesquisa
   int ret = ESP_OK;
   ret = esp_wifi_scan_start(&scan_conf, true);
   if (ret != ESP_OK) goto fim;

   // Busca APs encontrados
   ret = esp_wifi_scan_get_ap_num((uint16_t *) qtd_ap);
   if (ret != ESP_OK) goto fim;

   // Se nenhum Wi-Fi encontrado
   if (*qtd_ap == 0) goto fim; // Se nenhum ap na pesquisa, sai.
   if (*qtd_ap > 16) *qtd_ap = 16; // Limita a pesquisa em 16.

   ret = esp_wifi_scan_get_ap_records(qtd_ap, (wifi_ap_record_t *) &lista_ap);
   if (ret != ESP_OK) goto fim;

fim:
   return ret;
}

// Validar e habilitar os wi-fi configurados.
bool testa_configuracao_wifi() {
   for (int8_t i = 0; i < QTD_LOGIN_WIFI; i++) {
      if ((strlen(login_wifi[i].ssid) > 3)) {
         return 1;
      }
   }
   return 0;
}

static bool compare_bssid(uint8_t *bssid1, uint8_t *bssid2) {
   for (uint8_t i = 0; i < 6; i++) {
      if (*bssid1 != *bssid2) return 0;
      bssid1++;
      bssid2++;
   }
   return 1;
}

/******************************************************************************
 * Faz a procura de Wifis e compara se existe um Wi-Fi conhecido para conectar.
 ******************************************************************************/
static int wifi_app_start_scan() {
   int id = ID_NULL;
   bool procurar = 1;

   int i = 0, j = 0;
   int count = 0;
   int ret = ESP_FAIL;
   static uint16_t qtd_ap = 0;

   // Pode ser feito apenas na inicialização e alteração de config wifi pelo AP
   // Verifica se as configurações são válidas
   if (!testa_configuracao_wifi()) {
      return ID_NULL;
   }

#ifdef DEBUG_WIFI
   ESP_LOGE(LOG_WIFI, "Iniciando procura.");
#endif
   // Verifica se todos ja foram testados e limpa status de procura, considera o 3G
   count = 0;
   for (i = 0; i < (QTD_LOGIN_WIFI - 1); i++) {
      if (login_wifi[i].falhou_conectar) count++;
      login_wifi[i].status_procura = 0; // limpa status procura
#ifdef DEBUG_WIFI
      ESP_LOGE(LOG_WIFI, "Login[%02d]:'%s','%s',%d,%d,%d,(%02X:%02X:%02X:%02X:%02X:%02X)", i, login_wifi[i].ssid, login_wifi[i].pass, login_wifi[i].status_procura, login_wifi[i].falhou_conectar,
               login_wifi[i].bssid_set, login_wifi[i].bssid[0], login_wifi[i].bssid[1], login_wifi[i].bssid[2], login_wifi[i].bssid[3], login_wifi[i].bssid[4], login_wifi[i].bssid[5]);
#endif
   }

   // Se todos ja testados, limpa pra tentar novamente nos que deram errado
   if (count >= (QTD_LOGIN_WIFI - 1)) {
      procurar = 1;
#ifdef DEBUG_WIFI
      ESP_LOGW(LOG_WIFI, "Limpando a pesquisa");
#endif

      for (i = 0; i < (QTD_LOGIN_WIFI - 1); i++) {
         login_wifi[i].falhou_conectar = 0;
      }

      // Se está habilitado o envio pelo 3G, habilita conexão.
      if (config_http_client.config_envio_web.hab_envio_3g) {
#ifdef DEBUG_WIFI
         ESP_LOGW(LOG_WIFI, "Testando o 3G");
#endif
         id = QTD_LOGIN_WIFI - 1;
         return id;
      }
   }

   // Inicia pesquisa se o mutex estiver liberado
   xSemaphoreTake(mutex_pesquisa_wifi, portMAX_DELAY);

   if (procurar == 1) {
#ifdef DEBUG_WIFI
      ESP_LOGW(LOG_WIFI, "Scan");
#endif
      ret = wifi_scan(&qtd_ap);
      if (ret != ESP_OK) goto fim;
      else
         procurar = 0;
   }

   // -------------------------------------------------------------------------

   // Verifica todos os AP encontrados
   for (j = 0; j < qtd_ap; j++) {
#ifdef DEBUG_WIFI
      ESP_LOGI(LOG_WIFI, "AP%02d:'%s',%d,%d,%d,(%02X:%02X:%02X:%02X:%02X:%02X)", j, lista_ap[j].ssid, lista_ap[j].primary, lista_ap[j].rssi, lista_ap[j].authmode,
               lista_ap[j].bssid[0], lista_ap[j].bssid[1], lista_ap[j].bssid[2], lista_ap[j].bssid[3], lista_ap[j].bssid[4], lista_ap[j].bssid[5]);
#endif
      // Rejeita Wi-Fi sem nome
      if (strlen((char *) lista_ap[j].ssid) < 4) continue;

      // Verifica se existe destre os configurados - 3G também sinaliza que esta na lista.
      for (i = 0; i < (QTD_LOGIN_WIFI - 1); i++) {
         if (strlen((char *) login_wifi[i].ssid) != strlen((char *) lista_ap[j].ssid)) continue;
         if (strstr((char *) lista_ap[j].ssid, (char *) login_wifi[i].ssid) == NULL) continue;

         if (login_wifi[i].bssid_set) {
            if (!compare_bssid((uint8_t *) &lista_ap[j].bssid, (uint8_t *) &login_wifi[i].bssid)) continue;
         }

         login_wifi[i].status_procura = 1;
#ifdef DEBUG_WIFI
         ESP_LOGW(LOG_WIFI, "Login[%02d][%02d]:'%s','%s',%d,%d,%d,(%02X:%02X:%02X:%02X:%02X:%02X)", j, i, login_wifi[i].ssid, login_wifi[i].pass, login_wifi[i].status_procura, login_wifi[i].falhou_conectar,
                  login_wifi[i].bssid_set, login_wifi[i].bssid[0], login_wifi[i].bssid[1], login_wifi[i].bssid[2], login_wifi[i].bssid[3], login_wifi[i].bssid[4], login_wifi[i].bssid[5]);
#endif
      }
   }

   // Se não esta na lista, marca como falhou.
   for (i = 0; i < (QTD_LOGIN_WIFI - 1); i++) {
      if (login_wifi[i].status_procura == 0) login_wifi[i].falhou_conectar = 1;
   }

   // Se encontrado e ainda não falhou ao conectar, tenta conexão.
   for (i = 0; i < (QTD_LOGIN_WIFI - 1); i++) {
      if ((login_wifi[i].falhou_conectar == 0)) {
         if (login_wifi[i].status_procura == 1) {
            id = i;
            goto fim;
         }
         else {
            login_wifi[i].falhou_conectar = 1;
         }
      }
   }
fim:
   xSemaphoreGive(mutex_pesquisa_wifi);
   return id;
}

static void add_mdns_services() {
   // Configurando o hostname da aplicação
   char hostname[60];
   if (strlen(wifi_ap.ssid) < 32) sprintf((char *) hostname, "%s", (char *) wifi_ap.ssid);
   else
      sprintf((char *) hostname, "%s", "tecnnicconnect");

   //initialize mDNS service
   esp_err_t err = mdns_init();
   if (err) {
#ifdef DEBUG_WIFI
      ESP_LOGI(LOG_WIFI, "MDNS Init failed: %d\n", err);
#endif
      return;
   }

   // Seta o hostname
   mdns_hostname_set(hostname);
   mdns_instance_name_set(hostname);
   mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
   mdns_service_instance_name_set("_http", "_tcp", "Tecnnic Connect Web Server");

   esp_netif_set_hostname(app_sta_netif, hostname);
   esp_netif_set_hostname(app_ap_netif, hostname);
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

   // Configuração de hostname e serviços.
   add_mdns_services();

   // Inicializa driver TCP/IP
   config_tcpip_ap_fix();
}

/******************************************************************************
 * Inicia o modulo Wi-Fi do ESP no modo AP-STA.
 *****************************************************************************/
static void wifi_init() {
   app_netif_init();

   // Inicicou a aprte de camada netif, agora em relação ao wifi
   wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
   ESP_ERROR_CHECK(esp_wifi_init(&cfg));

#ifdef DEBUG_WIFI
   ESP_LOGI(LOG_WIFI, "APP config");
#endif

   ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
   wifi_ap_config();
   wifi_sta_config(-1);

   // Configurações gerais de wifi
   ESP_ERROR_CHECK(esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N));
   ESP_ERROR_CHECK(esp_wifi_set_protocol(ESP_IF_WIFI_AP, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N));
   ESP_ERROR_CHECK(esp_wifi_set_bandwidth(ESP_IF_WIFI_STA, WIFI_BW_HT20));
   ESP_ERROR_CHECK(esp_wifi_set_bandwidth(ESP_IF_WIFI_AP, WIFI_BW_HT20));
   ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
   ESP_ERROR_CHECK(esp_wifi_start());
}

/******************************************************************************
 * Função para desconectar do Wi-Fi e desabilitar a reconex?o.
 *****************************************************************************/
static void desconectar_do_wifi() {
   app_wifi_sta.status = DESCONECTADO;
#ifdef DEBUG_WIFI
   ESP_LOGW(LOG_WIFI, "Desconectando do Wifi.");
#endif
   // realtime.link_internet = 0;
   login_wifi[app_wifi_sta.id_scan].falhou_conectar = 1; // Seta que ja tentou conectar
   app_wifi_sta.id_connected = ID_NULL;
   app_wifi_sta.id_scan = ID_NULL;
   esp_wifi_disconnect();
}

/******************************************************************************
 * Função deve ser adicionada ao um timer de 1ms para contar corretamente.
 *****************************************************************************/
static void IRAM_ATTR wifi_timer_ms() {
   if (app_wifi_sta.retri_scan_timeout) app_wifi_sta.retri_scan_timeout--;
   if ((app_wifi_sta.status == CONECTANDO) && (wifi_conectar_ao_ap_timeout)) wifi_conectar_ao_ap_timeout--;
   if ((esp_restart_now) && (esp_restart_timeout > 0)) esp_restart_timeout--;
   if (wifi_retri_scan_timeout_3G) wifi_retri_scan_timeout_3G--;
   if (sys_time_loop_timeout) sys_time_loop_timeout--;
}

static volatile int conta_tentativas_conexao_ap = 0;

static void ciclo_desconectado() {
   int id = ID_NULL;
   int ret = ESP_FAIL;

   // A cada periodo de tempo, procura o wifi
   if (!app_wifi_sta.retri_scan_timeout) {
      app_wifi_sta.status = PROCURANDO_WIFI;

      // Se a estação nao está tentando conectar no roteador
      if (!sta_conectando_wifi) {
#ifdef DEBUG_HEAP
         print_heap("Wifi Scan init");
#endif
         id = wifi_app_start_scan();
#ifdef DEBUG_HEAP
         print_heap("Wifi Scan fim");
#endif
         if (id != ID_NULL) {
            ret = wifi_sta_config(id); // Configura o Wi-Fi e tenta conectar
            if (ret == ESP_OK) {
               wifi_conectar_ao_ap_timeout = WIFI_TEMPO_CONECTAR_AO_AP;
               app_wifi_sta.status = CONECTANDO;
               ret = esp_wifi_connect();
            }
            if (ret == ESP_OK) {
               sta_conectando_wifi = 1;
#ifdef DEBUG_WIFI
               ESP_LOGI(LOG_WIFI, "Conectando no Rot.");
#endif
            }
            else
               sta_conectando_wifi = 0;
         }
         else {
            // Não encontrou na pesquisa, recarrega tempos
            app_wifi_sta.status = DESCONECTADO;
#ifdef DEBUG_WIFI
            ESP_LOGI(LOG_WIFI, "Não encontrou Rot.");
#endif
            // Se falhou 5 vezes seguidas, fica sem procurar por um tempo maior
            if (++conta_tentativas_conexao_ap >= WIFI_QUANTIDADE_DE_PROCURAS) {
               app_wifi_sta.retri_scan_timeout = WIFI_TEMPO_RETORNO_DE_PROCURA;
               conta_tentativas_conexao_ap = 0;
            }
            else {
               app_wifi_sta.retri_scan_timeout = WIFI_RETRI_SCAN_TIMEOUT_AP;
            }
#ifdef DEBUG_WIFI
            ESP_LOGI(LOG_WIFI, "Aguardando %d ms.", app_wifi_sta.retri_scan_timeout);
#endif
         }
      }
   }
}

static void ciclo_conectado() {
   int id = ID_NULL;
   static int procurar_wifi = 0;
   // Limpa contagem de tentativas ao conectar, pra iniciar a procura com qtd 0.
   conta_tentativas_conexao_ap = 0;
   EventBits_t event_group = xEventGroupGetBits(app_event_group);

   // Se está no 3G
   if (app_wifi_sta.id_connected == (QTD_LOGIN_WIFI - 1)) {
      if (app_wifi_sta.link_wifi_web) {
         controle_reset_3g.conectado_sem_rede_timeout = 300000; // 5 min
      }
      else if ((!controle_reset_3g.conectado_sem_rede_timeout) && (controle_reset_3g.habilita_reset)) {
#ifdef DEBUG_WIFI
         ESP_LOGW(LOG_WIFI, "Reset do 3G por timeout em inatividade");
#endif
         registrar_ev.falha_link_modulo_3G = 1; // Gravar Evento
         controle_reset_3g.bloqueio_reset_3g_timeout_ms = TEMPO_BLOQUEIO_RESET_3G;
         controle_reset_3g.pulso_timeout_ms = TEMPO_PULSO_RESET; // Duração do pulso 1000 ms
         controle_reset_3g.contagem_erro = 0;

         // Desconecta do wi-fi porque está em erro
         desconectar_do_wifi();
         app_wifi_sta.retri_scan_timeout = 0;
      }

      // Se não esta enviando dados ao servidor e deu timeout pra procurar, procura wi-fi
      if ((!wifi_retri_scan_timeout_3G) && ((event_group & ENVIAR_HTTP) == 0)) procurar_wifi = 1;
   }

   // Procura de wifi, se não estiver em envio http
   if ((procurar_wifi) && ((event_group & ENVIAR_HTTP) == 0)) {
      procurar_wifi = 0;
#ifdef DEBUG_WIFI
      ESP_LOGW(LOG_WIFI, "Conectado em [%d] e procurando wi-fi.", app_wifi_sta.id_connected);
#endif
      // Sinaliza para não enviar pacote pela tarefa no core 2.
      app_wifi_sta.status = CONECTADO_E_PROCURANDO_WIFI;
#ifdef DEBUG_HEAP
      print_heap("Wifi Con scan ini");
#endif
      id = wifi_app_start_scan();
#ifdef DEBUG_HEAP
      print_heap("Wifi Con scan fim");
#endif
      // Encontrou e não é o 3G
      if ((id != ID_NULL) && (id != QTD_LOGIN_WIFI - 1)) {
#ifdef DEBUG_WIFI
         ESP_LOGW(LOG_WIFI, "Novo Wi-Fi encontrado-> %d", id);
#endif
         app_wifi_sta.retri_scan_timeout = 0;

         // Desconecta do wi-fi atual
         desconectar_do_wifi();

         // Conecta no wi-fi encontrado
         int ret = wifi_sta_config(id); // Configura o Wi-Fi e tenta conectar
         if (ret == ESP_OK) {
            wifi_conectar_ao_ap_timeout = WIFI_TEMPO_CONECTAR_AO_AP;
            app_wifi_sta.status = CONECTANDO;
            ret = esp_wifi_connect();
         }
         if (ret == ESP_OK) {
            sta_conectando_wifi = 1;
#ifdef DEBUG_WIFI
            ESP_LOGI(LOG_WIFI, "Conectando no Rot.");
#endif
         }
         else
            sta_conectando_wifi = 0;
      }
      else {
         // Não encontrou nenhum wi-fi para tentar conectar.
#ifdef DEBUG_WIFI
         ESP_LOGW(LOG_WIFI, "Não encontrou nenhum wi-fi válido.");
#endif
         // Se está no 3G recarrega tempo para buscar novamente
         if (app_wifi_sta.id_connected == (QTD_LOGIN_WIFI - 1)) {
            wifi_retri_scan_timeout_3G = WIFI_RETRI_SCAN_TIMEOUT_AP_3G;
#ifdef DEBUG_WIFI
            ESP_LOGW(LOG_WIFI, "No 3G, nova pesquisa em %d ms.", wifi_retri_scan_timeout_3G);
#endif
         }
      }
      app_wifi_sta.status = CONECTADO;
   }
}

/******************************************************************************
 * Função que verifique periodicamente os status do wifi e tarfas relacionadas.
 *****************************************************************************/
static void wifi_app_control(void *pvParameters) {
   // Inicia variaveis do wifi
   app_wifi_sta.retri_scan_timeout = 0; // WIFI_RETRI_SCAN_TIMEOUT_AP;
   app_wifi_sta.tempo_scan = WIFI_TEMPO_SCAN_AP_OFF;
   app_wifi_sta.status = DESABILITADO;
   mutex_pesquisa_wifi = xSemaphoreCreateMutex();

   xEventGroupWaitBits(app_event_group, LINK_COM_PIC, pdFALSE, pdFALSE, 1500 / portTICK_PERIOD_MS);

#ifdef DEBUG_WIFI
   ESP_LOGI(LOG_WIFI, "wifi_app_control init.");
   if (mutex_pesquisa_wifi != NULL) {
      ESP_LOGI(LOG_WIFI, "mutex ok");
   }
#endif

   wifi_init(); // Inicia o Wi-Fi
   esp_wifi_get_mac(ESP_IF_WIFI_STA, mac_address);
   app_sntp_init();
   start_webserver();

   while (1) {
#ifdef DEBUG_HEAP
      print_heap("wifi loop ini");
#endif
      switch (app_wifi_sta.status) {
         case DESCONECTADO:
         case DESABILITADO:
            ciclo_desconectado();
            break;
         case CONECTANDO:
            if (!wifi_conectar_ao_ap_timeout) {
               EventBits_t event_group = xEventGroupGetBits(app_event_group);
               if ((app_wifi_sta.id_connected == (QTD_LOGIN_WIFI - 1)) && (!wifi_retri_scan_timeout_3G) && ((event_group & ENVIAR_HTTP) == 0)) {
                  desconectar_do_wifi();
               }
            }
            break;
         case CONECTADO:
            ciclo_conectado();
            break;
         default:
            break;
      }

      // Ciclo aguardando IP
      if (app_wifi_sta.status == AGUARDANDO_IP) {
         if (!wifi_aguardando_ip_timeout) {
#ifdef DEBUG_WIFI
            ESP_LOGE(LOG_WIFI, "Não recebeu IP, desconectando.");
#endif
            desconectar_do_wifi();
         }
      }
      else {
         wifi_aguardando_ip_timeout = WIFI_TEMPO_AGUARDANDO_IP;
      }

      if ((esp_restart_now) && (esp_restart_timeout == 0)) {
         esp_restart_now = 0;
#ifdef DEBUG_WIFI
         ESP_LOGW(LOG_WIFI, "esp restart");
#endif
         desconectar_do_wifi();
         esp_wifi_stop();

         // Trava para não gravar eentos enquanto vai reiniar.
         xSemaphoreTake(mutex_nvs, portMAX_DELAY);
         fflush(stdout);
         esp_restart();
      }

      app_sys_time_loop();

      vTaskDelay(1000 / portTICK_PERIOD_MS);
#ifdef DEBUG_HEAP
      print_heap("wifi loop fim");
#endif
   }
}

// Inicia tarefa de cntole do Wi-Fi .
static void wifi_app_init() {
   xTaskCreatePinnedToCore(wifi_app_control, "wifi_app_control", 4000, NULL, 1, &taskhandle_wifi_app_control, 0);
   xTaskCreatePinnedToCore(task_wifi_http_send, "task_http_send", 18000, /*&tipo_request*/ NULL, 2, &taskHandle_wifi_http_send, 0); ///* uxTaskPriorityGet(NULL)*/
}