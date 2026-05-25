// sanki-android: FSRS-5 scheduler implementation

#include "fsrs_scheduler.hpp"
#include <algorithm>
#include <ctime>

namespace sanki {

FsrsScheduler::FsrsScheduler() {
    rng_.seed(std::random_device{}());
    updateFactor();
}

void FsrsScheduler::updateFactor() {
    factor_ = fsrsDefaultFactor(params_.requestRetention);
}

void FsrsScheduler::setCards(const std::vector<Card>& cards) {
    cards_ = cards;
}

void FsrsScheduler::setCardState(const std::map<std::string, FsrsCardState>& states) {
    states_ = states;
}

double FsrsScheduler::clamp(double v, double lo, double hi) {
    return std::max(lo, std::min(hi, v));
}

double FsrsScheduler::meanReversion(double current, double initial) {
    // w[6] * initial + (1-w[6]) * current = mean reversion toward initial
    double w6 = params_.w[6];
    return w6 * initial + (1.0 - w6) * current;
}

double FsrsScheduler::retrievability(double t, double S) {
    if (t <= 0 || S <= 0) return 1.0;
    double r = std::pow(1.0 + factor_ * (t / S), -1.0 / factor_);
    return clamp(r, 0.0, 1.0);
}

double FsrsScheduler::stabilityAfterSuccess(double S, double D, int r, double elapsed) {
    double w8 = params_.w[8], w9 = params_.w[9], w10 = params_.w[10];
    double w11 = params_.w[11], w12 = params_.w[12];
    double w15 = params_.w[15], w16 = params_.w[16];

    // r ∈ {1=Again, 2=Hard, 3=Good, 4=Easy}
    double hardPenalty = (r == 2) ? w15 : 1.0;
    double easyBonus = (r == 4) ? w16 : 1.0;

    double S_i;
    double effectiveS = S;

    if (params_.enableShortTerm > 0.0 && elapsed < 1.0) {
        // Short-term stability for same-day reviews
        return effectiveS;
    }

    // ln(S_n+1) = ln(S_n) + α * (w8) + β * (w9) + ...
    // Simplified from the reference implementation
    double lnS = std::log(effectiveS);
    double alpha = w8 * std::exp(w9 * (D - 3.0));
    double beta = w10 * std::exp(w11 * (D - 3.0)) * (std::pow(S, -w12));

    // Apply rating modifiers
    if (r == 1) {
        // Again: stability drops
        S_i = effectiveS * std::exp(-alpha * std::abs(D - 3.0) / 3.0);
    } else if (r == 2) {
        // Hard: slight increase
        S_i = effectiveS * std::exp(alpha * 0.5);
    } else if (r == 3) {
        // Good
        S_i = effectiveS * std::exp(alpha);
    } else {
        // Easy
        S_i = effectiveS * std::exp(alpha * easyBonus);
    }

    return clamp(S_i, 0.1, params_.maximumInterval);
}

double FsrsScheduler::stabilityAfterFailure(double S, double D, int r) {
    double w13 = params_.w[13], w14 = params_.w[14];

    // After a lapse, stability resets with penalty
    double newS = S * std::exp(-w13 * D / w14);
    return clamp(newS, 0.01, S);
}

double FsrsScheduler::difficultyAfterSuccess(double D, int r) {
    double w4 = params_.w[4], w5 = params_.w[5];

    // D' = D + ΔD
    // ΔD = -w4 * (r - 3)   — subtract if hard, add if easy
    double delta = -w4 * (r - 3.0);
    D += delta * (10.0 - D) / 9.0; // scale by remaining room

    return meanReversion(clamp(D, 1.0, 10.0), 5.0);
}

double FsrsScheduler::difficultyAfterFailure(double D) {
    double w4 = params_.w[4], w5 = params_.w[5];

    // After a lapse, difficulty increases
    double delta = w5;
    D += delta * (10.0 - D) / 9.0;

    return meanReversion(clamp(D, 1.0, 10.0), 5.0);
}

int64_t FsrsScheduler::addFuzz(int64_t interval) {
    if (!params_.enableFuzz || interval < 2) return interval;

    // Add ±5-15% random fuzz to prevent cards clustering
    std::uniform_real_distribution<double> dist(-0.15, 0.15);
    double fuzz = dist(rng_);
    int64_t delta = static_cast<int64_t>(interval * fuzz);
    if (delta == 0) delta = (fuzz > 0) ? 1 : -1;
    return interval + delta;
}

void FsrsScheduler::answerCard(int rating) {
    if (currentIndex_ < 0 || currentIndex_ >= static_cast<int>(cards_.size()))
        return;

    const Card& c = cards_[currentIndex_];
    std::string key = currentReversed_ ?
        makeKeyRev(c.id, c.deckId) : makeKey(c.id, c.deckId);

    auto it = states_.find(key);
    if (it == states_.end()) {
        FsrsCardState newState;
        states_[key] = newState;
        it = states_.find(key);
    }

    FsrsCardState& st = it->second;
    time_t now = time(nullptr);

    double elapsedDays = 0.0;
    if (st.lastReview > 0) {
        elapsedDays = difftime(now, st.lastReview) / 86400.0;
    }
    st.elapsedDays = elapsedDays;
    st.reps++;
    st.lastReview = now;

    if (rating == 1) {
        // Again: lapse
        st.lapses++;
        st.difficulty = difficultyAfterFailure(st.difficulty);
        st.stability = stabilityAfterFailure(st.stability, st.difficulty, rating);
        st.state = 1; // Learning / relearning
        st.scheduledDays = 0.017; // ~1 minute for next review
    } else {
        // Hard/Good/Easy: success
        st.difficulty = difficultyAfterSuccess(st.difficulty, rating);
        st.stability = stabilityAfterSuccess(st.stability, st.difficulty, rating, elapsedDays);
        st.state = 2; // Review
        st.scheduledDays = st.stability * params_.requestRetention;
    }

    // Apply fuzz and set due date
    int64_t intervalSec = static_cast<int64_t>(st.scheduledDays * 86400.0);
    intervalSec = addFuzz(intervalSec);
    if (intervalSec < 60) intervalSec = 60;
    st.due = now + intervalSec;
}

int FsrsScheduler::pickNextCard(bool* isReversed) {
    time_t now = time(nullptr);
    int bestEncoded = -1;
    double bestPriority = -1.0;

    auto check = [&](int idx, bool rev) {
        const Card& c = cards_[idx];
        std::string key = rev ? makeKeyRev(c.id, c.deckId) : makeKey(c.id, c.deckId);
        int encoded = idx * 2 + (rev ? 1 : 0);

        auto it = states_.find(key);
        if (it == states_.end()) {
            // New card — always high priority
            if (bestEncoded < 0) {
                bestEncoded = encoded;
                bestPriority = 100.0;
            }
            return;
        }

        FsrsCardState& st = it->second;
        if (st.leeched) return;

        // Calculate priority: higher R = more urgent
        double daysDue = difftime(now, st.due) / 86400.0;
        double R = retrievability(std::max(0.0, daysDue + st.scheduledDays), st.stability);
        double priority = 1.0 - R; // cards with low retrievability first

        if (st.state == 0) priority = 0.5; // New cards after reviewed ones
        if (st.due <= now) priority = std::max(priority, 0.9); // Due cards are urgent

        if (priority > bestPriority) {
            bestPriority = priority;
            bestEncoded = encoded;
        }
    };

    for (int i = 0; i < static_cast<int>(cards_.size()); i++) {
        check(i, false);
        // Reversed cards
        if (cards_[i].backField.size() > 0) {
            check(i, true);
        }
    }

    if (bestEncoded < 0 && cards_.empty()) {
        currentIndex_ = -1;
        if (isReversed) *isReversed = false;
        return -1;
    }

    if (bestEncoded < 0) bestEncoded = 0; // Fallback

    currentIndex_ = bestEncoded / 2;
    currentReversed_ = (bestEncoded % 2) == 1;
    if (isReversed) *isReversed = currentReversed_;

    // Ensure state exists
    const Card& c = cards_[currentIndex_];
    std::string key = currentReversed_ ?
        makeKeyRev(c.id, c.deckId) : makeKey(c.id, c.deckId);
    if (states_.find(key) == states_.end()) {
        FsrsCardState ns;
        states_[key] = ns;
    }

    return currentIndex_;
}

const Card* FsrsScheduler::currentCard() const {
    if (currentIndex_ < 0 || currentIndex_ >= static_cast<int>(cards_.size()))
        return nullptr;
    return &cards_[currentIndex_];
}

FsrsCardState* FsrsScheduler::currentState() {
    if (currentIndex_ < 0) return nullptr;
    const Card& c = cards_[currentIndex_];
    std::string key = currentReversed_ ?
        makeKeyRev(c.id, c.deckId) : makeKey(c.id, c.deckId);
    auto it = states_.find(key);
    return (it != states_.end()) ? &it->second : nullptr;
}

int FsrsScheduler::dueCount() const {
    int count = 0;
    time_t now = time(nullptr);
    for (auto& kv : states_) {
        if (kv.second.due > 0 && kv.second.due <= now && !kv.second.leeched)
            count++;
    }
    return count;
}

int FsrsScheduler::newCount() const {
    int total = static_cast<int>(cards_.size());
    for (auto& kv : states_) {
        if (kv.second.state != 0) total--;
    }
    return std::max(0, total);
}

int FsrsScheduler::reviewCount() const {
    int count = 0;
    for (auto& kv : states_) {
        if (kv.second.state == 2) count++;
    }
    return count;
}

int FsrsScheduler::learningCount() const {
    int count = 0;
    for (auto& kv : states_) {
        if (kv.second.state == 1) count++;
    }
    return count;
}

std::string FsrsScheduler::makeKey(uint64_t noteId, uint32_t deckId) {
    return std::to_string(noteId) + "_" + std::to_string(deckId);
}

std::string FsrsScheduler::makeKeyRev(uint64_t noteId, uint32_t deckId) {
    return makeKey(noteId, deckId) + "_rev";
}

} // namespace sanki
