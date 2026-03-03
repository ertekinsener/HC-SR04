#include "distance_sensor.h"

DistanceSensor::DistanceSensor(DistanceMeterTimerStrategyInterface& hw_strategy,
               uint32_t timeout_us)
    : hw_strategy_(hw_strategy),
      current_state_(State::kUninitialized),
      measurement_ticks_(0),
      timeout_ticks_(hw_strategy_.UsToTicks(timeout_us)),
      elapsed_ticks_(0) {}

void DistanceSensor::Initialize() {
  if (current_state_ == State::kUninitialized) {
    hw_strategy_.InitializeHardware();
    current_state_ = State::kReady;
  }
}

void DistanceSensor::StartMeasurement() {
  if (current_state_ == State::kReady || current_state_ == State::kTimeout) {
    hw_strategy_.ConfigureTriggerPulse();
    hw_strategy_.StartTriggerPulse();
    current_state_ = State::kSendingTrigger;
  }
}

void DistanceSensor::OnTimerOutputCompareInterrupt() {
  if (current_state_ == State::kSendingTrigger) {
    elapsed_ticks_ = 0;
    hw_strategy_.ConfigureInputCaptureForRisingEdge();
    hw_strategy_.StartInputCapture();
    current_state_ = State::kWaitingForRisingEdge;
  }
}

void DistanceSensor::OnTimerInputCaptureInterrupt() {
  if (current_state_ == State::kWaitingForRisingEdge) {
    hw_strategy_.ClearCounter();
    // capture_start_ = hw_strategy_.GetCapturedValue();
    hw_strategy_.ConfigureInputCaptureForFallingEdge();
    current_state_ = State::kWaitingForFallingEdge;
  } else if (current_state_ == State::kWaitingForFallingEdge) {
    // uint32_t capture_end = hw_strategy_.GetCapturedValue();
    measurement_ticks_ = hw_strategy_.GetCapturedValue();
    // if (capture_end >= capture_start_) {
    //   measurement_ticks_ = capture_end - capture_start_;
    // } else {
    //   measurement_ticks_ = (0xFFFFFFFF - capture_start_ + capture_end + 1);
    // }
    hw_strategy_.StopTimer();
    current_state_ = State::kDataReady;
  }
}

void DistanceSensor::OnTimerUpdateInterrupt() {
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

bool DistanceSensor::GetMeasurement(uint32_t& distance_cm) {
  if (current_state_ == State::kDataReady) {
    distance_cm =
        measurement_ticks_ / 58.0;  // convert microseconds to centimeters
    current_state_ = State::kReady;
    return true;
  }
  return false;
}

DistanceSensor::State DistanceSensor::GetCurrentState() const {
  return current_state_; }

void DistanceSensor::SetTimeout(uint32_t timeout_us) {
  timeout_ticks_ = hw_strategy_.UsToTicks(timeout_us);
}

void DistanceSensor::GetTimeout(uint32_t& timeout_us) const {
  timeout_us = hw_strategy_.TicksToUs(timeout_ticks_);
}
