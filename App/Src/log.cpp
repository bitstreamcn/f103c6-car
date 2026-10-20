/*
 * log.cpp
 *
 *  Created on: Apr 12, 2026
 *      Author: bitstream
 */
#include "main.h"
#include "log.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>


// 最大日志长度
#define LOG_MAX_LEN 128


bool ispause = false;

void Log::I(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);       // 绑定可变参数

    // 【关键】调用 vwrite 版本，把可变参数转发过去
    write("[I] ", format, ap);

    va_end(ap);
}

void Log::W(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);       // 绑定可变参数

    // 【关键】调用 vwrite 版本，把可变参数转发过去
    write("[W] ", format, ap);

    va_end(ap);
}

void Log::E(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);       // 绑定可变参数

    // 【关键】调用 vwrite 版本，把可变参数转发过去
    write("[E] ", format, ap);

    va_end(ap);
}



void Log::Init()
{

}

void Log::Pause()
{
	ispause = true;
}

void Log::Resume()
{
	ispause = false;
}

void Log::write(const char * tag, const char *format, va_list ap)
{
	//下载log，暂停输出
	if (ispause)
	{
		return;
	}
    // 获取互斥锁，如果已被占用则无限期等待
    //UINT status = tx_mutex_get(&log_mutex, TX_WAIT_FOREVER);
	//if (status == TX_SUCCESS)
	{
	    char buffer[LOG_MAX_LEN + 1];

	    int taglen = strlen(tag);

	    char * p = buffer;
	    snprintf(buffer, LOG_MAX_LEN, "%s", tag);
	    p += taglen;

	    vsnprintf(p, LOG_MAX_LEN - taglen, format, ap);
	    uint32_t len = strlen(buffer);
	    if (buffer[len - 1] != '\n')
	    {
	    	buffer[len] = '\n';
	    	buffer[len + 1] = '\0';
	    	len += 1;
	    }

		//if (0 != UART1_DMA_Send((uint8_t*)buffer, len))
		{
			//串口出问题，亮灯显示错误
		}
		buffer[len] = '\0';
		printf(buffer);

		// 释放互斥锁，供其他线程使用
		//tx_mutex_put(&log_mutex);
	}
}

#ifdef __cplusplus
extern "C" {
#endif

void LogInit(void)
{
	Log::Init();
}

void LogI(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    Log::write("[I] ", format, ap);
    va_end(ap);
}
void LogW(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    //Log::W(format, ap); // 调用 C++ 类
    Log::write("[W] ", format, ap);
    va_end(ap);
}
void LogE(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    //Log::E(format, ap); // 调用 C++ 类
    Log::write("[E] ", format, ap);
    va_end(ap);
}

void LogPause()
{
	Log::Pause();
}
void LogResume()
{
	Log::Resume();
}

#ifdef __cplusplus
}
#endif

