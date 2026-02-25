#ifndef CORE_INC_DISTANCEMETERTIMERSTRATEGYINTERFACE_H_
#define CORE_INC_DISTANCEMETERTIMERSTRATEGYINTERFACE_H_

#include <stdint.h>

class DistanceMeterTimerStrategyInterface {
public:
    virtual ~DistanceMeterTimerStrategyInterface() = default;

    virtual void InitializeHardware() = 0;
    virtual void ConfigureTriggerPulse() = 0;
    virtual void StartTriggerPulse() = 0;
    virtual void ConfigureInputCaptureForRisingEdge() = 0;
    virtual void ConfigureInputCaptureForFallingEdge() = 0;
    virtual void StartInputCapture() = 0;
    // ISR logic
    virtual bool IsOutputCompareInterrupt() = 0;
    virtual bool IsInputCaptureInterrupt() = 0;
    virtual bool IsUpdateInterrupt() = 0;
    virtual void ClearOutputCompareFlag() = 0;
    virtual void ClearInputCaptureFlag() = 0;
    virtual void ClearUpdateFlag() = 0;

    // Additional helper methods
    virtual uint32_t GetUpdateEventPeriod() = 0;
    virtual uint32_t GetCapturedValue() = 0;
    virtual void ClearCounter() = 0;
    virtual void StopTimer() = 0;
    virtual uint32_t UsToTicks(uint32_t us) = 0;
    virtual void ResetHardware() = 0; // reset hardware and clear pending flags, ready for next measurement
};

#endif /* CORE_INC_DISTANCEMETERTIMERSTRATEGYINTERFACE_H_ */
