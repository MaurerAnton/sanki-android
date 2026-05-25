// sanki-android: Statistics engine

#ifndef STATS_ENGINE_HPP
#define STATS_ENGINE_HPP

#include "card_model.hpp"
#include <map>
#include <string>
#include <vector>
#include <ctime>

namespace sanki {

struct RetentionPoint {
    double stabilityDays;
    double retentionRate;
};

struct HeatmapBucket {
    int hour;
    int count;
};

struct ReviewHistory {
    time_t timestamp;
    int rating; // 1-4
    uint64_t cardId;
};

class StatsEngine {
public:
    StatsEngine();

    // Record a review event
    void recordReview(uint64_t cardId, int rating, double stability, time_t when);

    // Calculate retention rate for given stability range
    double retentionRate(double minStability, double maxStability) const;

    // Generate retention curve data points
    std::vector<RetentionPoint> retentionCurve(int buckets = 20) const;

    // Review counts by day (last N days)
    std::vector<int> reviewsByDay(int days = 30) const;

    // Review heatmap by hour (0-23)
    std::vector<HeatmapBucket> reviewsByHour() const;

    // Current streak (consecutive days with at least 1 review)
    int currentStreak() const;

    // Total stats
    int64_t totalReviews() const { return static_cast<int64_t>(history_.size()); }
    int totalReviewDays() const;

    // Rating distribution (Again/Hard/Good/Easy counts)
    std::vector<int> ratingDistribution() const;

    // Average retention
    double averageRetention() const;

    // Clear history
    void clear();

    // Serialize for persistence
    std::string toJson() const;

private:
    std::vector<ReviewHistory> history_;

    static time_t dayStart(time_t t);
    static std::string isoTime(time_t t);
};

} // namespace sanki

#endif
