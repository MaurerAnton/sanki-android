// sanki-android: FSRS-5 scheduler
// Based on open-spaced-repetition/fsrs4anki specification v5
// https://github.com/open-spaced-repetition/fsrs4anki/wiki/The-Algorithm

#ifndef FSRS_SCHEDULER_HPP
#define FSRS_SCHEDULER_HPP

#include "card_model.hpp"
#include <map>
#include <string>
#include <vector>
#include <random>
#include <cmath>

namespace sanki {

struct FsrsCardState {
    double stability = 0.0;     // S — memory stability in days
    double difficulty = 0.0;    // D — inherent difficulty [1, 10]
    double elapsedDays = 0.0;   // days since last review
    double scheduledDays = 0.0; // scheduled interval in days
    int reps = 0;               // total repetitions
    int lapses = 0;             // total lapses
    int state = 0;              // 0=New, 1=Learning, 2=Review, 3=Relearning
    time_t due = 0;             // unix timestamp
    time_t lastReview = 0;
    bool leeched = false;
};

// FSRS-5 default parameters (pre-computed from community data)
struct FsrsParams {
    double requestRetention = 0.90;  // desired retention rate
    int maximumInterval = 36500;     // days (~100 years)
    double enableShortTerm = 1.0;    // enable short-term memory model
    bool enableFuzz = true;          // add fuzz to intervals

    // 17 weights (w0..w16)
    std::vector<double> w = {
        0.4872, 1.4003, 3.7145, 13.8206, 5.1618,
        1.2298, 0.8975, 0.031, 1.6474, 0.1367,
        1.0461, 2.1072, 0.0793, 0.3246, 1.587,
        0.2272, 2.8755
    };
};

// R = 0.9 -> factor = ln(0.9) / ln(0.5) ≈ -0.152 / -0.693 ≈ 0.2195
inline double fsrsDefaultFactor(double r) { return std::log(r) / std::log(0.5); }

class FsrsScheduler {
public:
    FsrsScheduler();

    void setParams(const FsrsParams& p) { params_ = p; updateFactor(); }
    const FsrsParams& params() const { return params_; }

    void setCards(const std::vector<Card>& cards);
    void setCardState(const std::map<std::string, FsrsCardState>& states);
    const std::map<std::string, FsrsCardState>& cardStates() const { return states_; }

    // Pick next due card. Returns card index, -1 if nothing due.
    int pickNextCard(bool* isReversed = nullptr);
    const Card* currentCard() const;
    FsrsCardState* currentState();

    // Answer: 1=Again, 2=Hard, 3=Good, 4=Easy
    void answerCard(int rating);

    // Get stats
    int dueCount() const;
    int newCount() const;
    int reviewCount() const;
    int learningCount() const;

    // Key helpers
    static std::string makeKey(uint64_t noteId, uint32_t deckId);
    static std::string makeKeyRev(uint64_t noteId, uint32_t deckId);

private:
    FsrsParams params_;
    double factor_ = 0.0;
    std::vector<Card> cards_;
    std::map<std::string, FsrsCardState> states_;

    int currentIndex_ = -1;
    bool currentReversed_ = false;

    std::mt19937 rng_;

    void updateFactor();
    double retrievability(double t, double S);
    double stabilityAfterSuccess(double S, double D, int r, double elapsed);
    double stabilityAfterFailure(double S, double D, int r);
    double difficultyAfterSuccess(double D, int r);
    double difficultyAfterFailure(double D);
    double meanReversion(double current, double initial);
    int64_t addFuzz(int64_t interval);
    double clamp(double v, double lo, double hi);
};

} // namespace sanki

#endif
