//  Procura apenas as chaves de eventos, o iterador retorna o ultimo alterado, logo é o ultimo indicesalvo.
static esp_err_t nvs_app_init_config_events() {
#ifdef DEBUG_NVS
   ESP_LOGI(LOG_NVS, "ini");
#endif
   xSemaphoreTake(mutex_nvs, portMAX_DELAY);
   nvs_iterator_t it = nvs_entry_find(report, report, NVS_TYPE_BLOB);
   uint16_t qtd_ev = 0;
   nvs_entry_info_t info;
   char buffer[16];
   bzero(buffer, 15);

   while (it != NULL) {
      // err = ESP_OK;
      nvs_entry_info(it, &info);
      it = nvs_entry_next(it);
      qtd_ev++;
      if (it == NULL) {
         sprintf(buffer, "%s", info.key);
         break;
      }
   };
   nvs_release_iterator(it); // Libera o iterador

   // Precisa desta proteção, pois string nula egra numero 0, então sempre pularia o evento 0.
   if (strlen(buffer) > 0) {
      if (qtd_ev > 0) {
         eventos_config.index_ev = (atol(buffer) + 1) % QUANTIDADE_MAX_EVENTOS; // Converter
      }
      else {
         eventos_config.index_ev = 0;
      }
   }
   else {
      eventos_config.index_ev = 0;
   }
   eventos_config.qtd_ev = qtd_ev;

#ifdef DEBUG_NVS
   ESP_LOGI(LOG_NVS, "qtd_ev:%u, index:%u, key:'%s'", eventos_config.qtd_ev, eventos_config.index_ev, buffer);
#endif

   xSemaphoreGive(mutex_nvs);
   return ESP_OK;
}

static esp_err_t app_nvs_init() {
   mutex_nvs = xSemaphoreCreateMutex();
   esp_err_t err;
   // Configurações da aplicação ----------------------------------------------
   err = nvs_flash_init();
   ESP_ERROR_CHECK(err);
#ifdef DEBUG_NVS
   nvs_stats_t nvs_stats;
   err = nvs_get_stats(memoria, &nvs_stats);
   ESP_LOGI(LOG_NVS, "P:%s", memoria);
   ESP_LOGI(LOG_NVS, "Count: err = (%d)", err);
   ESP_LOGI(LOG_NVS, "UsedEntries = (%d)", nvs_stats.used_entries);
   ESP_LOGI(LOG_NVS, "FreeEntries = (%d)", nvs_stats.free_entries);
   ESP_LOGI(LOG_NVS, "AllEntries = (%d)", nvs_stats.total_entries);
#endif
   // Eventos -----------------------------------------------------------------
   err = nvs_flash_init_partition(report);
   ESP_ERROR_CHECK(err);
#ifdef DEBUG_NVS
   err = nvs_get_stats(report, &nvs_stats);
   ESP_LOGI(LOG_NVS, "P:%s", report);
   ESP_LOGI(LOG_NVS, "Count: err = (%d)", err);
   ESP_LOGI(LOG_NVS, "UsedEntries = (%d)", nvs_stats.used_entries);
   ESP_LOGI(LOG_NVS, "FreeEntries = (%d)", nvs_stats.free_entries);
   ESP_LOGI(LOG_NVS, "AllEntries = (%d)", nvs_stats.total_entries);
#endif

   return err;
}

static esp_err_t nvs_app_read_config_wifi() {
   xSemaphoreTake(mutex_nvs, portMAX_DELAY);
   nvs_handle_t nvs_handle;
   esp_err_t err;
   err = nvs_open(memoria, NVS_READONLY, &nvs_handle); // Abrir
   if (err != ESP_OK)
      goto fim;

   size_t size = sizeof(login_wifi);
   err = nvs_get_blob(nvs_handle, "login_wifi", &login_wifi, &size); // Ler

#ifdef DEBUG_NVS
   for (int i = 0; i < QTD_LOGIN_WIFI; i++) {
      ESP_LOGI(LOG_NVS, "[%d]:SSID:'%s',PASS:'%s',IP:%d.%d.%d.%d",
               i, login_wifi[i].ssid, login_wifi[i].pass,
               MAKE8(login_wifi[i].net_ip4.ip, 0),
               MAKE8(login_wifi[i].net_ip4.ip, 1),
               MAKE8(login_wifi[i].net_ip4.ip, 2),
               MAKE8(login_wifi[i].net_ip4.ip, 3));
   }
#endif

fim:
   nvs_close(nvs_handle); // Fechar
   xSemaphoreGive(mutex_nvs);
   return err;
}

static esp_err_t nvs_app_write_config_module() {
   xSemaphoreTake(mutex_nvs, portMAX_DELAY);
   nvs_handle_t nvs_handle;
   esp_err_t err;

   err = nvs_open(memoria, NVS_READWRITE, &nvs_handle); // Abrir
   if (err != ESP_OK)
      goto fim;

   err = nvs_set_blob(nvs_handle, "modulo", &modulo, sizeof(modulo)); // seta valores
   if (err != ESP_OK)
      goto fim;

   err = nvs_commit(nvs_handle); // Publica

fim:
   nvs_close(nvs_handle); // Fechar
   xSemaphoreGive(mutex_nvs);
#ifdef DEBUG_NVS
   ESP_LOGI(LOG_NVS,"Gravou config com status=>%d", err);
#endif
   return err;
}

static esp_err_t nvs_app_write_config_block_device() {
   xSemaphoreTake(mutex_nvs, portMAX_DELAY);
   nvs_handle_t nvs_handle;
   esp_err_t err;

   err = nvs_open(memoria, NVS_READWRITE, &nvs_handle); // Abrir
   if (err != ESP_OK)
      goto fim;

   err = nvs_set_blob(nvs_handle, "block_device", &block_device, sizeof(block_device)); // seta valores
   if (err != ESP_OK)
      goto fim;

   err = nvs_commit(nvs_handle); // Publica

fim:
   nvs_close(nvs_handle); // Fechar
   xSemaphoreGive(mutex_nvs);
#ifdef DEBUG_NVS
   ESP_LOGI(LOG_NVS, "Gravou config com status=>%d", err);
#endif
   return err;
}

static esp_err_t nvs_app_write_config_http_client() {
   xSemaphoreTake(mutex_nvs, portMAX_DELAY);
   nvs_handle_t nvs_handle;
   esp_err_t err;

   err = nvs_open(memoria, NVS_READWRITE, &nvs_handle); // Abrir
   if (err != ESP_OK)
      goto fim;

   err = nvs_set_blob(nvs_handle, "http_client", &config_http_client, sizeof(config_http_client)); // seta valores
   if (err != ESP_OK)
      goto fim;

   err = nvs_commit(nvs_handle); // Publica

fim:
   nvs_close(nvs_handle); // Fechar
   xSemaphoreGive(mutex_nvs);
   return err;
}

static esp_err_t nvs_app_write_config_wifi() {
   xSemaphoreTake(mutex_nvs, portMAX_DELAY);
   nvs_handle_t nvs_handle;
   esp_err_t err;

   err = nvs_open(memoria, NVS_READWRITE, &nvs_handle); // Abrir
   if (err != ESP_OK)
      goto fim;

   err = nvs_set_blob(nvs_handle, "login_wifi", &login_wifi, sizeof(login_wifi));
   if (err != ESP_OK)
      goto fim;

   err = nvs_commit(nvs_handle); // Garante que gravou corretamente*/

fim:
   nvs_close(nvs_handle); // Fechar
   xSemaphoreGive(mutex_nvs);
   return err;
}

static esp_err_t carrega_configuracao_default(uint32_t versao) {
   if ((default_wifi) || (versao == 0))  {
#ifdef DEBUG_NVS
      ESP_LOGI(LOG_NVS,"Carregou as configuraçoes default_wifi");
#endif
      memset(&login_wifi, 0, sizeof(login_wifi_t));
      sprintf((char *) login_wifi[0].ssid, "TECNNIC1");
      sprintf((char *) login_wifi[0].pass, "tecnnicnet");
      login_wifi[0].ip4_fix = 1;
      login_wifi[0].net_ip4.ip = MAKE32(192, 168, 1, 240);
      login_wifi[0].net_ip4.gw = MAKE32(192, 168, 1, 1);
      login_wifi[0].net_ip4.netmask = MAKE32(255, 255, 255, 0);
      login_wifi[0].net_ip4.dns1 = MAKE32(8, 8, 8, 8);
      login_wifi[0].net_ip4.dns2 = MAKE32(4, 4, 4, 4);
      }

   if ((default_config) || (versao == 0)) {
#ifdef DEBUG_NVS
      ESP_LOGI(LOG_NVS,"Carregou as configuraçoes default_config");
#endif
      sprintf(modulo.nome, "%s", NOME_DISPOSITIVO);
      sprintf(modulo.passwifi, "%s", PASSWIFI);
      sprintf(modulo.numero_serie, "%s", NUMERO_SERIE);
      sprintf(modulo.web_key, "%s", WEB_KEY);
      sprintf(modulo.passconfig, "%s", PASSCONFIG);
      modulo.enable_sync_time = ENABLE_SYNC_TIME;
      // sprintf(modulo.versao_protocol, "%s", VERSAO_PROTOCOLO);
   }

   if ((default_server)|| (versao == 0))  {
#ifdef DEBUG_NVS
      ESP_LOGI(LOG_NVS,"Carregou as configuraçoes default_server");
#endif
      sprintf(config_http_client.host, "%s", HOST_API);
      sprintf(config_http_client.url_fixa, "%s", HOST_FIXO_URL_JSON);
      sprintf(config_http_client.host_fixo, "%s", HOST_FIXO);
      config_http_client.habilita_host_fixo = HABILITA_HOST_FIXO;
      config_http_client.habilita_envio_http = HABILITA_ENVIO_HTTP;
      config_http_client.tempo_registro = TEMPO_REGISTRO;
      config_http_client.config_envio_web.hab_envio_3g = HAB_ENVIO_3G;
      config_http_client.config_envio_web.hab_envio_wifi = HAB_ENVIO_WIFI;
      config_http_client.config_envio_web.tempo_envio_3g = TEMPO_ENVIO_3G;
      config_http_client.config_envio_web.tempo_envio_3g_user_on = TEMPO_ENVIO_3G_USER_ON;
      config_http_client.config_envio_web.tempo_envio_wifi = TEMPO_ENVIO_WIFI;
      config_http_client.config_envio_web.tempo_envio_wifi_user_on = TEMPO_ENVIO_WIFI_USER_ON;
      }

   modulo.versao_firmware = VERSAO_FIRMWARE;
   
   
   // Configurações de bloqueio.
   block_device.level = 0;
   block_device.dateRelease = 0;

   xSemaphoreTake(mutex_nvs, portMAX_DELAY);
   nvs_handle_t nvs_handle;
   esp_err_t err;

   err = nvs_open(memoria, NVS_READWRITE, &nvs_handle); // Abrir
   if (err != ESP_OK)
      goto fim;

   err = nvs_set_blob(nvs_handle, "modulo", &modulo, sizeof(modulo));
#ifdef DEBUG_NVS
   if (err == ESP_OK)
      ESP_LOGI(LOG_NVS, "Gravou as configurações do módulo.");
   else
      ESP_LOGE(LOG_NVS, "Erro as gravar as configurações do módulo.");
#endif

   err = nvs_set_blob(nvs_handle, "block_device", &block_device, sizeof(block_device));
#ifdef DEBUG_NVS
   if (err == ESP_OK)
      ESP_LOGI(LOG_NVS, "Gravou as configurações de bloqueio do device");
   else
      ESP_LOGE(LOG_NVS, "Erro as gravar as configurações de bloqueio do device.");
#endif

   err = nvs_set_blob(nvs_handle, "http_client", &config_http_client, sizeof(config_http_client));
#ifdef DEBUG_NVS
   if (err == ESP_OK)
      ESP_LOGI(LOG_NVS, "Gravou as configurações do Wi-Fi.");
   else
      ESP_LOGE(LOG_NVS, "Erro as gravar as configurações do http_client.");
#endif

   err = nvs_set_blob(nvs_handle, "login_wifi", &login_wifi, sizeof(login_wifi));
#ifdef DEBUG_NVS
   if (err == ESP_OK)
      ESP_LOGI(LOG_NVS, "Gravou as configurações do Wi-Fi.");
   else
      ESP_LOGE(LOG_NVS, "Erro as gravar as configurações do Wi-Fi.");
#endif

   err = nvs_commit(nvs_handle); // Garante que gravou corretamente
#ifdef DEBUG_NVS
   if (err == ESP_OK)
      ESP_LOGI(LOG_NVS, "Gravado com sucesso");
   else
      ESP_LOGE(LOG_NVS, "Erro ao gravar na NVS.");
#endif
fim:
   nvs_close(nvs_handle); // Fechar
   xSemaphoreGive(mutex_nvs);
   return err;
}

// Leitura NVS ----------------------------------------------------------------
static esp_err_t nvs_app_read_module() {
   xSemaphoreTake(mutex_nvs, portMAX_DELAY);
   nvs_handle_t nvs_handle;
   esp_err_t err;

   err = nvs_open(memoria, NVS_READONLY, &nvs_handle); // Abrir
   if (err != ESP_OK)
      goto fim;

   size_t size = sizeof(modulo);
   err = nvs_get_blob(nvs_handle, "modulo", &modulo, &size);

#ifdef DEBUG_NVS
   ESP_LOGI(LOG_NVS, "Passwifi: %s", modulo.passwifi);
   ESP_LOGI(LOG_NVS, "Nome: %s", modulo.nome);
   ESP_LOGI(LOG_NVS, "Numero_serie: %s", modulo.numero_serie);
   ESP_LOGI(LOG_NVS, "Passconfig: %s", modulo.passconfig);
   // ESP_LOGI(LOG_NVS, "Versao_protocol: %s", modulo.versao_protocol);
#endif

fim:
   nvs_close(nvs_handle); // Fechar
   xSemaphoreGive(mutex_nvs);

   // Reset inicial -----------------------------------------------------------
#ifdef DEBUG_NVS
   ESP_LOGI(LOG_NVS,"versao def : %d, versao mem: %d", VERSAO_FIRMWARE, modulo.versao_firmware);
#endif
   if (modulo.versao_firmware < VERSAO_FIRMWARE) {
      err = carrega_configuracao_default(modulo.versao_firmware);
   }

   return err;
}

static esp_err_t nvs_app_read_config_block_device() {
   xSemaphoreTake(mutex_nvs, portMAX_DELAY);
   nvs_handle_t nvs_handle;
   esp_err_t err;

   err = nvs_open(memoria, NVS_READONLY, &nvs_handle); // Abrir
   if (err != ESP_OK)
      goto fim;

   size_t size = sizeof(block_device);
   err = nvs_get_blob(nvs_handle, "block_device", &block_device, &size);

fim:
   nvs_close(nvs_handle); // Fechar
   xSemaphoreGive(mutex_nvs);

   return err;
}

static esp_err_t nvs_app_read_config_http_client() {
   xSemaphoreTake(mutex_nvs, portMAX_DELAY);
   nvs_handle_t nvs_handle;
   esp_err_t err;

   err = nvs_open(memoria, NVS_READONLY, &nvs_handle); // Abrir
   if (err != ESP_OK)
      goto fim;

   size_t size = sizeof(config_http_client);
   err = nvs_get_blob(nvs_handle, "http_client", &config_http_client, &size);

#ifdef DEBUG_NVS
   ESP_LOGI(LOG_NVS, "host: %s", config_http_client.host);
   ESP_LOGI(LOG_NVS, "url_fixa: %s", config_http_client.url_fixa);
   ESP_LOGI(LOG_NVS, "host_fixo: %s", config_http_client.host_fixo);
   ESP_LOGI(LOG_NVS, "habilita_envio_http: %d", config_http_client.habilita_envio_http);
   ESP_LOGI(LOG_NVS, "habilita_host_fixo: %d", config_http_client.habilita_host_fixo);
#endif

fim:
   nvs_close(nvs_handle); // Fechar
   xSemaphoreGive(mutex_nvs);

   // // Reset inicial -----------------------------------------------------------
   // if (strstr(modulo.versao_sw, "1.0.00") == NULL) { // Pendente alterar
   //    err = carrega_configuracao_default();
   // }

   return err;
}

static esp_err_t nvs_app_read_event(event_t *ev, int end) {
   xSemaphoreTake(mutex_nvs, portMAX_DELAY);
   nvs_handle_t nvs_handle;
   esp_err_t err;

   err = nvs_open_from_partition(report, report, NVS_READONLY, &nvs_handle);
   if (err != ESP_OK)
      goto fim;

   // Endereco do evento
   char key[15] = "";

   sprintf(key, "%05d", end % QUANTIDADE_MAX_EVENTOS); // Não deixa passar do limite de eventos

   event_t ev_temp;
   size_t size = sizeof(event_t);
   err = nvs_get_blob(nvs_handle, key, &ev_temp, &size);

   if (err != ESP_OK)
      goto fim;

   *ev = ev_temp;

fim:
   nvs_close(nvs_handle); // Fechar
   xSemaphoreGive(mutex_nvs);
   return err;
}

static esp_err_t nvs_app_write_event(event_t *ev) {
   xSemaphoreTake(mutex_nvs, portMAX_DELAY);
   nvs_handle_t nvs_handle;
   esp_err_t err;

   err = nvs_open_from_partition(report, report, NVS_READWRITE, &nvs_handle);
   if (err != ESP_OK)
      goto fim;

   // Endereco do evento
   char key[15] = "";
   int end = eventos_config.index_ev % QUANTIDADE_MAX_EVENTOS;
   sprintf(key, "%05d", end);
   ev->pos = end;
   err = nvs_set_blob(nvs_handle, key, ev, sizeof(event_t));
   if (err != ESP_OK)
      goto fim;

   // Estouro de evento volta pro 0
   end++;
   end %= QUANTIDADE_MAX_EVENTOS;

   sprintf(key, "%05d", end);
   err = nvs_erase_key(nvs_handle, key);

   if ((err != ESP_OK) && (err != ESP_ERR_NVS_NOT_FOUND))
      goto fim;

   err = nvs_commit(nvs_handle); // Garante que gravou corretamente*/

   // Incrementa contador de eventos e verifica estouro para voltar pro 0.
   if (err == ESP_OK) {
#ifdef DEBUG_NVS
      ESP_LOGW(LOG_NVS, "Gravou Evento: id:%d, tipo:%d", eventos_config.index_ev, ev->pic.tipo);
#endif
      eventos_config.index_ev++;
      if (eventos_config.index_ev >= QUANTIDADE_MAX_EVENTOS) {
         eventos_config.index_ev = 0;
      }

      if (eventos_config.qtd_ev < QUANTIDADE_MAX_EVENTOS - 1) { // porque sempre o limpo o evento da frente para encontrar o fim do relatorio.
         eventos_config.qtd_ev++;
      }
   }
fim:

   nvs_close(nvs_handle); // Fechar
   xSemaphoreGive(mutex_nvs);
   return err;
}

static esp_err_t nvs_app_clear_report() {
   xSemaphoreTake(mutex_nvs, portMAX_DELAY);
   nvs_handle_t nvs_handle;
   esp_err_t err;

   err = nvs_open_from_partition(report, report, NVS_READWRITE, &nvs_handle);
   if (err != ESP_OK)
      goto fim;

   err = nvs_erase_all(nvs_handle);
   if (err != ESP_OK)
      goto fim;

   err = nvs_commit(nvs_handle); // Garante que gravou corretamente*/

   nvs_close(nvs_handle); // Fechar
   xSemaphoreGive(mutex_nvs);
   return ESP_OK;
fim:
   nvs_close(nvs_handle); // Fechar
   xSemaphoreGive(mutex_nvs);
   return ESP_FAIL;
}

static esp_err_t nvs_app_read_config_events(eventos_config_t *ev_config) {
   xSemaphoreTake(mutex_nvs, portMAX_DELAY);
   ev_config->index_ev = eventos_config.index_ev;
   ev_config->qtd_ev = eventos_config.qtd_ev;
   xSemaphoreGive(mutex_nvs);
   return ESP_OK;
}