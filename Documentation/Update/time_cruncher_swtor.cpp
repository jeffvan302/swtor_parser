#include "time_cruncher_swtor.h"
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cstring>

namespace swtor {

// TimeCruncher implementation

TimeCruncher::TimeCruncher(
    std::shared_ptr<NTPTimeKeeper> ntp_keeper,
    bool enable_late_arrival_adjustment
) : ntp_keeper_(ntp_keeper),
    enable_late_arrival_adjustment_(enable_late_arrival_adjustment),
    initialized_(false),
    current_day_offset_(0),
    last_processed_combat_ms_(0),
    last_processed_epoch_ms_(0)
{
    stats_ = {0, 0, 0, 0, 0, 0};
}

TimeCruncher::~TimeCruncher() {
}

void TimeCruncher::reset() {
    initialized_ = false;
    current_day_offset_ = 0;
    last_processed_combat_ms_ = 0;
    last_processed_epoch_ms_ = 0;
    stats_ = {0, 0, 0, 0, 0, 0};
}

size_t TimeCruncher::processLines(std::vector<CombatLine>& lines) {
    size_t processed = 0;
    
    for (auto& line : lines) {
        if (processLine(line)) {
            processed++;
        }
    }
    
    return processed;
}

bool TimeCruncher::processLine(CombatLine& line) {
    uint32_t combat_ms = line.t.combat_ms;
    
    // Initialize base date if not yet done
    if (!initialized_) {
        initializeBaseDate(line);
    }
    
    // Check for midnight rollover
    // A time jump back of more than threshold indicates we crossed midnight
    if (initialized_ && combat_ms < last_processed_combat_ms_) {
        int64_t jump_back = last_processed_combat_ms_ - combat_ms;
        if (jump_back > MIDNIGHT_ROLLOVER_THRESHOLD_MS) {
            handleMidnightRollover();
            stats_.midnight_rollovers_detected++;
        }
    }
    
    // Reset base date on AreaEntered events (anchor points)
    if (isAreaEntered(line)) {
        // Use current NTP time as the new base
        auto ntp_now = ntp_keeper_->getNTPTime();
        base_date_ = ntp_now;
        
        // Reset day offset since we have a new anchor point
        current_day_offset_ = 0;
        
        stats_.area_entered_count++;
    }
    
    // Calculate the epoch time for this line
    line.t.refined_epoch_ms = calculateEpochMs(combat_ms);
    
    // Update tracking
    last_processed_combat_ms_ = combat_ms;
    last_processed_epoch_ms_ = line.t.refined_epoch_ms;
    stats_.total_lines_processed++;
    
    return true;
}

bool TimeCruncher::isAreaEntered(const CombatLine& line) const {
    // Check if this is an AreaEntered event
    // AreaEntered appears as the effect name in the event
    return std::string_view(line.evt.effect.name).find("AreaEntered") != std::string_view::npos;
}

void TimeCruncher::initializeBaseDate(const CombatLine& line) {
    // Use NTP time as the base
    auto ntp_now = ntp_keeper_->getNTPTime();
    
    // If this is an AreaEntered, use the current time directly
    // Otherwise, we'll infer the date based on the combat_ms
    if (isAreaEntered(line)) {
        base_date_ = ntp_now;
    } else {
        // Get current time components
        auto time_t_now = std::chrono::system_clock::to_time_t(ntp_now);
        std::tm tm_now;
        localtime_r(&time_t_now, &tm_now);
        
        // Set the base date to today at midnight
        tm_now.tm_hour = 0;
        tm_now.tm_min = 0;
        tm_now.tm_sec = 0;
        
        auto midnight = std::chrono::system_clock::from_time_t(std::mktime(&tm_now));
        base_date_ = midnight;
    }
    
    initialized_ = true;
    current_day_offset_ = 0;
}

void TimeCruncher::handleMidnightRollover() {
    // Add one day to the base date offset
    current_day_offset_++;
}

int64_t TimeCruncher::calculateEpochMs(uint32_t combat_ms) {
    // Convert combat_ms to epoch ms using current base date and day offset
    int64_t base_epoch_ms = combatMsToEpochMs(combat_ms);
    
    // Add day offset (in milliseconds)
    int64_t day_offset_ms = static_cast<int64_t>(current_day_offset_) * MS_PER_DAY;
    int64_t calculated_time = base_epoch_ms + day_offset_ms;
    
    // Adjust for late arrival if enabled
    if (enable_late_arrival_adjustment_) {
        calculated_time = adjustForLateArrival(calculated_time);
    }
    
    return calculated_time;
}

int64_t TimeCruncher::adjustForLateArrival(int64_t calculated_time) {
    // Get current NTP time
    int64_t current_ntp_ms = ntp_keeper_->getNTPTimeMs();
    
    // If the calculated time is in the future, it's not late
    if (calculated_time > current_ntp_ms) {
        return calculated_time;
    }
    
    // Calculate how late this entry is
    int64_t late_by_ms = current_ntp_ms - calculated_time;
    
    // If it's within reasonable bounds (up to MAX_LATE_ARRIVAL_MS)
    if (late_by_ms > 0 && late_by_ms <= MAX_LATE_ARRIVAL_MS) {
        stats_.total_late_arrival_adjustment_ms += late_by_ms;
        if (late_by_ms > stats_.max_late_arrival_ms) {
            stats_.max_late_arrival_ms = late_by_ms;
        }
        
        // The calculated time is accurate - the log entry just arrived late
        // We keep it as is, representing when the event actually occurred
    }
    
    return calculated_time;
}

int64_t TimeCruncher::combatMsToEpochMs(uint32_t combat_ms) const {
    // Convert base_date to time_t
    auto base_time_t = std::chrono::system_clock::to_time_t(base_date_);
    std::tm base_tm;
    localtime_r(&base_time_t, &base_tm);
    
    // combat_ms is milliseconds since midnight
    // Convert to hours, minutes, seconds, milliseconds
    uint32_t total_seconds = combat_ms / 1000;
    uint32_t ms_remainder = combat_ms % 1000;
    
    uint32_t hours = total_seconds / 3600;
    uint32_t minutes = (total_seconds % 3600) / 60;
    uint32_t seconds = total_seconds % 60;
    
    // Set the time components
    base_tm.tm_hour = hours;
    base_tm.tm_min = minutes;
    base_tm.tm_sec = seconds;
    
    // Convert back to time_point
    auto combined_time_t = std::mktime(&base_tm);
    auto combined_time = std::chrono::system_clock::from_time_t(combined_time_t);
    
    // Get epoch milliseconds
    auto epoch_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        combined_time.time_since_epoch()
    ).count();
    
    // Add the millisecond remainder
    epoch_ms += ms_remainder;
    
    return epoch_ms;
}

std::chrono::system_clock::time_point TimeCruncher::getBaseDate() const {
    return base_date_;
}

uint32_t TimeCruncher::getLastProcessedTime() const {
    return last_processed_combat_ms_;
}

TimeCruncher::Statistics TimeCruncher::getStatistics() const {
    return stats_;
}

// CombinedTime implementation

CombinedTime::CombinedTime(int64_t epoch_ms) : epoch_ms_(epoch_ms) {
}

CombinedTime CombinedTime::fromEpochMs(int64_t epoch_ms) {
    return CombinedTime(epoch_ms);
}

CombinedTime CombinedTime::fromCombatMs(
    uint32_t combat_ms,
    const std::chrono::system_clock::time_point& base_date
) {
    // Convert base_date to time_t
    auto base_time_t = std::chrono::system_clock::to_time_t(base_date);
    std::tm base_tm;
    localtime_r(&base_time_t, &base_tm);
    
    // combat_ms is milliseconds since midnight
    uint32_t total_seconds = combat_ms / 1000;
    uint32_t ms_remainder = combat_ms % 1000;
    
    uint32_t hours = total_seconds / 3600;
    uint32_t minutes = (total_seconds % 3600) / 60;
    uint32_t seconds = total_seconds % 60;
    
    // Set the time components
    base_tm.tm_hour = hours;
    base_tm.tm_min = minutes;
    base_tm.tm_sec = seconds;
    
    // Convert to epoch ms
    auto combined_time_t = std::mktime(&base_tm);
    auto combined_time = std::chrono::system_clock::from_time_t(combined_time_t);
    
    int64_t epoch_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        combined_time.time_since_epoch()
    ).count();
    
    epoch_ms += ms_remainder;
    
    return CombinedTime(epoch_ms);
}

int64_t CombinedTime::toEpochMs() const {
    return epoch_ms_;
}

std::chrono::system_clock::time_point CombinedTime::toTimePoint() const {
    return std::chrono::system_clock::time_point(std::chrono::milliseconds(epoch_ms_));
}

std::string CombinedTime::toISOString() const {
    auto tp = toTimePoint();
    auto time_t_val = std::chrono::system_clock::to_time_t(tp);
    std::tm tm_val;
    gmtime_r(&time_t_val, &tm_val);
    
    // Calculate milliseconds
    auto ms = epoch_ms_ % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(&tm_val, "%Y-%m-%dT%H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms;
    oss << 'Z';
    
    return oss.str();
}

std::string CombinedTime::toString(const std::string& format) const {
    auto tp = toTimePoint();
    auto time_t_val = std::chrono::system_clock::to_time_t(tp);
    std::tm tm_val;
    localtime_r(&time_t_val, &tm_val);
    
    std::ostringstream oss;
    oss << std::put_time(&tm_val, format.c_str());
    
    return oss.str();
}

} // namespace swtor
