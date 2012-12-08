#ifndef led_h
#define led_h

void led_init(void);
void led_set_rgb(uint8_t r, uint8_t g, uint8_t b);
void led_set_hsv(uint8_t h, uint8_t s, uint8_t v);
void led_pwm_p(void);
void led_pwm_s(void);

#endif
