/*
 * Flash.h
 *
 *  Created on: May 30, 2023
 *      Author: Bulat
 */

#ifndef FLASH_H_
#define FLASH_H_

#include "stm32f1xx_hal.h"
// --- Выбор мк ---
#define STM32F103
//#define STM32F407
// ----------------
#ifdef STM32F103
	#define PAGE_CAPACITY 1024		// все страницы по 1024 байт
#endif
#ifdef STM32F407
	#define PAGE_CAPACITY 131072	// емкость каждого сектора начиная с №5 (0x8020000) по №11 (0x80E0000)
#endif
class Flash {
public:
	Flash(uint32_t startAddr, uint16_t numPages, uint16_t numHalfWord);
	virtual ~Flash();
	bool ReadData(uint16_t *data);
	bool WriteData(uint16_t *data);
private:
	uint16_t* data_;
	void EraseFlash();						// стереть N страниц начиная с заданного адреса
	void SearchAddrLast();					//
	void Read();
	uint32_t startAddr_ = 0;				// начальный адрес во флэше
	uint32_t endAddr_ = 0;					// конечный адрес во флэше
	uint16_t numPages_ = 0;					// число страниц (F103) или секторов (F407)
	uint16_t numHalfWord_ = 0;				// число 16-бит. слов которые записываюися или считываются
	uint32_t errorCode_ = 0;				// код завершения операции (0 - ошибок нет)
	FLASH_EraseInitTypeDef EraseInitStruct;	// структура для очистки флеша
	uint32_t pageError_ = 0;				// переменная, в которую запишется адрес страницы при неудачном стирании
	uint32_t addrLastData_ = 0;				// адрес последнего блока данных ("1"-память чистая)
	uint32_t addrEmptySpace_ = 0;			// адрес начала свободной памяти
};

#endif /* FLASH_H_ */
