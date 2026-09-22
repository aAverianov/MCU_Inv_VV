/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *	Транслятор "Оптика - Модбас TCP"
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "Led.h"
#include "socket.h"
#include "dhcp.h"
#include "dns.h"
#include "w5500.h"
#include "mb.h"
#include "mbproto.h"
#include "mbutils.h"
#include "UARTDMA.h"
#include "Flash.h"
#include "buttons.h"
#include "adc_filter.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SW_VERSION	100
#define BOARD1
//#define BOARD2
//#define BOARD3
//#define BOARD4
//#define BOARD5
#define USE_FULL_ASSERT 
#define MODBUS_SOCKET   0
#define MBTCP_PORT      502
#define OPTI_UART		&huart3		// используемый UART для опт.связи
//#define ADDR_EXIT_CODE_FLASH	4	// порядковый номер в массиве куда будет записываться код завершения для операций с флэшем
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim1;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart3;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;
DMA_HandleTypeDef hdma_usart3_rx;
DMA_HandleTypeDef hdma_usart3_tx;
DMA_HandleTypeDef hdma_adc1;
ADC_HandleTypeDef hadc1;


/* USER CODE BEGIN PV */
// [ETHERNET (W5500)]--
wiz_NetInfo net_info = {
#ifdef BOARD1
	.mac = { 0xEA, 0x11, 0x22, 0x33, 0x44, 0x11 },
#endif // BOARD1
#ifdef BOARD2
	.mac = { 0xEA, 0x11, 0x22, 0x33, 0x44, 0x12 },
#endif // BOARD2
#ifdef BOARD3
	.mac = { 0xEA, 0x11, 0x22, 0x33, 0x44, 0x13 },
#endif // BOARD3
#ifdef BOARD4
	.mac = { 0xEA, 0x11, 0x22, 0x33, 0x44, 0x14 },
#endif // BOARD4
#ifdef BOARD5
	.mac = { 0xEA, 0x11, 0x22, 0x33, 0x44, 0x15 },
#endif // BOARD5
	.ip = { 192, 168, 100, 110 },
	.sn = { 255, 255, 255, 0 },
	.gw = { 0, 0, 0, 0 },
	.dns = { 0, 0, 0, 0 },
	.dhcp = NETINFO_STATIC,
};
union {
	uint32_t full;
	uint16_t ip16[2];
	uint8_t ip[4] = { 192, 168, 100, 110 };
} address;
bool isIpChanged = 0; // флаг: IP-адрес был изменен пользователем (через регистры Модбас)
uint32_t addressIpBak = 0; // для выявления изменений

uint16_t tmp = 0;
// --------------------
// [MODBUS] -----------
/*
Modbus специфицирует 4 типа данных:

Coils             (01 Read (0x)) — однобитовый тип, доступен для чтения и записи.
Discrete Inputs   (02 Read (1x)) — однобитовый тип, доступен только для чтения.
Holding Registers (03 Read (4x)) — 16-битовый знаковый или беззнаковый тип, доступен для чтения и записи.
Input Registers   (04 Read (3x)) — 16-битовый знаковый или беззнаковый тип, доступен только для чтения.
				  (05 Write Single Coil)
				  (06 Write Single Register)
				  (15 Write Multiple Coils)
				  (16 Write Multiple Registers)
*/
//---------------------------------
#define REG_INPUT_START       1        // нумерация регистров в массиве с "1"
#define REG_INPUT_NREGS       5       // кол-во регистров (3 - параметры; 1 - статус; 1 - код завершения записи во флэш)
#define REG_HOLDING_START     1        // нумерация регистров в массиве с "1"
#define REG_HOLDING_NREGS     5        // кол-во регистров (4 - ip-address, 1 - управление)
//---------------------------------
#define REG_COILS_START       1
#define REG_COILS_SIZE        8			// регистров нет

#define REG_DISCRETE_START    1
#define REG_DISCRETE_SIZE     8			// регистров нет
//---------------------------------
uint16_t usRegInputBuf[REG_INPUT_NREGS] = { 0 };
uint16_t usRegHoldingBuf[REG_HOLDING_NREGS] = { 0 };
uint8_t ucRegCoilsBuf[REG_COILS_SIZE / 8] = { 0 };
uint8_t ucRegDiscreteBuf[REG_DISCRETE_SIZE / 8] = { 0 };
// порядок в регистрах Input Registers
enum regInputBuf {
	V_ARC,
	I_D,
	SW_VER,
	STATUS,
	ADDR_EXIT_CODE_FLASH	// код завершения для операций с флэшем
};
// порядок в регистрах Holding Registers
enum regHoldingBuf {
	IP_OKTET1,
	IP_OKTET2,
	IP_OKTET3,
	IP_OKTET4,
	CONTROL					// код команды для преобразователя (только вкл/выкл)
};
// Биты состояния преобразователя
enum status {
	PSFLT,
	// (bit 0) 
	ERR_CODE_0,
	// (bit 1) 
	ERR_CODE_1,
	// (bit 2) 
	ERR_CODE_2,
	// (bit 3) 
	ERR,
	// (bit 4)
	LC_MODE,
	// (bit 5) 
	ARC,
	// (bit 6) 
	WORK,
	// (bit 7) 
	COMMUNICATION		// (bit 8)
};

typedef enum err_code 
{
	NO_ERROR,
	CONT_FLT,
	OC_FLT,
	TEMPR_FLT,
	COMM_FLT,
	
} err_code_t;
#define REG_STATUS_ERR_CODE_POS   (1U)									//Позиция начала поля (бит 1)
#define REG_STATUS_ERR_CODE_MASK  (0x07U << REG_STATUS_ERR_CODE_POS)	// Маска для поля (0b111 << 1 = 0b1110

err_code_t err_code = NO_ERROR;

typedef enum command
{
	STOP,
	RUN,
	RST_ERR, 
	
} command_t;

command_t R_comm = STOP;
uint8_t ch_rst = 0; //TODO NEED REFACTOR иногда не сраб от 1 раза, ввел счетчик

uint8_t flag_precharge_ok = 0;
uint8_t flag_init_wait = 0;
uint8_t flag_init_cmpl = 0;

uint8_t prev_control = 0;
uint16_t u_in = 0;

// Для синхронизации (непрерывно работающие таймеры)

const uint16_t timeToContact = 100; // 100 ms для формирования команды преобразователю
const uint16_t timeCommunication = 1000; // 1 s для формирования сигнала "связь ок" в Модбас
const uint16_t timeToButtons = 1; // 1 ms для кнопок
const uint16_t timeToADC = 10; // 10 ms
const uint16_t timing5 = 1000; // 1000 ms
// Для одноразовых таймеров (требуется запуск каждый раз)
const uint16_t timer1s = 1000; // 1000 ms
const uint16_t timer2s = 2000; // 2000 ms
const uint16_t timer3s = 3000; // 3000 ms
const uint16_t timer4s = 4000; // 

uint16_t iref_raw = 0;
uint16_t iref_f = 0;
FILTER_REG F_iref;
uint16_t buf_iref[COUNT_FILTER];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM1_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_SPI2_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_ADC1_Init(void);

/* USER CODE BEGIN PFP */

// *** MODBUS ***
eMBErrorCode eMBRegDiscreteCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNDiscrete); // чтение
eMBErrorCode eMBRegCoilsCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNCoils, eMBRegisterMode eMode); // чтение/запись
eMBErrorCode eMBRegInputCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNRegs); // чтение
eMBErrorCode eMBRegHoldingCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNRegs, eMBRegisterMode eMode); // чтение/запись

// *** W5500 ***
void W5500_Select(void) {
	HAL_GPIO_WritePin(SCS_GPIO_Port, SCS_Pin, GPIO_PIN_RESET);
}
void W5500_Unselect(void) {
	HAL_GPIO_WritePin(SCS_GPIO_Port, SCS_Pin, GPIO_PIN_SET);
}
void W5500_ReadBuff(uint8_t* buff, uint16_t len) {
	HAL_SPI_Receive(&hspi2, buff, len, HAL_MAX_DELAY);
}
void W5500_WriteBuff(uint8_t* buff, uint16_t len) {
	HAL_SPI_Transmit(&hspi2, buff, len, HAL_MAX_DELAY);
}
uint8_t W5500_ReadByte(void) {
	uint8_t byte;
	W5500_ReadBuff(&byte, sizeof(byte));
	return byte;
}
void W5500_WriteByte(uint8_t byte) {
	W5500_WriteBuff(&byte, sizeof(byte));
}
void init_w5500() {
	//printf("\r\ninit() called!\r\n");

	//printf("Registering W5500 callbacks...\r\n");
	reg_wizchip_cs_cbfunc(W5500_Select, W5500_Unselect);
	reg_wizchip_spi_cbfunc(W5500_ReadByte, W5500_WriteByte);
	reg_wizchip_spiburst_cbfunc(W5500_ReadBuff, W5500_WriteBuff);

	//printf("Calling wizchip_init()...\r\n");
	uint8_t rx_tx_buff_sizes[] = { 2, 2, 2, 2, 2, 2, 2, 2 };
	wizchip_init(rx_tx_buff_sizes, rx_tx_buff_sizes);
	setSHAR(net_info.mac);
#ifdef DEBUGG
	printf("IP:  %d.%d.%d.%d\r\nGW:  %d.%d.%d.%d\r\nNet: %d.%d.%d.%d\r\n" "DNS: %d.%d.%d.%d\r\n",
		net_info.ip[0],
		net_info.ip[1],
		net_info.ip[2],
		net_info.ip[3],
		net_info.gw[0],
		net_info.gw[1],
		net_info.gw[2],
		net_info.gw[3],
		net_info.sn[0],
		net_info.sn[1],
		net_info.sn[2],
		net_info.sn[3],
		net_info.dns[0],
		net_info.dns[1],
		net_info.dns[2],
		net_info.dns[3]);
#endif
	ctlnetwork(CN_SET_NETINFO, (void*) &net_info);
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
Led led(LED_GPIO_Port, LED_Pin, 50, 50, 100, 2000); // задать пин и параметры мигания для светодиода на плате
UARTDMA optics(OPTI_UART);
Flash flash(0x0801FC00, 1, 2); // исп. последняя(0x0801FC00) страница(1 шт.), данные будут занимать (2) 16-бит.слова
uint16_t var = 0;
uint16_t numBytes = 0; // кол-во принятых байтов (в одной посылке)
uint16_t var1Bak = 0;
uint16_t var2 = 0;
uint16_t var3 = 0;

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_DMA_Init();
	MX_TIM1_Init();
	MX_USART3_UART_Init();
	MX_SPI2_Init();
	MX_USART1_UART_Init();
	MX_ADC1_Init();
	HAL_Delay(2000);
	/* USER CODE BEGIN 2 */
	HAL_TIM_Base_Start_IT(&htim1); // канал для всяких нужд - запустить таймер, прерывания будут вызываться каждые 1 мс
	// подготовка IP-адреса
	bool isAddrExist = flash.ReadData(address.ip16); // считать адрес если он есть
						
	if (!RESET_IP_READ)			// Если есть запрос на сброс адреса								
	{
		// *** есть запрос на сброс
		if (isAddrExist)		// если во флэше есть какой-то адрес
		{
			// сравнить его с дефолтным
			if (address.ip[0] != net_info.ip[0] ||
				address.ip[1] != net_info.ip[1] ||
				address.ip[2] != net_info.ip[2] ||
				address.ip[3] != net_info.ip[3])
			{
				// если он не совпадает
								// загрузить дефолтное значение IP
				address.ip[0] = net_info.ip[0];	                        
				address.ip[1] = net_info.ip[1];
				address.ip[2] = net_info.ip[2];
				address.ip[3] = net_info.ip[3];
				// записать во флэш (код завершения в регистр модбас)
				usRegInputBuf[ADDR_EXIT_CODE_FLASH] = flash.WriteData(address.ip16);
			}
		}
		
	}
	// *** Нет запроса на сброс адреса
	else if(isAddrExist)		// и если был считан адрес
	{
		// использовать его
		net_info.ip[0] = address.ip[0]; 
		net_info.ip[1] = address.ip[1];	                        
		net_info.ip[2] = address.ip[2];	                        
		net_info.ip[3] = address.ip[3];	                        
	}
	// начальный сброс W5500
	HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_RESET);
	HAL_Delay(1);
	HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_SET);
	HAL_Delay(200);
	// занести адрес в регистры модбас 
	usRegHoldingBuf[0] = address.ip[0];               	
	usRegHoldingBuf[1] = address.ip[1];
	usRegHoldingBuf[2] = address.ip[2];
	usRegHoldingBuf[3] = address.ip[3];
	addressIpBak = address.full; // запомнить для выявления изменений в дальнейшем
	init_w5500();
	eMBTCPInit(MBTCP_PORT);
	HAL_Delay(200);
	eMBEnable();
	optics.InitReceive();
	// Set the interval for sending heartbeat packets automatically,
	// the unit time is 5s, so set it to 10s here 0 is not enabled.
	setSn_KPALVTR(MODBUS_SOCKET, 2);
	led.SetTiming(timeToContact, timeCommunication, timeToButtons, timeToADC, timing5); // установки таймингов 0,1,2,3,4 (значения см.выше)
	led.SetTimers(timer1s, timer2s, timer3s, timer4s); // установки для одноразовых таймеров 0,1,2,3
	
	//Init Loc/Dist_BTN state
	if (HAL_GPIO_ReadPin(BTN_DST_LOC_PORT, BTN_DST_LOC_PIN))
	{
		HAL_GPIO_WritePin(IND_LCON_PORT, IND_LCON_PIN, GPIO_PIN_SET);
		BIT_IN_TRUE(usRegInputBuf[STATUS], LC_MODE);
	}
	else
	{
		HAL_GPIO_WritePin(IND_LCON_PORT, IND_LCON_PIN, GPIO_PIN_RESET);	
		BIT_IN_FALSE(usRegInputBuf[STATUS], LC_MODE);
	}

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1)
	{
		if (flag_init_wait)
		{
			if (led.GetTimer(TIMER2) == 0)
			{
				flag_init_wait = 0;
				flag_init_cmpl = 1;
			}

		}
		if (led.Timing(TIME_TO_BUTTONS))
		{
			buttons_Process();
			
			if (button_IsPressed(BTN_DST_LOC)) 
			{
				HAL_GPIO_WritePin(IND_LCON_PORT, IND_LCON_PIN, GPIO_PIN_RESET);
				BIT_IN_FALSE(usRegInputBuf[STATUS], LC_MODE);
			}
			if (button_IsReleased(BTN_DST_LOC))
			{
				HAL_GPIO_WritePin(IND_LCON_PORT, IND_LCON_PIN, GPIO_PIN_SET);
				BIT_IN_TRUE(usRegInputBuf[STATUS], LC_MODE);
			}
			
			if (button_IsPressed(BTN_START_LOC)) 
			{
				if (BIT_TEST(usRegInputBuf[STATUS], LC_MODE))
				{
					R_comm = RUN;
				}
			}
			
			if (button_IsReleased(BTN_START_DST))
			{
				if (!(BIT_TEST(usRegInputBuf[STATUS], LC_MODE)))
				{
					R_comm = RUN;
				}
				
			}
			
			if (button_IsPressed(BTN_STOP_LOC) || !(button_IsHold(BTN_STOP_DST)) || button_IsReleased(BTN_STOP_DST) )
			{
				R_comm = STOP;
			}
			
			if (button_IsPressed(BTN_RST_LOC))
			{
				if ((BIT_TEST(usRegInputBuf[STATUS], ERR)) && (BIT_TEST(usRegInputBuf[STATUS], LC_MODE)))
				{
					if ((err_code == OC_FLT) || (err_code == TEMPR_FLT))		//TODO REFACTOR если ошибки Inv_VV то пробуем сбросить
					{
						R_comm = RST_ERR;
						ch_rst = 3;
					}
					else if(err_code == CONT_FLT)								//TODO REFACTOR
					{
						if (HAL_GPIO_ReadPin(CNTRL_CONTACTOR_PORT, CNTRL_CONTACTOR_PIN))
						{
							BIT_IN_FALSE(usRegInputBuf[STATUS], ERR);
							err_code = NO_ERROR;
						}
						
					}
				}
				
			}
			if (button_IsReleased(BTN_RST_DST))
			{
				if ((BIT_TEST(usRegInputBuf[STATUS], ERR)) && !(BIT_TEST(usRegInputBuf[STATUS], LC_MODE)))
				{
					if ((err_code == OC_FLT) || (err_code == TEMPR_FLT))		//TODO REFACTOR если ошибки Inv_VV то пробуем сбросить
					{
						R_comm = RST_ERR;
						ch_rst = 3;
					}
					else if(err_code == CONT_FLT)								//TODO REFACTOR
					{
						if (HAL_GPIO_ReadPin(CNTRL_CONTACTOR_PORT, CNTRL_CONTACTOR_PIN))
						{
							BIT_IN_FALSE(usRegInputBuf[STATUS], ERR);
							err_code = NO_ERROR;
						}
						
					}
				}
				
			}
			
			//TODO обработку команд по цифре переделать NEED REFACTOR
			if (!(prev_control == (uint8_t)usRegHoldingBuf[CONTROL]))
			{
				if (usRegHoldingBuf[CONTROL] == 2)
				{
					R_comm = STOP;
				}
				
				if (!(BIT_TEST(usRegInputBuf[STATUS], LC_MODE)))
				{
					if (usRegHoldingBuf[CONTROL] == 1)
					{
						R_comm = RUN;
					}
					
					if (usRegHoldingBuf[CONTROL] == 3)
					{
						R_comm = RST_ERR;
						ch_rst = 3;
					}
						
				}
				prev_control = usRegHoldingBuf[CONTROL];
			}
			
			//Проверка контактора и РКФ TODO сделать отдельную функцию
			
			
			if (flag_init_cmpl)
			{							
				if (!(HAL_GPIO_ReadPin(CNTRL_CONTACTOR_PORT, CNTRL_CONTACTOR_PIN)))
				{
					BIT_IN_TRUE(usRegInputBuf[STATUS], ERR);
					err_code = CONT_FLT;
					if (R_comm == RUN)				//TODO REFACTOR
					{
						R_comm = STOP;
						optics.Command(R_comm, 0);
					}
				}		
			
				if (!(HAL_GPIO_ReadPin(CNTRL_PS_PORT, CNTRL_PS_PIN)))
				{
				
					BIT_IN_TRUE(usRegInputBuf[STATUS], PSFLT);
				
				}
				else
				{
					BIT_IN_FALSE(usRegInputBuf[STATUS], PSFLT);
				}
			}
						
		}
		if (led.Timing(TIME_TO_ADC))
		{
			HAL_ADC_Start_DMA(&hadc1,
				(uint32_t *)&iref_raw,
				1U);
		}
		
		// время связи с преобразователем
		if (led.Timing(TIME_TO_CONTACT))		
		{
			iref_f = get_filter_value(&F_iref);
			if (((err_code) && (R_comm == RUN)) || (!flag_init_cmpl)) R_comm = STOP; //TODO NEED REFACTOR
			optics.Command(R_comm, uint8_t(iref_f >> 4)); // послать команду
			if (ch_rst)
			{
				ch_rst--;				
			}
			else if (R_comm == RST_ERR) R_comm = STOP;
			else ch_rst--; //отправка сброса только 3 раза за нажатие TODO разобраться почему не хватает 1
		}
		// если есть новые данные от преобразователя
		if (optics.Poll())						
		{
			usRegInputBuf[V_ARC] = optics.Voltage();
			usRegInputBuf[I_D] = optics.Current();
			//usRegInputBuf[V_IN] = optics.Power();
			usRegInputBuf[SW_VER] = SW_VERSION;
			u_in = optics.Power();
			usRegInputBuf[STATUS] &= 0xFF21; // очистить данные (1111 1111 0010 0001)
			//usRegInputBuf[STATUS] |= optics.Status(); // записать статус
			tmp = optics.Status();
			
			if (BIT_TEST(tmp, 3)) err_code = OC_FLT; //TODO NEED REFACTOR
			if (BIT_TEST(tmp, 2)) err_code = TEMPR_FLT;
			
			//update error_code
			tmp &= ~REG_STATUS_ERR_CODE_MASK;
			tmp |= ((err_code << REG_STATUS_ERR_CODE_POS) & REG_STATUS_ERR_CODE_MASK);
			if (err_code == CONT_FLT) BIT_IN_TRUE(tmp, ERR);
			usRegInputBuf[STATUS] |= tmp;
			
			//Analyze optics data						
			if ((u_in > 1850) && (!flag_precharge_ok))
			{
				HAL_GPIO_WritePin(CMD_CONTACTOR_PORT, CMD_CONTACTOR_PIN, GPIO_PIN_SET);
				flag_precharge_ok = 1;
				led.StartTimer(TIMER2); //задержка для сборки схемы 3000ms
				flag_init_wait = 1;
			}
					
			if (BIT_TEST(usRegInputBuf[STATUS], WORK)) 
			{
				HAL_GPIO_WritePin(IND_STOP_PORT, IND_STOP_PIN, GPIO_PIN_RESET);  
				HAL_GPIO_WritePin(SIG_RUN_PORT, SIG_RUN_PIN, GPIO_PIN_SET);
				
				if (BIT_TEST(usRegInputBuf[STATUS], ARC)) 
				{
					HAL_GPIO_WritePin(IND_ARC_PORT, IND_ARC_PIN, GPIO_PIN_SET);
					HAL_GPIO_WritePin(IND_ARC_FLT_PORT, IND_ARC_FLT_PIN, GPIO_PIN_RESET);
			
				}
				else 
				{
					HAL_GPIO_WritePin(IND_ARC_PORT, IND_ARC_PIN, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(IND_ARC_FLT_PORT, IND_ARC_FLT_PIN, GPIO_PIN_SET);
				}
			}
			else 
			{
				HAL_GPIO_WritePin(IND_STOP_PORT, IND_STOP_PIN, GPIO_PIN_SET);
				HAL_GPIO_WritePin(IND_ARC_PORT, IND_ARC_PIN, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(IND_ARC_FLT_PORT, IND_ARC_FLT_PIN, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(SIG_RUN_PORT, SIG_RUN_PIN, GPIO_PIN_RESET);
			}	
			if (BIT_TEST(usRegInputBuf[STATUS], ERR))
			{
				HAL_GPIO_WritePin(IND_FLT_PORT, IND_FLT_PIN, GPIO_PIN_SET);
			}
			else
			{
				HAL_GPIO_WritePin(IND_FLT_PORT, IND_FLT_PIN, GPIO_PIN_RESET);
				err_code = NO_ERROR;
			}
			if (BIT_TEST(usRegInputBuf[STATUS], PSFLT))
			{
				HAL_GPIO_WritePin(IND_PSFLT_PORT, IND_PSFLT_PIN, GPIO_PIN_SET);
			}
			else
			{
				HAL_GPIO_WritePin(IND_PSFLT_PORT, IND_PSFLT_PIN, GPIO_PIN_RESET);
			}
			
		}		
		
		// если данные недостоверны (с момента приема последней посылки прошло больше времени чем положено)
		if (!optics.GetStatusData())			
		{
			// обнулить все переметры
			usRegInputBuf[V_ARC] = 0;
			usRegInputBuf[I_D] = 0;
			u_in = 0;
			usRegInputBuf[STATUS] &= 0xFF3F; // очистка данных от преобразователя (1111 1111 0011 1111)
		}		
		
		
		// если есть изменение адреса
		if (isIpChanged) {						
			address.ip[0] = usRegHoldingBuf[IP_OKTET1];
			address.ip[1] = usRegHoldingBuf[IP_OKTET2];
			address.ip[2] = usRegHoldingBuf[IP_OKTET3];
			address.ip[3] = usRegHoldingBuf[IP_OKTET4];
			if (address.full != addressIpBak)		
			{	                                     
				usRegInputBuf[ADDR_EXIT_CODE_FLASH] = flash.WriteData(address.ip16); // записать во флэш
				addressIpBak = address.full;
			}
		}
		// формировать сигнал "Связь преобразователя со SCADA"
		if (led.Timing(TIME_COMMUNICATION))		
		{
			static bool checkbox = 0;
			if (checkbox) 
				BIT_IN_TRUE(usRegInputBuf[STATUS], COMMUNICATION);
			else 
				BIT_IN_FALSE(usRegInputBuf[STATUS], COMMUNICATION);
			checkbox = !checkbox;
		}
		// Обслуживание Modbus
		modbus_tcps(MODBUS_SOCKET, MBTCP_PORT);
		// коннект есть
		if (getPHYCFGR() == 191)
		{
			led.LongFlash(); // короткие редкие вспышки
		}
		// коннекта нет
		else if(getPHYCFGR() == 186)
		{
			usRegHoldingBuf[CONTROL] = 0; // команды для преобразователя не будут посылаться
			led.ShortFlash(); // частые короткие вспышки
		}
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
	RCC_PeriphCLKInitTypeDef PeriphClkInit = { 0 };

	/** Initializes the RCC Oscillators according to the specified parameters
	* in the RCC_OscInitTypeDef structure.
	*/
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}
	
	/** Initializes the CPU, AHB and APB buses clocks
	*/
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
								| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
	{
		Error_Handler();
	}
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
	PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

	/* USER CODE BEGIN SPI2_Init 0 */

	/* USER CODE END SPI2_Init 0 */

	/* USER CODE BEGIN SPI2_Init 1 */

	/* USER CODE END SPI2_Init 1 */
	/* SPI2 parameter configuration*/
	hspi2.Instance = SPI2;
	hspi2.Init.Mode = SPI_MODE_MASTER;
	hspi2.Init.Direction = SPI_DIRECTION_2LINES;
	hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi2.Init.NSS = SPI_NSS_SOFT;
	hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
	hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi2.Init.CRCPolynomial = 10;
	if (HAL_SPI_Init(&hspi2) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN SPI2_Init 2 */

	/* USER CODE END SPI2_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

	/* USER CODE BEGIN TIM1_Init 0 */

	/* USER CODE END TIM1_Init 0 */

	TIM_ClockConfigTypeDef sClockSourceConfig = { 0 };
	TIM_MasterConfigTypeDef sMasterConfig = { 0 };

	/* USER CODE BEGIN TIM1_Init 1 */

	/* USER CODE END TIM1_Init 1 */
	htim1.Instance = TIM1;
	htim1.Init.Prescaler = 7199;
	htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim1.Init.Period = 9;
	htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim1.Init.RepetitionCounter = 0;
	htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
	{
		Error_Handler();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
	{
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN TIM1_Init 2 */

	/* USER CODE END TIM1_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

	/* USER CODE BEGIN USART1_Init 0 */

	/* USER CODE END USART1_Init 0 */

	/* USER CODE BEGIN USART1_Init 1 */

	/* USER CODE END USART1_Init 1 */
	huart1.Instance = USART1;
	huart1.Init.BaudRate = 115200;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart1) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN USART1_Init 2 */

	/* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

	/* USER CODE BEGIN USART3_Init 0 */

	/* USER CODE END USART3_Init 0 */

	/* USER CODE BEGIN USART3_Init 1 */

	/* USER CODE END USART3_Init 1 */
	huart3.Instance = USART3;
	huart3.Init.BaudRate = 115200;
	huart3.Init.WordLength = UART_WORDLENGTH_8B;
	huart3.Init.StopBits = UART_STOPBITS_1;
	huart3.Init.Parity = UART_PARITY_NONE;
	huart3.Init.Mode = UART_MODE_TX_RX;
	huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart3.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart3) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN USART3_Init 2 */

	/* USER CODE END USART3_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

	/* DMA controller clock enable */
	__HAL_RCC_DMA1_CLK_ENABLE();

	/* DMA interrupt init */
	/* DMA1_Channel1_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
	/* DMA1_Channel2_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);
	/* DMA1_Channel3_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);
	/* DMA1_Channel4_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
	/* DMA1_Channel5_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	/* USER CODE BEGIN MX_GPIO_Init_1 */
	/* USER CODE END MX_GPIO_Init_1 */

	  /* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();

	/*Configure GPIO pin : RESET_IP_Pin */
	GPIO_InitStruct.Pin = RESET_IP_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(RESET_IP_GPIO_Port, &GPIO_InitStruct);

	
	GPIO_InitStruct.Pin = LED_Pin | IND_LCON_PIN | IND_PSFLT_PIN | IND_STOP_PIN | IND_FLT_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	
	GPIO_InitStruct.Pin = SCS_Pin | RST_Pin | SIG_RUN_PIN | CMD_CONTACTOR_PIN | IND_ARC_PIN | IND_ARC_FLT_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
	
	GPIO_InitStruct.Pin = BTN_START_DST_PIN | CNTRL_CONTACTOR_PIN | CNTRL_PS_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  
	GPIO_InitStruct.Pin = BTN_START_LOC_PIN | BTN_STOP_LOC_PIN | BTN_RST_LOC_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	GPIO_InitStruct.Pin = BTN_DST_LOC_PIN | BTN_STOP_DST_PIN | BTN_RST_DST_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	
	HAL_GPIO_WritePin(GPIOB, LED_Pin | IND_LCON_PIN | IND_PSFLT_PIN | IND_STOP_PIN | IND_FLT_PIN, GPIO_PIN_RESET);

	HAL_GPIO_WritePin(GPIOC, SCS_Pin | RST_Pin | SIG_RUN_PIN | CMD_CONTACTOR_PIN, GPIO_PIN_RESET);
	

	/* USER CODE BEGIN MX_GPIO_Init_2 */
	
	/* USER CODE END MX_GPIO_Init_2 */
}

static void MX_ADC1_Init(void)
{

	/* USER CODE BEGIN ADC1_Init 0 */

	/* USER CODE END ADC1_Init 0 */

	ADC_ChannelConfTypeDef sConfig = { 0 };

	/* USER CODE BEGIN ADC1_Init 1 */

	/* USER CODE END ADC1_Init 1 */

	/** Common config
	*/
	hadc1.Instance = ADC1;
	hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
	hadc1.Init.ContinuousConvMode = DISABLE;
	hadc1.Init.DiscontinuousConvMode = DISABLE;
	hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc1.Init.NbrOfConversion = 1;
	if (HAL_ADC_Init(&hadc1) != HAL_OK)
	{
		Error_Handler();
	}

	/** Configure Regular Channel
	*/
	sConfig.Channel = ADC_CHANNEL_4;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN ADC1_Init 2 */
	HAL_ADCEx_Calibration_Start(&hadc1);
	/* USER CODE END ADC1_Init 2 */

}
/* USER CODE BEGIN 4 */

eMBErrorCode
eMBRegHoldingCB(UCHAR *pucRegBuffer,
	USHORT usAddress,
	USHORT usNRegs,
	eMBRegisterMode eMode)
{
	eMBErrorCode eStatus = MB_ENOERR; // выходной статус - "ошибок нет"
	int iRegIndex;
	usAddress--; // it already plus one in modbus function method.
	// Проверка на допустимость адреса и количества регистров
	if ((usAddress >= REG_HOLDING_START) &&
		(usAddress + usNRegs <= REG_HOLDING_START + REG_HOLDING_NREGS))
	{
		iRegIndex = (int)(usAddress - REG_HOLDING_START);
		switch (eMode)
		{
			/* Pass current register values to the protocol stack. */
		case MB_REG_READ:
			while (usNRegs > 0)
			{
				*pucRegBuffer++ =
					(unsigned char)(usRegHoldingBuf[iRegIndex] >> 8);
				*pucRegBuffer++ =
					(unsigned char)(usRegHoldingBuf[iRegIndex] & 0xFF);
				iRegIndex++;
				usNRegs--;
			}
			break;
			/* Update current register values with new values from the protocol stack. */
		case MB_REG_WRITE:
			if (iRegIndex < 4) isIpChanged = 1; // если запись в регистры где хранится IP - зафиксировать изменение(флаг)
			while (usNRegs > 0)
			{
				usRegHoldingBuf[iRegIndex] = *pucRegBuffer++ << 8; // старший байт
				usRegHoldingBuf[iRegIndex] |= *pucRegBuffer++; // младший байт
				if (iRegIndex == CONTROL)					// если действие с регистром управления
				{
					// "отфильтровать"
					usRegHoldingBuf[iRegIndex] &= 0x0003; // рабочие - 2 мл.бита					
				}
				iRegIndex++;
				usNRegs--;
			}
		}
	}
	else
	{
		eStatus = MB_ENOREG; // выходной статус - "ошибка"(illegal register address)
	}
	return eStatus;
}

eMBErrorCode
eMBRegInputCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNRegs)
{
	eMBErrorCode eStatus = MB_ENOERR;
	int iRegIndex;

	usAddress--; // it already plus one in modbus function method.
	if ((usAddress >= REG_INPUT_START) && (usAddress + usNRegs <= REG_INPUT_START + REG_INPUT_NREGS))
	{
		iRegIndex = (int)(usAddress - REG_INPUT_START);
		while (usNRegs > 0)
		{
			*pucRegBuffer++ =
				(unsigned char)(usRegInputBuf[iRegIndex] >> 8);
			*pucRegBuffer++ =
				(unsigned char)(usRegInputBuf[iRegIndex] & 0xFF);
			iRegIndex++;
			usNRegs--;
		}
	}
	else
	{
		eStatus = MB_ENOREG;
	}

	return eStatus;
}

eMBErrorCode
eMBRegCoilsCB(UCHAR *pucRegBuffer,
	USHORT usAddress,
	USHORT usNCoils,
	eMBRegisterMode eMode)
{
	return MB_ENOREG;	// данный тип регистров не используется
	eMBErrorCode eStatus = MB_ENOERR;
	int iNCoils = (int)usNCoils;
	unsigned short usBitOffset;

	usAddress--; // it already plus one in modbus function method.
	/* Check if we have registers mapped at this block. */
	if ((usAddress >= REG_COILS_START) &&
		(usAddress + usNCoils <= REG_COILS_START + REG_COILS_SIZE))
	{
		usBitOffset = (unsigned short)(usAddress - REG_COILS_START);
		switch (eMode)
		{
			/* Read current values and pass to protocol stack. */
		case MB_REG_READ:
			while (iNCoils > 0)
			{
				*pucRegBuffer++ =
					xMBUtilGetBits(ucRegCoilsBuf,
					usBitOffset,
					(unsigned char)(iNCoils > 8 ? 8 : iNCoils));
				iNCoils -= 8;
				usBitOffset += 8;
			}
			break;
			/* Update current register values. */
		case MB_REG_WRITE:
			while (iNCoils > 0)
			{
				xMBUtilSetBits(ucRegCoilsBuf,
					usBitOffset,
					(unsigned char)(iNCoils > 8 ? 8 : iNCoils),
					*pucRegBuffer++);
				iNCoils -= 8;
				usBitOffset += 8;
			}
			break;
		}
	}
	else
	{
		eStatus = MB_ENOREG;
	}
	return eStatus;
}

eMBErrorCode
eMBRegDiscreteCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNDiscrete)
{
	eMBErrorCode eStatus = MB_ENOERR;
	short iNDiscrete = (short)usNDiscrete;
	unsigned short usBitOffset;

	return MB_ENOREG;	// данный тип регистров не используется

	usAddress--; // it already plus one in modbus function method.
	/* Check if we have registers mapped at this block. */
	if ((usAddress >= REG_DISCRETE_START) &&
		(usAddress + usNDiscrete <= REG_DISCRETE_START + REG_DISCRETE_SIZE))
	{
		usBitOffset = (unsigned short)(usAddress - REG_DISCRETE_START);
		while (iNDiscrete > 0)
		{
			*pucRegBuffer++ =
				xMBUtilGetBits( ucRegDiscreteBuf,
				usBitOffset,
				(unsigned char)(iNDiscrete >
								   8 ? 8 : iNDiscrete));
			iNDiscrete -= 8;
			usBitOffset += 8;
		}
	}
	else
	{
		eStatus = MB_ENOREG;
	}
	return eStatus;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart == OPTI_UART) {
		optics.SetStatusTx(); // флаг - окончание отправки
	}
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1)
	{
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
	   ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	 /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
