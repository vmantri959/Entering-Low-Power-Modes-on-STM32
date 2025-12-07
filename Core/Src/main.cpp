/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <cassert>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart4;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_UART4_Init(void);
/* USER CODE BEGIN PFP */
class LowPowerModes
{
protected:
	GPIO_TypeDef* GPIO_Port;
	uint16_t GPIO_Pin;
	bool wakeup_on_falling_edge;
public:
	LowPowerModes(GPIO_TypeDef  *_GPIOx, uint16_t _GPIO_Pin, bool _wakeup_on_falling_edge)
	:
		GPIO_Port(_GPIOx),
		GPIO_Pin(_GPIO_Pin),
		wakeup_on_falling_edge(_wakeup_on_falling_edge)
	{}

	virtual void enter_low_power_mode(void) = 0;
	virtual void exit_low_power_mode(void) = 0;

	virtual ~LowPowerModes() = default;
};

class STOP2Mode : public LowPowerModes
{
public:
	STOP2Mode(GPIO_TypeDef  *_GPIOx, uint16_t _GPIO_Pin, bool _wakeup_on_falling_edge) : LowPowerModes(_GPIOx, _GPIO_Pin,  _wakeup_on_falling_edge) {
	  //TODO: Make it configurable for any pin
	  assert(_GPIOx == WAKEUP_PIN_GPIO_Port);
	  assert(_GPIO_Pin == WAKEUP_PIN_Pin);

	  /*Configure GPIO pin : WAKEUP_PIN_Pin */
	  GPIO_InitTypeDef GPIO_InitStruct = {0};
	  GPIO_InitStruct.Pin = WAKEUP_PIN_Pin;
	  if(this->wakeup_on_falling_edge)
		  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
	  else
		  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;

	  GPIO_InitStruct.Pull = GPIO_NOPULL;
	  HAL_GPIO_Init(WAKEUP_PIN_GPIO_Port, &GPIO_InitStruct);
	}

	~STOP2Mode() = default;

	void enter_low_power_mode(void) override
	{
		SysTick->CTRL &= ~(SysTick_CTRL_ENABLE_Msk << SysTick_CTRL_ENABLE_Pos); //disable systick
	    HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
	    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
	    __HAL_GPIO_EXTI_CLEAR_IT(this->GPIO_Pin);
		/*TODO:Disable any further interrupts here, except the pin interrupt intended to wake-up the processor*/
		HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);
	}

	void exit_low_power_mode(void) override
	{
		SysTick->CTRL |= (SysTick_CTRL_ENABLE_Msk << SysTick_CTRL_ENABLE_Pos); //enable systick
		SystemClock_Config();
	    HAL_NVIC_DisableIRQ(EXTI0_IRQn);
		/*TODO:Enable any further interrupts here*/
	}
};

class StandbyMode : public LowPowerModes
{
public:
	StandbyMode(GPIO_TypeDef  *_GPIOx, uint16_t _GPIO_Pin, bool _wakeup_on_falling_edge) : LowPowerModes(_GPIOx, _GPIO_Pin,  _wakeup_on_falling_edge)
	{
		//TODO: Make it for all of the wake-up pins
		if((PWR->SR1) & PWR_SR1_SBF)
		{
			const char display_message[] = "\r\nWaking up from Standby Mode!\r\n";
			HAL_UART_Transmit(&huart4, (const uint8_t*)display_message, sizeof(display_message)/sizeof(char), HAL_MAX_DELAY);
			PWR->SCR |= PWR_SCR_CSBF; //clear flag
		}
	};
	~StandbyMode() = default;

	void enter_low_power_mode(void) override
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

	void exit_low_power_mode(void) override {}; //Nothing to do here since the processor is reset, but with the standby mode enabled.
};


class ShutdownMode : public LowPowerModes
{
public:
	ShutdownMode(GPIO_TypeDef  *_GPIOx, uint16_t _GPIO_Pin, bool _wakeup_on_falling_edge) : LowPowerModes(_GPIOx, _GPIO_Pin,  _wakeup_on_falling_edge) {};
	~ShutdownMode() = default;

	void enter_low_power_mode(void) override
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

	void exit_low_power_mode(void) override {}; //Nothing to do here since the processor is reset.
};

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_UART4_Init();
  /* USER CODE BEGIN 2 */

  ShutdownMode* low_power_mode = new ShutdownMode(WAKEUP_PIN_GPIO_Port, WAKEUP_PIN_Pin, false);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  HAL_Delay(5000); //Delay can vary based on how long it takes for the signal to settle.
	  const char user_prompt[] = "\r\nWaking Up!\r\n";
	  HAL_UART_Transmit(&huart4, (uint8_t*)user_prompt, sizeof(user_prompt), HAL_MAX_DELAY);
	  low_power_mode->enter_low_power_mode();
	  low_power_mode->exit_low_power_mode();
  }
  delete low_power_mode;
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief UART4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART4_Init(void)
{

  /* USER CODE BEGIN UART4_Init 0 */

  /* USER CODE END UART4_Init 0 */

  /* USER CODE BEGIN UART4_Init 1 */

  /* USER CODE END UART4_Init 1 */
  huart4.Instance = UART4;
  huart4.Init.BaudRate = 115200;
  huart4.Init.WordLength = UART_WORDLENGTH_8B;
  huart4.Init.StopBits = UART_STOPBITS_1;
  huart4.Init.Parity = UART_PARITY_NONE;
  huart4.Init.Mode = UART_MODE_TX_RX;
  huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart4.Init.OverSampling = UART_OVERSAMPLING_16;
  huart4.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart4.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart4.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart4) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart4, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart4, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART4_Init 2 */

  /* USER CODE END UART4_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_11|GPIO_PIN_12, GPIO_PIN_SET);

  /*Configure GPIO pins : PF11 PF12 */
  GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /* EXTI interrupt init*/

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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
