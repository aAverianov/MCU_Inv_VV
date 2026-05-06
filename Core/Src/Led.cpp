#include "Led.h"

Led::Led(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, uint32_t timeShort, uint32_t timeShortPause, uint32_t timeLong, uint32_t timeLongPause) {
	_GPIOx = GPIOx;
	_GPIO_Pin = GPIO_Pin;
	_timeShort = timeShort;
	_timeShortPause = timeShortPause;
	_timeLong = timeLong;
	_timeLongPause = timeLongPause;
	_time0 = 0;
	_timing0 = 1000;
	_time1 = 0;
	_timing1 = 1000;
	_time2 = 0;
	_timing2 = 1000;
	_time3 = 0;
	_timing3 = 1000;
	_time4 = 0;
	_timing4 = 1000;
	_timeCurrent = 0;
	_status = false;
	_timer0 = 10000;
	// таймер-0 может останавливаться и продолжать отсчет
	_timeDown0 = 0;			// изначально таймер остановлен
	_stopTimer0 = 0;		// таймер разморожен
	_timer1 = 10000;
	_timeDown1 = 0;
	_timer2 = 10000;
	_timeDown2 = 0;
	_timer3 = 10000;
	_timeDown3 = 0;
	// таймеры 4,5 - с фикс.отсчетами
	_timer4 = 60000;		// 60 сек
	_timeDown4 = 0;
	_timer5 = 7000;			// 7 сек
	_timeDown5 = 0;
	_stopWatch = 0;			// разрешение отсчета секундомера
	_timeStopWatch = 0;		// время секундомера
}

void Led::Tick() {
	_timeCurrent++;
	// отсчеты для периодических таймингов
	_time0++;
	_time1++;
	_time2++;
	_time3++;
	_time4++;
	// отсчеты для одноразовых таймеров
	if(_timeDown0 > 0 && !_stopTimer0) _timeDown0--;
	if(_timeDown1 > 0)_timeDown1--;
	if(_timeDown2 > 0)_timeDown2--;
	if(_timeDown3 > 0)_timeDown3--;
	if(_timeDown4 > 0)_timeDown4--;
	if(_timeDown5 > 0)_timeDown5--;
	// отсчет для секундомера
	if (_stopWatch) _timeStopWatch++;
}

// Запустить секундомер
void Led::RunStopWatch() {
	_timeStopWatch = 0;		// обнулить секундомер
	_stopWatch = 1;
}

// Остановить секундомер
// Выдает зафиксированное время в мс
uint32_t Led::StopStopWatch() {
	_stopWatch = 1;
	return _timeStopWatch;
}

// Управление короткими вспышками
void Led::ShortFlash() {
	if (_status) {								// горит?
		if (_timeCurrent > _timeShort) {				// 	да, время вышло?
			HAL_GPIO_WritePin(_GPIOx, _GPIO_Pin, GPIO_PIN_RESET);	//		да, выкл. led
			_timeCurrent = 0;					//			обнулить счетчик
			_status = false;					//			сбросить флаг "Светодиод вкл"
		}
	}
	else {									// выключен
		if (_timeCurrent > _timeShortPause) {				// 	время вышло?
			HAL_GPIO_WritePin(_GPIOx, _GPIO_Pin, GPIO_PIN_SET);	//	да, вкл. led
			_timeCurrent = 0;					//		обнулить счетчик
			_status = true;						//		установить флаг "Светодиод вкл"
		}
	}
}

// Управление длинными вспышками
void Led::LongFlash() {
	if (_status) {								// горит?
		if (_timeCurrent > _timeLong) {					// 	да, время вышло?
			HAL_GPIO_WritePin(_GPIOx, _GPIO_Pin, GPIO_PIN_RESET);	//		да, выкл. led
			_timeCurrent = 0;					//			обнулить счетчик
			_status = false;
		}
	}
	else {									// выключен
		if (_timeCurrent > _timeLongPause) {				// 	время вышло?
			HAL_GPIO_WritePin(_GPIOx, _GPIO_Pin, GPIO_PIN_SET);	//	да, вкл. led
			_timeCurrent = 0;					//		обнулить счетчик
			_status = true;
		}
	}
}

// Установка времени для внутренних таймеров (работают постоянно)
void Led::SetTiming(uint32_t timing0, uint32_t timing1, uint32_t timing2, uint32_t timing3, uint32_t timing4) {
	_timing0 = timing0;
	_timing1 = timing1;
	_timing2 = timing2;
	_timing3 = timing3;
	_timing4 = timing4;
}

// Запуск одноразовых таймеров
void Led::StartTimer(uint16_t timerNumber) {
	switch (timerNumber) {
	case 0:
		_timeDown0 = _timer0;	// загрузить таймер исх.значением
		break;
	case 1:
		_timeDown1 = _timer1;
		break;
	case 2:
		_timeDown2 = _timer2;
		break;
	case 3:
		_timeDown3 = _timer3;
		break;
	case 4:
		_timeDown4 = _timer4;
		break;
	case 5:
		_timeDown5 = _timer5;
		break;
	}
}

// Остановить таймер (заморозить)
void Led::StopTimer(uint16_t timerNumber) {
	switch (timerNumber) {
	case 0:
		_stopTimer0 = 1;	// остановить таймер
		break;
	}

}

// Продолжить таймер (рааморозить)
void Led::ResumeTimer(uint16_t timerNumber) {
	switch (timerNumber) {
	case 0:
		_stopTimer0 = 0;	//  таймер разморозить
		break;
	}

}
// Установка времен для одноразовых таймеров
// timer-4 всегда 1 мин. timer-5 всегда 7 сек.
void Led::SetTimers(uint32_t timer0, uint32_t timer1, uint32_t timer2, uint32_t timer3) {
	_timer0 = timer0;
	_timer1 = timer1;
	_timer2 = timer2;
	_timer3 = timer3;
	//_timer4 = timer4;
}

// Получить оставшееся время для таймера(одноразового) в тиках (мс)
// 0 - если таймер остановлен
uint32_t Led::GetTimer(uint16_t timerNumber) {
	uint32_t remain = 0;
	switch (timerNumber) {
    case 0:
    	remain = _timeDown0;
	break;
    case 1:
    	remain = _timeDown1;
	break;
    case 2:
    	remain = _timeDown2;
	break;
    case 3:
    	remain = _timeDown3;
	break;
    case 4:
    	remain = _timeDown4;
	break;
    case 5:
    	remain = _timeDown5;
	break;
	}
	return remain;
}

// Получить состояние тайминга(для постоянно работающих таймеров)
// "1" - сработал и начал отсчет заново
// "0" - еще не сработал
bool Led::Timing(uint16_t timerNumber) {
  bool code = false;
  switch (timerNumber) {
    case 0:
      if (_time0 > _timing0) {
	_time0 = 0;
	code = true;
      }
      else code = false;
      break;
    case 1:
      if (_time1 > _timing1) {
        _time1 = 0;
	code = true;
      }
      else code = false;
      break;
    case 2:
      if (_time2 > _timing2) {
	_time2 = 0;
	code = true;
      }
      else code = false;
      break;
    case 3:
      if (_time3 > _timing3) {
	_time3 = 0;
	code = true;
      }
      else code = false;
      break;
    case 4:
      if (_time4 > _timing4) {
	_time4 = 0;
	code = true;
      }
      else code = false;
      break;
    default:
      code = false;
  }
  return code;
}

// Установка времен для коротких миганий
void Led::SetShortTime(uint32_t timeShort, uint32_t timeShortPause) {
	_timeShort = timeShort;
	_timeShortPause = timeShortPause;
}

// Установка времен для длинных миганий
void Led::SetLongTime(uint32_t timeLong, uint32_t timeLongPause) {
	_timeLong = timeLong;
	_timeLongPause = timeLongPause;
}

