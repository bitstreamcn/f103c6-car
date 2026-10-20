/*
 * application.h
 *
 *  Created on: 2026年8月15日
 *      Author: bitstream
 */

#ifndef INC_APPLICATION_H_
#define INC_APPLICATION_H_

#include <stdint.h>

class Application
{
public:
	void Init();
	void Loop();

	void UartTxCplt();
	void Servo();

	void IICResponse();
	void IICReceive();
	void IICRx();
	void IICTx();
	void IICError();
	void IICDone();
private:
	typedef struct __attribute__((packed)){
		volatile int8_t motor; //电机驱动+-100
		int8_t servo; //方向驱动+-100
		uint8_t led:1; //大灯
		uint8_t beep:1; //BB响
		uint8_t unused:6;
		uint8_t validate; //校验码
	}st_rx;      // 接收的数据
	st_rx rx_data;

	st_rx status_data;

	typedef struct __attribute__((packed)){
		uint8_t voltage; //电压，例11.2V = 112
		uint8_t led:1; //大灯
		uint8_t beep:1; //BB响
		uint8_t unused:6;
		uint8_t validate; //校验码
	}st_tx;
	st_tx tx_data;      // 发送的数据
	volatile bool is_read_data_ready = false;
	volatile bool is_write_data_ready = false;
	volatile bool iic_listen_error = false;
	volatile bool iic_error = false;

	volatile uint32_t iic_active_tick = 0; //iic活动时间，用来检测多久没收到数据了
	volatile bool     iic_active = false;

	volatile uint8_t duty = 0;

	float voltage = 0;

	void PushData();
	void IICDataUpdate();
};

extern Application app;

#endif /* INC_APPLICATION_H_ */
