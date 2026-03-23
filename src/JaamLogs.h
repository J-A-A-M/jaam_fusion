#pragma once

#include <Arduino.h>
#include <cstring>
#include <mutex>
#include <Print.h>
#include <ctime>

#if TELNET_ENABLED
#include <TelnetSpy.h>
extern TelnetSpy SerialAndTelnet;
static Print* _LOG_BASE = &SerialAndTelnet;
#else
static Print* _LOG_BASE = &Serial;
#endif

// Log entry structure
struct LogEntry {
    char tag[16];          // [TAG] from log message
    char message[128];     // Log message content
    time_t timestamp;      // When the log was captured (Unix timestamp from NTP)
    uint8_t level;         // 0=DEBUG, 1=INFO, 2=WARNING, 3=ERROR
};

class JaamLogsManager;

// Custom Print wrapper that intercepts all log output
class LoggingPrint : public Print {
public:
    LoggingPrint(Print* baseStream) : baseStream(baseStream), logsManager(nullptr) {}

    void setLogsManager(JaamLogsManager* manager) {
        logsManager = manager;
    }

    void setLogsEnabled(bool enabled) {
        logsEnabled = enabled;
    }

    bool isLogsEnabled() const {
        return logsEnabled;
    }

    virtual size_t write(uint8_t c) override {
        if (baseStream && logsEnabled) {
            baseStream->write(c);
        }
        return 1;
    }

    virtual size_t write(const uint8_t* buffer, size_t size) override;

    // Initialize the base stream
    void begin(unsigned long speed) {
#if TELNET_ENABLED
        SerialAndTelnet.begin(speed);
#else
        Serial.begin(speed);
#endif
    }

private:
    Print* baseStream;
    JaamLogsManager* logsManager;
    bool logsEnabled = true;
    char lineBuffer[512];
    size_t linePos = 0;

    void processLine(const char* line);
};

class JaamLogsManager {
public:
    static JaamLogsManager& getInstance() {
        static JaamLogsManager instance;
        return instance;
    }

    // Initialize logs manager
    void begin();

    // Add a log entry to the circular buffer (uses NTP-synced system time)
    void addLog(const char* tag, const char* message, uint8_t level = 1);

    // Get recent logs as JSON array (limit: max number of logs to return)
    String getLogsJson(int limit = 100);

    // Get total number of logs captured
    int getLogCount() const {
        std::lock_guard<std::mutex> lock(logMutex);
        return logCount;
    }

    // Clear all logs
    void clearLogs();

    // Get the logging print wrapper
    LoggingPrint* getLoggingPrint() { return loggingPrint; }

private:
    static const int MAX_LOGS = 100;  // Circular buffer size (~6KB total)
    LogEntry logBuffer[MAX_LOGS];     // Static circular buffer
    int writeIndex = 0;               // Where next log will be written
    int logCount = 0;                 // Number of logs currently stored (0 to MAX_LOGS)
    mutable std::mutex logMutex;
    LoggingPrint* loggingPrint = nullptr;

    JaamLogsManager() = default;
    ~JaamLogsManager() = default;

    // Prevent copying
    JaamLogsManager(const JaamLogsManager&) = delete;
    JaamLogsManager& operator=(const JaamLogsManager&) = delete;

    friend class LoggingPrint;};

extern JaamLogsManager& logsManager;

// Create a global logging print instance
extern LoggingPrint loggingStream;

// Replace LOG macro with our logging wrapper
#define LOG loggingStream
