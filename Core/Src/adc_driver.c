#include "adc_filter.h"

void filter_sred(uint16_t ADC_val, uint16_t* buf, FILTER_REG* filter_reg) {
	if (filter_reg->Reg.Flag) {
		filter_reg->Reg.Filter_sum -= buf[filter_reg->Reg.Index];
		filter_reg->Reg.Filter_sum += ADC_val;
		buf[filter_reg->Reg.Index] = ADC_val;
		if (filter_reg->Reg.Index >= COUNT_FILTER - 1) {
			filter_reg->Reg.Index = 0;
		}
		else {
			filter_reg->Reg.Index++;
		}
	}
	else {
		filter_reg->Reg.Filter_sum += ADC_val;
		buf[filter_reg->Reg.Index] = ADC_val;
		if (filter_reg->Reg.Index >= COUNT_FILTER - 1) {
			filter_reg->Reg.Index = 0;
			filter_reg->Reg.Flag = 1;
		}
		else {
			filter_reg->Reg.Index++;
		}
	}
}

uint16_t get_filter_value(FILTER_REG* filter_reg) {
	
	return (filter_reg->Reg.Filter_sum / COUNT_FILTER);
}