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
 * Windows-compatible version using Winsock2
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
    int64_t getOffsetMs() const;

    int64_t utc_offset_to_local_ms();

    // Check if synchronization has been performed successfully
    bool isSynchronized() const;

    void set_local_offset(int64_t val);
    int64_t get_local_offset() const;

	// Get the current local time adjusted by NTP offset
    ///std::chrono::local_time getLocalTime() const;
    
    /// <summary>
    /// Returns the midnight time of the input
    /// </summary>
    /// <param name="input">date value</param>
    /// <returns>zero hour of the input date</returns>
    std::chrono::system_clock::time_point getZeroHour(std::chrono::system_clock::time_point input);

    /// <summary>
    /// Returns the local time adjusted for time zone.  Dangerous since it already includes time zone in an assumed system time.  Makes future calculations faster.
    /// </summary>
    /// <returns></returns>
    std::chrono::system_clock::time_point getLocalTime() const;

    /// <summary>
    /// Adjust time by adding ms to input time.
    /// </summary>
    /// <param name="input"></param>
    /// <param name="ms"></param>
    /// <returns></returns>
    std::chrono::system_clock::time_point adjust_time(std::chrono::system_clock::time_point input, int64_t ms);

    /// <summary>
    /// Adjust time by adding values to input time.
    /// </summary>
    /// <returns></returns>
    std::chrono::system_clock::time_point adjust_time(std::chrono::system_clock::time_point input, int64_t days, int64_t hours, int64_t min, int64_t sec, int64_t ms);

    /// <summary>
    /// Returns the NTP Time in GMT time.
    /// </summary>
    /// <returns></returns>
    std::chrono::system_clock::time_point getNTPTime() const;
    int64_t getNTPTimeMs() const;


    /// <summary>
	/// Get the current local time in refined_epoch_ms
    /// </summary>
    /// <returns></returns>
    inline int64_t get_LocalTime_in_epoch_ms() const {
		return time_point_to_epoch_ms(getLocalTime());
	}

    /// <summary>
	/// Convert time_point to refined_epoch_ms
    /// </summary>
    /// <param name="tp"></param>
    /// <returns></returns>
    inline int64_t time_point_to_epoch_ms(const std::chrono::system_clock::time_point& tp) const {
        using namespace std::chrono;
        // Convert time_point to refined_epoch_ms
        return duration_cast<milliseconds>(tp.time_since_epoch()).count();
    }

    /// <summary>
	/// Convert refined_epoch_ms to time_point
    /// </summary>
    /// <param name="ms_since_epoch"></param>
    /// <returns></returns>
    inline std::chrono::system_clock::time_point epoch_ms_to_time_point(int64_t ms_since_epoch) const {
        using namespace std::chrono;
        // Convert refined_epoch_ms to time_point
        return system_clock::time_point(milliseconds(ms_since_epoch));
    }

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
    mutable std::mutex mutex2_;
    bool synchronized_;
    int64_t offset_ms_;
    int64_t zone_offset_ms_;
    NTPResult last_result_;
    
    // Windows-specific: track if WSA is initialized
    static bool wsa_initialized_;
    static std::mutex wsa_mutex_;
};

} // namespace swtor
