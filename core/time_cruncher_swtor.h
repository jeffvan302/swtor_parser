#pragma once
#include "swtor_parser.h"
#include "ntp_timekeeper_swtor.h"
#include <memory>
#include <chrono>
#include <cstdint>



namespace swtor {

/**
 * @class TimeCruncher
 * @brief Processes CombatLine objects and assigns refined_epoch_ms timestamps
 * 
 * This class handles the complexities of SWTOR combat logs:
 * - Log entries only have time (combat_ms), not date
 * - Log entries are in server time (NTP synchronized) adjusted to local timezone
 * - Entries can arrive up to 3 seconds late
 * - Midnight rollovers need to be detected and handled
 * - AreaEntered events serve as anchor points for date inference
 * 
 * The class maintains state across multiple calls to handle streaming log data.
 */
class TimeCruncher {
public:
    /**
     * @brief Constructor
     * @param ntp_keeper Shared pointer to NTPTimeKeeper for time synchronization
     * @param enable_late_arrival_adjustment If true, adjust for entries arriving late
     */
    explicit TimeCruncher(
        std::shared_ptr<NTPTimeKeeper> ntp_keeper,
        bool enable_late_arrival_adjustment = true
    );

    /**
     * @brief Constructor with raw pointer
     * @param ntp_keeper Raw pointer to NTPTimeKeeper for time synchronization
     * @param enable_late_arrival_adjustment If true, adjust for entries arriving late
     */
    explicit TimeCruncher(
        NTPTimeKeeper *ntp_keeper,
        bool enable_late_arrival_adjustment = true
    );

    /**
     * @brief Destructor
     */
    ~TimeCruncher();

    /**
     * @brief Process a batch of combat lines and assign refined_epoch_ms
     * @param lines Vector of CombatLine objects to process
     * @return Number of lines successfully processed
     * 
     * This method processes lines sequentially, maintaining state across calls.
     * Lines should be provided in chronological order.
     */
    size_t processLines(std::vector<CombatLine>& lines);

    /**
     * @brief Process a single combat line
     * @param line CombatLine to process
     * @return True if processing was successful
     * 
     * This is useful for streaming log data one line at a time.
     * Sets line.t.refined_epoch_ms to the calculated epoch timestamp.
     */
    bool processLine(CombatLine& line);

    /**
     * @brief Get the current base date being used
     * @return Base date as system_clock time_point
     */
    std::chrono::system_clock::time_point getBaseDate() const;

    /**
     * @brief Get the last processed time
     * @return Last processed combat_ms value
     */
    uint32_t getLastProcessedTime() const;

    /**
     * @brief Get statistics about processing
     */
    struct Statistics {
        /// <summary>
        /// Total number of lines processed
        /// </summary>
        size_t total_lines_processed{ 0 };
        /// <summary>
        /// Number of AreaEntered events processed
        /// </summary>
        size_t area_entered_count{ 0 };
        /// <summary>
        /// Number of midnight rollovers detected
        /// </summary>
        size_t midnight_rollovers_detected{ 0 };
        /// <summary>
        /// Number of time jumps detected
        /// </summary>
        size_t time_jumps_detected{ 0 };
        /// <summary>
        /// Total late arrival adjustment in milliseconds
        /// </summary>
        int64_t total_late_arrival_adjustment_ms{ 0 };
        /// <summary>
        /// Maximum late arrival detected in milliseconds
        /// </summary>
        int64_t max_late_arrival_ms{ 0 };
    };

    /// <summary>
    /// Get statistics about processing
    /// </summary>
    /// <returns>Statistics structure</returns>
    Statistics getStatistics() const;

private:
    /**
     * @brief Check if this line is an AreaEntered event
     */
    bool isAreaEntered(const CombatLine& line) const;

    /// <summary>
    /// Set the base date from a combat line
    /// </summary>
    /// <param name="line">Combat line to extract date from</param>
    /// <returns>True if base date was set successfully</returns>
    bool set_Base_Date(const CombatLine& line);

    /**
     * @brief Initialize the base date from the first AreaEntered or current time
     * @param line CombatLine to use for initialization
     */
    void initializeBaseDate(const CombatLine& line);


    /// <summary>
    /// Convert time point to milliseconds since epoch
    /// </summary>
    /// <param name="tp">Time point to convert</param>
    /// <returns>Milliseconds since epoch</returns>
    inline int64_t from_time_point(const std::chrono::system_clock::time_point& tp) const {
        using namespace std::chrono;
        // Convert time_point to refined_epoch_ms
        return duration_cast<milliseconds>(tp.time_since_epoch()).count();
    }

    /// <summary>
    /// Convert milliseconds since epoch to time point
    /// </summary>
    /// <param name="ms_since_epoch">Milliseconds since epoch</param>
    /// <returns>Time point</returns>
    inline std::chrono::system_clock::time_point to_time_point(int64_t ms_since_epoch) const {
        using namespace std::chrono;
        // Convert refined_epoch_ms to time_point
        return system_clock::time_point(milliseconds(ms_since_epoch));
    }

    /// <summary>
    /// Calculate epoch milliseconds from combat milliseconds
    /// </summary>
    /// <param name="combat_ms">Combat milliseconds from log</param>
    /// <returns>Epoch milliseconds</returns>
    inline int64_t calculateEpochMs(uint32_t combat_ms) const {
        return base_date_epoch_ms_ + static_cast<int64_t>(combat_ms);
	}

    /**
     * @brief Update base date when detecting midnight rollover
     */
    void handleMidnightRollover();

    
    /// <summary>
    /// Get the threshold for detecting proximity to midnight
    /// </summary>
    /// <returns>Threshold in milliseconds</returns>
    inline int64_t CloseToMidnightThreshold() const {
        return MS_PER_DAY - MIDNIGHT_ROLLOVER_THRESHOLD_MS;
	}

    /// <summary>
    /// Shared pointer to NTP time keeper
    /// </summary>
    std::shared_ptr<NTPTimeKeeper> ntp_keeper_;
    /// <summary>
    /// Whether to adjust for late-arriving entries
    /// </summary>
    bool enable_late_arrival_adjustment_;
    
    /// <summary>
    /// Whether the time cruncher has been initialized
    /// </summary>
    bool initialized_;
    /// <summary>
    /// Whether we are close to midnight
    /// </summary>
    bool midnight_close_;
    /// <summary>
    /// Base date for current processing session
    /// </summary>
    std::chrono::system_clock::time_point base_date_;
	/// <summary>
	/// Base date in epoch milliseconds
	/// </summary>
	int64_t base_date_epoch_ms_;
    /// <summary>
    /// Number of days added due to midnight rollovers
    /// </summary>
    int current_day_offset_;
    /// <summary>
    /// Last processed combat_ms value
    /// </summary>
    uint32_t last_processed_combat_ms_;
    /// <summary>
    /// Last processed epoch_ms value
    /// </summary>
    int64_t last_processed_epoch_ms_;
    
    /// <summary>
    /// Processing statistics
    /// </summary>
    Statistics stats_;
    
    /// <summary>
    /// Threshold for detecting midnight rollover (1 minute)
    /// </summary>
    static constexpr int64_t MIDNIGHT_ROLLOVER_THRESHOLD_MS = 60000;
    /// <summary>
    /// Maximum allowed late arrival time (5 seconds)
    /// </summary>
    static constexpr int64_t MAX_LATE_ARRIVAL_MS = 5000;
    /// <summary>
    /// Milliseconds per day
    /// </summary>
    static constexpr int64_t MS_PER_DAY = 86400000;
};

} // namespace swtor
