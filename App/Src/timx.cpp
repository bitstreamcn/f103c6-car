/*
 * timx.cpp
 *
 *  Created on: May 4, 2026
 *      Author: bitstream
 */

#include "application.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "tim.h"

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2) {
    	app.Servo();
    }
}


#ifdef __cplusplus
}
#endif


