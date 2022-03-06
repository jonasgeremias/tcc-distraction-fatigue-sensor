// Metodo OPTIONS -------------------------------------------------------------
static esp_err_t http_handle_options(httpd_req_t *req) {
   httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
   httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "X-Test, X-Requested-With, Content-Type, Accept, Origin, Authorization");
   httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
   httpd_resp_send_chunk(req, NULL, 0);
   return ESP_OK;
}

// Método GET -----------------------------------------------------------------
static esp_err_t http_handle_get(httpd_req_t *req) {
   static_content_t *content = NULL;

   char response[1500] = "";
#ifdef DEBUG_HEAP
   print_heap("AP Get");
#endif
#ifdef DEBUG_AP_SERVER
   ESP_LOGW(LOG_SERVER, "Req->%s", req->uri);
#endif
   for (uint32_t i = 0; i < ARRAY_SIZE_OF(content_list); i++) {
      // Existe a rota
      if (strstr(req->uri, content_list[i].path) != NULL) {
         // Os textos são do mesmo tamanho
         if (strlen(req->uri) == strlen(content_list[i].path)) {
            // Conteudo não é nulo
            if (content_list[i].data_start != NULL) {
               content = &(content_list[i]);
               break;
            }
         }
      }
   }

   // Se foi alguma requisição de arquivos estáticos
   if (content != NULL) {
      // Executando a função caso exista.
      if (content->function != NULL) {
         content->function();
      }

      ESP_ERROR_CHECK(httpd_resp_set_type(req, content->content_type));
      if (content->is_gzip) {
         ESP_ERROR_CHECK(httpd_resp_set_hdr(req, "Content-Encoding", "gzip"));
      }
      httpd_resp_send_chunk(req, (const char *) content->data_start, content->data_end - content->data_start);
      httpd_resp_send_chunk(req, NULL, 0);
      return ESP_OK;
   }

   // Responde os json --------------------------------------------------------
   if ((strstr(req->uri, "/uploadconfigwifi") != NULL) && (strlen(req->uri) == 17)) { // JSON configuração Wifi
      gera_json_configwifi((char *) &response);
      httpd_resp_set_type(req, "application/json");
      httpd_resp_sendstr(req, response);
      return ESP_OK;
   }
   else if ((strstr(req->uri, "/uploadscanwifi") != NULL) && (strlen(req->uri) == 15)) { // Escanear Wi-Fi
      httpd_resp_set_type(req, "application/json");
      ap_gera_json_scanwifi((char *) &response);
      httpd_resp_sendstr(req, response);
      return ESP_OK;
   }
   else if ((strstr(req->uri, "/uploadrealtimefront") != NULL) && (strlen(req->uri) == 20)) {
      httpd_resp_set_type(req, "application/json");
      ap_get_realtimejson((char *) &response);
      httpd_resp_sendstr(req, response);
      return ESP_OK;
   }
   else if ((strstr(req->uri, "/uploadrealtime") != NULL) && (strlen(req->uri) == 15)) {
      httpd_resp_set_type(req, "application/json");
      ap_get_realtime((char *) &response);
      httpd_resp_sendstr(req, response);
      return ESP_OK;
   }
   else if ((strstr(req->uri, "/uploadconfigaccess") != NULL) && (strlen(req->uri) == 19)) {
      httpd_resp_set_type(req, "application/json");
      ap_get_configaccess((char *) &response);
      httpd_resp_sendstr(req, response);
      return ESP_OK;
   }
   else if ((strstr(req->uri, "/restartmodule") != NULL) && (strlen(req->uri) == 14)) {
      httpd_resp_set_type(req, "application/json");
      sprintf(response, "{\"AM\":0}");
      httpd_resp_sendstr(req, response);
      set_restart_module();
      return ESP_OK;
   }
   else if ((strstr(req->uri, "/otastatus") != NULL) && (strlen(req->uri) == 10)) {
#ifdef DEBUG_AP_OTA
      ESP_LOGI(LOG_OTA, "otastatus");
#endif
      sprintf(response, "{\"status\":%d,\"compile_time\":\"%s\",\"compile_date\":\"%s\"}", status_update_ota, __TIME__, __DATE__);
      httpd_resp_sendstr(req, response);
      return ESP_OK;
   }

   // Se chegou até aqui, nao existe a rota.
   httpd_resp_send_404(req);
   return ESP_OK;
}

static int validateBssid(char *bssid_string, uint8_t *bssid) {
   uint8_t bssid_temp[6];
   int ret = sscanf(bssid_string, "%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX", &bssid_temp[0], &bssid_temp[1], &bssid_temp[2], &bssid_temp[3], &bssid_temp[4], &bssid_temp[5]);

   if (ret != 6)
      return CONFIG_ERROR_DATA_INVALID;

   for (int i = 0; i < 6; i++) {
      *bssid = bssid_temp[i];
      bssid++;
   }

   return CONFIG_OK;
}

static int validateIPaddress(char *ip_char, uint32_t *ip) {
   int a, b, c, d;
   int ret = sscanf(ip_char, "%d.%d.%d.%d", &a, &b, &c, &d);
   if (ret != 4)
      return CONFIG_ERROR_DATA_INVALID;
   *ip = MAKE32(a, b, c, d);
   return CONFIG_OK;
}

static int get_config_web_server(char *struct_json, int length) {
   cJSON *json_cmd = cJSON_Parse(struct_json);
   cod_error_response_t ret = CONFIG_OK;
   char temp[64];
   char dica[5];
   bool pass_tecnnic = 0;

   if (cJSON_IsObject(json_cmd)) {
      strcpy(temp, cJSON_GetObjectItem(json_cmd, "DX")->valuestring);
      if (strlen(temp) == 4) {
         strcpy(dica, temp);
      }
      else {
         bzero(dica, 5);
      }

      strcpy(temp, cJSON_GetObjectItem(json_cmd, "DB")->valuestring);

      // Verifica senha -------------------------------------------------------
      if (testa_senha_dinamica((char *) &temp, (char *) &dica) == ESP_OK) pass_tecnnic = 1;
      if (pass_tecnnic) {
         ret = CONFIG_OK;
      }
      else
         ret = CONFIG_ERROR_PASS_IN;
   }
   else
      ret = CONFIG_ERROR_DATA_INVALID;

   if (json_cmd != NULL) cJSON_Delete(json_cmd);

   if (ret == CONFIG_OK) {
      bzero(struct_json, length);
      char *p = (char *) struct_json;
      p += sprintf(p, "{\"AM\":0,");
      p += sprintf(p, "\"AB\":{");
      p += sprintf(p, "\"FS\":%d,", config_http_client.habilita_envio_http);
      p += sprintf(p, "\"FX\":%d,", config_http_client.habilita_host_fixo);
      p += sprintf(p, "\"GA\":\"%s\",", config_http_client.host);
      p += sprintf(p, "\"FZ\":\"%s\",", config_http_client.url_fixa);
      p += sprintf(p, "\"FY\":\"%s\",", config_http_client.host_fixo);
      p += sprintf(p, "\"FQ\":%d,", config_http_client.tempo_registro);
      p += sprintf(p, "\"GC\":%d,", config_http_client.config_envio_web.hab_envio_3g);
      p += sprintf(p, "\"GB\":%d,", config_http_client.config_envio_web.hab_envio_wifi);
      p += sprintf(p, "\"FT\":%d,", config_http_client.config_envio_web.tempo_envio_3g);
      p += sprintf(p, "\"FV\":%d,", config_http_client.config_envio_web.tempo_envio_3g_user_on);
      p += sprintf(p, "\"FU\":%d,", config_http_client.config_envio_web.tempo_envio_wifi);
      p += sprintf(p, "\"FW\":%d}}", config_http_client.config_envio_web.tempo_envio_wifi_user_on);
   }
   return ret;
}

static int set_config_web_server(char *struct_json) {
   cJSON *json_config = cJSON_Parse(struct_json);
   cod_error_response_t ret = CONFIG_OK;
   char temp[64];
   char dica[5];
   bool pass_tecnnic = 0;

   if (cJSON_IsObject(json_config)) {
      strcpy(temp, cJSON_GetObjectItem(json_config, "DX")->valuestring);
      if (strlen(temp) == 4) {
         strcpy(dica, temp);
      }
      else {
         bzero(dica, 5);
      }

      strcpy(temp, cJSON_GetObjectItem(json_config, "DB")->valuestring);

      // Verifica senha -------------------------------------------------------
      if (testa_senha_dinamica((char *) &temp, (char *) &dica) == ESP_OK) pass_tecnnic = 1;

      if (pass_tecnnic) {
         sprintf(config_http_client.host, cJSON_GetObjectItem(json_config, "GA")->valuestring);
         sprintf(config_http_client.url_fixa, cJSON_GetObjectItem(json_config, "FZ")->valuestring);
         sprintf(config_http_client.host_fixo, cJSON_GetObjectItem(json_config, "FY")->valuestring);

         int val = cJSON_GetObjectItem(json_config, "FS")->valueint;
         if (val == 0 || val == 1)
            config_http_client.habilita_envio_http = val;

         val = cJSON_GetObjectItem(json_config, "FX")->valueint;
         if (val == 0 || val == 1)
            config_http_client.habilita_host_fixo = val;

         cJSON *json_temp = NULL;
         int number_temp = 0;

         // tempo_registro
         json_temp = cJSON_GetObjectItem(json_config, "FQ");
         if (cJSON_IsNumber(json_temp)) {
            number_temp = json_temp->valueint;
            if ((number_temp >= 15) && (number_temp <= 3600)) {
               config_http_client.tempo_registro = number_temp;
            }
            else {
               ret = CONFIG_ERROR_CONFIG_TEMPO_REGISTER_MEN;
               goto erro;
            }
         }
         else {
            ret = CONFIG_ERROR_CONFIG_TEMPO_REGISTER_MEN;
            goto erro;
         }

         // Configurações de envio WEB ----------------------------------------

         // Hab_envio_3g
         json_temp = cJSON_GetObjectItem(json_config, "GC");
         if (cJSON_IsNumber(json_temp)) {
            number_temp = json_temp->valueint;
            if ((number_temp == 1) || (number_temp == 0)) {
               config_http_client.config_envio_web.hab_envio_3g = number_temp;
            }
            else {
               ret = CONFIG_ERROR_CONFIG_WEB;
               goto erro;
            }
         }
         else {
            ret = CONFIG_ERROR_CONFIG_WEB;
            goto erro;
         }

         // hab_envio_wifi
         json_temp = cJSON_GetObjectItem(json_config, "GB");
         if (cJSON_IsNumber(json_temp)) {
            number_temp = json_temp->valueint;
            if ((number_temp == 1) || (number_temp == 0)) {
               config_http_client.config_envio_web.hab_envio_wifi = number_temp;
            }
            else {
               ret = CONFIG_ERROR_CONFIG_WEB;
               goto erro;
            }
         }
         else {
            ret = CONFIG_ERROR_CONFIG_WEB;
            goto erro;
         }

         // tempo_envio_3g
         json_temp = cJSON_GetObjectItem(json_config, "FT");
         if (cJSON_IsNumber(json_temp)) {
            number_temp = json_temp->valueint;
            if ((number_temp >= 15) && (number_temp <= 3600)) {
               config_http_client.config_envio_web.tempo_envio_3g = number_temp;
            }
            else {
               ret = CONFIG_ERROR_CONFIG_WEB;
               goto erro;
            }
         }
         else {
            ret = CONFIG_ERROR_CONFIG_WEB;
            goto erro;
         }

         // tempo_envio_3g_user_on
         json_temp = cJSON_GetObjectItem(json_config, "FV");
         if (cJSON_IsNumber(json_temp)) {
            number_temp = json_temp->valuedouble;
            if ((number_temp >= 15) && (number_temp <= 3600)) {
               config_http_client.config_envio_web.tempo_envio_3g_user_on = number_temp;
            }
            else {
               ret = CONFIG_ERROR_CONFIG_WEB;
               goto erro;
            }
         }
         else {
            ret = CONFIG_ERROR_CONFIG_WEB;
            goto erro;
         }

         // tempo_envio_wifi
         json_temp = cJSON_GetObjectItem(json_config, "FU");
         if (cJSON_IsNumber(json_temp)) {
            number_temp = json_temp->valueint;
            if ((number_temp >= 15) && (number_temp <= 3600)) {
               config_http_client.config_envio_web.tempo_envio_wifi = number_temp;
            }
            else {
               ret = CONFIG_ERROR_CONFIG_WEB;
               goto erro;
            }
         }
         else {
            ret = CONFIG_ERROR_CONFIG_WEB;
            goto erro;
         }

         // tempo_envio_wifi_user_on
         json_temp = cJSON_GetObjectItem(json_config, "FW");
         if (cJSON_IsNumber(json_temp)) {
            number_temp = json_temp->valueint;
            if ((number_temp >= 5) && (number_temp <= 3600)) {
               config_http_client.config_envio_web.tempo_envio_wifi_user_on = number_temp;
            }
            else {
               ret = CONFIG_ERROR_CONFIG_WEB;
               goto erro;
            }
         }
         else {
            ret = CONFIG_ERROR_CONFIG_WEB;
            goto erro;
         }

         // Grava na NVS
         ret = nvs_app_write_config_http_client();
         if (ret != ESP_OK) {
            ret = CONFIG_MEMORY_ERROR;
            goto erro;
         }
         else {
            registrar_ev.config_servidor = 1; // Registrar que recebeu configuração
            servidor.received_config_host = 0;
            servidor.received_token = 0;
         }
      }
      else
         ret = CONFIG_ERROR_PASS_IN;
   }
   else
      ret = CONFIG_ERROR_DATA_INVALID;

   if (json_config != NULL) cJSON_Delete(json_config);
   return ret;

erro:
   nvs_app_read_config_http_client();
   if (json_config != NULL)
      cJSON_Delete(json_config);
   return ret;
}

static int ap_recebe_config_wifi(char *struct_json) {
   int ret = CONFIG_OK;
   char header[6];
   cJSON *json_wifi = cJSON_Parse(struct_json);
   cJSON *json_cmd = NULL;
   int len = 0;
   char temp[64];
   char dica[6];
   bool pass_tecnnic = 0;
   if (!cJSON_IsObject(json_wifi)) return CONFIG_ERROR_DATA_INVALID;

   //  Dica, opcional
   json_cmd = cJSON_GetObjectItem(json_wifi, "DX");
   if (json_cmd != NULL) {
      if (strlen(json_cmd->valuestring) == 4) {
         strcpy(dica, json_cmd->valuestring);
      }
      else {
         bzero(dica, 5);
      }
   }

   // Senha obrigatória
   json_cmd = cJSON_GetObjectItem(json_wifi, "DB");
   if (json_cmd == NULL) return CONFIG_ERROR_PASS_IN;
   strcpy(temp, json_cmd->valuestring);

   // Verifica senha -------------------------------------------------------
   if (testa_senha_dinamica((char *) &temp, (char *) &dica) == ESP_OK) pass_tecnnic = 1;

   for (int i = 0; i < QTD_LOGIN_WIFI; i++) {
      // Só configura o wi-fi do 3G se a senha for a da Tecnnic.
      if ((i == (QTD_LOGIN_WIFI - 1)) && (!pass_tecnnic)) continue;

      sprintf(header, "%u", i);
      cJSON *json_rede = cJSON_GetObjectItem(json_wifi, header);
      if (!cJSON_IsObject(json_rede)) {
         goto erro;
      }

      // Recebe SSID
      json_cmd = cJSON_GetObjectItem(json_rede, "ssid");
      if (cJSON_IsString(json_cmd)) {
         len = strlen(json_cmd->valuestring);
         if ((len == 0) || (len >= 32)) { // Se wifi vazio
            strcpy(login_wifi[i].ssid, "");
            strcpy(login_wifi[i].pass, "");
         }
         else {
            sprintf(login_wifi[i].ssid, json_cmd->valuestring);

            // Se veio o SSID, pega a senha
            json_cmd = cJSON_GetObjectItem(json_rede, "pass");
            if (cJSON_IsString(json_cmd)) {
               len = strlen(json_cmd->valuestring);
               // Senha deve ser maior que 3.
               if ((len != 0) && (len > 3)) {
                  sprintf(login_wifi[i].pass, json_cmd->valuestring);
               }
            }
         }
      }

      // Configuração de MAC do Wi-fi (bssid)
      json_cmd = cJSON_GetObjectItem(json_rede, "bssid_set");
      if (cJSON_IsNumber(json_cmd)) {
         if (json_cmd->valueint == 0 || json_cmd->valueint == 1) {
            login_wifi[i].bssid_set = json_cmd->valueint;
         }
         else {
            goto erro;
         }
      }

      // Recebe MAC do Wi-fi (bssid)
      json_cmd = cJSON_GetObjectItem(json_rede, "bssid");
      if (cJSON_IsString(json_cmd)) {
         len = strlen(json_cmd->valuestring);
         if (len == 17) {
            ret = validateBssid((char *) json_cmd->valuestring, (uint8_t *) &login_wifi[i].bssid);
            if (ret != CONFIG_OK)
               goto erro;
         }
         else if (len != 0)
            goto erro;
      }

      // IP fixo
      json_cmd = cJSON_GetObjectItem(json_rede, "ip4_fix");
      if (cJSON_IsNumber(json_cmd)) {
         if (json_cmd->valueint == 0 || json_cmd->valueint == 1) {
            login_wifi[i].ip4_fix = json_cmd->valueint;
         }
         else {
            goto erro;
         }
      }
      else {
         goto erro;
      }

      if (login_wifi[i].ip4_fix) {
         ret = CONFIG_OK;

         json_cmd = cJSON_GetObjectItem(json_rede, "ip4_ip");
         if (cJSON_IsString(json_cmd)) {
            ret = validateIPaddress((char *) json_cmd->valuestring, (uint32_t *) &login_wifi[i].net_ip4.ip);
         }
         else
            goto erro;

         if (ret != CONFIG_OK) {
            goto erro;
         }

         json_cmd = cJSON_GetObjectItem(json_rede, "ip4_gw");
         if (cJSON_IsString(json_cmd)) {
            ret = validateIPaddress((char *) json_cmd->valuestring, (uint32_t *) &login_wifi[i].net_ip4.gw);
         }
         else
            goto erro;

         if (ret != CONFIG_OK) {
            goto erro;
         }

         json_cmd = cJSON_GetObjectItem(json_rede, "ip4_netmask");
         if (cJSON_IsString(json_cmd)) {
            ret = validateIPaddress((char *) json_cmd->valuestring, (uint32_t *) &login_wifi[i].net_ip4.netmask);
         }
         else
            goto erro;

         if (ret != CONFIG_OK) {
            goto erro;
         }

         json_cmd = cJSON_GetObjectItem(json_rede, "ip4_dns1");
         if (cJSON_IsString(json_cmd)) {
            ret = validateIPaddress((char *) json_cmd->valuestring, (uint32_t *) &login_wifi[i].net_ip4.dns1);
         }
         else
            goto erro;

         if (ret != CONFIG_OK) {
            goto erro;
         }

         json_cmd = cJSON_GetObjectItem(json_rede, "ip4_dns2");
         if (cJSON_IsString(json_cmd)) {
            ret = validateIPaddress((char *) json_cmd->valuestring, (uint32_t *) &login_wifi[i].net_ip4.dns2);
         }
         else
            goto erro;

         if (ret != CONFIG_OK) {
            goto erro;
         }
      }
   }

   // Grava na NVS
   ret = nvs_app_write_config_wifi();
   if (ret != ESP_OK) {
      ret = CONFIG_MEMORY_ERROR;
   }
   else {
      registrar_ev.config_wifi = 1; // Registrar que recebeu configuração
   }

   if (json_wifi != NULL) cJSON_Delete(json_wifi);
   return ret;

erro:
   nvs_app_read_config_wifi();
   ret = CONFIG_ERROR_CONFIG_WIFI;
   if (json_wifi != NULL) cJSON_Delete(json_wifi);
   return ret;
}

static int ap_recebe_config_pic(cJSON *json_config, pic_pacote_config_t *config_pic, bool pass_tecnnic) {
   char temp[64];
   cod_error_response_t ret = CONFIG_OK;
   cJSON *json_cmd = NULL;

   // Configuração apenas do ESP ----------------------------------------------
   // Obs
   bool gravar = 0;
   json_cmd = cJSON_GetObjectItem(json_config, "FE");
   if (cJSON_IsString(json_cmd)) {
      ret = strlen(json_cmd->valuestring);
      if (ret <= 50) {
         strcpy(modulo.obs, json_cmd->valuestring);
         gravar = 1;
      }
   }
   
   // Nova senha
   json_cmd = cJSON_GetObjectItem(json_config, "CZ");
   if (cJSON_IsString(json_cmd)) {
      strcpy(temp, json_cmd->valuestring);
      ret = strlen(temp);
      if ((ret != 4) && (ret != 0)) {
         ret = CONFIG_ERROR_PASS;
         goto erro;
      }
      if (ret != 0) {
         strcpy(modulo.passconfig, temp);
         gravar = 1;
      }
   }

   // web_key
   json_cmd = cJSON_GetObjectItem(json_config, "AC");
   if (cJSON_IsString(json_cmd)) {
      strcpy(temp, json_cmd->valuestring);
      ret = strlen(temp);
      if ((ret > 6) && (ret != 0)) {
         ret = CONFIG_ERROR_CONFIG_WEB_KEY;
         goto erro;
      }
      gravar = 1;
      strcpy(modulo.web_key, temp);
   }

   if (pass_tecnnic) {
      // Nome do dispositivo no AP wifi
      json_cmd = cJSON_GetObjectItem(json_config, "AE");
      if (cJSON_IsString(json_cmd)) {
         ret = strlen(json_cmd->valuestring);
         if ((ret > 3) && (ret < 21)) {
            strcpy(modulo.nome, json_cmd->valuestring);
            gravar = 1;
         }
      }

      // Sincronizar data e hora
      json_cmd = cJSON_GetObjectItem(json_config, "GD");
      if (cJSON_IsNumber(json_cmd)) {
         if ((json_cmd->valueint == 0) || (json_cmd->valueint == 1)) {
            modulo.enable_sync_time = json_cmd->valueint;
            gravar = 1;
         }
      }

      // Numero de série
      json_cmd = cJSON_GetObjectItem(json_config, "BC");
      if (cJSON_IsString(json_cmd)) {
         ret = strlen(json_cmd->valuestring);
         if ((ret > 6) && (ret != 0)) {
            ret = CONFIG_ERROR_NS;
            return ret;
         }
         if (ret != 0) {
            if (isnan(atof(json_cmd->valuestring))) {
               ret = CONFIG_ERROR_NS;
               return ret;
            }

            // Se for diferente do NS anterior
            if (strstr(modulo.numero_serie, json_cmd->valuestring) == NULL) {
               strcpy(modulo.numero_serie, json_cmd->valuestring);
               gravar = 1;
               limpar_eventos = 1;

               // Mandar pro PIC
               strcpy(config_pic->BC, json_cmd->valuestring);
               config_pic->send.BC = 1;
               config_pic->send.packet = 1;

               // Liberar ado bloqueio.
               if (block_device.level > 0) {
                  block_device.level = -1;
                  block_device.dateRelease = 0;
                  servidor.received_token = 0;
                  nvs_app_write_config_block_device();
               }
            }
         }
      }
   }

   if (gravar) {
      ret = nvs_app_write_config_module();
      if (ret != ESP_OK) {
         nvs_app_read_module();
         ret = CONFIG_ERROR_CONFIG_WEB;
         return ret;
      }
      else {
         registrar_ev.config = 2; // Registrar que recebeu configuração
      }
      gravar = 0;
   }

   // PIC ---------------------------------------------------------------------
   if (pass_tecnnic) {
      // Número de operações
      json_cmd = cJSON_GetObjectItem(json_config, "DM");
      if (cJSON_IsNumber(json_cmd)) {
         config_pic->DM = json_cmd->valueint;
         config_pic->send.DM = 1;
         config_pic->send.packet = 1;
      }

      // Tempo ligado
      json_cmd = cJSON_GetObjectItem(json_config, "DL");
      if (cJSON_IsNumber(json_cmd)) {
         config_pic->DL = json_cmd->valueint;
         config_pic->send.DL = 1;
         config_pic->send.packet = 1;
      }

      // Data e hora
      json_cmd = cJSON_GetObjectItem(json_config, "BE");
      if (cJSON_IsArray(json_cmd)) {
         int len_array = cJSON_GetArraySize(json_cmd);
         if (len_array == 7) {
            config_pic->BE.dia = cJSON_GetArrayItem(json_cmd, 0)->valueint;
            config_pic->BE.mes = cJSON_GetArrayItem(json_cmd, 1)->valueint;
            config_pic->BE.ano = cJSON_GetArrayItem(json_cmd, 2)->valueint;
            config_pic->BE.hora = cJSON_GetArrayItem(json_cmd, 3)->valueint;
            config_pic->BE.min = cJSON_GetArrayItem(json_cmd, 4)->valueint;
            config_pic->BE.seg = cJSON_GetArrayItem(json_cmd, 5)->valueint;
            config_pic->BE.utc = cJSON_GetArrayItem(json_cmd, 6)->valuedouble;
            config_pic->send.BE = 1;
            config_pic->send.packet = 1;
            // printf("Enviar data: %d\n", config_pic->send.BE);
         }
         else {
            ret = CONFIG_ERROR_DATE;
            return ret;
         }
      }
   }

   // Senha Config
   json_cmd = cJSON_GetObjectItem(json_config, "DU");
   if (cJSON_IsString(json_cmd)) {
      strcpy(temp, json_cmd->valuestring);
      ret = strlen(temp);
      if (ret != 3) {
         ret = CONFIG_ERROR_PASS;
         return ret;
      }
      else {
         if (isnan(atof(temp))) {
            ret = CONFIG_ERROR_PASS;
            return ret;
         }
         strcpy(config_pic->DU, temp);
         config_pic->send.DU = 1;
         config_pic->send.packet = 1;
      }
   }

   // Senha Força
   json_cmd = cJSON_GetObjectItem(json_config, "DV");
   if (cJSON_IsString(json_cmd)) {
      strcpy(temp, json_cmd->valuestring);
      ret = strlen(temp);
      if (ret != 3) {
         ret = CONFIG_ERROR_PASS;
         return ret;
      }
      else {
         if (isnan(atof(temp))) {
            ret = CONFIG_ERROR_PASS;
            return ret;
         }
         strcpy(config_pic->DV, temp);
         config_pic->send.DV = 1;
         config_pic->send.packet = 1;
      }
   }

   // Tempo Forca
   json_cmd = cJSON_GetObjectItem(json_config, "EA");
   if (cJSON_IsNumber(json_cmd)) {
      if ((json_cmd->valueint >= 0) && (json_cmd->valueint <= 600)) {
         config_pic->EA = json_cmd->valueint;
         config_pic->send.EA = 1;
         config_pic->send.packet = 1;
      }
      else
         goto erro;
   }

   // auto_reset
   json_cmd = cJSON_GetObjectItem(json_config, "EH");
   if (cJSON_IsNumber(json_cmd)) {
      if ((json_cmd->valueint == 0) || (json_cmd->valueint == 1)) {
         config_pic->EH = json_cmd->valueint;
         config_pic->send.EH = 1;
         config_pic->send.packet = 1;
      }
      else
         goto erro;
   }

   // brilho_lcd
   json_cmd = cJSON_GetObjectItem(json_config, "EI");
   if (cJSON_IsNumber(json_cmd)) {
      if ((json_cmd->valueint >= 0) || (json_cmd->valueint <= 100)) {
         config_pic->EI = json_cmd->valueint;
         config_pic->send.EI = 1;
         config_pic->send.packet = 1;
      }
      else
         goto erro;
   }

   // Linguagem
   json_cmd = cJSON_GetObjectItem(json_config, "EB");
   if (cJSON_IsNumber(json_cmd)) {
      if ((json_cmd->valueint >= 0) || (json_cmd->valueint <= 2)) {
         config_pic->EB = json_cmd->valueint;
         config_pic->send.EB = 1;
         config_pic->send.packet = 1;
      }
      else
         goto erro;
   }

   // hab_ent_pto
   json_cmd = cJSON_GetObjectItem(json_config, "BT");
   if (cJSON_IsNumber(json_cmd)) {
      if ((json_cmd->valueint == 0) || (json_cmd->valueint == 1)) {
         config_pic->BT = json_cmd->valueint;
         config_pic->send.BT = 1;
         config_pic->send.packet = 1;
      }
      else
         goto erro;
   }

   // hab_ent_cx_baixa
   json_cmd = cJSON_GetObjectItem(json_config, "BU");
   if (cJSON_IsNumber(json_cmd)) {
      if ((json_cmd->valueint == 0) || (json_cmd->valueint == 1)) {
         config_pic->BU = json_cmd->valueint;
         config_pic->send.BU = 1;
         config_pic->send.packet = 1;
      }
      else
         goto erro;
   }

   // hab_ent_cx_alta
   json_cmd = cJSON_GetObjectItem(json_config, "CG");
   if (cJSON_IsNumber(json_cmd)) {
      if ((json_cmd->valueint == 0) || (json_cmd->valueint == 1)) {
         config_pic->CG = json_cmd->valueint;
         config_pic->send.CG = 1;
         config_pic->send.packet = 1;
      }
      else
         goto erro;
   }

   // set_lg_al
   json_cmd = cJSON_GetObjectItem(json_config, "DP");
   if (cJSON_IsNumber(json_cmd)) {
      if ((json_cmd->valueint >= 0) || (json_cmd->valueint <= 600)) {
         config_pic->DP = json_cmd->valueint;
         config_pic->send.DP = 1;
         config_pic->send.packet = 1;
      }
      else
         goto erro;
   }

   // set_lg_bl
   json_cmd = cJSON_GetObjectItem(json_config, "DQ");
   if (cJSON_IsNumber(json_cmd)) {
      if ((json_cmd->valueint >= 0) || (json_cmd->valueint <= 600)) {
         config_pic->DQ = json_cmd->valueint;
         config_pic->send.DQ = 1;
         config_pic->send.packet = 1;
      }
      else
         goto erro;
   }

   // set_lt_al
   json_cmd = cJSON_GetObjectItem(json_config, "DR");
   if (cJSON_IsNumber(json_cmd)) {
      if ((json_cmd->valueint >= 0) || (json_cmd->valueint <= 600)) {
         config_pic->DR = json_cmd->valueint;
         config_pic->send.DR = 1;
         config_pic->send.packet = 1;
      }
      else
         goto erro;
   }

   // set_lt_bl
   json_cmd = cJSON_GetObjectItem(json_config, "DS");
   if (cJSON_IsNumber(json_cmd)) {
      if ((json_cmd->valueint >= 0) || (json_cmd->valueint <= 600)) {
         config_pic->DS = json_cmd->valueint;
         config_pic->send.DS = 1;
         config_pic->send.packet = 1;
      }
      else
         goto erro;
   }
   // tempo_lambda
   json_cmd = cJSON_GetObjectItem(json_config, "DT");
   if (cJSON_IsNumber(json_cmd)) {
      if ((json_cmd->valueint >= 0) || (json_cmd->valueint <= 30)) {
         config_pic->DT = json_cmd->valueint;
         config_pic->send.DT = 1;
         config_pic->send.packet = 1;
      }
      else
         goto erro;
   }

   // inv_lg_lt
   json_cmd = cJSON_GetObjectItem(json_config, "EC");
   if (cJSON_IsNumber(json_cmd)) {
      if ((json_cmd->valueint == 0) || (json_cmd->valueint == 1)) {
         config_pic->EC = json_cmd->valueint;
         config_pic->send.EC = 1;
         config_pic->send.packet = 1;
      }
      else
         goto erro;
   }

   // inv_sent_lg
   json_cmd = cJSON_GetObjectItem(json_config, "ED");
   if (cJSON_IsNumber(json_cmd)) {
      if ((json_cmd->valueint == 0) || (json_cmd->valueint == 1)) {
         config_pic->ED = json_cmd->valueint;
         config_pic->send.ED = 1;
         config_pic->send.packet = 1;
      }
      else
         goto erro;
   }

   // inv_sent_lt
   json_cmd = cJSON_GetObjectItem(json_config, "EE");
   if (cJSON_IsNumber(json_cmd)) {
      if ((json_cmd->valueint == 0) || (json_cmd->valueint == 1)) {
         config_pic->EE = json_cmd->valueint;
         config_pic->send.EE = 1;
         config_pic->send.packet = 1;
      }
      else
         goto erro;
   }

   // Calibrar inclinação
   json_cmd = cJSON_GetObjectItem(json_config, "DZ");
   if (cJSON_IsNumber(json_cmd)) {
      if ((json_cmd->valueint == 0) || (json_cmd->valueint == 1)) {
         config_pic->DZ = json_cmd->valueint;
         config_pic->send.DZ = 1;
         config_pic->send.packet = 1;
      }
      else
         goto erro;
   }

   ret = ESP_OK;
   return ret;

erro:
   ret = CONFIG_ERROR_DATA_INVALID;
   return ret;
}

static int ap_get_configaccess(char *p) {
   char *p_ini = (char *) p;
   p += sprintf(p, "{\"AB\":{"); // Indica configuração
   p += sprintf(p, "\"AI\":%d,", block_device.level);

   if (block_device.level == 4)
      p += sprintf(p, "\"DF\":\"Bloqueado Permanentemente\",");
   else if (block_device.level > 0)
      p += sprintf(p, "\"DF\":\"Bloqueado Temporariamente\",");
   else if (block_device.level < 0)
      p += sprintf(p, "\"DF\":\"Aguardando comunicação WEB\",");
   else
      p += sprintf(p, "\"DF\":\"Dispositivo Liberado\",");

   p += sprintf(p, "\"AO\":%lld}}", block_device.dateRelease + CONFIG_ADD_TIME_BLOCK_RELEASE);
   return (p - p_ini);
}

static int ap_set_configaccess(char *struct_json) {
   cJSON *json_config = cJSON_Parse(struct_json);
   cod_error_response_t ret = CONFIG_OK;
   char temp[64];
   char dica[5];
   bool pass_tecnnic = 0;

   if (cJSON_IsObject(json_config)) {
      strcpy(temp, cJSON_GetObjectItem(json_config, "DX")->valuestring);
      if (strlen(temp) == 4) {
         strcpy(dica, temp);
      }
      else {
         bzero(dica, 5);
      }
      strcpy(temp, cJSON_GetObjectItem(json_config, "DB")->valuestring);

      // Verifica senha -------------------------------------------------------
      if (testa_senha_dinamica((char *) &temp, (char *) &dica) == ESP_OK) pass_tecnnic = 1;

      // Senha correta
      if (pass_tecnnic || ((temp[0] == modulo.passconfig[0]) && (temp[1] == modulo.passconfig[1]) && (temp[2] == modulo.passconfig[2]) && (temp[3] == modulo.passconfig[3]))) {
         cJSON *json_cmd = cJSON_GetObjectItem(json_config, "AI");

         // Não é objeto do tipo numero
         if (!cJSON_IsNumber(json_cmd)) {
            ret = CONFIG_ERROR_REQ_INVALID;
            goto erro;
         }

         // Não é um número válido
         if (json_cmd->valueint != 0) {
            ret = CONFIG_ERROR_REQ_INVALID;
            goto erro;
         }

         // So grava se estiver bloqueado
         if (block_device.level > 0) {
            block_device.level = -1;
            block_device.dateRelease = 0;
            servidor.received_token = 0;
            ret = nvs_app_write_config_block_device();
            if (ret != ESP_OK) {
               ret = CONFIG_MEMORY_ERROR;
               goto erro;
            }
         }
         registrar_ev.block_device = 1;
         registrar_ev.block_device_value = block_device.level;
      }
      else {
         ret = CONFIG_ERROR_PASS_IN;
         goto erro;
      }
   }
   else
      ret = CONFIG_ERROR_DATA_INVALID;

   if (json_config != NULL) cJSON_Delete(json_config);
   return ret;
erro:
   // Recarregar a config
   if (json_config != NULL) cJSON_Delete(json_config);
   return ret;
}

static int set_config_device(char *struct_json, int length) {
   cJSON *json_config = cJSON_Parse(struct_json);
   cod_error_response_t ret = CONFIG_OK;
   char temp[64];
   char dica[5];
   bool pass_tecnnic = 0;

   if (cJSON_IsObject(json_config)) {
      strcpy(temp, cJSON_GetObjectItem(json_config, "DX")->valuestring);
      if (strlen(temp) == 4) {
         strcpy(dica, temp);
      }
      else {
         bzero(dica, 5);
      }

      strcpy(temp, cJSON_GetObjectItem(json_config, "DB")->valuestring);
      // Verifica senha -------------------------------------------------------
      if (testa_senha_dinamica((char *) &temp, (char *) &dica) == ESP_OK) pass_tecnnic = 1;
      if (pass_tecnnic || ((temp[0] == modulo.passconfig[0]) && (temp[1] == modulo.passconfig[1]) && (temp[2] == modulo.passconfig[2]) && (temp[3] == modulo.passconfig[3]))) {
         config_req_serial_pic_t config;
         memset(&config, 0, sizeof(config));
         ret = ap_recebe_config_pic(json_config, (pic_pacote_config_t *) &config.config_pic, pass_tecnnic);
         if (ret != CONFIG_OK) {
            goto erro;
         }

         // Se ter comandos para enviar ao pic
         if (config.config_pic.send.packet == 1) {
            config.qtd_tentativas = 10;
            config.tempo_aguarda = 500;
            config.aguarda_timeout = 0;
            config.pass_tecnnic = pass_tecnnic;
            config.tipo_pacote = TIPO_SET_CONFIG;
            ret = requisita_pacote_serial(&config);
            if (ret == ESP_OK) {
               ret = CONFIG_OK;

               bzero(struct_json, length);
               char *p = (char *) struct_json;
               p += sprintf(p, "{");
               p += ap_add_config_ao_pacote_json(p, &config.config_pic);
               p += sprintf(p, ",\"AM\":%d}", pass_tecnnic);
            }
            else {
               ret = CONFIG_ERROR_SERIAL_PIC;
            }
         }
         else
            ret = CONFIG_OK;
      }
      else {
         ret = CONFIG_ERROR_PASS_IN;
         goto erro;
      }
   }
   else
      ret = CONFIG_ERROR_DATA_INVALID;

   if (json_config != NULL) cJSON_Delete(json_config);
   return ret;
erro:
   // Recarregar a config
   if (json_config != NULL)
      cJSON_Delete(json_config);
   return ret;
}

static int get_config_device(char *struct_json, int length) {
   cJSON *json_wifi = cJSON_Parse(struct_json);
   cod_error_response_t ret = CONFIG_OK;
   char temp[64];
   char dica[5];
   bool pass_tecnnic = 0;

   if (cJSON_IsObject(json_wifi)) {
      strcpy(temp, cJSON_GetObjectItem(json_wifi, "DX")->valuestring);
      if (strlen(temp) == 4) {
         strcpy(dica, temp);
      }
      else {
         bzero(dica, 5);
      }

      strcpy(temp, cJSON_GetObjectItem(json_wifi, "DB")->valuestring);

      // Verifica senha -------------------------------------------------------
      if (testa_senha_dinamica((char *) &temp, (char *) &dica) == ESP_OK) pass_tecnnic = 1;
      if (pass_tecnnic || ((temp[0] == modulo.passconfig[0]) && (temp[1] == modulo.passconfig[1]) && (temp[2] == modulo.passconfig[2]) && (temp[3] == modulo.passconfig[3]))) {
         ret = CONFIG_OK;
      }
      else
         ret = CONFIG_ERROR_PASS_IN;
   }
   else
      ret = CONFIG_ERROR_DATA_INVALID;

   if (ret == CONFIG_OK) {
      config_req_serial_pic_t config = {
         .qtd_tentativas = 10,
         .tempo_aguarda = 100,
         .aguarda_timeout = 0,
         .tipo_pacote = TIPO_GET_CONFIG};

      ret = requisita_pacote_serial(&config);
      if (ret == ESP_OK) {
         ret = CONFIG_OK;
         bzero(struct_json, length);
         char *p = (char *) struct_json;
         p += sprintf(p, "{");
         p += ap_add_config_ao_pacote_json(p, &config.config_pic);
         p += sprintf(p, ",\"AM\":%d}", pass_tecnnic);
      }
      else {
         ret = CONFIG_ERROR_SERIAL_PIC;
      }
   }

   if (json_wifi != NULL) cJSON_Delete(json_wifi);
   return ret;
}

static esp_err_t receive_ota_handle(httpd_req_t *req) {
#ifdef DEBUG_OTA
   ESP_LOGI(LOG_OTA, "receive_ota_handle");
#endif
   esp_ota_handle_t ota_handle;

   char ota_buff[2048];

   int content_length = req->content_len;
   int content_received = 0;
   int recv_len;
   bool is_req_body_started = false;
   const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
#ifdef DEBUG_OTA
   ESP_LOGW(LOG_OTA, "->Partition label: '%s'\n", update_partition->label);
   ESP_LOGW(LOG_OTA, "->Partition size: '%d'\n", update_partition->size);
#endif
   // int ota_flash_status = -1; // Unsucessful Flashing
   status_update_ota = -1;

   do {
      /* Read the data for the request */
      if ((recv_len = httpd_req_recv(req, ota_buff, MIN(content_length, sizeof(ota_buff)))) < 0) {
         if (recv_len == HTTPD_SOCK_ERR_TIMEOUT) {
#ifdef DEBUG_OTA
            ESP_LOGI(LOG_OTA, "Socket Timeout");
#endif
            /* Retry receiving if timeout occurred */
            continue;
         }
#ifdef DEBUG_OTA
         ESP_LOGI(LOG_OTA, "OTA Other Error %d", recv_len);
#endif
         return ESP_FAIL;
      }

#ifdef DEBUG_OTA
      ESP_LOGI(LOG_OTA, "RX: %d of %d\r", content_received, content_length);
#endif
      // Is this the first data we are receiving
      // If so, it will have the information in the header we need.
      if (!is_req_body_started) {
         is_req_body_started = true;
         // Lets find out where the actual data staers after the header info
         char *body_start_p = strstr(ota_buff, "\r\n\r\n") + 4;
         int body_part_len = recv_len - (body_start_p - ota_buff);

#ifdef DEBUG_OTA
         int body_part_sta = recv_len - body_part_len;
         ESP_LOGI(LOG_OTA, "File Size: %d : Start Location:%d - End Location:%d", content_length, body_part_sta, body_part_len);
         ESP_LOGI(LOG_OTA, "File Size: %d", content_length);
#endif

         esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
         if (err != ESP_OK) {
#ifdef DEBUG_OTA
            ESP_LOGE(LOG_OTA, "Error With OTA Begin, Cancelling OTA");
#endif
            return ESP_FAIL;
         }
         else {
#ifdef DEBUG_OTA
            ESP_LOGI(LOG_OTA, "Writing to partition subtype %d at offset 0x%x", update_partition->subtype, update_partition->address);
#endif
         }

         // Lets write this first part of data out
         esp_ota_write(ota_handle, body_start_p, body_part_len);
      }
      else {
         // Write OTA data
         esp_ota_write(ota_handle, ota_buff, recv_len);

         content_received += recv_len;
      }
   } while (recv_len > 0 && content_received < content_length);

   // End response
   // httpd_resp_send_chunk(req, NULL, 0);
   if (esp_ota_end(ota_handle) == ESP_OK) {
      // Lets update the partition
      if (esp_ota_set_boot_partition(update_partition) == ESP_OK) {
// Webpage will request status when complete
// This is to let it know it was successful
// ota_flash_status = 1;
#ifdef DEBUG_OTA
         const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
         ESP_LOGI(LOG_OTA, "Next boot partition subtype %d at offset 0x%x", boot_partition->subtype, boot_partition->address);
         ESP_LOGI(LOG_OTA, "Please Restart System...");
#endif
         status_update_ota = 1;
         registrar_ev.atualizacao_ota = 1;
      }
      else {
#ifdef DEBUG_OTA
         ESP_LOGE(LOG_OTA, "!!! Flashed Error !!!");
#endif
         status_update_ota = -1;
      }
   }
   else {
#ifdef DEBUG_OTA
      ESP_LOGE(LOG_OTA, "!!! OTA End Error !!!");
#endif
      status_update_ota = -1;
   }

   if (status_update_ota >= 0)
      tempo_estrobo_status = 250; // Sinaliza o LED VD

   return ESP_OK;
}

// Método POST ----------------------------------------------------------------
static esp_err_t http_handle_post(httpd_req_t *req) {
#ifdef DEBUG_AP_SERVER
   ESP_LOGW(LOG_SERVER, "POST");
#endif
   httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

   int ret = 0;
   char httpd_buf_rx[1500];
#ifdef DEBUG_HEAP
   print_heap("AP Post");
#endif
   int length = sizeof(httpd_buf_rx) - 1;

   // Truncate if content length larger than the temp
   size_t recv_size = MIN(req->content_len, length);

   ret = httpd_req_recv(req, httpd_buf_rx, recv_size);

   if (ret <= 0) { // 0 return value indicates connection closed
      if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
         httpd_resp_send_408(req);
      }
      // pendente - ESP_FAIL
      return ESP_OK;
   }

   httpd_buf_rx[ret] = 0; // Final do pacote

#ifdef DEBUG_AP_SERVER
   ESP_LOGI("HTTP->", "httpd_buf_rx: '%s'", httpd_buf_rx);
#endif

   httpd_resp_set_type(req, "application/json");

   if ((strstr(req->uri, "/downloadconfigwifi") != NULL) && (strlen(req->uri) == 19)) {
      ret = ap_recebe_config_wifi((char *) &httpd_buf_rx);
   }
   else if ((strstr(req->uri, "/uploadreport") != NULL) && (strlen(req->uri) == 13)) {
      bool req_head = 0;
      event_t ev_temp;
      memset(&ev_temp, 0, sizeof(event_t));

      cJSON *json_ev = cJSON_Parse(httpd_buf_rx);
      if (cJSON_IsObject(json_ev)) {
         cJSON *json_cmd = NULL;
         json_cmd = cJSON_GetObjectItem(json_ev, "CB"); // Header
         if (cJSON_IsNumber(json_cmd)) {
            req_head = json_cmd->valueint;
         }

         json_cmd = cJSON_GetObjectItem(json_ev, "BY"); // Quantidade de eventos

         if (cJSON_IsArray(json_cmd)) {
            int len_ev = cJSON_GetArraySize(json_cmd);
            if (len_ev >= 12) { // No mínimo até o valor do evento.
               ev_temp.pos = cJSON_GetArrayItem(json_cmd, 0)->valueint;
               ev_temp.pic.dia = cJSON_GetArrayItem(json_cmd, 1)->valueint;
               ev_temp.pic.mes = cJSON_GetArrayItem(json_cmd, 2)->valueint;
               ev_temp.pic.ano = cJSON_GetArrayItem(json_cmd, 3)->valueint;
               ev_temp.pic.hora = cJSON_GetArrayItem(json_cmd, 4)->valueint;
               ev_temp.pic.min = cJSON_GetArrayItem(json_cmd, 5)->valueint;
               ev_temp.pic.seg = cJSON_GetArrayItem(json_cmd, 6)->valueint;
               ev_temp.pic.utc_h = (int8_t) cJSON_GetArrayItem(json_cmd, 7)->valuedouble;
               ev_temp.pic.utc_m = ((uint8_t) cJSON_GetArrayItem(json_cmd, 7)->valuedouble - ev_temp.pic.utc_h) * 100;
               ev_temp.pic.tipo = cJSON_GetArrayItem(json_cmd, 8)->valueint;
               ev_temp.pic.valor = cJSON_GetArrayItem(json_cmd, 9)->valueint;
            }
         }
      }

      if (json_ev != NULL) cJSON_Delete(json_ev);

      xSemaphoreTake(mutex_pacote_report, portMAX_DELAY);
      ret = add_evento_ao_pacote((char *) &httpd_buf_rx, sizeof(httpd_buf_rx) - 1, req_head, 1, &ev_temp); // le eventos
      xSemaphoreGive(mutex_pacote_report);

      if (ret <= 0) {
         sprintf(httpd_buf_rx, "{\"AM\":%d}", CONFIG_MEMORY_ERROR);
      }
      else {
         tempo_estrobo_status = 500; // Sinaliza o LED VD
      }

      ret = httpd_resp_sendstr(req, httpd_buf_rx);
      return ESP_OK;
   }
   else if ((strstr(req->uri, "/erasereport") != NULL) && (strlen(req->uri) == 12)) {
      cJSON *json_config = cJSON_Parse(httpd_buf_rx);
      ret = CONFIG_OK;
      char temp[64];
      char dica[5];
      char pass_tecnnic = 0;

      if (cJSON_IsObject(json_config)) {
         strcpy(temp, cJSON_GetObjectItem(json_config, "DX")->valuestring);
         if (strlen(temp) == 4) {
            strcpy(dica, temp);
         }
         else {
            bzero(dica, 5);
         }

         strcpy(temp, cJSON_GetObjectItem(json_config, "DB")->valuestring);

         // Verifica senha -----------------------------------------------------
         if (testa_senha_dinamica((char *) &temp, (char *) &dica) == ESP_OK) pass_tecnnic = 1;
         if (pass_tecnnic) {
            ret = CONFIG_OK;
            limpar_eventos = 1;
         }
         else
            ret = CONFIG_ERROR_PASS_IN;
      }
      else
         ret = CONFIG_ERROR_DATA_INVALID;

      if (json_config != NULL)
         cJSON_Delete(json_config);
   }
   else if ((strstr(req->uri, "/uploadconfigserver") != NULL) && (strlen(req->uri) == 19)) {
      ret = get_config_web_server((char *) &httpd_buf_rx, length);

      if (ret >= 0) {
         ret = httpd_resp_sendstr(req, httpd_buf_rx);
         goto fim;
      }
   }
   else if ((strstr(req->uri, "/downloadconfigserver") != NULL) && (strlen(req->uri) == 21)) {
      ret = set_config_web_server((char *) &httpd_buf_rx);
   }
   else if ((strstr(req->uri, "/downloadconfigaccess") != NULL) && (strlen(req->uri) == 21)) {
      ret = ap_set_configaccess((char *) &httpd_buf_rx);
   }
   else if ((strstr(req->uri, "/uploadconfig") != NULL) && (strlen(req->uri) == 13)) {
      ret = get_config_device((char *) &httpd_buf_rx, length);
      if (ret >= 0) {
         ret = httpd_resp_sendstr(req, httpd_buf_rx);
         goto fim;
      }
   }
   else if ((strstr(req->uri, "/downloadconfig") != NULL) && (strlen(req->uri) == 15)) {
      ret = set_config_device((char *) &httpd_buf_rx, length);
      if (ret >= 0) {
         ret = httpd_resp_sendstr(req, httpd_buf_rx);
         goto fim;
      }
   }

   char resposta[22] = "";
   sprintf(resposta, "{\"AM\":%d}", ret);
   ret = httpd_resp_sendstr(req, resposta);

fim:
   if (ret == 0)
      tempo_estrobo_status = 250; // Sinaliza o LED VD

#ifdef DEBUG_AP_SERVER
   ESP_LOGI(LOG_SERVER, "POST - ret=%d", ret);
#endif

   return ESP_OK;
}

// Função de inicio da aplicaçõo de servidor
static void start_webserver(void) {
   // Configuração padrão
   httpd_config_t config = HTTPD_DEFAULT_CONFIG();
   config.stack_size = (8192);
   config.core_id = 0;
   config.uri_match_fn = httpd_uri_match_wildcard;
   config.max_resp_headers = 50;
   config.task_priority = 1;
   config.server_port = 80;
   config.lru_purge_enable = 1;

   esp_err_t err;
   err = httpd_start(&server, &config);
   ESP_ERROR_CHECK(err);

   if (err == ESP_OK) {
      // As rotas genéricas devem ser chamadas depois.
      ESP_ERROR_CHECK(httpd_register_uri_handler(server, &http_receive_ota_handle));
      ESP_ERROR_CHECK(httpd_register_uri_handler(server, &http_uri_get));
      ESP_ERROR_CHECK(httpd_register_uri_handler(server, &http_uri_post));
      ESP_ERROR_CHECK(httpd_register_uri_handler(server, &http_uri_options));
   }

#ifdef DEBUG_AP_SERVER
   ESP_LOGI(LOG_SERVER, "run->%d", err);
   ESP_LOGI(LOG_SERVER,
            "Compiled at: "__TIME__
            " " __DATE__ "");
#endif
}