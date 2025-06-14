#include <Arduino.h>

#if TELNET_ENABLED
#include <TelnetSpy.h>

extern TelnetSpy SerialAndTelnet;
#define LOG SerialAndTelnet
#else
#define LOG Serial
#endif
