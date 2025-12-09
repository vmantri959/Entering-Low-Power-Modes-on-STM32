/*
 * low_power_modes.cpp
 *
 *  Created on: Dec 7, 2025
 *      Author: Viraj Mantri
 */
extern "C"
{
#include "main.h"
}

#include <cassert>
#include <iostream>
#include "low_power_modes.hpp"

extern void SystemClock_Config(void);
extern void MX_GPIO_Init(void);

namespace power {

	LowPowerModes::LowPowerModes(GPIO_TypeDef  *_GPIOx, uint16_t _GPIO_Pin, bool _wakeup_on_falling_edge)
	:
		GPIO_Port(_GPIOx),
		GPIO_Pin(_GPIO_Pin),
		wakeup_on_falling_edge(_wakeup_on_falling_edge)
	{}


	STOP2Mode::STOP2Mode(GPIO_TypeDef  *_GPIOx, uint16_t _GPIO_Pin, bool _wakeup_on_falling_edge) : LowPowerModes(_GPIOx, _GPIO_Pin,  _wakeup_on_falling_edge) {}

	void STOP2Mode::enter_low_power_mode(void)
	{
		HAL_SuspendTick(); //disable systick
		__HAL_GPIO_EXTI_CLEAR_IT(this->GPIO_Pin);
		/*TODO:Disable any further interrupts here, except the pin interrupt intended to wake-up the processor*/
		HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);
	}

	void STOP2Mode::exit_low_power_mode(void)
	{
		HAL_ResumeTick(); //enable systick
		SystemClock_Config();
		/*TODO:Enable any further interrupts and peripherals here*/
	}


	StandbyMode::StandbyMode(GPIO_TypeDef  *_GPIOx, uint16_t _GPIO_Pin, bool _wakeup_on_falling_edge) : LowPowerModes(_GPIOx, _GPIO_Pin,  _wakeup_on_falling_edge)
	{
		if((PWR->SR1) & PWR_SR1_SBF)
		{
			PWR->SCR |= PWR_SCR_CSBF; //clear flag. This flag indicates we are waking up from standby mode.
		}
	};

	void StandbyMode::enter_low_power_mode(void)
	{
		HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN1);
		if(this->wakeup_on_falling_edge)
		{
			HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1_HIGH);
		}
		else
		{
			HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1_LOW);
		}
		//Clear the flag!
		PWR->SCR |= PWR_SCR_CWUF1;
		HAL_PWR_EnterSTANDBYMode();
	}

	void StandbyMode::exit_low_power_mode(void) {}; //Nothing to do here since the processor is reset, but with the standby mode enabled.


	ShutdownMode::ShutdownMode(GPIO_TypeDef  *_GPIOx, uint16_t _GPIO_Pin, bool _wakeup_on_falling_edge) : LowPowerModes(_GPIOx, _GPIO_Pin,  _wakeup_on_falling_edge) {};

	void ShutdownMode::enter_low_power_mode(void)
	{
		HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN1);
		if(this->wakeup_on_falling_edge)
		{
			HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1_HIGH);
		}
		else
		{
			HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1_LOW);
		}
		//Clear the flag!
		PWR->SCR |= PWR_SCR_CWUF1;
		HAL_PWREx_EnterSHUTDOWNMode();
	}

	void ShutdownMode::exit_low_power_mode(void) {}; //Nothing to do here since the processor is reset.
}
