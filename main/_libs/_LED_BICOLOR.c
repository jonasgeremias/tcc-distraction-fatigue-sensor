#ifndef __LED_BICOLOR__
   #ifndef LED_BICOLOR_BASE_TEMPO_MS
      #error "defina a base de tempo do timer para LED_BICOLOR_BASE_TEMPO_MS."
   #endif
   #ifndef LED_VD
      #error "defina o pino do LED_VD."
   #endif
   #ifndef LED_VM
      #error "defina o pino do LED_VM."
   #endif

uint16_t pisca_led_status_timeout = 0;
uint16_t led_status_anterior = 0xffff;
uint16_t tempo_estrobo_status = 0;
uint16_t tempo_estrobo_erro = 0;
uint16_t troca_modo_led_status_timeout = 0;
static void controle_led_status(uint8_t modo);

static void IRAM_ATTR led_bicolor_timer() {
   if (pisca_led_status_timeout) pisca_led_status_timeout--;
   if (tempo_estrobo_status) tempo_estrobo_status--;
   if (troca_modo_led_status_timeout) troca_modo_led_status_timeout--;
	if (tempo_estrobo_erro) tempo_estrobo_erro--;
}

static void led_bicolor_configura_pinos() {
   gpio_pad_select_gpio(LED_VD);
   gpio_set_direction(LED_VD, GPIO_MODE_OUTPUT);
   gpio_pad_select_gpio(LED_VM);
   gpio_set_direction(LED_VM, GPIO_MODE_OUTPUT);
   gpio_set_level(LED_VD, 1);
   gpio_set_level(LED_VM, 0);
}

// Saidas LEDs ----------------------------------------------------------------
static void controle_led_status(uint8_t modo) {
   static bool s_led_vd = 0, s_led_vm = 0;
	static uint8_t led_status_anterior = 0;

	// Se mudar de status tem que atualizar na hora
   if (modo != led_status_anterior) {
      pisca_led_status_timeout = 0;
      troca_modo_led_status_timeout = 0;
   }
   led_status_anterior = modo;
   
   if (!modo) { // Desliga
      s_led_vd = 0;
      s_led_vm = 0;
   }
   else if (modo == 1) {  // Liga direto verde
      s_led_vd = 1;
      s_led_vm = 0;
   }
   else if (modo == 2) {  // Liga direto vermelho
      s_led_vd = 0;
      s_led_vm = 1;
   }
   else if (modo == 3) { // Pisca lento verde
      if (!pisca_led_status_timeout) {
         pisca_led_status_timeout = (350 / LED_BICOLOR_BASE_TEMPO_MS);
			s_led_vd = !s_led_vd;
         s_led_vm = 0;
      }
   }
   else if (modo == 4) { // Pisca lento vermelho
      if (!pisca_led_status_timeout) {
         pisca_led_status_timeout = (350 / LED_BICOLOR_BASE_TEMPO_MS);
			s_led_vm = !s_led_vm;
         s_led_vd = 0;
      }
   }
   else if (modo == 5) { // Pisca r�pido verde
      if (!pisca_led_status_timeout) {
         pisca_led_status_timeout = (50 / LED_BICOLOR_BASE_TEMPO_MS);
			s_led_vd = !s_led_vd;
         s_led_vm = 0;
      }
   }
   else if (modo == 6) { // Pisca r�pido vermelho
      if (!pisca_led_status_timeout) {
         pisca_led_status_timeout = (50 / LED_BICOLOR_BASE_TEMPO_MS);
			s_led_vm = !s_led_vm;
         s_led_vd = 0;
      }
   }
   else if (modo == 7) { // Estrobo por 0,25s a cada 1s
      if (!troca_modo_led_status_timeout || (troca_modo_led_status_timeout >= (1000 / LED_BICOLOR_BASE_TEMPO_MS))) troca_modo_led_status_timeout = (1000 / LED_BICOLOR_BASE_TEMPO_MS);
      if (troca_modo_led_status_timeout <= (250 / LED_BICOLOR_BASE_TEMPO_MS)) {
         if (!pisca_led_status_timeout) {
            pisca_led_status_timeout = (50 / LED_BICOLOR_BASE_TEMPO_MS);
            s_led_vd = !s_led_vd;
            s_led_vm = 0;
         }
      }
      else {
         s_led_vd = 0;
         s_led_vm = 0;
      }
   }
   else if (modo == 8) { // Estrobo por 0,25s a cada 1s
      if (!troca_modo_led_status_timeout || (troca_modo_led_status_timeout >= (1000 / LED_BICOLOR_BASE_TEMPO_MS))) troca_modo_led_status_timeout = (1000 / LED_BICOLOR_BASE_TEMPO_MS);
      if (troca_modo_led_status_timeout <= (250 / LED_BICOLOR_BASE_TEMPO_MS)) {
         if (!pisca_led_status_timeout) {
            pisca_led_status_timeout = (50 / LED_BICOLOR_BASE_TEMPO_MS);
            s_led_vm = !s_led_vm;
            s_led_vd = 0;
         }
      }
      else {
         s_led_vd = 0;
         s_led_vm = 0;
      }
   }
   else if (modo == 9) { // Estrobo por 0,25s a cada 1s
      if (!troca_modo_led_status_timeout || (troca_modo_led_status_timeout >= (500 / LED_BICOLOR_BASE_TEMPO_MS))) troca_modo_led_status_timeout = (500 / LED_BICOLOR_BASE_TEMPO_MS);
      if (troca_modo_led_status_timeout <= (100 / LED_BICOLOR_BASE_TEMPO_MS)) {
         if (!pisca_led_status_timeout) {
            pisca_led_status_timeout = (100 / LED_BICOLOR_BASE_TEMPO_MS);
            s_led_vd = !s_led_vd;
            s_led_vm = 0;
         }
      }
      else {
         s_led_vd = 0;
         s_led_vm = 0;
      }
   }
   else if (modo == 10) { // Estrobo por 0,25s a cada 1s
      if (!troca_modo_led_status_timeout || (troca_modo_led_status_timeout >= (500 / LED_BICOLOR_BASE_TEMPO_MS))) troca_modo_led_status_timeout = (500 / LED_BICOLOR_BASE_TEMPO_MS);
      if (troca_modo_led_status_timeout <= (100 / LED_BICOLOR_BASE_TEMPO_MS)) {
         if (!pisca_led_status_timeout) {
            pisca_led_status_timeout = (100 / LED_BICOLOR_BASE_TEMPO_MS);
            s_led_vm = !s_led_vm;
            s_led_vd = 0;
         }
      }
      else {
         s_led_vd = 0;
         s_led_vm = 0;
      }
   }
	gpio_set_level(LED_VD, s_led_vd);
   gpio_set_level(LED_VM, s_led_vm);
}
#endif
#define __LED_BICOLOR__