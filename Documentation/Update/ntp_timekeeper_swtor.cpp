#include "ntp_timekeeper_swtor.h"
#include <cstring>
#include <stdexcept>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <poll.h>
#include <ctime>

namespace swtor {

// NTP epoch offset: NTP uses 1900, Unix uses 1970
// 70 years * 365.25 days * 24 hours * 3600 seconds
constexpr uint32_t NTP_EPOCH_OFFSET = 2208988800UL;
constexpr int NTP_PORT = 123;
constexpr uint32_t NTP_FRACTION_TO_MS = 4294967; // 2^32 / 1000

const std::vector<std::string> NTPTimeKeeper::DEFAULT_NTP_SERVERS = {
    "0.pool.ntp.org",
    "1.pool.ntp.org",
    "2.pool.ntp.org",
    "3.pool.ntp.org",
    "time.windows.com",
    "time-a-g.nist.gov",
    "time-b-g.nist.gov",
    "time-c-g.nist.gov"
};

NTPTimeKeeper::NTPTimeKeeper(
    const std::vector<std::string>& servers,
    int timeout_ms
) : servers_(servers.empty() ? DEFAULT_NTP_SERVERS : servers),
    timeout_ms_(timeout_ms),
    synchronized_(false),
    offset_ms_(0)
{
}

NTPTimeKeeper::~NTPTimeKeeper() {
}

bool NTPTimeKeeper::synchronize(bool force) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (synchronized_ && !force) {
        return true;
    }

    // Try each server until one succeeds
    for (const auto& server : servers_) {
        NTPResult result = queryNTPServer(server);
        
        if (result.success) {
            offset_ms_ = result.offset_ms;
            last_result_ = result;
            synchronized_ = true;
            return true;
        }
        
        last_result_ = result; // Store the last attempt even if failed
    }

    synchronized_ = false;
    return false;
}

int64_t NTPTimeKeeper::getOffsetMs() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return offset_ms_;
}

bool NTPTimeKeeper::isSynchronized() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return synchronized_;
}

std::chrono::system_clock::time_point NTPTimeKeeper::getNTPTime() const {
    auto now = std::chrono::system_clock::now();
    return convertToNTP(now);
}

int64_t NTPTimeKeeper::getNTPTimeMs() const {
    auto ntp_time = getNTPTime();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        ntp_time.time_since_epoch()
    ).count();
}

NTPResult NTPTimeKeeper::getLastResult() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_result_;
}

std::chrono::system_clock::time_point NTPTimeKeeper::convertToNTP(
    const std::chrono::system_clock::time_point& local_time
) const {
    int64_t offset = getOffsetMs();
    auto offset_duration = std::chrono::milliseconds(offset);
    return local_time - offset_duration;
}

std::chrono::system_clock::time_point NTPTimeKeeper::convertToLocal(
    const std::chrono::system_clock::time_point& ntp_time
) const {
    int64_t offset = getOffsetMs();
    auto offset_duration = std::chrono::milliseconds(offset);
    return ntp_time + offset_duration;
}

NTPPacket NTPTimeKeeper::createNTPPacket() {
    NTPPacket packet;
    std::memset(&packet, 0, sizeof(NTPPacket));
    
    // Set version (4) and mode (3 = client)
    packet.li_vn_mode = 0x1B; // 00 011 011 (LI=0, VN=3, Mode=3)
    
    return packet;
}

NTPResult NTPTimeKeeper::queryNTPServer(const std::string& server) {
    NTPResult result;
    result.success = false;
    result.server = server;
    result.offset_ms = 0;
    result.round_trip_ms = 0;

    // Resolve server address
    struct addrinfo hints, *servinfo;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    int rv = getaddrinfo(server.c_str(), std::to_string(NTP_PORT).c_str(), &hints, &servinfo);
    if (rv != 0) {
        result.error_message = "DNS lookup failed: " + std::string(gai_strerror(rv));
        return result;
    }

    // Create socket
    int sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (sockfd < 0) {
        result.error_message = "Failed to create socket";
        freeaddrinfo(servinfo);
        return result;
    }

    // Set socket timeout
    struct timeval tv;
    tv.tv_sec = timeout_ms_ / 1000;
    tv.tv_usec = (timeout_ms_ % 1000) * 1000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    // Create and send NTP packet
    NTPPacket send_packet = createNTPPacket();
    
    // T1: Client transmit timestamp
    auto t1 = std::chrono::system_clock::now();
    
    ssize_t sent = sendto(sockfd, &send_packet, sizeof(send_packet), 0,
                          servinfo->ai_addr, servinfo->ai_addrlen);
    
    if (sent < 0) {
        result.error_message = "Failed to send packet";
        close(sockfd);
        freeaddrinfo(servinfo);
        return result;
    }

    // Receive response
    NTPPacket recv_packet;
    struct sockaddr_storage their_addr;
    socklen_t addr_len = sizeof(their_addr);
    
    ssize_t received = recvfrom(sockfd, &recv_packet, sizeof(recv_packet), 0,
                                (struct sockaddr*)&their_addr, &addr_len);
    
    // T4: Client receive timestamp
    auto t4 = std::chrono::system_clock::now();
    
    close(sockfd);
    freeaddrinfo(servinfo);

    if (received < 0) {
        result.error_message = "Failed to receive packet (timeout or error)";
        return result;
    }

    if (received < (ssize_t)sizeof(recv_packet)) {
        result.error_message = "Incomplete packet received";
        return result;
    }

    // Parse response and calculate offset
    result = parseNTPResponse(recv_packet, t1, t4);
    result.server = server;
    result.query_time = t1;
    
    return result;
}

NTPResult NTPTimeKeeper::parseNTPResponse(
    const NTPPacket& packet,
    std::chrono::system_clock::time_point t1,
    std::chrono::system_clock::time_point t4
) {
    NTPResult result;
    result.success = false;

    // Check if this is a valid response
    if ((packet.li_vn_mode & 0x07) != 4) { // Mode should be 4 (server)
        result.error_message = "Invalid NTP mode in response";
        return result;
    }

    // Check stratum (0 = invalid, 16+ = unsynchronized)
    if (packet.stratum == 0 || packet.stratum >= 16) {
        result.error_message = "Server not synchronized (stratum " + 
                              std::to_string(packet.stratum) + ")";
        return result;
    }

    // Convert NTP timestamps to time_points
    // T2: Server receive timestamp
    auto t2 = ntpToTimePoint(ntohl(packet.recv_timestamp_sec),
                             ntohl(packet.recv_timestamp_frac));
    
    // T3: Server transmit timestamp
    auto t3 = ntpToTimePoint(ntohl(packet.trans_timestamp_sec),
                             ntohl(packet.trans_timestamp_frac));

    // Calculate offset using NTP algorithm
    // offset = ((T2 - T1) + (T3 - T4)) / 2
    auto delta1 = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
    auto delta2 = std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t4);
    int64_t offset_ms = (delta1.count() + delta2.count()) / 2;

    // Calculate round-trip delay
    // delay = (T4 - T1) - (T3 - T2)
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t1);
    auto server_time = std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2);
    int64_t delay_ms = total_time.count() - server_time.count();

    // Sanity checks
    if (std::abs(offset_ms) > 24 * 3600 * 1000) { // More than 24 hours off
        result.error_message = "Offset unreasonably large: " + std::to_string(offset_ms) + "ms";
        return result;
    }

    if (delay_ms < 0 || delay_ms > 10000) { // Negative or > 10 second delay
        result.error_message = "Round-trip delay unreasonable: " + std::to_string(delay_ms) + "ms";
        return result;
    }

    result.success = true;
    result.offset_ms = offset_ms;
    result.round_trip_ms = delay_ms;
    result.error_message.clear();

    return result;
}

std::chrono::system_clock::time_point NTPTimeKeeper::ntpToTimePoint(
    uint32_t seconds,
    uint32_t fraction
) {
    // Convert NTP seconds (since 1900) to Unix seconds (since 1970)
    int64_t unix_seconds = static_cast<int64_t>(seconds) - NTP_EPOCH_OFFSET;
    
    // Convert fraction to milliseconds
    // NTP fraction is in 1/(2^32) seconds
    int64_t milliseconds = (static_cast<uint64_t>(fraction) * 1000ULL) >> 32;
    
    auto total_ms = unix_seconds * 1000 + milliseconds;
    return std::chrono::system_clock::time_point(std::chrono::milliseconds(total_ms));
}

} // namespace swtor
