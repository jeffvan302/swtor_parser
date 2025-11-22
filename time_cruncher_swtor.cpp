#include "time_cruncher_swtor.h"
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cstring>

namespace swtor {

// Windows-compatible time functions
#ifdef _WIN32
    #define LOCALTIME_SAFE(tm_ptr, time_ptr) localtime_s(tm_ptr, time_ptr)
    #define GMTIME_SAFE(tm_ptr, time_ptr) gmtime_s(tm_ptr, time_ptr)
#else
    #define LOCALTIME_SAFE(tm_ptr, time_ptr) localtime_r(time_ptr, tm_ptr)
    #define GMTIME_SAFE(tm_ptr, time_ptr) gmtime_r(time_ptr, tm_ptr)
#endif

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

TimeCruncher::TimeCruncher(
    NTPTimeKeeper * ntp_keeper,
    bool enable_late_arrival_adjustment
) : ntp_keeper_(ntp_keeper),
enable_late_arrival_adjustment_(enable_late_arrival_adjustment),
initialized_(false),
current_day_offset_(0),
last_processed_combat_ms_(0),
last_processed_epoch_ms_(0)
{
    stats_ = { 0, 0, 0, 0, 0, 0 };
    priority = -10;
}

std::string TimeCruncher::name() const {
    return "Time Cruncher";
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


void TimeCruncher::ingest(CombatLine& line) {
    processLine(line);
}


bool TimeCruncher::processLine(CombatLine& line) {
    uint32_t combat_ms = line.t.combat_ms;
    
    // Initialize base date if not yet done
    initializeBaseDate(line);
   

    // Check for midnight rollover
    // A time jump back of more than threshold indicates we crossed midnight
    if (initialized_ && combat_ms < (MIDNIGHT_ROLLOVER_THRESHOLD_MS * 2) && midnight_close_) {
		int64_t jump_forward_ms = MS_PER_DAY + combat_ms;
        line.t.refined_epoch_ms = calculateEpochMs(jump_forward_ms);
    }
    else {
        line.t.refined_epoch_ms = calculateEpochMs(combat_ms);
    }

    if (combat_ms < last_processed_combat_ms_) {
        // Time jump detected
        stats_.time_jumps_detected++;
	}
    
    // Reset base date on AreaEntered events (anchor points)
    
    
    // Calculate the epoch time for this line
    
    
    // Update tracking
    last_processed_combat_ms_ = combat_ms;
    if (combat_ms > CloseToMidnightThreshold()) {
		midnight_close_ = true;
	}
    else if (midnight_close_ && combat_ms > (MIDNIGHT_ROLLOVER_THRESHOLD_MS/2) && combat_ms < CloseToMidnightThreshold()) {
		midnight_close_ = false;
        handleMidnightRollover();
    }
    last_processed_epoch_ms_ = line.t.refined_epoch_ms;
    stats_.total_lines_processed++;
    
    return true;
}

bool TimeCruncher::isAreaEntered(const CombatLine& line) const {
    // Check if this is an AreaEntered event
    // AreaEntered appears as the effect name in the event
    return (line == swtor::EventType::AreaEntered);
}

bool TimeCruncher::set_Base_Date(const CombatLine& line) {
    auto ntp_now = ntp_keeper_->getLocalTime();
    std::chrono::system_clock::time_point zero_hour = ntp_keeper_->getZeroHour(ntp_now);
    std::chrono::system_clock::time_point line_time = ntp_keeper_->adjust_time(zero_hour, line.t.combat_ms);
    while (line_time > ntp_now) {
        // If the line time is in the future, roll back one day
        zero_hour = ntp_keeper_->adjust_time(zero_hour, -1, 0, 0, 0, 0);
        line_time = ntp_keeper_->adjust_time(zero_hour, line.t.combat_ms);
    }
    midnight_close_ = false;
    base_date_ = zero_hour;
	base_date_epoch_ms_ = from_time_point(base_date_);
    current_day_offset_ = 0;
    initialized_ = true;
	return true;
}

void TimeCruncher::initializeBaseDate(const CombatLine& line) {
    // Use NTP time as the base
    // If this is an AreaEntered, use the current time directly
    // Otherwise, we'll infer the date based on the combat_ms
    if (isAreaEntered(line)) {
		set_Base_Date(line);
        stats_.area_entered_count++;
    } else {
        if (!initialized_) {
            set_Base_Date(line);
        }
    }
    
    
}

void TimeCruncher::handleMidnightRollover() {
    // Add one day to the base date offset
    current_day_offset_++;
    stats_.midnight_rollovers_detected++;
	base_date_ = ntp_keeper_->adjust_time(base_date_, 1, 0, 0, 0, 0);
    base_date_epoch_ms_ = from_time_point(base_date_);
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


} // namespace swtor
