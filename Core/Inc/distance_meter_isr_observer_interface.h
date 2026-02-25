#ifndef CORE_INC_DISTANCE_METER_ISR_OBSERVER_INTERFACE_H_
#define CORE_INC_DISTANCE_METER_ISR_OBSERVER_INTERFACE_H_

class DistanceMeterIsrObserverInterface
{
public:
    virtual ~DistanceMeterIsrObserverInterface() = default;
    virtual void OnTimerOutputCompareInterrupt() = 0;
    virtual void OnTimerInputCaptureInterrupt() = 0;
    virtual void OnTimerUpdateInterrupt() = 0;
};

#endif /* CORE_INC_DISTANCE_METER_ISR_OBSERVER_INTERFACE_H_ */
