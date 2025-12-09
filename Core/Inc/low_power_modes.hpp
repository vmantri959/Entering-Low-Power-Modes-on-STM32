/*
 * low_power_modes.hpp
 *
 *  Created on: Dec 7, 2025
 *      Author: Viraj Mantri
 */

#ifndef INC_LOW_POWER_MODES_HPP_
#define INC_LOW_POWER_MODES_HPP_

namespace power {

	class LowPowerModes
	{
		protected:
			GPIO_TypeDef* GPIO_Port;
			uint16_t GPIO_Pin;
			bool wakeup_on_falling_edge;
			void configure_wakeup_pin(uint32_t wakeup_pin);
		public:
			LowPowerModes(GPIO_TypeDef  *_GPIOx, uint16_t _GPIO_Pin, bool _wakeup_on_falling_edge);
			virtual void enter_low_power_mode(void) = 0;
			virtual void exit_low_power_mode(void) = 0;
			virtual ~LowPowerModes() = default;
	};

	class STOP2Mode : public LowPowerModes
	{
		public:
			STOP2Mode(GPIO_TypeDef  *_GPIOx, uint16_t _GPIO_Pin, bool _wakeup_on_falling_edge);
			void enter_low_power_mode(void) override;
			void exit_low_power_mode(void) override;
			~STOP2Mode() = default;

	};

	class StandbyMode : public LowPowerModes
	{
		public:
		    StandbyMode(GPIO_TypeDef  *_GPIOx, uint16_t _GPIO_Pin, bool _wakeup_on_falling_edge);
			void enter_low_power_mode(void) override;
			void exit_low_power_mode(void) override;
			~StandbyMode() = default;
	};


	class ShutdownMode : public LowPowerModes
	{
		public:
			ShutdownMode(GPIO_TypeDef  *_GPIOx, uint16_t _GPIO_Pin, bool _wakeup_on_falling_edge);
			void enter_low_power_mode(void) override;
			void exit_low_power_mode(void) override;
			~ShutdownMode() = default;
	};
}




#endif /* INC_LOW_POWER_MODES_HPP_ */
