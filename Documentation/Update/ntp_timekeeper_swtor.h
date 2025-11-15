#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>

namespace swtor {

/**
 * @brief NTP packet structure as defined in RFC 5905
 */
struct NTPPacket {
    uint8_t li_vn_mode;
    uint8_t stratum;
    uint8_t poll;
    uint8_t precision;
    uint32_t root_delay;
    uint32_t root_dispersion;
    uint32_t ref_id;
    uint32_t ref_timestamp_sec;
    uint32_t ref_timestamp_frac;
    uint32_t orig_timestamp_sec;
    uint32_t orig_timestamp_frac;
    uint32_t recv_timestamp_sec;
    uint32_t recv_timestamp_frac;
    uint32_t trans_timestamp_sec;
    uint32_t trans_timestamp_frac;
};

/**
 * @brief Result of an NTP query
 */
struct NTPResult {
    bool success{ false };
    int64_t offset_ms{ 0 };
    int64_t round_trip_ms{ 0 };
    std::chrono::system_clock::time_point query_time{};
    std::string server{};
    std::string error_message{};
};

/**
 * @class NTPTimeKeeper
 * @brief Manages NTP time synchronization and calculates offset between local and NTP time
 * 
 * This class queries NTP servers to determine the difference between the local system clock
 * and NTP time. The result is cached and can be queried multiple times without re-querying.
 */
class NTPTimeKeeper {
public:
    static const std::vector<std::string> DEFAULT_NTP_SERVERS;

    explicit NTPTimeKeeper(
        const std::vector<std::string>& servers = {},
        int timeout_ms = 5000
    );
    ~NTPTimeKeeper();

    // Synchronize with NTP servers
    bool synchronize(bool force = false);

    // Get the offset between local time and NTP time in milliseconds
    // (positive means local is ahead of NTP)
    int64_t getOffsetMs() const;

    // Check if synchronization has been performed successfully
    bool isSynchronized() const;

    // Get the current NTP time
    std::chrono::system_clock::time_point getNTPTime() const;
    int64_t getNTPTimeMs() const;

    // Get the last synchronization result
    NTPResult getLastResult() const;

    // Convert between local and NTP time
    std::chrono::system_clock::time_point convertToNTP(
        const std::chrono::system_clock::time_point& local_time
    ) const;
    
    std::chrono::system_clock::time_point convertToLocal(
        const std::chrono::system_clock::time_point& ntp_time
    ) const;

private:
    NTPResult queryNTPServer(const std::string& server);
    NTPPacket createNTPPacket();
    NTPResult parseNTPResponse(
        const NTPPacket& packet,
        std::chrono::system_clock::time_point t1,
        std::chrono::system_clock::time_point t4
    );
    
    static std::chrono::system_clock::time_point ntpToTimePoint(
        uint32_t seconds,
        uint32_t fraction
    );

    std::vector<std::string> servers_;
    int timeout_ms_;
    
    mutable std::mutex mutex_;
    bool synchronized_;
    int64_t offset_ms_;
    NTPResult last_result_;
};

} // namespace swtor
