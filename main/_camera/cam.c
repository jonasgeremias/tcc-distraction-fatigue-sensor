#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include "esp_camera.h"

static const char *TAG = "Camera";
#define MOUNT_POINT "/sdcard"


#define BOARD_ESP32CAM_AITHINKER 1
#ifdef BOARD_ESP32CAM_AITHINKER
#define CAM_PIN_PWDN (gpio_num_t) 32
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK (gpio_num_t) 0
#define CAM_PIN_SIOD (gpio_num_t) 26
#define CAM_PIN_SIOC (gpio_num_t) 27
#define CAM_PIN_D7 (gpio_num_t) 35
#define CAM_PIN_D6 (gpio_num_t) 34
#define CAM_PIN_D5 (gpio_num_t) 39
#define CAM_PIN_D4 (gpio_num_t) 36
#define CAM_PIN_D3 (gpio_num_t) 21
#define CAM_PIN_D2 (gpio_num_t) 19
#define CAM_PIN_D1 (gpio_num_t) 18
#define CAM_PIN_D0 (gpio_num_t) 5
#define CAM_PIN_VSYNC (gpio_num_t) 25
#define CAM_PIN_HREF (gpio_num_t) 23
#define CAM_PIN_PCLK (gpio_num_t) 22
#endif


static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sscb_sda = CAM_PIN_SIOD,
    .pin_sscb_scl = CAM_PIN_SIOC,
    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    // XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 10000000,
    .ledc_timer = LEDC_TIMER_0, //LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0, //LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_RGB565, //PIXFORMAT_RGB565, // PIXFORMAT_RGB565, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_VGA,    // FRAMESIZE_CIF, QQVGA-UXGA Do not use sizes above QVGA when not JPEG
    .jpeg_quality = 1, // 0-63 lower number means higher quality
    .fb_count = 2       // if more than one, i2s runs in continuous mode. Use only with JPEG
};

static esp_err_t init_camera()
{
    // initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }

    return ESP_OK;
}


static esp_err_t init_sdcard()
{
  esp_err_t ret = ESP_FAIL;

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 500,
      .allocation_unit_size = 16 * 1024
  };
  sdmmc_card_t *card;

  const char mount_point[] = MOUNT_POINT;
  ESP_LOGI(TAG, "Initializing SD card");

  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

  ESP_LOGI(TAG, "Mounting SD card...");
  gpio_set_pull_mode((gpio_num_t) 15, GPIO_PULLUP_ONLY);   // CMD, needed in 4- and 1- line modes
  gpio_set_pull_mode((gpio_num_t) 2, GPIO_PULLUP_ONLY);    // D0, needed in 4- and 1-line modes
  gpio_set_pull_mode((gpio_num_t) 4, GPIO_PULLUP_ONLY);    // D1, needed in 4-line mode only
  gpio_set_pull_mode((gpio_num_t) 12, GPIO_PULLUP_ONLY);   // D2, needed in 4-line mode only
  gpio_set_pull_mode((gpio_num_t) 13, GPIO_PULLUP_ONLY);   // D3, needed in 4- and 1-line modes

  ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

  if (ret == ESP_OK)
  {
    ESP_LOGI(TAG, "SD card mount successfully!");
  }
  else
  {
    ESP_LOGE(TAG, "Failed to mount SD card VFAT filesystem. Error: %s", esp_err_to_name(ret));
  }

  return ret;
}
