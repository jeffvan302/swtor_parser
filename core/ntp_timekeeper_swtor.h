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
    /// <summary>
    /// Leap indicator, version number, and mode
    /// </summary>
    uint8_t li_vn_mode;
    /// <summary>
    /// Stratum level of the server
    /// </summary>
    uint8_t stratum;
    /// <summary>
    /// Poll interval
    /// </summary>
    uint8_t poll;
    /// <summary>
    /// Precision of the system clock
    /// </summary>
    uint8_t precision;
    /// <summary>
    /// Root delay
    /// </summary>
    uint32_t root_delay;
    /// <summary>
    /// Root dispersion
    /// </summary>
    uint32_t root_dispersion;
    /// <summary>
    /// Reference identifier
    /// </summary>
    uint32_t ref_id;
    /// <summary>
    /// Reference timestamp seconds
    /// </summary>
    uint32_t ref_timestamp_sec;
    /// <summary>
    /// Reference timestamp fraction
    /// </summary>
    uint32_t ref_timestamp_frac;
    /// <summary>
    /// Origin timestamp seconds
    /// </summary>
    uint32_t orig_timestamp_sec;
    /// <summary>
    /// Origin timestamp fraction
    /// </summary>
    uint32_t orig_timestamp_frac;
    /// <summary>
    /// Receive timestamp seconds
    /// </summary>
    uint32_t recv_timestamp_sec;
    /// <summary>
    /// Receive timestamp fraction
    /// </summary>
    uint32_t recv_timestamp_frac;
    /// <summary>
    /// Transmit timestamp seconds
    /// </summary>
    uint32_t trans_timestamp_sec;
    /// <summary>
    /// Transmit timestamp fraction
    /// </summary>
    uint32_t trans_timestamp_frac;
};

/**
 * @brief Result of an NTP query
 */
struct NTPResult {
    /// <summary>
    /// True if query was successful
    /// </summary>
    bool success{ false };
    /// <summary>
    /// Time offset in milliseconds
    /// </summary>
    int64_t offset_ms{ 0 };
    /// <summary>
    /// Round trip time in milliseconds
    /// </summary>
    int64_t round_trip_ms{ 0 };
    /// <summary>
    /// Time point when query was made
    /// </summary>
    std::chrono::system_clock::time_point query_time{};
    /// <summary>
    /// NTP server queried
    /// </summary>
    std::string server{};
    /// <summary>
    /// Error message if query failed
    /// </summary>
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
    /// <summary>
    /// Default NTP servers to query
    /// </summary>
    static const std::vector<std::string> DEFAULT_NTP_SERVERS;

    /// <summary>
    /// Constructor
    /// </summary>
    /// <param name="servers">List of NTP servers to use (uses defaults if empty)</param>
    /// <param name="timeout_ms">Timeout in milliseconds for NTP queries (default 5000)</param>
    explicit NTPTimeKeeper(
        const std::vector<std::string>& servers = {},
        int timeout_ms = 5000
    );
    /// <summary>
    /// Destructor
    /// </summary>
    ~NTPTimeKeeper();

    /// <summary>
    /// Synchronize with NTP servers
    /// </summary>
    /// <param name="force">Force synchronization even if already synchronized</param>
    /// <returns>True if synchronization was successful</returns>
    bool synchronize(bool force = false);

    /// <summary>
    /// Get the offset between local time and NTP time in milliseconds
    /// </summary>
    /// <returns>Offset in milliseconds</returns>
    int64_t getOffsetMs() const;

    /// <summary>
    /// Get the UTC offset to local time in milliseconds
    /// </summary>
    /// <returns>UTC offset in milliseconds</returns>
    int64_t utc_offset_to_local_ms();

    /// <summary>
    /// Check if synchronization has been performed successfully
    /// </summary>
    /// <returns>True if synchronized</returns>
    bool isSynchronized() const;

    /// <summary>
    /// Set the local time zone offset
    /// </summary>
    /// <param name="val">Offset value in milliseconds</param>
    void set_local_offset(int64_t val);
    /// <summary>
    /// Get the local time zone offset
    /// </summary>
    /// <returns>Offset in milliseconds</returns>
    int64_t get_local_offset() const;

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
    /// <param name="input">Input time point</param>
    /// <param name="ms">Milliseconds to add</param>
    /// <returns>Adjusted time point</returns>
    std::chrono::system_clock::time_point adjust_time(std::chrono::system_clock::time_point input, int64_t ms);

    /// <summary>
    /// Adjust time by adding values to input time.
    /// </summary>
    /// <param name="input">Input time point</param>
    /// <param name="days">Days to add</param>
    /// <param name="hours">Hours to add</param>
    /// <param name="min">Minutes to add</param>
    /// <param name="sec">Seconds to add</param>
    /// <param name="ms">Milliseconds to add</param>
    /// <returns>Adjusted time point</returns>
    std::chrono::system_clock::time_point adjust_time(std::chrono::system_clock::time_point input, int64_t days, int64_t hours, int64_t min, int64_t sec, int64_t ms);

    /// <summary>
    /// Returns the NTP Time in GMT time.
    /// </summary>
    /// <returns>NTP time as time point</returns>
    std::chrono::system_clock::time_point getNTPTime() const;
    /// <summary>
    /// Get NTP time in milliseconds since epoch
    /// </summary>
    /// <returns>NTP time in milliseconds</returns>
    int64_t getNTPTimeMs() const;


    /// <summary>
	/// Get the current local time in refined_epoch_ms
    /// </summary>
    /// <returns>Local time in milliseconds since epoch</returns>
    inline int64_t get_LocalTime_in_epoch_ms() const {
		return time_point_to_epoch_ms(getLocalTime());
	}

    /// <summary>
	/// Convert time_point to refined_epoch_ms
    /// </summary>
    /// <param name="tp">Time point to convert</param>
    /// <returns>Milliseconds since epoch</returns>
    inline int64_t time_point_to_epoch_ms(const std::chrono::system_clock::time_point& tp) const {
        using namespace std::chrono;
        // Convert time_point to refined_epoch_ms
        return duration_cast<milliseconds>(tp.time_since_epoch()).count();
    }

    /// <summary>
	/// Convert refined_epoch_ms to time_point
    /// </summary>
    /// <param name="ms_since_epoch">Milliseconds since epoch</param>
    /// <returns>Time point</returns>
    inline std::chrono::system_clock::time_point epoch_ms_to_time_point(int64_t ms_since_epoch) const {
        using namespace std::chrono;
        // Convert refined_epoch_ms to time_point
        return system_clock::time_point(milliseconds(ms_since_epoch));
    }

    /// <summary>
    /// Get the last synchronization result
    /// </summary>
    /// <returns>NTP result structure</returns>
    NTPResult getLastResult() const;

    /// <summary>
    /// Convert local time to NTP time
    /// </summary>
    /// <param name="local_time">Local time point</param>
    /// <returns>NTP time point</returns>
    std::chrono::system_clock::time_point convertToNTP(
        const std::chrono::system_clock::time_point& local_time
    ) const;
    
    /// <summary>
    /// Convert NTP time to local time
    /// </summary>
    /// <param name="ntp_time">NTP time point</param>
    /// <returns>Local time point</returns>
    std::chrono::system_clock::time_point convertToLocal(
        const std::chrono::system_clock::time_point& ntp_time
    ) const;

private:
    /// <summary>
    /// Query a specific NTP server
    /// </summary>
    /// <param name="server">Server address</param>
    /// <returns>NTP result</returns>
    NTPResult queryNTPServer(const std::string& server);
    /// <summary>
    /// Create an NTP packet for querying
    /// </summary>
    /// <returns>NTP packet</returns>
    NTPPacket createNTPPacket();
    /// <summary>
    /// Parse NTP server response
    /// </summary>
    /// <param name="packet">Received NTP packet</param>
    /// <param name="t1">Client send time</param>
    /// <param name="t4">Client receive time</param>
    /// <returns>Parsed NTP result</returns>
    NTPResult parseNTPResponse(
        const NTPPacket& packet,
        std::chrono::system_clock::time_point t1,
        std::chrono::system_clock::time_point t4
    );
    
    /// <summary>
    /// Convert NTP timestamp to system clock time point
    /// </summary>
    /// <param name="seconds">NTP timestamp seconds</param>
    /// <param name="fraction">NTP timestamp fraction</param>
    /// <returns>System clock time point</returns>
    static std::chrono::system_clock::time_point ntpToTimePoint(
        uint32_t seconds,
        uint32_t fraction
    );

    /// <summary>
    /// List of NTP servers to query
    /// </summary>
    std::vector<std::string> servers_;
    /// <summary>
    /// Timeout for NTP queries in milliseconds
    /// </summary>
    int timeout_ms_;
    
    /// <summary>
    /// Mutex for thread safety
    /// </summary>
    mutable std::mutex mutex_;
    /// <summary>
    /// Secondary mutex for thread safety
    /// </summary>
    mutable std::mutex mutex2_;
    /// <summary>
    /// Whether synchronization has been performed
    /// </summary>
    bool synchronized_;
    /// <summary>
    /// Calculated time offset in milliseconds
    /// </summary>
    int64_t offset_ms_;
    /// <summary>
    /// Time zone offset in milliseconds
    /// </summary>
    int64_t zone_offset_ms_;
    /// <summary>
    /// Last synchronization result
    /// </summary>
    NTPResult last_result_;
    
    /// <summary>
    /// Windows-specific: track if WSA is initialized
    /// </summary>
    static bool wsa_initialized_;
    /// <summary>
    /// Mutex for WSA initialization
    /// </summary>
    static std::mutex wsa_mutex_;
};

} // namespace swtor
