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
#include "low_power_modes.hpp"

extern void SystemClock_Config(void);
extern UART_HandleTypeDef huart4;

namespace power {

	LowPowerModes::LowPowerModes(GPIO_TypeDef  *_GPIOx, uint16_t _GPIO_Pin, bool _wakeup_on_falling_edge)
	:
		GPIO_Port(_GPIOx),
		GPIO_Pin(_GPIO_Pin),
		wakeup_on_falling_edge(_wakeup_on_falling_edge)
	{}


	STOP2Mode::STOP2Mode(GPIO_TypeDef  *_GPIOx, uint16_t _GPIO_Pin, bool _wakeup_on_falling_edge) : LowPowerModes(_GPIOx, _GPIO_Pin,  _wakeup_on_falling_edge) {
			  //TODO: Make it configurable for any pin
			  assert(_GPIOx == USER_WAKEUP_GPIO_Port);
			  assert(_GPIO_Pin == USER_WAKEUP_Pin);

			  /*Configure GPIO pin : WAKEUP_PIN_Pin */
			  GPIO_InitTypeDef GPIO_InitStruct = {0};
			  GPIO_InitStruct.Pin = USER_WAKEUP_Pin;
			  if(this->wakeup_on_falling_edge)
				  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
			  else
				  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;

			  GPIO_InitStruct.Pull = GPIO_NOPULL;
			  HAL_GPIO_Init(USER_WAKEUP_GPIO_Port, &GPIO_InitStruct);
	}


	void STOP2Mode::enter_low_power_mode(void)
	{
		SysTick->CTRL &= ~(SysTick_CTRL_ENABLE_Msk << SysTick_CTRL_ENABLE_Pos); //disable systick
		HAL_NVIC_SetPriority(EXTI13_IRQn, 0, 0);
		HAL_NVIC_EnableIRQ(EXTI13_IRQn);
		__HAL_GPIO_EXTI_CLEAR_IT(this->GPIO_Pin);
		/*TODO:Disable any further interrupts here, except the pin interrupt intended to wake-up the processor*/
		HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);
	}

	void STOP2Mode::exit_low_power_mode(void)
	{
		SysTick->CTRL |= (SysTick_CTRL_ENABLE_Msk << SysTick_CTRL_ENABLE_Pos); //enable systick
		SystemClock_Config();
		HAL_NVIC_DisableIRQ(EXTI13_IRQn);
		/*TODO:Enable any further interrupts here*/
	}


	StandbyMode::StandbyMode(GPIO_TypeDef  *_GPIOx, uint16_t _GPIO_Pin, bool _wakeup_on_falling_edge) : LowPowerModes(_GPIOx, _GPIO_Pin,  _wakeup_on_falling_edge)
	{
		//TODO: Make it for all of the wake-up pins
		if((PWR->SR1) & PWR_SR1_SBF)
		{
			const char display_message[] = "\r\nWaking up from Standby Mode!\r\n";
			HAL_UART_Transmit(&huart4, (const uint8_t*)display_message, sizeof(display_message)/sizeof(char), HAL_MAX_DELAY);
			PWR->SCR |= PWR_SCR_CSBF; //clear flag
		}
	};

	void StandbyMode::enter_low_power_mode(void)
	{
		HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN2);
		if(this->wakeup_on_falling_edge)
		{
			HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN2_HIGH);
		}
		else
		{
			HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN2_LOW);
		}
		//Clear the flag!
		PWR->SCR |= PWR_SCR_CWUF2;
		HAL_PWR_EnterSTANDBYMode();
	}

	void StandbyMode::exit_low_power_mode(void) {}; //Nothing to do here since the processor is reset, but with the standby mode enabled.


	ShutdownMode::ShutdownMode(GPIO_TypeDef  *_GPIOx, uint16_t _GPIO_Pin, bool _wakeup_on_falling_edge) : LowPowerModes(_GPIOx, _GPIO_Pin,  _wakeup_on_falling_edge) {};

	void ShutdownMode::enter_low_power_mode(void)
	{
		HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN2);
		if(this->wakeup_on_falling_edge)
		{
			HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN2_HIGH);
		}
		else
		{
			HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN2_LOW);
		}
		//Clear the flag!
		PWR->SCR |= PWR_SCR_CWUF2;
		HAL_PWREx_EnterSHUTDOWNMode();
	}

	void ShutdownMode::exit_low_power_mode(void) {}; //Nothing to do here since the processor is reset.
}
