#include "Ava_NetworkLock.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

static StaticSemaphore_t avaNetworkMutexStorage;
static SemaphoreHandle_t avaNetworkMutex = nullptr;

static SemaphoreHandle_t avaGetNetworkMutex()
{
    if (avaNetworkMutex == nullptr)
    {
        avaNetworkMutex = xSemaphoreCreateMutexStatic(
            &avaNetworkMutexStorage
        );
    }

    return avaNetworkMutex;
}

bool avaNetworkLockAcquire(TickType_t timeoutTicks)
{
    SemaphoreHandle_t mutex = avaGetNetworkMutex();

    if (mutex == nullptr)
    {
        return false;
    }

    return xSemaphoreTake(
        mutex,
        timeoutTicks
    ) == pdTRUE;
}

void avaNetworkLockRelease()
{
    SemaphoreHandle_t mutex = avaGetNetworkMutex();

    if (mutex == nullptr)
    {
        return;
    }

    xSemaphoreGive(mutex);
}
