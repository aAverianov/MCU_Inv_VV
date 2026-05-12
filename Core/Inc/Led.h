#ifndef LED_H
#define LED_H
#include "stm32f1xx_hal.h"
#include "main.h"
//Для физического управления светодиодом
/*
#define LEDON				HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET)
#define LEDOFF				HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET)
#define LEDTOGGLE			HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin)
#define LEDREAD				HAL_GPIO_ReadPin(LED_GPIO_Port, LED_Pin)
*/

// Для синхронизации (непрерывно работающие таймеры)
enum timings {
	TIME_TO_CONTACT,	// тайминг для связи с преобразователем
	TIME_COMMUNICATION,	// тайминг для формирования периодического сигнала "связь в порядке"
	TIME_TO_BUTTONS,
	TIMING4,
	TIMING5,
};
// Для одноразовых таймеров (требуется запуск каждый раз)
enum timers {
	TIMER_1S, // таймер одноразовый 1 сек
	TIMER2,
	TIMER3,
	TIMER4,
};

class Led {
public:
	Led(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, uint32_t timeShort, uint32_t timeShortPause, uint32_t timeLong, uint32_t timeLongPause);
	void Tick();					// тик++
	void SetTiming(uint32_t timing0, uint32_t timing1, uint32_t timing2, uint32_t timing3, uint32_t timing4);  	// установка времени для таймингов (постоянно работающие таймеры))
	bool Timing(uint16_t timerNumber);				// получить состояние тайминга
	void SetTimers(uint32_t timer0, uint32_t timer1, uint32_t timer2, uint32_t timer3);	// установки для одноразовых таймеров
	void StartTimer(uint16_t timerNumber);			// запустить одноразовый таймер
	void StopTimer(uint16_t timerNumber);
	void ResumeTimer(uint16_t timerNumber);
	uint32_t GetTimer(uint16_t timerNumber);		// получить состояние одноразового таймера
	void ShortFlash();				// формирователь коротких вспышек
	void LongFlash();				// формирователь длинных вспышек
	void SetShortTime(uint32_t timeShort, uint32_t timeShortPause);		// установка времен для коротких вспышек
	void SetLongTime(uint32_t timeLong, uint32_t timeLongPause);		// установка времен для длинных вспышек
	void RunStopWatch();
	uint32_t StopStopWatch();
private:
	GPIO_TypeDef *_GPIOx;			// порт
	uint16_t _GPIO_Pin;				// маска вывода
	bool _status;					// текущее состояние led (on/off)
	uint32_t _timeCurrent;			// текущее время в одном цикле моргания(мс)
	// таймеры постоянно работающие
	uint32_t _time0;				// время для тайминга-0 в тиках (мс)
	uint32_t _timing0;				// длительность времени в тайминге в тиках (мс)
	uint32_t _time1;				// время для тайминга-1 в тиках (мс)
	uint32_t _timing1;				// длительность времени в тайминге в тиках (мс)
	uint32_t _time2;				// время для тайминга-2 в тиках (мс)
	uint32_t _timing2;				// длительность времени в тайминге в тиках (мс)
	uint32_t _time3;				// время для тайминга-3 в тиках (мс)
	uint32_t _timing3;				// длительность времени в тайминге в тиках (мс)
	uint32_t _time4;				// время для тайминга-4 в тиках (мс)
	uint32_t _timing4;				// длительность времени в тайминге в тиках (мс)
	// таймеры одноразовые (требуют запуска на каждый цикл)
	uint32_t _timer0;				// время для таймера-0 в тиках (мс)
	uint32_t _timeDown0;			// текущий отсчет для таймера-0 в тиках (мс)
	bool _stopTimer0;				// для заморозки таймера-0 (только для него одного)
	uint32_t _timer1;				// время для таймера-1 в тиках (мс)
	uint32_t _timeDown1;			// текущий отсчет для таймера-1 в тиках (мс)
	uint32_t _timer2;				// время для таймера-2 в тиках (мс)
	uint32_t _timeDown2;			// текущий отсчет для таймера-2 в тиках (мс)
	uint32_t _timer3;				// время для таймера-3 в тиках (мс)
	uint32_t _timeDown3;			// текущий отсчет для таймера-3 в тиках (мс)
	// таймеры с фикс.отсчетами (одноразовые)
	uint32_t _timer4;				// время для таймера-4 в тиках (мс)
	uint32_t _timeDown4;			// текущий отсчет для таймера-4 в тиках (мс)
	uint32_t _timer5;				// время для таймера-4 в тиках (мс)
	uint32_t _timeDown5;			// текущий отсчет для таймера-4 в тиках (мс)
	// для светодиодов
	uint32_t _timeShort;				// длительность короткой вспышки (мс)
	uint32_t _timeShortPause;			// длительность паузы при короткой вспышке (мс)
	uint32_t _timeLong;				// длительность долгой вспышки (мс)
	uint32_t _timeLongPause;			// длительность паузы при длинной вспышке (мс)
	// секундомер
	bool _stopWatch;				// разрешение отсчета секундомера
	uint32_t _timeStopWatch;		// время секундомера
};
#endif
