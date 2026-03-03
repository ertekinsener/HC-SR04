#ifndef CORE_INC_DISTANCE_SENSOR_H_
#define CORE_INC_DISTANCE_SENSOR_H_

#include <stdint.h>
#include "distance_meter_isr_observer_interface.h"
#include "distance_meter_timer_strategy_interface.h"
class DistanceSensor : public DistanceMeterIsrObserverInterface {
 public:
  // Flattens the composite state for efficient C++ execution
  enum class State {
    kUninitialized,
    kReady,
    kSendingTrigger,
    kWaitingForRisingEdge,   // Inside ECHO_PROCESSING
    kWaitingForFallingEdge,  // Inside ECHO_PROCESSING
    kDataReady,
    kTimeout
  };

  DistanceSensor(DistanceMeterTimerStrategyInterface& hw_strategy,
                 uint32_t timeout_us = 10000);

  void Initialize();
  void StartMeasurement();
  bool GetMeasurement(uint32_t& distance_cm);
  State GetCurrentState() const;
  void SetTimeout(uint32_t timeout_us);
  void GetTimeout(uint32_t& timeout_us) const;

  // DistanceMeterIsrObserverInterface implementations
  void OnTimerOutputCompareInterrupt() override;
  void OnTimerInputCaptureInterrupt() override;
  void OnTimerUpdateInterrupt() override;
 private:
  DistanceMeterTimerStrategyInterface& hw_strategy_;
  volatile State current_state_;
  volatile uint32_t measurement_ticks_;
  volatile uint32_t timeout_ticks_;
  volatile uint32_t elapsed_ticks_;
};

#endif /* CORE_INC_DISTANCE_SENSOR_H_ */
