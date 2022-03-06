#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_system.h>
#include <sys/param.h>
#include <esp_http_server.h>
#include <esp_ota_ops.h>

#ifdef DEBUG_AP_SERVER
const char *LOG_SERVER = "SERVER";
#endif

#ifdef DEBUG_OTA
const char *LOG_OTA = "OTA";
#endif

httpd_handle_t server = NULL;

// Carrega os arquivos da memoria do ESP32
#define ARRAY_SIZE_OF(a) (sizeof(a) / sizeof(a[0]))

// Index
extern const unsigned char index_html_start[] asm("_binary_index_html_start");
extern const unsigned char index_html_end[] asm("_binary_index_html_end");
// Style and frameworks
extern const unsigned char styles_css_start[] asm("_binary_styles_css_start");
extern const unsigned char styles_css_end[] asm("_binary_styles_css_end");
extern const unsigned char mini_css_start[] asm("_binary_mini_css_start");
extern const unsigned char mini_css_end[] asm("_binary_mini_css_end");
// favicon
extern const unsigned char favicon_png_start[] asm("_binary_favicon_png_start");
extern const unsigned char favicon_png_end[] asm("_binary_favicon_png_end");
// Tela configuração do Wi-Fi
extern const unsigned char configwifi_html_start[] asm("_binary_configwifi_html_start");
extern const unsigned char configwifi_html_end[] asm("_binary_configwifi_html_end");
extern const unsigned char configwifi_js_start[] asm("_binary_configwifi_js_start");
extern const unsigned char configwifi_js_end[] asm("_binary_configwifi_js_end");
// Tela realtime
extern const unsigned char realtime_js_start[] asm("_binary_realtime_js_start");
extern const unsigned char realtime_js_end[] asm("_binary_realtime_js_end");
extern const unsigned char realtime_html_start[] asm("_binary_realtime_html_start");
extern const unsigned char realtime_html_end[] asm("_binary_realtime_html_end");
// Tela report
extern const unsigned char report_js_start[] asm("_binary_report_js_start");
extern const unsigned char report_js_end[] asm("_binary_report_js_end");
extern const unsigned char report_html_start[] asm("_binary_report_html_start");
extern const unsigned char report_html_end[] asm("_binary_report_html_end");
// Tela configuração do servidor
extern const unsigned char configwebserver_js_start[] asm("_binary_configwebserver_js_start");
extern const unsigned char configwebserver_js_end[] asm("_binary_configwebserver_js_end");
extern const unsigned char configwebserver_html_start[] asm("_binary_configwebserver_html_start");
extern const unsigned char configwebserver_html_end[] asm("_binary_configwebserver_html_end");
// Tela configuração de acesso
extern const unsigned char configaccess_js_start[] asm("_binary_configaccess_js_start");
extern const unsigned char configaccess_js_end[] asm("_binary_configaccess_js_end");
extern const unsigned char configaccess_html_start[] asm("_binary_configaccess_html_start");
extern const unsigned char configaccess_html_end[] asm("_binary_configaccess_html_end");
// Tela configuração do servidor
extern const unsigned char configdevice_js_start[] asm("_binary_configdevice_js_start");
extern const unsigned char configdevice_js_end[] asm("_binary_configdevice_js_end");
extern const unsigned char configdevice_html_start[] asm("_binary_configdevice_html_start");
extern const unsigned char configdevice_html_end[] asm("_binary_configdevice_html_end");
// Tela OTA
extern const unsigned char ota_html_start[] asm("_binary_ota_html_start");
extern const unsigned char ota_html_end[] asm("_binary_ota_html_end");
// Arquivo Manifest
extern const unsigned char manifest_json_start[] asm("_binary_manifest_json_start");
extern const unsigned char manifest_json_end[] asm("_binary_manifest_json_end");
// imagens de potencia de Wi-Fi
extern const unsigned char iconwifi_lg_png_start[] asm("_binary_iconwifi_lg_png_start");
extern const unsigned char iconwifi_lg_png_end[] asm("_binary_iconwifi_lg_png_end");
extern const unsigned char iconwifi_md_png_start[] asm("_binary_iconwifi_md_png_start");
extern const unsigned char iconwifi_md_png_end[] asm("_binary_iconwifi_md_png_end");
extern const unsigned char iconwifi_sm_png_start[] asm("_binary_iconwifi_sm_png_start");
extern const unsigned char iconwifi_sm_png_end[] asm("_binary_iconwifi_sm_png_end");
// Tela configuração do servidor
extern const unsigned char utils_js_start[] asm("_binary_utils_js_start");
extern const unsigned char utils_js_end[] asm("_binary_utils_js_end");

typedef void (*function_t)(void);

// Strutura para verifiar arquivos estáticos
typedef struct static_content {
   const char *path;
   const unsigned char *data_start;
   const unsigned char *data_end;
   const char *content_type;
   bool is_gzip;
   function_t function;
} static_content_t;

int status_update_ota = 0;
static void ota_clear_status() {
   status_update_ota = 0;
}

static void set_restart_module() {
   esp_restart_timeout = 2000;
   esp_restart_now = 1;
}

// Restart, não está na rota de arquivos estáticos, porque precisa configurar o tempo de restart.
extern const unsigned char restart_html_start[] asm("_binary_restart_html_start");
extern const unsigned char restart_html_end[] asm("_binary_restart_html_end");

// Método OPTIONS -------------------------------------------------------------
static esp_err_t http_handle_options(httpd_req_t *req);

// Método GET -----------------------------------------------------------------
static esp_err_t http_handle_get(httpd_req_t *req);

// Recebe WIFI ----------------------------------------------------------------
static esp_err_t ap_recebe_config_wifi(char *struct_json);

// Método POST ----------------------------------------------------------------
static esp_err_t http_handle_post(httpd_req_t *req);

// Método para OTA ------------------------------------------------------------
static esp_err_t receive_ota_handle(httpd_req_t *req);

// Registros de estruturar de eventos -----------------------------------------
static httpd_uri_t http_uri_get = {
   .uri = "/*",
   .method = HTTP_GET,
   .handler = http_handle_get,
   .user_ctx = NULL};

static httpd_uri_t http_uri_post = {
   .uri = "/*",
   .method = HTTP_POST,
   .handler = http_handle_post,
   .user_ctx = NULL};

static httpd_uri_t http_uri_options = {
   .uri = "/*",
   .method = HTTP_OPTIONS,
   .handler = http_handle_options,
   .user_ctx = NULL};

static httpd_uri_t http_receive_ota_handle = {
   .uri = "/otaupdatefirmware",
   .method = HTTP_POST,
   .handler = receive_ota_handle,
   .user_ctx = NULL};

// Função de inicio da aplicaçõo de servidor
static void start_webserver(void);
static int get_config_web_server(char *struct_json, int length);
static int set_config_web_server(char *struct_json);
static int get_config_device(char *struct_json, int length);
static int set_config_device(char *struct_json, int length);
static int ap_recebe_config_pic(cJSON *json_config, pic_pacote_config_t *config_pic, bool pass_tecnnic);
static int ap_get_configaccess(char *p);
static int ap_set_configaccess(char *struct_json);

static static_content_t content_list[] = {
   {"/", index_html_start, index_html_end, "text/html", false, NULL},
   {"/index.html", index_html_start, index_html_end, "text/html", false, NULL},
   {"/configwifi.html", configwifi_html_start, configwifi_html_end, "text/html", false, NULL},
   {"/mini.css", mini_css_start, mini_css_end, "text/css", false, NULL},
   {"/styles.css", styles_css_start, styles_css_end, "text/css", false, NULL},
   {"/configwifi.js", configwifi_js_start, configwifi_js_end, "text/javascript", false, NULL},
   {"/favicon.png", favicon_png_start, favicon_png_end, "image/x-icon", false, NULL},
   {"/manifest.json", manifest_json_start, manifest_json_end, "application/json", false, NULL},
   {"/configwebserver.html", configwebserver_html_start, configwebserver_html_end, "text/html", false, NULL},
   {"/configwebserver.js", configwebserver_js_start, configwebserver_js_end, "text/javascript", false, NULL},
   {"/configdevice.html", configdevice_html_start, configdevice_html_end, "text/html", false, NULL},
   {"/configdevice.js", configdevice_js_start, configdevice_js_end, "text/javascript", false, NULL},
   {"/configaccess.html", configaccess_html_start, configaccess_html_end, "text/html", false, NULL},
   {"/configaccess.js", configaccess_js_start, configaccess_js_end, "text/javascript", false, NULL},
   {"/report.html", report_html_start, report_html_end, "text/html", false, NULL},
   {"/report.js", report_js_start, report_js_end, "text/javascript", false, NULL},
   {"/realtime.html", realtime_html_start, realtime_html_end, "text/html", false, NULL},
   {"/realtime.js", realtime_js_start, realtime_js_end, "text/javascript", false, NULL},
   {"/utils.js", utils_js_start, utils_js_end, "text/javascript", false, NULL},
   {"/iconwifi_lg.png", iconwifi_lg_png_start, iconwifi_lg_png_end, "image", false, NULL},
   {"/iconwifi_md.png", iconwifi_md_png_start, iconwifi_md_png_end, "image", false, NULL},
   {"/iconwifi_sm.png", iconwifi_sm_png_start, iconwifi_sm_png_end, "image", false, NULL},
   {"/ota.html", ota_html_start, ota_html_end, "text/html", false, &ota_clear_status},
   {"/restart_module.html", restart_html_start, restart_html_end, "text/html", false, &set_restart_module}};