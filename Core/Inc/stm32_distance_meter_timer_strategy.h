#ifndef CORE_INC_STM32_DISTANCE_METER_TIMER_STRATEGY_H_
#define CORE_INC_STM32_DISTANCE_METER_TIMER_STRATEGY_H_
#include "DistanceMeterTimerStrategyInterface.h"
#include "distance_meter_isr_observer_interface.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_tim.h"

class Stm32DistanceMeterTimerStrategy
    : public DistanceMeterTimerStrategyInterface {
 public:
  Stm32DistanceMeterTimerStrategy() : observer_(nullptr) { instance_ = this; }
  ~Stm32DistanceMeterTimerStrategy() override = default;

  void InitializeHardware() override;
  void ConfigureTriggerPulse() override;
  void StartTriggerPulse() override;
  void ConfigureInputCaptureForRisingEdge() override;
  void ConfigureInputCaptureForFallingEdge() override;
  void StartInputCapture() override;

  // ISR logic
  bool IsOutputCompareInterrupt() override;
  bool IsInputCaptureInterrupt() override;
  bool IsUpdateInterrupt() override;
  void ClearOutputCompareFlag() override;
  void ClearInputCaptureFlag() override;
  void ClearUpdateFlag() override;

  // Additional helper methods
  uint32_t GetUpdateEventPeriod() override;
  uint32_t GetCapturedValue() override;
  void ClearCounter() override;
  void StopTimer() override;
  uint32_t UsToTicks(uint32_t us) override;
  void ResetHardware() override;  // reset hardware and clear pending flags,
                                  // ready for next measurement
  void ProcessISR();
  // observer methods
  void AddObserver(DistanceMeterIsrObserverInterface* observer);
  void RemoveObserver();
  void NotifyTimerOutputCompareInterrupt();
  void NotifyTimerInputCaptureInterrupt();
  void NotifyTimerUpdateInterrupt();
  static Stm32DistanceMeterTimerStrategy* instance_;
  static void ISR();

 private:
  TIM_TypeDef* timer_instance_ = TIM3;
  LL_TIM_InitTypeDef tim_init_struct_ = {};
  LL_TIM_OC_InitTypeDef tim_oc_init_struct_ = {};
  LL_TIM_IC_InitTypeDef tim_ic_init_struct_ = {};

  LL_GPIO_InitTypeDef echo_gpio_init_struct_ = {};
  LL_GPIO_InitTypeDef trig_gpio_init_struct_ = {};
  GPIO_TypeDef* echo_gpio_port_ = GPIOA;
  GPIO_TypeDef* trig_gpio_port_ = GPIOA;
  uint32_t timer_peripheral_enable_clock_mask_ = LL_APB1_GRP1_PERIPH_TIM3;
  uint32_t trig_gpio_port_enable_clock_mask_ = LL_AHB1_GRP1_PERIPH_GPIOA;
  uint32_t echo_gpio_port_enable_clock_mask_ = LL_AHB1_GRP1_PERIPH_GPIOA;

  uint32_t pulse_width_us_ = 50;
  DistanceMeterIsrObserverInterface* observer_;
};

#endif /* CORE_INC_STM32_DISTANCE_METER_TIMER_STRATEGY_H_ */
