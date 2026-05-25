// sanki-android: Core API implementation

#include "sanki_core.hpp"
#include <sstream>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace sanki {

SankiCore::SankiCore(const std::string& dataDir)
    : dataDir_(dataDir), decksDir_(dataDir + "/decks"),
      sessionManager_(dataDir) {
    fs::create_directories(decksDir_);
}

std::vector<DeckInfo> SankiCore::listDecks() {
    std::vector<DeckInfo> result;
    auto paths = DeckLoader::listDecks(decksDir_);
    for (const auto& path : paths) {
        DeckInfo info;
        info.path = path;
        info.name = fs::path(path).filename().string();

        DeckLoader loader;
        if (loader.openDeck(path)) {
            info.cardCount = loader.cardCount();
            loader.close();
        }
        result.push_back(info);
    }
    return result;
}

bool SankiCore::importApkg(const std::string& apkgPath) {
    std::string dest = DeckLoader::extractApkg(apkgPath, decksDir_);
    return !dest.empty();
}

bool SankiCore::deleteDeck(const std::string& deckPath) {
    return fs::remove_all(deckPath) > 0;
}

bool SankiCore::createSession(const std::string& name,
                                const std::vector<std::string>& deckPaths,
                                DeckMode mode) {
    std::vector<Card> allCards;

    for (size_t i = 0; i < deckPaths.size(); i++) {
        DeckLoader loader;
        if (!loader.openDeck(deckPaths[i])) continue;

        auto cards = loader.loadAllCards(static_cast<uint32_t>(i));
        for (auto& c : cards) {
            allCards.push_back(c);
        }
        loader.close();
    }

    if (allCards.empty()) return false;

    currentSession_ = sessionManager_.createSession(name, deckPaths, allCards, mode);
    scheduler_.setSession(currentSession_);

    // Load existing SM-2 data if any
    auto sm2Data = sessionManager_.loadSm2Data(name);
    scheduler_.setCardDataMap(sm2Data);

    sessionManager_.saveSession(currentSession_);
    sessionLoaded_ = true;
    return true;
}

std::vector<std::string> SankiCore::listSessions() {
    return sessionManager_.listSessions();
}

bool SankiCore::deleteSession(const std::string& name) {
    return sessionManager_.deleteSession(name);
}

bool SankiCore::loadSession(const std::string& name) {
    auto sm2Data = sessionManager_.loadSm2Data(name);
    scheduler_.setCardDataMap(sm2Data);

    // Load session info from file
    std::string path = sessionManager_.dataDir() + "/sessions/" + name + ".json";
    std::ifstream file(path);
    if (!file) return false;

    // Reconstruct cards from deck paths (stored in session file)
    // For now, just load SM-2 data — session reconstruction needs more logic
    // TODO: full session reload from JSON

    sessionLoaded_ = true;
    return true;
}

std::string SankiCore::formatCardJson(const Card& card, bool showBack) {
    std::ostringstream json;
    json << "{";
    json << "\"id\":" << card.id;
    json << ",\"deckId\":" << card.deckId;
    json << ",\"count\":" << card.count;

    std::string text = showBack ? card.backField : card.frontField;
    // Escape for JSON
    std::string escaped;
    for (char c : text) {
        switch (c) {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += c;
        }
    }
    json << ",\"text\":\"" << escaped << "\"";
    json << ",\"showBack\":" << (showBack ? "true" : "false");
    json << "}";
    return json.str();
}

std::string SankiCore::getNextCard() {
    bool isReversed = false;
    int idx = scheduler_.pickNextCard(&isReversed);
    if (idx < 0) return ""; // No more cards

    const Card* card = scheduler_.currentCard();
    if (!card) return "";

    // Build JSON response with both front and back
    std::ostringstream json;
    json << "{";
    json << "\"id\":" << card->id;
    json << ",\"deckId\":" << card->deckId;
    json << ",\"count\":" << card->count;
    json << ",\"isReversed\":" << (isReversed ? "true" : "false");

    auto escape = [](const std::string& s) -> std::string {
        std::string out;
        for (char c : s) {
            switch (c) {
                case '"': out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default: out += c;
            }
        }
        return out;
    };

    std::string frontText = card->frontField;
    std::string backText = card->backField;

    if (isReversed) std::swap(frontText, backText);

    json << ",\"frontText\":\"" << escape(frontText) << "\"";
    json << ",\"backText\":\"" << escape(backText) << "\"";

    // Stats
    auto stats = scheduler_.getStats();
    json << ",\"stats\":{";
    json << "\"new\":" << stats.totalNew;
    json << ",\"learning\":" << stats.totalLearning;
    json << ",\"review\":" << stats.totalReview;
    json << ",\"due\":" << stats.dueCount;
    json << "}";

    // SM-2 state for current card
    std::string key = isReversed ?
        Sm2Scheduler::makeKeyRev(card->id, card->deckId) :
        Sm2Scheduler::makeKey(card->id, card->deckId);
    auto& map = scheduler_.cardDataMap();
    auto it = map.find(key);
    if (it != map.end()) {
        json << ",\"state\":\"" << Sm2Scheduler::stateName(it->second.state) << "\"";
        json << ",\"interval\":" << it->second.interval;
        json << ",\"easeFactor\":" << it->second.easeFactor;
        json << ",\"reps\":" << it->second.reps;
        json << ",\"lapses\":" << it->second.lapses;
    }

    json << "}";
    return json.str();
}

bool SankiCore::answerCard(int quality) {
    if (quality < 1 || quality > 4) return false;
    scheduler_.answerCard(quality);
    return true;
}

bool SankiCore::canUndo() {
    return scheduler_.canUndo();
}

bool SankiCore::undo() {
    scheduler_.undo();
    return true;
}

SessionStats SankiCore::getStats() {
    return scheduler_.getStats();
}

std::string SankiCore::getSessionSummary() {
    auto stats = scheduler_.getStats();
    std::ostringstream json;
    json << "{";
    json << "\"new\":" << stats.totalNew;
    json << ",\"learning\":" << stats.totalLearning;
    json << ",\"review\":" << stats.totalReview;
    json << ",\"due\":" << stats.dueCount;
    json << ",\"again\":" << stats.sessionAgain;
    json << ",\"hard\":" << stats.sessionHard;
    json << ",\"good\":" << stats.sessionGood;
    json << ",\"easy\":" << stats.sessionEasy;
    json << ",\"leeched\":" << stats.sessionLeeched;
    json << ",\"reviewsToday\":" << stats.reviewsDoneToday;
    json << ",\"newToday\":" << stats.newDoneToday;
    json << "}";
    return json.str();
}

void SankiCore::setCramMode(bool cram) {
    scheduler_.setCramMode(cram);
}

void SankiCore::saveState() {
    if (sessionLoaded_) {
        sessionManager_.saveSm2Data(currentSession_.name,
                                     scheduler_.cardDataMap());
    }
}

void SankiCore::loadState(const std::string& sessionName) {
    auto data = sessionManager_.loadSm2Data(sessionName);
    scheduler_.setCardDataMap(data);
}

} // namespace sanki
