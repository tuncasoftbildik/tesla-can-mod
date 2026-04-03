#pragma once

#include <cstdint>
#include <cstring>

struct LogEntry
{
    uint32_t timestamp;
    char message[80];
};

class LogBuffer
{
public:
    static constexpr int MAX_ENTRIES = 50;

    void add(const char *msg)
    {
        entries_[head_].timestamp = millis();
        strncpy(entries_[head_].message, msg, sizeof(entries_[head_].message) - 1);
        entries_[head_].message[sizeof(entries_[head_].message) - 1] = '\0';
        head_ = (head_ + 1) % MAX_ENTRIES;
        if (count_ < MAX_ENTRIES)
            count_++;
    }

    int count() const { return count_; }
    int head() const { return head_; }

    const LogEntry &get(int index) const
    {
        int actual = (head_ - count_ + index + MAX_ENTRIES) % MAX_ENTRIES;
        return entries_[actual];
    }

private:
    LogEntry entries_[MAX_ENTRIES] = {};
    int head_ = 0;
    int count_ = 0;
};

inline LogBuffer globalLog;
