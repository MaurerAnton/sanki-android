// sanki-android: Main C++ core API
// Provides a clean C++ interface for the entire sanki engine

#ifndef SANKI_CORE_HPP
#define SANKI_CORE_HPP

#include "card_model.hpp"
#include "deck_loader.hpp"
#include "sm2_scheduler.hpp"
#include "session_manager.hpp"
#include <string>
#include <vector>
#include <functional>

namespace sanki {

class SankiCore {
public:
    SankiCore(const std::string& dataDir);

    // --- Deck management ---
    std::vector<DeckInfo> listDecks();
    bool importApkg(const std::string& apkgPath);
    bool deleteDeck(const std::string& deckPath);

    // --- Session management ---
    bool createSession(const std::string& name,
                       const std::vector<std::string>& deckPaths,
                       DeckMode mode = DeckMode::SM2);
    std::vector<std::string> listSessions();
    bool deleteSession(const std::string& name);
    bool loadSession(const std::string& name);

    // --- Study ---
    // Get the next card. Returns JSON with front/back fields and card info.
    std::string getNextCard();  // returns JSON or "" if done
    bool answerCard(int quality); // 1=Again, 2=Hard, 3=Good, 4=Easy
    bool canUndo();
    bool undo();

    // --- Stats ---
    SessionStats getStats();
    std::string getSessionSummary(); // JSON

    // --- Settings ---
    void setCramMode(bool cram);
    Sm2Config& getSm2Config() { return scheduler_.config(); }
    void setSm2Config(const Sm2Config& cfg) { scheduler_.setConfig(cfg); }

    // --- Persistence ---
    void saveState();
    void loadState(const std::string& sessionName);

    // --- Info ---
    std::string getDataDir() const { return dataDir_; }
    std::string getDecksDir() const { return decksDir_; }

private:
    std::string dataDir_;
    std::string decksDir_;

    SessionManager sessionManager_;
    DeckLoader deckLoader_;
    Sm2Scheduler scheduler_;
    Session currentSession_;
    bool sessionLoaded_ = false;

    std::string formatCardJson(const Card& card, bool showBack);
};

} // namespace sanki

#endif // SANKI_CORE_HPP
