#include "stm32_distance_meter_timer_strategy.h"
class DistanceMeter;

Stm32DistanceMeterTimerStrategy* Stm32DistanceMeterTimerStrategy::instance_ =
    nullptr;

void Stm32DistanceMeterTimerStrategy::InitializeHardware() {
  LL_APB1_GRP1_EnableClock(timer_peripheral_enable_clock_mask_);
  LL_AHB1_GRP1_EnableClock(trig_gpio_port_enable_clock_mask_);
  LL_AHB1_GRP1_EnableClock(echo_gpio_port_enable_clock_mask_);

  echo_gpio_init_struct_.Pin = LL_GPIO_PIN_6;
  echo_gpio_init_struct_.Mode = LL_GPIO_MODE_ALTERNATE;
  echo_gpio_init_struct_.Speed = LL_GPIO_SPEED_FREQ_LOW;
  echo_gpio_init_struct_.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  echo_gpio_init_struct_.Pull = LL_GPIO_PULL_NO;
  echo_gpio_init_struct_.Alternate = LL_GPIO_AF_2;
  LL_GPIO_Init(echo_gpio_port_, &echo_gpio_init_struct_);

  trig_gpio_init_struct_.Pin = LL_GPIO_PIN_7;
  trig_gpio_init_struct_.Mode = LL_GPIO_MODE_ALTERNATE;
  trig_gpio_init_struct_.Speed = LL_GPIO_SPEED_FREQ_LOW;
  trig_gpio_init_struct_.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  trig_gpio_init_struct_.Pull = LL_GPIO_PULL_NO;
  trig_gpio_init_struct_.Alternate = LL_GPIO_AF_2;
  LL_GPIO_Init(trig_gpio_port_, &trig_gpio_init_struct_);

  tim_init_struct_.Prescaler = 83;
  tim_init_struct_.CounterMode = LL_TIM_COUNTERMODE_UP;
  tim_init_struct_.Autoreload = pulse_width_us_ + 10;
  tim_init_struct_.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
  LL_TIM_Init(timer_instance_, &tim_init_struct_);
  LL_TIM_SetOnePulseMode(timer_instance_, LL_TIM_ONEPULSEMODE_SINGLE);
  LL_TIM_SetTriggerOutput(timer_instance_, LL_TIM_TRGO_RESET);
  LL_TIM_DisableMasterSlaveMode(timer_instance_);
  LL_TIM_DisableARRPreload(timer_instance_);

  tim_oc_init_struct_.OCMode = LL_TIM_OCMODE_INACTIVE;
  tim_oc_init_struct_.OCState = LL_TIM_OCSTATE_ENABLE;
  tim_oc_init_struct_.OCNState = LL_TIM_OCSTATE_DISABLE;
  tim_oc_init_struct_.CompareValue =
      pulse_width_us_;  // note that there is a switch delay for 5 clocks,
                        // so the compare value should be greater than 5
  tim_oc_init_struct_.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
  LL_TIM_OC_Init(timer_instance_, LL_TIM_CHANNEL_CH2, &tim_oc_init_struct_);
  LL_TIM_OC_DisableFast(timer_instance_, LL_TIM_CHANNEL_CH2);

  tim_ic_init_struct_.ICActiveInput = LL_TIM_ACTIVEINPUT_DIRECTTI;
  tim_ic_init_struct_.ICPrescaler = LL_TIM_ICPSC_DIV1;
  tim_ic_init_struct_.ICFilter = LL_TIM_IC_FILTER_FDIV1;
  tim_ic_init_struct_.ICPolarity = LL_TIM_IC_POLARITY_RISING;
  LL_TIM_IC_Init(timer_instance_, LL_TIM_CHANNEL_CH1, &tim_ic_init_struct_);

  //   enable interrupt and set priority if necessary
  NVIC_SetPriority(TIM3_IRQn, 0);
  NVIC_EnableIRQ(TIM3_IRQn);

  // LL_TIM_EnableIT_CC1(timer_instance_); // IC
  // LL_TIM_EnableIT_CC2(timer_instance_); // OC
  LL_TIM_EnableIT_UPDATE(timer_instance_);
}

void Stm32DistanceMeterTimerStrategy::ConfigureTriggerPulse() {
  LL_TIM_DisableCounter(timer_instance_);
  LL_TIM_SetCounter(timer_instance_, 0);

  tim_oc_init_struct_.OCMode = LL_TIM_OCMODE_FORCED_ACTIVE;
  LL_TIM_OC_Init(timer_instance_, LL_TIM_CHANNEL_CH2, &tim_oc_init_struct_);

  tim_oc_init_struct_.OCMode = LL_TIM_OCMODE_INACTIVE;
  LL_TIM_OC_Init(timer_instance_, LL_TIM_CHANNEL_CH2, &tim_oc_init_struct_);

  LL_TIM_DisableIT_CC1(timer_instance_);
  LL_TIM_CC_DisableChannel(timer_instance_, LL_TIM_CHANNEL_CH1);

  LL_TIM_ClearFlag_CC2(timer_instance_);
  LL_TIM_EnableIT_CC2(timer_instance_);

  LL_TIM_SetAutoReload(timer_instance_, pulse_width_us_ + 10);
  LL_TIM_SetOnePulseMode(timer_instance_, LL_TIM_ONEPULSEMODE_SINGLE);

  LL_TIM_CC_EnableChannel(timer_instance_, LL_TIM_CHANNEL_CH2);
}

void Stm32DistanceMeterTimerStrategy::StartTriggerPulse() {
  LL_TIM_EnableCounter(timer_instance_);
}

void Stm32DistanceMeterTimerStrategy::ConfigureInputCaptureForRisingEdge() {
  LL_TIM_DisableCounter(timer_instance_);
  LL_TIM_SetCounter(timer_instance_, 0);

  tim_ic_init_struct_.ICPolarity = LL_TIM_IC_POLARITY_RISING;
  LL_TIM_IC_Init(timer_instance_, LL_TIM_CHANNEL_CH1, &tim_ic_init_struct_);

  LL_TIM_DisableIT_CC2(timer_instance_);
  tim_oc_init_struct_.OCMode = LL_TIM_OCMODE_FORCED_INACTIVE;
  LL_TIM_OC_Init(timer_instance_, LL_TIM_CHANNEL_CH2, &tim_oc_init_struct_);

  LL_TIM_ClearFlag_CC1(timer_instance_);
  LL_TIM_EnableIT_CC1(timer_instance_);

  LL_TIM_SetAutoReload(timer_instance_,
                       0xFFFFFFFF);  // max period for input capture
  LL_TIM_SetOnePulseMode(timer_instance_, LL_TIM_ONEPULSEMODE_REPETITIVE);

  LL_TIM_CC_EnableChannel(timer_instance_, LL_TIM_CHANNEL_CH1);
}

void Stm32DistanceMeterTimerStrategy::ConfigureInputCaptureForFallingEdge() {
  tim_ic_init_struct_.ICPolarity = LL_TIM_IC_POLARITY_FALLING;
  LL_TIM_IC_Init(timer_instance_, LL_TIM_CHANNEL_CH1, &tim_ic_init_struct_);
}

void Stm32DistanceMeterTimerStrategy::StartInputCapture() {
  LL_TIM_EnableCounter(timer_instance_);
}

bool Stm32DistanceMeterTimerStrategy::IsOutputCompareInterrupt() {
  return LL_TIM_IsActiveFlag_CC2(timer_instance_);
}

bool Stm32DistanceMeterTimerStrategy::IsInputCaptureInterrupt() {
  return LL_TIM_IsActiveFlag_CC1(timer_instance_);
}

bool Stm32DistanceMeterTimerStrategy::IsUpdateInterrupt() {
  return LL_TIM_IsActiveFlag_UPDATE(timer_instance_);
}

void Stm32DistanceMeterTimerStrategy::ClearOutputCompareFlag() {
  LL_TIM_ClearFlag_CC2(timer_instance_);
}

void Stm32DistanceMeterTimerStrategy::ClearInputCaptureFlag() {
  LL_TIM_ClearFlag_CC1(timer_instance_);
}

void Stm32DistanceMeterTimerStrategy::ClearUpdateFlag() {
  LL_TIM_ClearFlag_UPDATE(timer_instance_);
}

uint32_t Stm32DistanceMeterTimerStrategy::GetUpdateEventPeriod() {
  return LL_TIM_GetAutoReload(timer_instance_) + 1;  // period is ARR + 1
}

uint32_t Stm32DistanceMeterTimerStrategy::GetCapturedValue() {
  return LL_TIM_IC_GetCaptureCH1(timer_instance_);
}

void Stm32DistanceMeterTimerStrategy::ClearCounter() {
  LL_TIM_SetCounter(timer_instance_, 0);
}

void Stm32DistanceMeterTimerStrategy::StopTimer() {
  LL_TIM_DisableCounter(timer_instance_);
}

uint32_t Stm32DistanceMeterTimerStrategy::UsToTicks(uint32_t us) {
  return us;
}  // us to ticks, since timer clock is 1MHz after prescaler

void Stm32DistanceMeterTimerStrategy::ResetHardware() {
  LL_TIM_ClearFlag_CC1(timer_instance_);
  LL_TIM_ClearFlag_CC2(timer_instance_);
  LL_TIM_ClearFlag_UPDATE(timer_instance_);
  LL_TIM_SetCounter(timer_instance_, 0);
  LL_TIM_DisableCounter(timer_instance_);
}

void Stm32DistanceMeterTimerStrategy::ProcessISR() {
  if (IsOutputCompareInterrupt()) {
    ClearOutputCompareFlag();
    LL_TIM_ClearFlag_CC2(timer_instance_);
    NotifyTimerOutputCompareInterrupt();
  }
  if (IsInputCaptureInterrupt()) {
    ClearInputCaptureFlag();
    NotifyTimerInputCaptureInterrupt();
  }
  if (IsUpdateInterrupt()) {
    ClearUpdateFlag();
    NotifyTimerUpdateInterrupt();
  }
}

void Stm32DistanceMeterTimerStrategy::AddObserver(
    DistanceMeterIsrObserverInterface* observer) {
  observer_ = observer;
}

void Stm32DistanceMeterTimerStrategy::RemoveObserver() { observer_ = nullptr; }

void Stm32DistanceMeterTimerStrategy::NotifyTimerOutputCompareInterrupt() {
  if (observer_) {
    observer_->OnTimerOutputCompareInterrupt();
  }
}

void Stm32DistanceMeterTimerStrategy::NotifyTimerInputCaptureInterrupt() {
  if (observer_) {
    observer_->OnTimerInputCaptureInterrupt();
  }
}

void Stm32DistanceMeterTimerStrategy::NotifyTimerUpdateInterrupt() {
  if (observer_) {
    observer_->OnTimerUpdateInterrupt();
  }
}

void Stm32DistanceMeterTimerStrategy::ISR() {
  if (Stm32DistanceMeterTimerStrategy::instance_ == nullptr) return;
  // Call the non-static method
  Stm32DistanceMeterTimerStrategy::instance_->ProcessISR();
}

#ifdef __cplusplus
extern "C" {
#endif
void TIM3_IRQHandler(void) { Stm32DistanceMeterTimerStrategy::ISR(); }
#ifdef __cplusplus
}
#endif