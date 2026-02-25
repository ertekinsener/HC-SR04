#include "app.h"

#include "distance_sensor.h"
#include "stm32_distance_meter_timer_strategy.h"
#include "stm32f4xx_ll_utils.h"
// #include <cstdio>
Stm32DistanceMeterTimerStrategy timer_strategy;
DistanceSensor sensor(timer_strategy);
void App() {
  timer_strategy.AddObserver(&sensor);
  uint32_t measurement_ticks = 0;
  sensor.Initialize();
  while (1) {
    sensor.StartMeasurement();
    LL_mDelay(10000);
    if (sensor.GetMeasurement(measurement_ticks)) {
      // Process measurement_ticks value
      measurement_ticks = 0;
      // printf("Measurement: %lu ticks\n", measurement_ticks);
    } else {
      // Handle case where measurement is not ready (e.g., timeout)
      measurement_ticks = 0;
      // printf("Measurement not ready or timeout occurred\n");
    }
  }
}