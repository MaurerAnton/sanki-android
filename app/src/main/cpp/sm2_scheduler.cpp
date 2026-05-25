// sanki-android: SM-2 scheduler implementation
// Ported from sanki-enhanced cardView/modes/sm2/sm2.cpp

#include "sm2_scheduler.hpp"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <climits>

namespace sanki {

Sm2Scheduler::Sm2Scheduler() {
    rng_.seed(std::random_device{}());
}

void Sm2Scheduler::setConfig(const Sm2Config& config) {
    config_ = config;
}

void Sm2Scheduler::setSession(const Session& session) {
    session_ = session;
    cards_ = session.cards;
}

void Sm2Scheduler::setCards(const std::vector<Card>& cards) {
    cards_ = cards;
}

void Sm2Scheduler::setCardDataMap(const std::map<std::string, Sm2CardData>& data) {
    cardDataMap_ = data;
}

std::string Sm2Scheduler::makeKey(uint64_t noteId, uint32_t deckId) {
    return std::to_string(noteId) + "_" + std::to_string(deckId);
}

std::string Sm2Scheduler::makeKeyRev(uint64_t noteId, uint32_t deckId) {
    return makeKey(noteId, deckId) + "_rev";
}

int64_t Sm2Scheduler::clampInterval(int64_t interval) {
    if (interval < config_.minInterval && config_.minInterval > 0)
        interval = config_.minInterval;
    if (interval > config_.maxInterval)
        interval = config_.maxInterval;
    return interval;
}

int Sm2Scheduler::daysFromNow(time_t due) {
    if (due == 0) return -1;
    time_t now = time(nullptr);
    double diff = difftime(due, now);
    return static_cast<int>(diff / 86400.0);
}

bool Sm2Scheduler::isNewDay() {
    time_t now = time(nullptr);
    struct tm today_tm;
    localtime_r(&now, &today_tm);
    today_tm.tm_hour = 0;
    today_tm.tm_min = 0;
    today_tm.tm_sec = 0;
    time_t today = mktime(&today_tm);

    if (todayDate_ != 0 && todayDate_ != today) {
        reviewsDoneToday_ = 0;
        newDoneToday_ = 0;
    }
    todayDate_ = today;
    return true;
}

void Sm2Scheduler::answerCard(int answer) {
    if (currentIndex_ < 0 || currentIndex_ >= static_cast<int>(cards_.size()))
        return;

    saveUndoSnap();
    undoActive_ = true;

    const Card& c = cards_[currentIndex_];
    std::string key = currentIsReversed_ ?
        makeKeyRev(c.id, c.deckId) : makeKey(c.id, c.deckId);

    auto it = cardDataMap_.find(key);
    if (it == cardDataMap_.end()) {
        Sm2CardData newData;
        newData.noteId = c.id;
        newData.deckId = c.deckId;
        newData.easeFactor = config_.startingEase;
        cardDataMap_[key] = newData;
        it = cardDataMap_.find(key);
    }

    Sm2CardData& data = it->second;
    time_t now = time(nullptr);
    data.lastReview = now;
    data.reps++;

    if (data.state == Sm2State::Review) reviewsDoneToday_++;
    else if (data.state == Sm2State::New) newDoneToday_++;

    // Track session stats
    switch (answer) {
        case 1: sessionAgain_++; break;
        case 2: sessionHard_++; break;
        case 3: sessionGood_++; break;
        case 4: sessionEasy_++; break;
    }

    // --- NEW / LEARNING state ---
    if (data.state == Sm2State::New || data.state == Sm2State::Learning) {
        data.reps--; // Don't count initial reps

        if (answer == 4) {
            // Easy: graduate immediately
            data.interval = config_.easyInterval;
            data.easeFactor = config_.startingEase;
            data.state = Sm2State::Review;
            data.due = now + data.interval;
        } else if (answer == 3) {
            // Good: advance to next learning step
            data.stepIndex++;
            auto& steps = config_.learningSteps;
            if (data.stepIndex >= static_cast<int>(steps.size())) {
                data.interval = config_.graduatingInterval;
                data.easeFactor = config_.startingEase;
                data.state = Sm2State::Review;
                data.due = now + data.interval;
            } else {
                data.state = Sm2State::Learning;
                data.due = now + steps[data.stepIndex];
            }
        } else if (answer == 2) {
            // Hard: stay at current step
            data.state = Sm2State::Learning;
            auto& steps = config_.learningSteps;
            if (data.stepIndex < static_cast<int>(steps.size())) {
                data.due = now + steps[data.stepIndex];
            }
        } else {
            // Again: reset to step 0
            data.stepIndex = 0;
            data.state = Sm2State::Learning;
            data.due = now + config_.learningSteps[0];
        }
        return;
    }

    // --- REVIEW state ---
    if (data.state == Sm2State::Review) {
        if (answer == 1) {
            // Again: lapse
            data.lapses++;
            if (data.lapses >= config_.leechThreshold && config_.leechThreshold > 0) {
                if (!data.leeched) {
                    data.leeched = true;
                    sessionLeeched_++;
                }
            }
            data.easeFactor = std::max(1.3, data.easeFactor - 0.20);
            data.interval = static_cast<int64_t>(data.interval * config_.newIntervalFactor);
            data.stepIndex = 0;
            data.state = Sm2State::Learning;
            data.due = now + config_.learningSteps[0];
        } else if (answer == 2) {
            // Hard
            data.easeFactor = std::max(1.3, data.easeFactor - 0.15);
            data.interval = static_cast<int64_t>(data.interval * config_.hardIntervalMultiplier);
            data.interval = clampInterval(data.interval);
            data.interval = static_cast<int64_t>(data.interval * config_.intervalModifier);
            data.interval = clampInterval(data.interval);
            data.due = now + data.interval;
        } else if (answer == 3) {
            // Good
            data.interval = static_cast<int64_t>(data.interval * data.easeFactor);
            data.interval = clampInterval(data.interval);
            data.interval = static_cast<int64_t>(data.interval * config_.intervalModifier);
            data.interval = clampInterval(data.interval);
            data.due = now + data.interval;
        } else if (answer == 4) {
            // Easy
            data.interval = static_cast<int64_t>(data.interval * data.easeFactor *
                (1.0 + config_.easyBonus));
            data.easeFactor += 0.15;
            data.interval = clampInterval(data.interval);
            data.interval = static_cast<int64_t>(data.interval * config_.intervalModifier);
            data.interval = clampInterval(data.interval);
            data.due = now + data.interval;
        }
    }
}

void Sm2Scheduler::saveUndoSnap() {
    if (currentIndex_ < 0) return;
    const Card& c = cards_[currentIndex_];
    std::string key = currentIsReversed_ ?
        makeKeyRev(c.id, c.deckId) : makeKey(c.id, c.deckId);

    auto it = cardDataMap_.find(key);
    if (it != cardDataMap_.end()) {
        undoSavedData_ = it->second;
    } else {
        undoSavedData_ = Sm2CardData();
        undoSavedData_.noteId = c.id;
        undoSavedData_.deckId = c.deckId;
    }
    undoCardIndex_ = currentIndex_;
}

void Sm2Scheduler::undo() {
    if (!undoActive_ || undoCardIndex_ < 0) return;
    undoActive_ = false;

    const Card& c = cards_[undoCardIndex_];
    std::string key = makeKey(c.id, c.deckId);
    cardDataMap_[key] = undoSavedData_;
    currentIndex_ = undoCardIndex_;
}

int Sm2Scheduler::pickNextCard(bool* isReversed) {
    isNewDay();
    time_t now = time(nullptr);

    // Build list of due cards (index * 2 + reversed_flag)
    std::vector<int> dueIndices;
    int firstNewIndex = -1;

    auto checkCard = [&](int cardIndex, bool reversed) {
        const Card& c = cards_[cardIndex];
        std::string key = reversed ? makeKeyRev(c.id, c.deckId) : makeKey(c.id, c.deckId);
        int encodedIdx = cardIndex * 2 + (reversed ? 1 : 0);

        auto it = cardDataMap_.find(key);
        if (it == cardDataMap_.end()) {
            if (!reversed && firstNewIndex == -1) {
                firstNewIndex = encodedIdx;
            }
            return;
        }

        Sm2CardData& data = it->second;

        // Apply filter
        bool skip = false;
        if (data.leeched) {
            skip = (studyFilter_ != StudyFilter::Leech);
        } else if (studyFilter_ == StudyFilter::Leech) {
            skip = true;
        } else if (studyFilter_ == StudyFilter::New && data.state != Sm2State::New) {
            skip = true;
        } else if (studyFilter_ == StudyFilter::Review && data.state != Sm2State::Review) {
            skip = true;
        } else if (studyFilter_ == StudyFilter::Flagged && !data.flagged) {
            skip = true;
        }

        if (!skip && data.state == Sm2State::Review) {
            if (config_.maxReviewsPerDay > 0 && reviewsDoneToday_ >= config_.maxReviewsPerDay && !cramMode_)
                return;
            if (cramMode_ || (data.due > 0 && data.due <= now)) {
                dueIndices.push_back(encodedIdx);
            }
        } else if (!skip && data.state == Sm2State::Learning) {
            if (cramMode_ || (data.due > 0 && data.due <= now)) {
                dueIndices.push_back(encodedIdx);
            }
        }
    };

    for (int i = 0; i < static_cast<int>(cards_.size()); i++) {
        checkCard(i, false);
        if (config_.reversedMode) {
            checkCard(i, true);
        }
    }

    int chosenEncoded;

    if (!dueIndices.empty()) {
        // Shuffle and pick first due
        std::shuffle(dueIndices.begin(), dueIndices.end(), rng_);
        chosenEncoded = dueIndices.front();
    } else if (firstNewIndex >= 0) {
        chosenEncoded = firstNewIndex;
    } else {
        // Find the card due soonest
        int fewestDaysAway = INT_MAX;
        int bestEncoded = 0;

        for (int i = 0; i < static_cast<int>(cards_.size()); i++) {
            for (int rev = 0; rev < (config_.reversedMode ? 2 : 1); rev++) {
                const Card& c = cards_[i];
                std::string key = rev ? makeKeyRev(c.id, c.deckId) : makeKey(c.id, c.deckId);
                auto it = cardDataMap_.find(key);
                if (it != cardDataMap_.end()) {
                    Sm2CardData& data = it->second;
                    if (data.suspended) continue;
                    if (data.leeched) continue;
                    if (data.due > 0 && data.due > now) {
                        int days = daysFromNow(data.due);
                        if (days < fewestDaysAway) {
                            fewestDaysAway = days;
                            bestEncoded = i * 2 + rev;
                        }
                    }
                }
            }
        }

        if (fewestDaysAway < INT_MAX) {
            chosenEncoded = bestEncoded;
            // Force this card to be due now
            int bestIndex = bestEncoded / 2;
            bool bestRev = (bestEncoded % 2) == 1;
            std::string key = bestRev ?
                makeKeyRev(cards_[bestIndex].id, cards_[bestIndex].deckId) :
                makeKey(cards_[bestIndex].id, cards_[bestIndex].deckId);
            cardDataMap_[key].due = now + config_.learningSteps[0];
        } else {
            // Nothing at all — completely done
            currentIndex_ = -1;
            if (isReversed) *isReversed = false;
            return -1;
        }
    }

    currentIndex_ = chosenEncoded / 2;
    currentIsReversed_ = (chosenEncoded % 2) == 1;
    cardFrontShown_ = true;

    // Ensure card data exists
    const Card& c = cards_[currentIndex_];
    std::string key = currentIsReversed_ ?
        makeKeyRev(c.id, c.deckId) : makeKey(c.id, c.deckId);
    if (cardDataMap_.find(key) == cardDataMap_.end()) {
        Sm2CardData newData;
        newData.noteId = c.id;
        newData.deckId = c.deckId;
        newData.easeFactor = config_.startingEase;
        cardDataMap_[key] = newData;
    }

    if (isReversed) *isReversed = currentIsReversed_;
    return currentIndex_;
}

const Card* Sm2Scheduler::currentCard() const {
    if (currentIndex_ < 0 || currentIndex_ >= static_cast<int>(cards_.size()))
        return nullptr;
    return &cards_[currentIndex_];
}

SessionStats Sm2Scheduler::getStats() const {
    SessionStats stats;
    stats.reviewsDoneToday = reviewsDoneToday_;
    stats.newDoneToday = newDoneToday_;
    stats.sessionAgain = sessionAgain_;
    stats.sessionHard = sessionHard_;
    stats.sessionGood = sessionGood_;
    stats.sessionEasy = sessionEasy_;
    stats.sessionLeeched = sessionLeeched_;

    for (const auto& kv : cardDataMap_) {
        const auto& d = kv.second;
        if (d.state == Sm2State::New) stats.totalNew++;
        else if (d.state == Sm2State::Learning) stats.totalLearning++;
        else if (d.state == Sm2State::Review) stats.totalReview++;
    }

    // Count due cards
    time_t now = time(nullptr);
    for (const auto& kv : cardDataMap_) {
        const auto& d = kv.second;
        if (d.due > 0 && d.due <= now && d.state != Sm2State::New) {
            stats.dueCount++;
        }
    }

    return stats;
}

void Sm2Scheduler::resetDailyCounts() {
    reviewsDoneToday_ = 0;
    newDoneToday_ = 0;
}

int Sm2Scheduler::dueCount() const {
    int count = 0;
    time_t now = time(nullptr);
    for (const auto& kv : cardDataMap_) {
        const auto& d = kv.second;
        if (d.due > 0 && d.due <= now && d.state != Sm2State::New) {
            count++;
        }
    }
    return count;
}

const char* Sm2Scheduler::stateName(Sm2State s) {
    switch (s) {
        case Sm2State::New: return "New";
        case Sm2State::Learning: return "Learning";
        case Sm2State::Review: return "Review";
        default: return "Unknown";
    }
}

const char* Sm2Scheduler::answerName(Sm2Answer a) {
    switch (a) {
        case Sm2Answer::Again: return "Again";
        case Sm2Answer::Hard: return "Hard";
        case Sm2Answer::Good: return "Good";
        case Sm2Answer::Easy: return "Easy";
        default: return "Unknown";
    }
}

} // namespace sanki
