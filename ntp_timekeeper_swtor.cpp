#include "ntp_timekeeper_swtor.h"
#include <cstring>
#include <ctime>

// Windows networking headers
#include <winsock2.h>
#include <ws2tcpip.h>

// Link with Winsock library
#pragma comment(lib, "ws2_32.lib")

namespace swtor {

// NTP epoch offset: NTP uses 1900, Unix uses 1970
constexpr uint32_t NTP_EPOCH_OFFSET = 2208988800UL;
constexpr int NTP_PORT = 123;
constexpr uint32_t NTP_FRACTION_TO_MS = 4294967; // 2^32 / 1000

// Static members for WSA initialization
bool NTPTimeKeeper::wsa_initialized_ = false;
std::mutex NTPTimeKeeper::wsa_mutex_;

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

// Helper to initialize Winsock
static bool initializeWSA() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        return false;
    }
    return true;
}

NTPTimeKeeper::NTPTimeKeeper(
    const std::vector<std::string>& servers,
    int timeout_ms
) : servers_(servers.empty() ? DEFAULT_NTP_SERVERS : servers),
    timeout_ms_(timeout_ms),
    synchronized_(false),
    offset_ms_(0)
{
    // Initialize Winsock if not already done
    std::lock_guard<std::mutex> lock(wsa_mutex_);
    if (!wsa_initialized_) {
        wsa_initialized_ = initializeWSA();
    }
}

NTPTimeKeeper::~NTPTimeKeeper() {
    // Note: We don't call WSACleanup() here because other instances might still need it
}

bool NTPTimeKeeper::synchronize(bool force) {
    
    utc_offset_to_local_ms();

    std::lock_guard<std::mutex> lock(mutex_);
    
    if (synchronized_ && !force) {
        return true;
    }

    // Check if WSA is initialized
    if (!wsa_initialized_) {
        last_result_.success = false;
        last_result_.error_message = "Winsock initialization failed";
        synchronized_ = false;
        return false;
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
        
        last_result_ = result;
    }

    synchronized_ = false;
    return false;
}

void NTPTimeKeeper::set_local_offset(int64_t val) {
    std::lock_guard<std::mutex> lock(mutex2_);
    zone_offset_ms_ = val;
}

int64_t NTPTimeKeeper::get_local_offset() const {
    std::lock_guard<std::mutex> lock(mutex2_);
    return zone_offset_ms_;
}

int64_t NTPTimeKeeper::getOffsetMs() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return (offset_ms_);
}

bool NTPTimeKeeper::isSynchronized() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return synchronized_;
}



int64_t NTPTimeKeeper::utc_offset_to_local_ms() 
{
    using namespace std::chrono;
    auto now_sys = std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now());
    auto tz = current_zone();
    auto info = tz->get_info(now_sys);
    auto total_offset = info.offset + info.save;
    int64_t val = duration_cast<milliseconds>(total_offset).count();
    set_local_offset(val);
    return val;
}

std::chrono::system_clock::time_point NTPTimeKeeper::getZeroHour(std::chrono::system_clock::time_point input) {
    using namespace std::chrono;
    system_clock::time_point results = floor<days>(input);
    return results;
}


std::chrono::system_clock::time_point NTPTimeKeeper::getNTPTime() const {
    auto now = std::chrono::system_clock::now();
    return convertToNTP(now);
}

std::chrono::system_clock::time_point NTPTimeKeeper::adjust_time(std::chrono::system_clock::time_point input, int64_t ms) {
    int64_t offset = ms;
    auto offset_duration = std::chrono::milliseconds(offset);
    auto results = input + offset_duration;
    return results;
}

std::chrono::system_clock::time_point NTPTimeKeeper::adjust_time(std::chrono::system_clock::time_point input, int64_t days, int64_t hours, int64_t min, int64_t sec, int64_t ms) {
    int64_t offset = days * 24; // convert to hours
    offset = (offset + hours) * 60; // convert to min
    offset = (offset + min) * 60; // convert to sec
    offset = (offset + sec) * 1000; // convert to millisec
    offset = offset + ms;
    auto offset_duration = std::chrono::milliseconds(offset);
    auto results = input + offset_duration;
    return results;
}

std::chrono::system_clock::time_point NTPTimeKeeper::getLocalTime() const {
    auto now = getNTPTime();
    int64_t offset = get_local_offset();
    auto offset_duration = std::chrono::milliseconds(offset);
    now = now + offset_duration;
    return (now);
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
    
    // Set version (3) and mode (3 = client)
    packet.li_vn_mode = 0x1B; // 00 011 011
    
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
    hints.ai_protocol = IPPROTO_UDP;

    int rv = getaddrinfo(server.c_str(), std::to_string(NTP_PORT).c_str(), &hints, &servinfo);
    if (rv != 0) {
        result.error_message = "DNS lookup failed: " + std::to_string(rv);
        return result;
    }

    // Create socket
    SOCKET sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (sockfd == INVALID_SOCKET) {
        result.error_message = "Failed to create socket: " + std::to_string(WSAGetLastError());
        freeaddrinfo(servinfo);
        return result;
    }

    // Set socket timeout
    DWORD timeout_dw = static_cast<DWORD>(timeout_ms_);
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_dw, sizeof(timeout_dw));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout_dw, sizeof(timeout_dw));

    // Create and send NTP packet
    NTPPacket send_packet = createNTPPacket();
    
    // T1: Client transmit timestamp
    auto t1 = std::chrono::system_clock::now();
    
    int sent = sendto(sockfd, (const char*)&send_packet, sizeof(send_packet), 0,
                      servinfo->ai_addr, (int)servinfo->ai_addrlen);
    
    if (sent == SOCKET_ERROR) {
        result.error_message = "Failed to send packet: " + std::to_string(WSAGetLastError());
        closesocket(sockfd);
        freeaddrinfo(servinfo);
        return result;
    }

    // Receive response
    NTPPacket recv_packet;
    struct sockaddr_storage their_addr;
    int addr_len = sizeof(their_addr);
    
    int received = recvfrom(sockfd, (char*)&recv_packet, sizeof(recv_packet), 0,
                            (struct sockaddr*)&their_addr, &addr_len);
    
    // T4: Client receive timestamp
    auto t4 = std::chrono::system_clock::now();
    
    closesocket(sockfd);
    freeaddrinfo(servinfo);

    if (received == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error == WSAETIMEDOUT) {
            result.error_message = "Timeout waiting for response";
        } else {
            result.error_message = "Failed to receive packet: " + std::to_string(error);
        }
        return result;
    }

    if (received < (int)sizeof(recv_packet)) {
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
    // Need to convert from network byte order (big-endian) to host byte order
    auto ntohl_func = [](uint32_t netlong) -> uint32_t {
        return ((netlong & 0xFF000000) >> 24) |
               ((netlong & 0x00FF0000) >> 8) |
               ((netlong & 0x0000FF00) << 8) |
               ((netlong & 0x000000FF) << 24);
    };

    // T2: Server receive timestamp
    auto t2 = ntpToTimePoint(ntohl_func(packet.recv_timestamp_sec),
                             ntohl_func(packet.recv_timestamp_frac));
    
    // T3: Server transmit timestamp
    auto t3 = ntpToTimePoint(ntohl_func(packet.trans_timestamp_sec),
                             ntohl_func(packet.trans_timestamp_frac));

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
    int64_t milliseconds = (static_cast<uint64_t>(fraction) * 1000ULL) >> 32;
    
    auto total_ms = unix_seconds * 1000 + milliseconds;
    return std::chrono::system_clock::time_point(std::chrono::milliseconds(total_ms));
}

} // namespace swtor
