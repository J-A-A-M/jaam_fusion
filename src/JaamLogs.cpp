#include "JaamLogs.h"
#include <ArduinoJson.h>

#if TELNET_ENABLED
TelnetSpy SerialAndTelnet;
#endif

// Global instances
JaamLogsManager& logsManager = JaamLogsManager::getInstance();
LoggingPrint loggingStream(_LOG_BASE);

// LoggingPrint::write implementation - intercept all output
size_t LoggingPrint::write(const uint8_t* buffer, size_t size) {
    // if (!logsEnabled) {
    //     linePos = 0;
    //     return size;
    // }

    if (baseStream && logsEnabled) {
        baseStream->write(buffer, size);
    }

    // Process characters for log buffer
    for (size_t i = 0; i < size; i++) {
        uint8_t c = buffer[i];
        
        if (c == '\n') {
            // End of line - process the buffer
            if (linePos > 0) {
                lineBuffer[linePos] = '\0';
                processLine(lineBuffer);
                linePos = 0;
            }
        } else if (c != '\r') {
            // Add character to line buffer (skip carriage returns)
            if (linePos < sizeof(lineBuffer) - 1) {
                lineBuffer[linePos++] = c;
            }
        }
    }
    
    return size;
}

void LoggingPrint::processLine(const char* line) {
    if (!logsManager || line[0] == '\0') return;
    
    // Skip leading whitespace
    const char* trimmedLine = line;
    while (*trimmedLine == ' ' || *trimmedLine == '\t') {
        trimmedLine++;
    }
    
    // Skip empty lines
    if (*trimmedLine == '\0') return;
    
    // Try to extract tag from format [TAG] message
    const char* tagStart = nullptr;
    const char* tagEnd = nullptr;
    const char* messageStart = trimmedLine;
    
    if (trimmedLine[0] == '[') {
        // Find tag between [ and ]
        tagStart = trimmedLine + 1;
        tagEnd = strchr(tagStart, ']');
        
        if (tagEnd) {
            messageStart = tagEnd + 1;
            // Skip space after ]
            if (*messageStart == ' ') messageStart++;
            
            // Extract tag
            size_t tagLen = tagEnd - tagStart;
            if (tagLen > 0 && tagLen <= 15) {
                char tag[16];
                strncpy(tag, tagStart, tagLen);
                tag[tagLen] = '\0';
                
                logsManager->addLog(tag, messageStart, 1);
                return;
            }
        }
    }
    
    // If no tag found, use default
    logsManager->addLog("LOG", trimmedLine, 1);
}

void JaamLogsManager::begin() {
    std::lock_guard<std::mutex> lock(logMutex);
    
    if (!loggingPrint) {
        loggingPrint = &loggingStream;
        loggingPrint->setLogsManager(this);
    }
}

void JaamLogsManager::addLog(const char* tag, const char* message, uint8_t level) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    LogEntry& entry = logBuffer[writeIndex];
    entry.timestamp = time(nullptr);  // NTP-synced system time
    entry.level = level;
    
    // Extract tag safely
    if (tag) {
        strncpy(entry.tag, tag, sizeof(entry.tag) - 1);
        entry.tag[sizeof(entry.tag) - 1] = '\0';
    } else {
        strcpy(entry.tag, "LOG");
    }
    
    // Extract message safely and remove control characters
    if (message) {
        int msgIdx = 0;
        for (int i = 0; message[i] && msgIdx < (int)sizeof(entry.message) - 1; i++) {
            unsigned char c = message[i];
            // Skip control characters and newlines/carriage returns
            if (c >= 32 && c < 127) {  // Only ASCII printable characters
                entry.message[msgIdx++] = c;
            } else if (c == '\t' || c == ' ') {  // Allow tabs and spaces
                entry.message[msgIdx++] = c;
            }
            // Skip other control characters
        }
        entry.message[msgIdx] = '\0';
    } else {
        strcpy(entry.message, "");
    }
    
    // Move to next position in circular buffer
    writeIndex = (writeIndex + 1) % MAX_LOGS;
    if (logCount < MAX_LOGS) {
        logCount++;
    }
}

String JaamLogsManager::getLogsJson(int limit) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    JsonDocument doc;
    JsonArray logs = doc["logs"].to<JsonArray>();
    
    if (logCount == 0) {
        String response;
        serializeJson(doc, response);
        return response;
    }
    
    // Validate limit
    if (limit <= 0) limit = 100;
    if (limit > logCount) limit = logCount;
    
    // Determine starting index in circular buffer
    // If buffer not full: oldest log is at index 0
    // If buffer full: oldest log is at writeIndex (next position to overwrite)
    int oldestIdx = (logCount < MAX_LOGS) ? 0 : writeIndex;
    int logsToSkip = logCount - limit;
    
    // Iterate through logs from oldest to newest
    for (int i = 0; i < limit; i++) {
        // Simple circular iteration from oldest
        int bufferIdx = (oldestIdx + logsToSkip + i) % MAX_LOGS;
        const LogEntry& entry = logBuffer[bufferIdx];
        
        JsonObject logObj = logs.add<JsonObject>();
        logObj["timestamp"] = entry.timestamp;
        logObj["tag"] = entry.tag;
        logObj["message"] = entry.message;
        logObj["level"] = entry.level;
    }
    
    String response;
    serializeJson(doc, response);
    return response;
}

void JaamLogsManager::clearLogs() {
    std::lock_guard<std::mutex> lock(logMutex);
    writeIndex = 0;
    logCount = 0;
    memset(logBuffer, 0, sizeof(logBuffer));
}


