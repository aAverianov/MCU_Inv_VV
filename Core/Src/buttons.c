#include "buttons.h"

static Button_t buttons[BTN_COUNT] = {
	{ BTN_START_LOC_PORT, BTN_START_LOC_PIN, 1, 1, 1, 0, false, false },
	{ BTN_STOP_LOC_PORT, BTN_STOP_LOC_PIN, 1, 1, 1, 0, false, false }, 
	{ BTN_RST_LOC_PORT, BTN_RST_LOC_PIN, 1, 1, 1, 0, false, false }, 
	{ BTN_DST_LOC_PORT, BTN_DST_LOC_PIN, 1, 1, 1, 0, false, false },
	{ BTN_START_DST_PORT, BTN_START_DST_PIN, 1, 1, 1, 0, false, false },
	{ BTN_STOP_DST_PORT, BTN_STOP_DST_PIN, 1, 1, 1, 0, false, false }, 
	{ BTN_RST_DST_PORT, BTN_RST_DST_PIN, 1, 1, 1, 0, false, false }, 
};

void buttons_Process(void)
{
	uint32_t now = HAL_GetTick(); 

	for (uint8_t i = 0; i < BTN_COUNT; i++) {
		Button_t *b = &buttons[i];
		uint8_t raw = (HAL_GPIO_ReadPin(b->port, b->pin) == GPIO_PIN_SET) ? 1 : 0;
		
		if (raw != b->raw) {
			b->raw         = raw;
			b->change_tick = now;
		}

			if ((now - b->change_tick) >= DEBOUNCE_MS) {
			b->prev   = b->stable;
			b->stable = b->raw;

			
			if (b->prev == 1 && b->stable == 0) {
				b->pressed = true;
			}
			
			if (b->prev == 0 && b->stable == 1) {
				b->released = true;
			}
		}
	}
}


bool button_IsPressed(uint8_t idx)
{
	if (idx >= BTN_COUNT) return false;
	bool ev = buttons[idx].pressed;
	buttons[idx].pressed = false; 
	return ev;
}

bool button_IsReleased(uint8_t idx)
{
	if (idx >= BTN_COUNT) return false;
	bool ev = buttons[idx].released;
	buttons[idx].released = false;
	return ev;
}

