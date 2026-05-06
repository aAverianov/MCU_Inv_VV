/*
 * UARTDMA.h
 *
 *  Created on: xx xxxx 2023 г.
 *      Author: Bulat
 *      UART должен быть настроен:
 *      1.	USART (DMA)RX - mode:circular - byte, increment address - memory
 *      	USART (DMA)TX - mode:normal - byte, increment address - memory
 *      	USART (NVIC) - global interrupt
 *      DMA вызывает два прерывания, первое после отправки половины буфера (его надо отключить
 *      , а второе при завершении. Чтоб отключить половинку, нужно в файле
 *      stm32f1xx_hal_uart.c найти функцию HAL_UART_Transmit_DMA(...)
 *      и закомментировать строчку…
 *      						* Set the UART DMA Half transfer complete callback *
 *		вот эту (str.1421)->	//huart->hdmatx->XferHalfCpltCallback = UART_DMATxHalfCplt;
 *		2. Здесь определить:
 *			размер кольцевого буфера,
 *			размер массива для пользовательских переменных,
 *		3. Начальный адрес пользовательских адресов и номер UART задаются при вызове конструктора
 *		4. Для корректной работы надо периодически вызывать метод Poll() который проверяет кольцевой буфер,
 *			считывает, парсит. В рез-те в структуре для чтения будет всегда актуальное состояние всех
 *			пользовательских переменных
 */
//#ifdef __cplusplus
//extern "C" {
//#endif

#ifndef SRC_UARTDMA_H_
#define SRC_UARTDMA_H_
#include "stm32f1xx_hal.h"
#include "Led.h"

#define    DWT_CYCCNT    *(volatile uint32_t *)0xE0001004
#define    DWT_CONTROL   *(volatile uint32_t *)0xE0001000
#define    SCB_DEMCR     *(volatile uint32_t *)0xE000EDFC

#define		ID_COMMAND 0xA5
extern Led led;

const uint16_t ringBufferSize_ = 20;		// размер кольцевого буфера
const uint8_t amountOfData_ = 8;			// размер массива (в байтах) в котором хранятся данные считанные из uart (включая crc)
const uint16_t amountOfData16_ = 4;			//		-//-	  (в 16-бит.словах)
const uint8_t amountOfCommand_ = 3;			// размер массива (в байтах) в котором хранится команда для uart (включая crc)


class UARTDMA {
public:
	UARTDMA(UART_HandleTypeDef *uart);	// указатель на структуру uart
	void InitReceive();					// Запуск циклического чтения UART через DMA
	bool GetStatusTx();					// Получить статус передатчика UART (готов/не готов к передаче)
	void SetStatusTx();					// Установить статус передатчика: готов к передаче
	bool GetStatusData();
	uint16_t Poll();					// Опрос UART и чтение данных если они есть. Необходимо постоянно вызывать в цикле
	void Command(uint8_t command);		// Посылка в UART команды для преобразователя
	uint16_t Voltage();					// Чтение напряжения из соотв.позиции массива структуры
	uint16_t Current();					// Чтение тока из соотв.позиции массива структуры
	uint16_t Power();					// Чтение мощности из соотв.позиции массива структуры
	uint16_t Status();					// Чтение состояния блока из соотв.позиции массива структуры
private:
	UART_HandleTypeDef* optiUart;
	bool flagTxComplete_;
	uint8_t rxBuffer_[ringBufferSize_] = { 0 };		// кольцевой буфер
	uint16_t writeIndex_ = 0;
	uint16_t readIndex_ = 0;
	uint16_t getCount_ = 0;
	// Объединение для работы с данными (запись)
	// пока команда состоит из 
	union {
		uint8_t allData[amountOfCommand_] = { 0 };
		struct {
			uint8_t id;				// ид команды
			uint8_t command;		// команда
			uint8_t crc;
		};
	} write;
	
	bool isDataReliable_ = 0;		// признак достоверности данных
	// Объединение для работы с данными (чтение)
	union {
		uint64_t data64 = 0;
		uint16_t allData16[amountOfData16_];
		uint8_t allData8[amountOfData_];
		struct {
			uint16_t uArc : 16;			// напряжение дуги
			uint16_t iArc : 16;			// ток дуги
			uint16_t pArc : 16;			// мощность дуги
			uint8_t status;				// статус
			uint8_t crc;				// контрольная сумма
		};
	} read;
	//uint8_t m[8] = { 0x31, 0x31, 0x32, 0x32, 0x33, 0x33, 0x34, 0x20 };
	uint16_t Available(void);		// Возвращает кол-во непрочитанных байтов из буфера
	uint8_t Read(void);				// Выдает очередной байт данных из приемного буфера
	void Request();						// Запрос на отправку данных в UART
	uint16_t CalculateCRC16(uint8_t *puchMsg, uint16_t usDataLen);
	uint8_t CalculateCRC8(uint8_t *pblock, uint8_t len);
};

#endif /* SRC_UARTDMA_H_ */
//#ifdef __cplusplus
//}
//#endif
