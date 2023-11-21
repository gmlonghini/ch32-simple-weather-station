#include "debug.h"
#include "aht10.h"
#include "oled_display.h"

#define AHT_10_ADDRESS (0x38 << 1)

void I2C_User_Init(uint32_t bound, uint16_t address);
void I2C_Write_AHT10(uint8_t* data, uint32_t len);
void I2C_Read_AHT10(uint8_t* data, uint32_t len);

void I2C_User_Init(uint32_t bound, uint16_t address) {
	GPIO_InitTypeDef GPIO_InitStructure = { 0 };
	I2C_InitTypeDef I2C_InitTSturcture = { 0 };

	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd( RCC_APB1Periph_I2C1, ENABLE);

	//SCL - PC2
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init( GPIOC, &GPIO_InitStructure);

	//SDA - PC1
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init( GPIOC, &GPIO_InitStructure);

	I2C_InitTSturcture.I2C_ClockSpeed = bound;
	I2C_InitTSturcture.I2C_Mode = I2C_Mode_I2C;
	I2C_InitTSturcture.I2C_DutyCycle = I2C_DutyCycle_2;
	I2C_InitTSturcture.I2C_OwnAddress1 = address;
	I2C_InitTSturcture.I2C_Ack = I2C_Ack_Enable;
	I2C_InitTSturcture.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_Init( I2C1, &I2C_InitTSturcture);

	I2C_Cmd( I2C1, ENABLE);
	I2C_AcknowledgeConfig( I2C1, ENABLE);

}

void I2C_Write_AHT10(uint8_t *data, uint32_t len) {
	I2C_GenerateSTART( I2C1, ENABLE);
	while (!I2C_CheckEvent( I2C1, I2C_EVENT_MASTER_MODE_SELECT));

	I2C_Send7bitAddress( I2C1, AHT_10_ADDRESS, I2C_Direction_Transmitter);

	while (!I2C_CheckEvent( I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

	while (len) {
		if (I2C_GetFlagStatus( I2C1, I2C_FLAG_TXE) != RESET) {
			I2C_SendData( I2C1, data[0]);
			data++;
			len--;
		}
	}

	while (!I2C_CheckEvent( I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
	I2C_GenerateSTOP( I2C1, ENABLE);

}

void I2C_Read_AHT10(uint8_t *data, uint32_t len) {
	I2C_GenerateSTART( I2C1, ENABLE);
	while (!I2C_CheckEvent( I2C1, I2C_EVENT_MASTER_MODE_SELECT));

	I2C_Send7bitAddress( I2C1, AHT_10_ADDRESS, I2C_Direction_Receiver);

	while (!I2C_CheckEvent( I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

	while (len) {
		if (I2C_GetFlagStatus( I2C1, I2C_FLAG_RXNE) != RESET) {
			data[0] = I2C_ReceiveData( I2C1);
			data++;
			len--;
		}
	}

	I2C_GenerateSTOP( I2C1, ENABLE);

}

int main(void) {

	char str[20];
	aht10_driver_t drv;
	drv.read = I2C_Read_AHT10;
	drv.write = I2C_Write_AHT10;

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	Delay_Init();
	I2C_User_Init(100000, 0x02);

	Delay_Ms(100);

	ssd1306_Init();

	aht10_init(&drv);
	aht10_command_send(AHT10_CMD_RESET);
	Delay_Ms(20);
	aht10_read_status();
	Delay_Ms(10);
	aht10_command_send(AHT10_CMD_INIT);
	aht10_read_status();
	Delay_Ms(10);
	aht10_read_status();
	Delay_Ms(10);

	while (1) {

		aht10_command_send(AHT10_CMD_MEASURE);
		Delay_Ms(100);
		aht10_data_t ahtRawData = aht10_data_get();

		ssd1306_Fill(Black);
		ssd1306_SetCursor(5, 0);
		ssd1306_WriteString("CH32 Weather Station", Font_6x8, White);

		ssd1306_SetCursor(0, 20);
		//According to datasheet, T[C] = (St / 2^20)*200-50
		sprintf(str, "Temperature: %d C", (int) (((float) ahtRawData.temperature / 1048576) * 200) - 50);
		ssd1306_WriteString(str, Font_7x10, White);

		ssd1306_SetCursor(0, 45);
		//According to datasheet, RH[%] = (Srh / 2^20)*100%
		sprintf(str, "Humidity: %d %%", (int) (((float) ahtRawData.humidity / 1048576) * 100));
		ssd1306_WriteString(str, Font_7x10, White);

		ssd1306_UpdateScreen();

		Delay_Ms(1000);
	}
}

