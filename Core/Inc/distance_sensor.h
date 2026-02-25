#ifndef CORE_INC_DISTANCE_SENSOR_H_
#define CORE_INC_DISTANCE_SENSOR_H_

#include <stdint.h>
#include "distance_meter_isr_observer_interface.h"
#include "DistanceMeterTimerStrategyInterface.h"
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
                 uint32_t timeout_us = 10000)
      : hw_strategy_(hw_strategy),
        current_state_(State::kUninitialized),
        measurement_ticks_(0),
        timeout_ticks_(hw_strategy_.UsToTicks(timeout_us)),
        elapsed_ticks_(0) {}

  void Initialize() {
    if (current_state_ == State::kUninitialized) {
      hw_strategy_.InitializeHardware();
      current_state_ = State::kReady;
    }
  }

  void StartMeasurement() {
    if (current_state_ == State::kReady || current_state_ == State::kTimeout) {
      hw_strategy_.ConfigureTriggerPulse();
      hw_strategy_.StartTriggerPulse();
      current_state_ = State::kSendingTrigger;
    }
  }

  void OnTimerOutputCompareInterrupt() override {
    if (current_state_ == State::kSendingTrigger) {
      elapsed_ticks_ = 0;
      hw_strategy_.ConfigureInputCaptureForRisingEdge();
      hw_strategy_.StartInputCapture();
      current_state_ = State::kWaitingForRisingEdge;
    }
  }

  void OnTimerInputCaptureInterrupt() override {
    if (current_state_ == State::kWaitingForRisingEdge) {
      hw_strategy_.ClearCounter();
      capture_start_ = hw_strategy_.GetCapturedValue();
      hw_strategy_.ConfigureInputCaptureForFallingEdge();
      current_state_ = State::kWaitingForFallingEdge;
    } else if (current_state_ == State::kWaitingForFallingEdge) {
      uint32_t capture_end = hw_strategy_.GetCapturedValue();

      if (capture_end >= capture_start_) {
        measurement_ticks_ = capture_end - capture_start_;
      } else {
        measurement_ticks_ = (0xFFFFFFFF - capture_start_ + capture_end + 1);
      }
      hw_strategy_.StopTimer();
      current_state_ = State::kDataReady;
    }
  }

  void OnTimerUpdateInterrupt() override {
    if (current_state_ == State::kWaitingForRisingEdge ||
        current_state_ == State::kWaitingForFallingEdge) {
      elapsed_ticks_ = elapsed_ticks_ + hw_strategy_.GetUpdateEventPeriod();
      if (elapsed_ticks_ >= timeout_ticks_) {
        hw_strategy_.StopTimer();
        hw_strategy_.ResetHardware();
        current_state_ = State::kTimeout;
      }
    }
  }

  bool GetMeasurement(uint32_t& out_ticks) {
    if (current_state_ == State::kDataReady) {
      out_ticks = measurement_ticks_;
      current_state_ = State::kReady;
      return true;
    }
    return false;
  }

  State GetCurrentState() const { return current_state_; }

  void SetTimeout(uint32_t timeout_us) {
    timeout_ticks_ = hw_strategy_.UsToTicks(timeout_us);
  }

 private:
  DistanceMeterTimerStrategyInterface& hw_strategy_;
  volatile State current_state_;
  volatile uint32_t capture_start_;
  volatile uint32_t measurement_ticks_;
  volatile uint32_t timeout_ticks_;
  volatile uint32_t elapsed_ticks_;
};

#endif /* CORE_INC_DISTANCE_SENSOR_H_ */
