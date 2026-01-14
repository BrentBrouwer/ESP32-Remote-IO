#include "Logging.h"
#include <Arduino.h>

void LogWithTime(const char* msg)
{
    Serial.printf("<%lu> %s\n", msg);
}