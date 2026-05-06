/*
 * Flash.cpp
 *
 *  Created on: May 30, 2023
 *      Author: Bulat
 */

#include "Flash.h"

Flash::Flash(uint32_t startAddr, uint16_t numPages, uint16_t numHalfWord) {
	startAddr_ = startAddr;			// начальный адрес во флэше
	numPages_ = numPages;			// число страниц (F103) или секторов (F407)
	numHalfWord_ = numHalfWord;		// число 16-бит. слов которые записываюися или считываются
	endAddr_ = startAddr_ + numPages_ * PAGE_CAPACITY;	// конечный адрес
	data_ = new uint16_t[numHalfWord];
	for (int i = 0; i < numHalfWord_; i++)
	{
		data_[i] = 0;				// почистить
	}
	SearchAddrLast();				// искать данные
	if (addrLastData_ == 0)			// если требуется стирание
	{
		EraseFlash();
	}
	Read();							// считать данные(если они есть)
}

Flash::~Flash() {
	// TODO Auto-generated destructor stub
}


/* Метод очистки сектора флэш-памяти
 * Коды ошибок:
 * HAL_FLASHEx_Erase(FLASH_EraseInitTypeDef *pEraseInit, uint32_t *PageError):
  	  HAL_OK       = 0x00U,
  	  HAL_ERROR    = 0x01U,
  	  HAL_BUSY     = 0x02U,
  	  HAL_TIMEOUT  = 0x03U

  HAL_FLASH_GetError():
  	  HAL_FLASH_ERROR_NONE      0x00U  /!< No error /
  	  HAL_FLASH_ERROR_PROG      0x01U  /!< Programming error /
  	  HAL_FLASH_ERROR_WRP       0x02U  /!< Write protection error /
  	  HAL_FLASH_ERROR_OPTV      0x04U  /!< Option validity error /
 */
void Flash::EraseFlash() {
	errorCode_ = 0;											// 0 - ошибок нет
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES; 		// постраничная очистка,(если: FLASH_TYPEERASE_MASSERASE - тогда очистка всего флеша)
	EraseInitStruct.PageAddress = startAddr_;				// страница подлежащая стиранию
	EraseInitStruct.NbPages = numPages_; 					// кол-во страниц для стирания
	HAL_FLASH_Unlock(); 									// разблокировать флеш
	if (HAL_FLASHEx_Erase(&EraseInitStruct, &pageError_) != HAL_OK)
	{
		errorCode_ = HAL_FLASH_GetError();					// получить код ошибки
	}
	HAL_FLASH_Lock();
	if (errorCode_ == 0)		// если стирание успешно
	{
		addrLastData_ = 1;				// признак - память чиста
		addrEmptySpace_ = startAddr_;	// запись во флэш будет вестись с самого начала
	}
}

/*
* Метод ПОИСКА ПОСЛЕДНИХ ДАННЫХ и свободного места во флэше
*
* вычисляет адрес и записывает его в переменную
* 	в которой находится начало актуального блока данных,
* 	либо записывает в нее "1" если данных нет (память чиста)
* 	либо "0" если в памяти бардак и надо стереть эту страницу
* вычисляет адрес начала свободной памяти и записывает его в переменную
* 	либо "0" если в памяти бардак и надо стереть эту страницу
*/
void Flash::SearchAddrLast() {
	uint32_t addr;
	addr = startAddr_;
	addrLastData_ = 1;
	addrEmptySpace_ = addr;							// адрес начала свободной памяти для начала...
	while (*(uint32_t*) addr != 0xFFFFFFFF) 		// пока в ячейке памяти не появится значение "все единицы"(признак свободного пространства)
	{
		addrLastData_ = addr;						// возможно, это адрес последнего блока данных
		addr = *(uint32_t*)addr;					// получить адрес следующего блока данных
		if (addr < startAddr_ || addr > endAddr_) {	// если полученный адрес некорректен
			addrLastData_ = 0;						// тогда признак неправильных данных -> надо стирать память
			addrEmptySpace_ = 0;					// свободной памяти пока нет (появится после стирания)
 			break;									// и на выход
 		}
		addrEmptySpace_ = addr;						// возможно, это адрес начала свободной памяти
	}
}

void Flash::Read() {
	uint32_t addr;
	if (addrLastData_ > 1)
	{
		addr = addrLastData_ + 4;					// адрес данных (т.к. сначала идет адрес 32-битнйый)
		for (int i = 0; i < numHalfWord_; i++)
		{
			data_[i] = *(uint32_t*)addr;				// читать данные
			addr += 2;								// след.адрес
		}
	}
}

bool Flash::ReadData(uint16_t *data) {
	if (addrLastData_ == 1) return 0;	// данных нет
	for (int i = 0; i < numHalfWord_; i++)
	{
		data[i] = data_[i];				// читать данные
	}
	return 1;
}

bool Flash::WriteData(uint16_t *data) {
	uint32_t addr = addrEmptySpace_;
	uint32_t addrNext = addrEmptySpace_ + 4 + numHalfWord_ * 2;	// адрес след.блока данных = адрес(32 бита)+данные(n*16 бит)
	if ((addrNext + 4) > endAddr_)								// если блок   [адрес(32 бита)+данные(n*16 бит)+резерв(32 бита)]
	{															// выходит за пределы
		EraseFlash();											// стереть
	}
	if (errorCode_ == 0)	// если ошибок нет
	{
		HAL_FLASH_Unlock(); 									// разблокировать флеш
		// первые 32 бита это адрес следующего блока данных
		if(HAL_FLASH_Program(0x02U, addr, addrNext) != HAL_OK) 	// если запись вызвала ошибку (в первую ячейку блока данных адрес который указывает на след.свободный блок памяти)
		{
			errorCode_ = HAL_FLASH_GetError();					// получить код ошибки
		}
		if (errorCode_ == 0)
		{
			addr += 4;		// теперь данные
			for(int i = 0; i < numHalfWord_; i++)
			{
				if (HAL_FLASH_Program(0x01U, addr, data[i]) != HAL_OK) 	// запись 16-битными полусловами
				{
					errorCode_ = HAL_FLASH_GetError();					// получить код ошибки
					break;
				}
				addr += 2;	// следующие 2 байта
			}
			HAL_FLASH_Lock(); 										// заблокировать флеш
		}
	}
	return errorCode_;
}
