// sanki-android: card data model (Qt-free)
// Derived from sanki-enhanced mainMenu/sessions/sessionStruct.h and cardView/modes/sm2/sm2.h

#ifndef CARD_MODEL_HPP
#define CARD_MODEL_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <ctime>

namespace sanki {

enum class DeckMode {
    None = 0,
    CompletelyRandomised = 1,
    RandomisedNoRepeating = 2,
    Boxes = 3,
    SM2 = 4,
};

struct Card {
    uint64_t id = 0;        // note id from Anki DB
    uint32_t deckId = 0;    // index into deck path list
    uint32_t count = 0;     // times this card appeared

    // For Boxes mode
    uint32_t againCount = 0;
    uint32_t hardCount = 0;
    uint32_t goodCount = 0;
    uint32_t easyCount = 0;

    // Cached card content
    std::string frontField;
    std::string backField;
};

enum class Sm2State {
    New = 0,
    Learning = 1,
    Review = 2
};

enum class Sm2Answer {
    Again = 1,
    Hard = 2,
    Good = 3,
    Easy = 4
};

struct Sm2CardData {
    uint64_t noteId = 0;
    uint32_t deckId = 0;
    double easeFactor = 2.5;
    int64_t interval = 0;       // seconds
    int32_t reps = 0;
    int32_t lapses = 0;
    Sm2State state = Sm2State::New;
    int32_t stepIndex = 0;
    bool leeched = false;
    bool suspended = false;
    bool flagged = false;
    time_t due = 0;             // unix timestamp
    time_t lastReview = 0;
};

struct Sm2Config {
    std::vector<int64_t> learningSteps = {60, 600};  // seconds
    int64_t graduatingInterval = 86400;    // 1 day
    int64_t easyInterval = 345600;         // 4 days
    double startingEase = 2.5;
    double easyBonus = 0.15;
    double intervalModifier = 1.0;
    double hardIntervalMultiplier = 1.2;
    double newIntervalFactor = 0.0;
    int64_t maxInterval = 3153600000LL;   // ~100 years
    int64_t minInterval = 0;
    int32_t leechThreshold = 8;
    int32_t maxReviewsPerDay = 200;
    bool reversedMode = false;
    bool twoButtonMode = false;
};

struct Session {
    std::string name;
    DeckMode mode = DeckMode::SM2;
    std::vector<std::string> deckPaths;
    std::vector<Card> cards;
    time_t created = 0;
    time_t lastUsed = 0;
    uint64_t playedMs = 0;
    uint32_t playedCount = 0;
    bool inverted = false;
};

struct DeckInfo {
    std::string path;
    std::string name;
    int cardCount = 0;
};

struct SessionStats {
    int totalNew = 0;
    int totalLearning = 0;
    int totalReview = 0;
    int reviewsDoneToday = 0;
    int newDoneToday = 0;
    int sessionAgain = 0;
    int sessionHard = 0;
    int sessionGood = 0;
    int sessionEasy = 0;
    int sessionLeeched = 0;
    int dueCount = 0;
};

} // namespace sanki

#endif // CARD_MODEL_HPP
