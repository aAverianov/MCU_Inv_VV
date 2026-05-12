#ifndef BUTTONS_H
#define BUTTONS_H
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BTN_COUNT	7
#define DEBOUNCE_MS     20

enum buttons {
	BTN_START_LOC,
	BTN_STOP_LOC,
	BTN_RST_LOC,
	BTN_DST_LOC,
	BTN_START_DST,
	BTN_STOP_DST,
	BTN_RST_DST,	
}; 

#define BTN_START_LOC_PORT       GPIOA
#define BTN_START_LOC_PIN        GPIO_PIN_7
#define BTN_STOP_LOC_PORT        GPIOA
#define BTN_STOP_LOC_PIN         GPIO_PIN_6
#define BTN_RST_LOC_PORT         GPIOA
#define BTN_RST_LOC_PIN          GPIO_PIN_5
#define BTN_DST_LOC_PORT         GPIOB
#define BTN_DST_LOC_PIN			 GPIO_PIN_11
#define BTN_START_DST_PORT       GPIOC
#define BTN_START_DST_PIN        GPIO_PIN_5
#define BTN_STOP_DST_PORT        GPIOB
#define BTN_STOP_DST_PIN         GPIO_PIN_0
#define BTN_RST_DST_PORT         GPIOB
#define BTN_RST_DST_PIN          GPIO_PIN_1

typedef struct {
	GPIO_TypeDef *port; 
	uint16_t      pin; /* (GPIO_PIN_x)  */
	uint8_t       raw; 
	uint8_t       stable;
	uint8_t       prev; 
	uint32_t      change_tick;
	bool          pressed; 
	bool          released; 
} Button_t;

void buttons_Process(void);

bool button_IsPressed(uint8_t idx);

bool button_IsReleased(uint8_t idx);

#ifdef __cplusplus
}
#endif
	
#endif /* BUTTONS_H_ */

