#include "app.h"

/* Let the (planned) temperature sensor settle before the first reading. */
void sensor_calibrate(void)
{
    uint32_t t0 = millis();
    while (millis() - t0 < 1500u) {
        /* wait for the sensor reference to stabilise */
    }
}
