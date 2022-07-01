
// Timer ----------------------------------------------------------------------

#ifndef __TIMER_0__
   #ifndef TIMER_ISR_VOID
      #define TIMER_0_ERROR
      #error "defina a funcao TIMER_ISR_VOID"
   #endif

   #ifndef TIMER_0_ERROR
      #include <stddef.h>
      #include "esp_intr_alloc.h"
      #include "esp_attr.h"
      #include "driver/timer.h"
      #include <stdio.h>
      #include <string.h>
static intr_handle_t s_timer_handle;

static void IRAM_ATTR
   timer_isr(void *arg) {
   TIMERG0.int_clr_timers.t0 = 1;
   TIMERG0.hw_timer[0].config.alarm_en = 1;
   TIMER_ISR_VOID;
}

static void init_timer(int timer_period_us) {
   timer_config_t config;
   memset(&config, 0, sizeof(timer_config_t));
   config.alarm_en = (timer_alarm_t) true;
   config.counter_en = (timer_start_t) false;
   config.intr_type = TIMER_INTR_LEVEL;
   config.counter_dir = TIMER_COUNT_UP;
   config.auto_reload = (timer_autoreload_t) true;
   config.divider = 80;
   timer_init(TIMER_GROUP_0, TIMER_0, &config);
   timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
   timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, timer_period_us);
   timer_enable_intr(TIMER_GROUP_0, TIMER_0);
   timer_isr_register(TIMER_GROUP_0, TIMER_0, &timer_isr, NULL, 0, &s_timer_handle);
   timer_start(TIMER_GROUP_0, TIMER_0);
}
   #endif
#endif
#define __TIMER_0__