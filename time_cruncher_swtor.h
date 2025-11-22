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
class TimeCruncher: parse_plugin {
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

    explicit TimeCruncher(
        NTPTimeKeeper *ntp_keeper,
        bool enable_late_arrival_adjustment = true
    );

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

	void ingest(CombatLine& line) override;
    
    std::string name() const override;

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
     * @brief Reset the internal state
     * 
     * Call this when starting to process a new log file or when you want
     * to reset the date tracking state.
     */
    void reset() override;

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
        size_t total_lines_processed{ 0 };
        size_t area_entered_count{ 0 };
        size_t midnight_rollovers_detected{ 0 };
        size_t time_jumps_detected{ 0 };
        int64_t total_late_arrival_adjustment_ms{ 0 };
        int64_t max_late_arrival_ms{ 0 };
    };

    Statistics getStatistics() const;

private:
    /**
     * @brief Check if this line is an AreaEntered event
     */
    bool isAreaEntered(const CombatLine& line) const;

    bool set_Base_Date(const CombatLine& line);

    /**
     * @brief Initialize the base date from the first AreaEntered or current time
     * @param line CombatLine to use for initialization
     */
    void initializeBaseDate(const CombatLine& line);


    inline int64_t from_time_point(const std::chrono::system_clock::time_point& tp) const {
        using namespace std::chrono;
        // Convert time_point to refined_epoch_ms
        return duration_cast<milliseconds>(tp.time_since_epoch()).count();
    }

    inline std::chrono::system_clock::time_point to_time_point(int64_t ms_since_epoch) const {
        using namespace std::chrono;
        // Convert refined_epoch_ms to time_point
        return system_clock::time_point(milliseconds(ms_since_epoch));
    }

    inline int64_t calculateEpochMs(uint32_t combat_ms) const {
        return base_date_epoch_ms_ + static_cast<int64_t>(combat_ms);
	}

    /**
     * @brief Update base date when detecting midnight rollover
     */
    void handleMidnightRollover();

    
    inline int64_t CloseToMidnightThreshold() const {
        return MS_PER_DAY - MIDNIGHT_ROLLOVER_THRESHOLD_MS;
	}

    std::shared_ptr<NTPTimeKeeper> ntp_keeper_;
    bool enable_late_arrival_adjustment_;
    
    // State tracking
    bool initialized_;
    bool midnight_close_;
    std::chrono::system_clock::time_point base_date_;
	int64_t base_date_epoch_ms_;
    int current_day_offset_;  // Days added due to midnight rollovers
    uint32_t last_processed_combat_ms_;
    int64_t last_processed_epoch_ms_;
    
    // Statistics
    Statistics stats_;
    
    // Configuration
    static constexpr int64_t MIDNIGHT_ROLLOVER_THRESHOLD_MS = 60000; // 1 minute
    static constexpr int64_t MAX_LATE_ARRIVAL_MS = 5000; // 5 seconds max
    static constexpr int64_t MS_PER_DAY = 86400000; // 24 * 60 * 60 * 1000
};

} // namespace swtor
