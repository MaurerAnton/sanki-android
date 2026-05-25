// sanki-android: SM-2 scheduler (Qt-free port)
// Algorithm derived from sanki-enhanced cardView/modes/sm2/sm2.cpp

#ifndef SM2_SCHEDULER_HPP
#define SM2_SCHEDULER_HPP

#include "card_model.hpp"
#include <map>
#include <string>
#include <random>

namespace sanki {

class Sm2Scheduler {
public:
    Sm2Scheduler();

    // Configure the scheduler
    void setConfig(const Sm2Config& config);
    Sm2Config& config() { return config_; }

    // Set the session cards and deck paths
    void setSession(const Session& session);
    void setCards(const std::vector<Card>& cards);

    // Load/save card data for persistence (JSON strings keyed by card key)
    void setCardDataMap(const std::map<std::string, Sm2CardData>& data);
    const std::map<std::string, Sm2CardData>& cardDataMap() const { return cardDataMap_; }

    // Pick the next card to study. Returns -1 if done/nothing due.
    // Sets *isReversed if we're showing the reversed side.
    int pickNextCard(bool* isReversed = nullptr);

    // Answer the current card: 1=Again, 2=Hard, 3=Good, 4=Easy
    void answerCard(int answer);

    // Get the current card content (front/back HTML)
    const Card* currentCard() const;
    bool currentIsReversed() const { return currentIsReversed_; }

    // Undo support
    bool canUndo() const { return undoActive_; }
    void undo();

    // Stats
    SessionStats getStats() const;

    // Session summary
    void resetDailyCounts();
    int dueCount() const;

    // Cram mode: review everything regardless of schedule
    void setCramMode(bool cram) { cramMode_ = cram; }
    bool cramMode() const { return cramMode_; }

    // Filtering
    enum class StudyFilter { All, New, Review, Leech, Flagged };
    void setStudyFilter(StudyFilter f) { studyFilter_ = f; }

    // Key generation (compatible with sanki-enhanced format)
    static std::string makeKey(uint64_t noteId, uint32_t deckId);
    static std::string makeKeyRev(uint64_t noteId, uint32_t deckId);

    // State helpers
    static const char* stateName(Sm2State s);
    static const char* answerName(Sm2Answer a);

private:
    Sm2Config config_;
    std::vector<Card> cards_;
    std::map<std::string, Sm2CardData> cardDataMap_;
    Session session_;

    int currentIndex_ = -1;
    bool currentIsReversed_ = false;
    bool cardFrontShown_ = true;

    // Daily tracking
    int reviewsDoneToday_ = 0;
    int newDoneToday_ = 0;
    time_t todayDate_ = 0;

    // Session counters
    int sessionAgain_ = 0;
    int sessionHard_ = 0;
    int sessionGood_ = 0;
    int sessionEasy_ = 0;
    int sessionLeeched_ = 0;

    // Cram/filter
    bool cramMode_ = false;
    StudyFilter studyFilter_ = StudyFilter::All;

    // Undo
    bool undoActive_ = false;
    Sm2CardData undoSavedData_;
    int undoCardIndex_ = -1;

    // Internal
    std::mt19937 rng_;

    void saveUndoSnap();
    int64_t clampInterval(int64_t interval);
    int daysFromNow(time_t due);
    bool isNewDay();
};

} // namespace sanki

#endif // SM2_SCHEDULER_HPP
