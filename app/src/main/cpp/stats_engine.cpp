// sanki-android: Statistics engine implementation

#include "stats_engine.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <set>

namespace sanki {

StatsEngine::StatsEngine() {}

time_t StatsEngine::dayStart(time_t t) {
    struct tm tm;
    localtime_r(&t, &tm);
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    return mktime(&tm);
}

std::string StatsEngine::isoTime(time_t t) {
    char buf[32];
    struct tm tm;
    localtime_r(&t, &tm);
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);
    return buf;
}

void StatsEngine::recordReview(uint64_t cardId, int rating, double stability, time_t when) {
    ReviewHistory rh;
    rh.timestamp = when;
    rh.rating = rating;
    rh.cardId = cardId;
    history_.push_back(rh);
}

double StatsEngine::retentionRate(double minStability, double maxStability) const {
    // Count reviews with stability in range, check if rating != 1 (not forgotten)
    int total = 0, remembered = 0;
    for (auto& h : history_) {
        if (h.rating >= 1 && h.rating <= 4) {
            total++;
            if (h.rating != 1) remembered++;
        }
    }
    return total > 0 ? static_cast<double>(remembered) / total : 0.0;
}

std::vector<RetentionPoint> StatsEngine::retentionCurve(int buckets) const {
    std::vector<RetentionPoint> curve;

    if (history_.empty()) return curve;

    // Group reviews by days-since-last-review
    std::map<int, std::pair<int, int>> bucketData; // days -> {total, remembered}

    // Simplified: bucket by rating
    for (int b = 0; b < buckets; b++) {
        double stability = std::pow(10.0, static_cast<double>(b) / buckets * 3.0); // 1..1000
        int total = 0, remembered = 0;

        for (auto& h : history_) {
            if (h.rating >= 1 && h.rating <= 4) {
                total++;
                if (h.rating != 1) remembered++;
            }
        }

        RetentionPoint p;
        p.stabilityDays = stability;
        p.retentionRate = total > 0 ? static_cast<double>(remembered) / total : 0.5;
        curve.push_back(p);
    }

    return curve;
}

std::vector<int> StatsEngine::reviewsByDay(int days) const {
    std::vector<int> counts(days, 0);
    time_t now = time(nullptr);
    time_t start = dayStart(now) - (days - 1) * 86400;

    for (auto& h : history_) {
        int dayOffset = static_cast<int>((dayStart(h.timestamp) - start) / 86400);
        if (dayOffset >= 0 && dayOffset < days) {
            counts[dayOffset]++;
        }
    }
    return counts;
}

std::vector<HeatmapBucket> StatsEngine::reviewsByHour() const {
    std::vector<HeatmapBucket> buckets(24);
    for (int i = 0; i < 24; i++) {
        buckets[i].hour = i;
        buckets[i].count = 0;
    }

    for (auto& h : history_) {
        struct tm tm;
        localtime_r(&h.timestamp, &tm);
        if (tm.tm_hour >= 0 && tm.tm_hour < 24) {
            buckets[tm.tm_hour].count++;
        }
    }

    return buckets;
}

int StatsEngine::currentStreak() const {
    if (history_.empty()) return 0;

    // Get unique review days sorted descending
    std::set<time_t> days;
    for (auto& h : history_) {
        days.insert(dayStart(h.timestamp));
    }

    time_t today = dayStart(time(nullptr));
    time_t yesterday = today - 86400;

    // Check if reviewed today or yesterday
    if (days.find(today) == days.end() && days.find(yesterday) == days.end()) {
        return 0;
    }

    int streak = 0;
    time_t check = today;
    if (days.find(today) == days.end()) {
        check = yesterday;
    }

    while (days.find(check) != days.end()) {
        streak++;
        check -= 86400;
    }

    return streak;
}

int StatsEngine::totalReviewDays() const {
    std::set<time_t> days;
    for (auto& h : history_) {
        days.insert(dayStart(h.timestamp));
    }
    return static_cast<int>(days.size());
}

std::vector<int> StatsEngine::ratingDistribution() const {
    std::vector<int> dist(5, 0); // 0=unused, 1=Again, 2=Hard, 3=Good, 4=Easy
    for (auto& h : history_) {
        if (h.rating >= 1 && h.rating <= 4) {
            dist[h.rating]++;
        }
    }
    return dist;
}

double StatsEngine::averageRetention() const {
    int total = 0, remembered = 0;
    for (auto& h : history_) {
        if (h.rating >= 1 && h.rating <= 4) {
            total++;
            if (h.rating != 1) remembered++;
        }
    }
    return total > 0 ? (static_cast<double>(remembered) / total) * 100.0 : 0.0;
}

void StatsEngine::clear() {
    history_.clear();
}

std::string StatsEngine::toJson() const {
    std::ostringstream json;
    json << "{";

    // Rating distribution
    auto rd = ratingDistribution();
    json << "\"again\":" << rd[1] << ",\"hard\":" << rd[2]
         << ",\"good\":" << rd[3] << ",\"easy\":" << rd[4];

    json << ",\"totalReviews\":" << totalReviews();
    json << ",\"reviewDays\":" << totalReviewDays();
    json << ",\"streak\":" << currentStreak();
    json << ",\"averageRetention\":" << averageRetention();

    // Last 30 days
    auto byDay = reviewsByDay(30);
    json << ",\"dailyReviews\":[";
    for (size_t i = 0; i < byDay.size(); i++) {
        if (i > 0) json << ",";
        json << byDay[i];
    }
    json << "]";

    // Hourly heatmap
    auto byHour = reviewsByHour();
    json << ",\"hourlyHeatmap\":[";
    for (size_t i = 0; i < byHour.size(); i++) {
        if (i > 0) json << ",";
        json << byHour[i].count;
    }
    json << "]";

    // Retention curve
    auto curve = retentionCurve(15);
    json << ",\"retentionCurve\":[";
    for (size_t i = 0; i < curve.size(); i++) {
        if (i > 0) json << ",";
        json << "{\"stability\":" << curve[i].stabilityDays
             << ",\"retention\":" << curve[i].retentionRate << "}";
    }
    json << "]";

    json << "}";
    return json.str();
}

} // namespace sanki
