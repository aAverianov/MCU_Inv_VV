/*
 * UARTDMA.cpp
 *
 *  Created on: 20 june 2023
 *      Author: admin
 */

#include "UARTDMA.h"
#include "stm32f1xx_hal_uart.h"
//bool flagTxComplete = 1;

UARTDMA::UARTDMA(UART_HandleTypeDef *uart)
{
	SCB_DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // разрешение счётчика
	DWT_CONTROL |= DWT_CTRL_CYCCNTENA_Msk;   // запуск счётчика
	optiUart = uart;
	flagTxComplete_ = 1;
}

// Инициализация UART
void UARTDMA::InitReceive() {
	HAL_UART_Receive_DMA(optiUart, rxBuffer_, ringBufferSize_);
}
// Методы начинающиеся с Read берут данные из массива в который нужные данные были помещены заранее
// при помощи методов начинающихся с Command

// Метод выдачи напряжения дуги
uint16_t UARTDMA::Voltage() {
	return read.uArc;
}

// Метод выдачи тока дуги
uint16_t UARTDMA::Current() {
	return read.iArc;
}

// Метод выдачи мощности дуги
uint16_t UARTDMA::Power() {
	return read.pArc;
}

// Метод выдачи состояния блока
uint16_t UARTDMA::Status() {
	return (uint16_t)read.status;
}


// Метод для выдачи в uart
// перед вызовом надо подготовить структуру
// Вх.: указатель на структуру
void UARTDMA::Request() {
	// Готовность порта UART
	while(!flagTxComplete_)
	{
		asm ("nop");						// ждать готовности
	}
	flagTxComplete_ = 0;					// сбросить флаг (готовность передатчика к новым командам)
	HAL_UART_Transmit_DMA(optiUart, write.allData, amountOfCommand_);
}

// Метод записи командной последовательности для исп.у-ва
// Вх.: байт команды
void UARTDMA::Command(uint8_t comm) {
	write.id = ID_COMMAND;
	write.command = comm;
	write.crc = CalculateCRC8(write.allData, 2); 
	Request();
}

// Выдает готовность UART к передаче
// "0" - не готов
// "1" - готов
bool UARTDMA::GetStatusTx() {
	return flagTxComplete_;
}

// Выдает состояние данных 
// "0" - данные не достоверны
// "1" - данные достоверны
bool UARTDMA::GetStatusData() {
	return isDataReliable_;
}

// Устанавливает статус передатчика: готов к передаче
void UARTDMA::SetStatusTx() {
	flagTxComplete_ = 1;
}


// Возвращает кол-во непрочитанных байтов
uint16_t UARTDMA::Available()
{
	getCount_ = __HAL_DMA_GET_COUNTER(optiUart->hdmarx);	// Возвращает кол-во оставшихся единиц данных в текущей передаче канала DMA
	writeIndex_ = getCount_ * (0 - ringBufferSize_) / ringBufferSize_ + ringBufferSize_;
	return writeIndex_ - readIndex_;
}

// Выдает очередной байт данных из приемного буфера
uint8_t UARTDMA::Read() {
	uint8_t res = rxBuffer_[readIndex_++];				// получить очередной байт
	if(readIndex_ == ringBufferSize_)					// если указатель дошел до конца буфера
	{
		readIndex_ = 0;									// указатель на начало буфера
		InitReceive();								// запустить прием
	}
	return res;
}

// Проверка приемного буфера и если есть непрочитанные данные
// парсинг и пересылка их в структуру. Пересылка осуществляется только в том случае
// если пакет данных удовлетворяет условиям.
// Вх.:  -
// Вых.: 0 - данных нет, либо посылка не соответствует стандарту
//		 x - кол-во принятых байт в посылке соответствующей стандарту
uint16_t UARTDMA::Poll() {
	uint64_t backup = read.data64;
	if (led.GetTimer(TIMER_1S) == 0)			// отсчет закончен?
	{
		isDataReliable_ = 0;				// да, данные потеряли достоверность
		read.data64 = 0;					// обнулить все данные
		backup = 0;		
	}
	uint16_t i = 0;							// простой счетчик байтов
	while (Available())					// пока есть данные в буфере
	{
		read.allData8[i++] = Read();	// сохранить очередной байт
		if (i == amountOfData_)				// если пакет полностью принят
		{
			// проверить CRC
			uint8_t crc = CalculateCRC8(read.allData8, amountOfData_ - 1);
			if (read.crc == crc)
			{
				isDataReliable_ = 1;		// данные достоверны
				led.StartTimer(TIMER_1S);		// запустить отсчет
			}
			else
			{
				i = 0;		// контр.сумма неправильна - кол-во принятых байт считать равным нулю

			}
			break;
		}
		//HAL_Delay(1);						// без этой задержки данные не успевают
		DWT->CYCCNT = 0; while (DWT->CYCCNT < 7200) {} // задержка 15 мкс (15 * 72 )
	}
	if (i != amountOfData_)					// если посылка нестандартная (кол-во байт в посылке неправильное
	{										// или ошибка в контр.сумме)
		read.data64 = backup;				// восстановить старые данные
		i = 0;
	}
	return i;
}


uint16_t UARTDMA::CalculateCRC16(uint8_t *puchMsg, uint16_t usDataLen)   		// Расчёт контрольной суммы
{
#include "CRC16.h"
	uint8_t  uchCRCHi = 0xFF;             /* Инициализация последнего байта CRC  */
	uint8_t  uchCRCLo = 0xFF;             /* Инициализация первого байта CRC   */
	uint16_t uIndex;                           /* will index into CRC lookup table  */
	while (usDataLen--)                         /* pass through message buffer  */
	{

		uIndex = uchCRCHi ^ *puchMsg++;             // Расчёт CRC по мануалу
		uchCRCHi = uchCRCLo ^ auchCRCHi[uIndex];
		uchCRCLo = auchCRCLo[uIndex];
	}
	return (uchCRCLo << 8 | uchCRCHi) ;
}

uint8_t UARTDMA::CalculateCRC8(uint8_t *pblock, uint8_t len)
{
#include "CRC8.h"
	unsigned char crc = 0x00;
 
	while (len--)
		crc = crc8Table[crc ^ *pblock++];
 
	return crc;
}
