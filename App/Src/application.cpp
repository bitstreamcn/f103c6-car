/*
 * application.cpp
 *
 *  Created on: 2026年8月15日
 *      Author: bitstream
 */
#include "main.h"
#include "application.h"
#include <cstdio>
#include "usart.h"
#include "i2c.h"
#include "log.h"
#include <cstring>
#include "tim.h"
#include "common.h"
#include "adc.h"

#define VREF 3.3f           // 参考电压
#define ADC_MAX 4095.0f     // 12位ADC最大值
#define DIV_RATIO 10.1f     // 分压比 (9.1k + 1k) / 1k = 10.1

Application app;

struct __attribute__((packed)){
	volatile int8_t IICReceive;
	volatile int8_t IICResponse;
	volatile int8_t IICRx;
	volatile int8_t IICTx;
	volatile int8_t IICError;
	volatile int8_t IICDone;
	volatile uint8_t errcount;
	volatile int8_t step;
}iic_state;

#define abs(x) (x>0?x:-x)

/**
 * @brief 读取并计算实际电压 (带一阶低通滤波)
 * @retval 实际输入电压 (单位: V)
 */
float Read_Battery_Voltage(void)
{
    uint32_t adc_value = 0;

    // 1. 启动 ADC 并等待转换完成
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
    {
        // 2. 读取 ADC 原始值 (0 ~ 4095)
        adc_value = HAL_ADC_GetValue(&hadc1);
    }
    HAL_ADC_Stop(&hadc1); // 读取完毕后停止ADC，节省功耗

    // 3. 计算实际电压
    // 公式: (ADC值 / 4095) * 3.3V * 10.1(分压比)
    float voltage = ((float)adc_value / ADC_MAX) * VREF * DIV_RATIO;

    return voltage;
}

void Application::Init()
{
	HAL_StatusTypeDef status = HAL_I2C_EnableListen_IT(&hi2c1);
	if (status != HAL_OK)
	{
		Log::I("HAL_I2C_EnableListen_IT error:%x", status);
		iic_listen_error = true;
	}

	status_data.servo = 90;

	// 🚨 F1系列专属：ADC 自校准，消除零点误差
	HAL_ADCEx_Calibration_Start(&hadc1);

	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
	HAL_TIM_Base_Start_IT(&htim2);
}


#define I2C_SCL_PORT  GPIOB
#define I2C_SCL_PIN   GPIO_PIN_6
#define I2C_SDA_PORT  GPIOB
#define I2C_SDA_PIN   GPIO_PIN_7

// 手动解锁 I2C 总线
void I2C_Unstick_Bus(I2C_HandleTypeDef *hi2c)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 1. 暂时关闭 I2C 外设
    __HAL_I2C_DISABLE(hi2c);

    // 2. 将 SCL 和 SDA 配置为开漏输出模式
    GPIO_InitStruct.Pin = I2C_SCL_PIN | I2C_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD; // 开漏输出
    GPIO_InitStruct.Pull = GPIO_NOPULL;        // 依赖外部上拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(I2C_SCL_PORT, &GPIO_InitStruct);

    // 3. 确保 SDA 和 SCL 初始为高电平
    HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_SET);

    // 4. 产生最多 9 个 SCL 时钟脉冲
    for (uint8_t i = 0; i < 9; i++)
    {
        // 拉低 SCL
        HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_RESET);
        HAL_Delay(1); // 简单延时，给总线反应时间

        // 拉高 SCL
        HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
        HAL_Delay(1);
    }

    // 5. 产生一个 STOP 信号 (SCL 为高时，SDA 由低变高)
    HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_RESET); // SDA 拉低
    HAL_Delay(1);
    HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_SET);  // SDA 拉高 (产生 STOP)
    HAL_Delay(1);

    // 6. 将引脚交还给 I2C 外设 (恢复为复用功能)
    // 这一步实际上会在 MX_I2C1_Init() 中完成，但保险起见这里可以再初始化一次
    // HAL_I2C_DeInit(hi2c);
    // MX_I2C1_Init();

    // 7. 重新使能 I2C 外设
    __HAL_I2C_ENABLE(hi2c);
}

/**
 * @brief 设置舵机转动角度
 * @param angle: 目标角度 (0 ~ 180 度)
 */
void Set_Servo_Angle(uint8_t angle)
{
    if (angle > 180) angle = 180; // 限制最大角度

    // 将 0~180 度线性映射到 500~2500 的比较值
    // 公式: CCR = 500 + (angle / 180) * (2500 - 500)
    uint16_t pulse = 500 + (uint16_t)((uint32_t)angle * 2000 / 180);

    // 设置 CCR 寄存器
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pulse);
}

void Application::Loop()
{

	while (1)
	{
		int32_t tick = HAL_GetTick();

		Set_Servo_Angle(status_data.servo);

		HAL_GPIO_WritePin(GPIOB, CAR_LED_Pin, status_data.led ? GPIO_PIN_SET : GPIO_PIN_RESET);

	    static uint8_t beep_state = 0;  // 0: 响状态, 1: 停状态
	    static uint32_t beep_timer = 0; // 记录进入当前状态的时间戳
		if (status_data.beep)
		{
			if (beep_state == 0) // 当前是"响"状态
			{
				HAL_GPIO_WritePin(GPIOB, BEEP_Pin, GPIO_PIN_SET); // 开启蜂鸣器
				// 检查是否响了 1000ms
				if ((tick - beep_timer) >= 1000)
				{
					beep_state = 1;       // 切换到"停"状态
					beep_timer = tick;    // 记录切换时间
				}
			} else if (beep_state == 1) // 当前是"停"状态
			{
				HAL_GPIO_WritePin(GPIOB, BEEP_Pin, GPIO_PIN_RESET); // 关闭蜂鸣器
				// 检查是否停了 3000ms
				if ((tick - beep_timer) >= 3000) {
					beep_state = 0;       // 切换回"响"状态
					beep_timer = tick;    // 记录切换时间
				}
			}
		}
		else
		{
			HAL_GPIO_WritePin(GPIOB, BEEP_Pin, GPIO_PIN_RESET);
			beep_state = 0;
			beep_timer = tick;
		}

		IICDataUpdate();

		static uint32_t recv_tick = 0;
		//如果2秒内没有收到控制信号，就停止电机
		if ((tick - recv_tick) > 2000)
		{
			status_data.motor = 0;
		}
		//收到姿态数据
		if (is_read_data_ready)
		{
			uint8_t v = (rx_data.beep + rx_data.led + rx_data.motor + rx_data.servo) & 0xff;
			if (v == rx_data.validate)
			{
				memcpy(&status_data, &rx_data, sizeof(st_rx));
				duty = abs(status_data.motor);
				if (duty > 100) duty = 100;
			}
			is_read_data_ready = false;
			recv_tick = tick;
		}

		//检查iic开始通信到结束时间，如果超时，说明iic在操作中间没有返回
		if (iic_active && (tick - iic_active_tick) > 10)
		{
			iic_error = true;
		}

		//检查i2c是否存在错误，或者超过2秒没有收到请求
		if (iic_error)
		{
			iic_state.errcount += 1;
			iic_active = false;
			iic_active_tick = tick;
			iic_error = false;
			//Log::I("IICError");
			uint32_t error = HAL_I2C_GetError(&hi2c1);
			if (error == HAL_I2C_ERROR_BERR)
			{
				//Log::I("HAL_I2C_GetError error:%x", error);
			}
		    // 通过串口或 LED 指示 error 值
		    // 常见错误：
		    // HAL_I2C_ERROR_BERR  (总线错误)
		    // HAL_I2C_ERROR_ARLO  (仲裁丢失)
		    // HAL_I2C_ERROR_OVR   (过载/欠载)
		    // HAL_I2C_ERROR_AF    (应答失败)
		    HAL_I2C_DeInit(&hi2c1);
		    I2C_Unstick_Bus(&hi2c1);
		    HAL_I2C_Init(&hi2c1);

			HAL_StatusTypeDef status = HAL_I2C_EnableListen_IT(&hi2c1);
			if (status != HAL_OK)
			{
				//Log::I("IICError HAL_I2C_EnableListen_IT error:%x", status);
				iic_listen_error = true;
			}
		}

		if (iic_listen_error)
		{
			HAL_StatusTypeDef status = HAL_I2C_EnableListen_IT(&hi2c1);
			if (status != HAL_OK)
			{
				iic_listen_error = true;
			}
			else
			{
				iic_listen_error = false;
			}
		}

		static uint32_t prev_tick = 0;
		if ((tick - prev_tick) > 1000)
		{
			float vol = Read_Battery_Voltage();
			voltage = vol;
			prev_tick = tick;
		}

		HAL_Delay(1);
	}
}


void Application::IICReceive()
{
	iic_active_tick = HAL_GetTick();
	iic_active = true;
	//Log::I("IICReceive");

	if (!is_read_data_ready)
	{
		HAL_StatusTypeDef status = HAL_I2C_Slave_Seq_Receive_IT(&hi2c1, (uint8_t *)&rx_data, sizeof(rx_data), I2C_LAST_FRAME);  //接收数据
		if (status != HAL_OK)
		{
			Log::I("IICReceive HAL_I2C_Slave_Seq_Receive_IT err:%x", status);
			iic_listen_error = true;
		}
	}
	else
	{
		static st_rx dropdata;
		HAL_StatusTypeDef status = HAL_I2C_Slave_Seq_Receive_IT(&hi2c1, (uint8_t *)&dropdata, sizeof(st_rx), I2C_LAST_FRAME);  //接收数据
		if (status != HAL_OK)
		{
			Log::I("IICReceive HAL_I2C_Slave_Seq_Receive_IT err:%x", status);
			iic_listen_error = true;
		}
	}

	memset(&iic_state, 0, sizeof(iic_state) - 2);
	iic_state.step = 1;
	iic_state.IICReceive = iic_state.step;
}

void Application::IICRx()
{
	iic_state.step += 1;

	iic_state.IICRx = iic_state.step;
	//接收完主机写的数据, 解析数据
	is_read_data_ready = true;
}

void Application::IICTx()
{
	iic_state.step += 1;
	iic_state.IICTx = iic_state.step;
	//Log::I("IICTx");
	// 读操作完成
	is_write_data_ready = false;
}

void Application::IICError()
{
	iic_state.step += 1;
	iic_state.IICError = iic_state.step;

	iic_error = true;
	iic_active = false;
}

void Application::IICResponse()
{
	iic_state.step += 1;
	//Log::I("IICResponse reg_addr:%d", reg_addr);
	iic_state.IICResponse = iic_state.step;
	if (is_write_data_ready)
	{
		HAL_StatusTypeDef status = HAL_I2C_Slave_Seq_Transmit_IT(&hi2c1, (uint8_t *)&tx_data, sizeof(tx_data), I2C_LAST_FRAME);
		if (status != HAL_OK)
		{
			Log::I("IICResponse HAL_I2C_Slave_Seq_Transmit_IT error1:%x", status);
			iic_listen_error = true;
		}
	}
	else
	{
		static st_tx zero_data;
		static bool isinit = false;
		if (!isinit)
		{
			zero_data.voltage = 0;
			zero_data.led = 0;
			zero_data.beep = 0;
			zero_data.validate = 0;
			isinit = true;
		}
		HAL_StatusTypeDef status = HAL_I2C_Slave_Seq_Transmit_IT(&hi2c1, (uint8_t *)&zero_data, sizeof(zero_data), I2C_LAST_FRAME);
		if (status != HAL_OK)
		{
			Log::I("IICResponse HAL_I2C_Slave_Seq_Transmit_IT error2:%x", status);
			iic_listen_error = true;
		}
	}
}


void Application::IICDone()
{
	iic_state.step += 1;
	iic_state.IICDone = iic_state.step;
	//Log::I("IICDone");
	// 完成一次通信，清除状态
	HAL_StatusTypeDef status = HAL_I2C_EnableListen_IT(&hi2c1);
	if (status != HAL_OK)
	{
		Log::I("IICDone HAL_I2C_EnableListen_IT error:%x", status);
		iic_listen_error = true;
	}
	iic_active = false;
}


void Application::PushData()
{
	if (is_write_data_ready)
	{
		return;
	}
	//Log::I("PushData");
	tx_data.beep = HAL_GPIO_ReadPin(GPIOB, BEEP_Pin) == GPIO_PIN_SET ? 1 : 0;
	tx_data.led = HAL_GPIO_ReadPin(GPIOB, CAR_LED_Pin) == GPIO_PIN_SET ? 1 : 0;
	tx_data.voltage = voltage * 10;

	int16_t validate =
			tx_data.voltage + tx_data.led + tx_data.beep;
	tx_data.validate = validate & 0xff;
	is_write_data_ready = true;
}

void Application::IICDataUpdate()
{
	static uint32_t refresh_tick = 0;
	uint32_t now = HAL_GetTick();
	uint32_t remain = now - refresh_tick;
	uint32_t ticks = 1000 / 50; //50hz更新率
	if (remain > ticks)
	{
		refresh_tick = now;
		if (!is_write_data_ready)
		{
			PushData();
		}
	}
}


void Application::Servo()
{
    static bool is_on_phase = false; // false: 初始处于 OFF 阶段

    int16_t motor = status_data.motor;
    uint8_t duty = (uint8_t)(motor < 0 ? -motor : motor);
    if (duty >= 100) duty = 99;

    // ==========================================
    // 边缘情况处理：0% 或 100% 占空比
    // 不需要高频切换，直接固定电平，恢复 1ms 周期
    // ==========================================
    if (duty == 0)
    {
        __HAL_TIM_SET_AUTORELOAD(&htim2, 99); // 恢复标准 1ms 周期 (1000Hz)
        is_on_phase = false; // 重置状态机

        if (duty == 0)
        {
            // 0% 占空比：持续刹车 (OFF)
            GPIOB->BSRR = IN1_Pin | IN2_Pin;
        }
        else // duty == 100
        {
            // 100% 占空比：持续输出 (ON)
            if (motor > 0) {
                GPIOB->BSRR = IN1_Pin;
                GPIOB->BSRR = (uint32_t)IN2_Pin << 16;
            } else if (motor < 0) {
                GPIOB->BSRR = (uint32_t)IN1_Pin << 16;
                GPIOB->BSRR = IN2_Pin;
            } else {
                GPIOB->BSRR = IN1_Pin | IN2_Pin;
            }
        }
        return; // 直接返回，不执行后面的动态逻辑
    }

    // ==========================================
    // 正常情况：0 < duty < 100 的动态 ARR 逻辑
    // ==========================================
    if (is_on_phase)
    {
        // ===== 刚刚结束 ON 阶段，进入 OFF 阶段 =====
        // 刹车/OFF：两端都拉高
        GPIOB->BSRR = IN1_Pin | IN2_Pin;

        // 计算 OFF 时间
        uint16_t off_time = 100 - duty;
        if (off_time < 2) off_time = 2; // 🚨 关键保护：防止 off_time=1 导致 ARR=0

        __HAL_TIM_SET_AUTORELOAD(&htim2, off_time - 1);

        is_on_phase = false;
    }
    else
    {
        // ===== 刚刚结束 OFF 阶段，进入 ON 阶段 =====
        if (motor > 0) {
            // 正转
            GPIOB->BSRR = IN1_Pin;
            GPIOB->BSRR = (uint32_t)IN2_Pin << 16;
        } else if (motor < 0) {
            // 反转
            GPIOB->BSRR = (uint32_t)IN1_Pin << 16;
            GPIOB->BSRR = IN2_Pin;
        } else {
            // motor=0 刹车
            GPIOB->BSRR = IN1_Pin | IN2_Pin;
        }

        // 计算 ON 时间
        uint16_t on_time = duty;
        if (on_time < 2) on_time = 2; // 🚨 关键保护：防止 on_time=1 导致 ARR=0

        __HAL_TIM_SET_AUTORELOAD(&htim2, on_time - 1);

        is_on_phase = true;
    }
}


void Application::UartTxCplt()
{

}

#ifdef __cplusplus
extern "C" {
#endif


// UART DMA 发送完成回调函数
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    // 确保是你使用的串口
    if (huart->Instance == USART1)
    {
        // 可以调用你 Application 类里的方法，或者留空
        app.UartTxCplt();
    }
}

#ifdef __cplusplus
}
#endif
