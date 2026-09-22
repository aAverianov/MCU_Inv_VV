#ifndef ADC_FILTER_H
#define ADC_FILTER_H
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define COUNT_FILTER    16

	typedef union
	{
		uint32_t Val;
		struct
		{
			unsigned Flag : 1;
			unsigned Index : 8;
			unsigned Filter_sum : 23;
		} Reg;
	} FILTER_REG;

	void filter_sred(uint16_t ADC_val, uint16_t* buf, FILTER_REG* filter_reg);
	uint16_t get_filter_value(FILTER_REG* filter_reg);

#ifdef __cplusplus
}
#endif
	
#endif /* ADC_FILTER_H_ */

