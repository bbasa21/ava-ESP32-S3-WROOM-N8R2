#ifndef AVA_NETWORK_LOCK_H
#define AVA_NETWORK_LOCK_H

#include <Arduino.h>

// ==================================================
// AVA NETWORK LOCK
// Serializes HTTPS/TLS users such as Weather and OTA.
// ==================================================

bool avaNetworkLockAcquire(TickType_t timeoutTicks);
void avaNetworkLockRelease();

class AvaNetworkLockGuard
{
public:
    explicit AvaNetworkLockGuard(
        TickType_t timeoutTicks = portMAX_DELAY
    )
        : locked(avaNetworkLockAcquire(timeoutTicks))
    {
    }

    ~AvaNetworkLockGuard()
    {
        if (locked)
        {
            avaNetworkLockRelease();
        }
    }

    bool isLocked() const
    {
        return locked;
    }

private:
    bool locked;
};

#endif
