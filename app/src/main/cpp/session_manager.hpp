// sanki-android: Session persistence with JSON

#ifndef SESSION_MANAGER_HPP
#define SESSION_MANAGER_HPP

#include "card_model.hpp"
#include "sm2_scheduler.hpp"
#include <string>
#include <vector>
#include <map>

namespace sanki {

class SessionManager {
public:
    SessionManager(const std::string& dataDir);

    // Create a new session
    Session createSession(const std::string& name,
                          const std::vector<std::string>& deckPaths,
                          const std::vector<Card>& cards,
                          DeckMode mode = DeckMode::SM2);

    // Save session to disk (JSON)
    bool saveSession(const Session& session);

    // Save SM-2 card data for a session
    bool saveSm2Data(const std::string& sessionName,
                     const std::map<std::string, Sm2CardData>& data);

    // Load SM-2 card data for a session
    std::map<std::string, Sm2CardData> loadSm2Data(const std::string& sessionName);

    // List all saved sessions
    std::vector<std::string> listSessions();

    // Load a session
    bool loadSession(const std::string& name, Session& session);

    // Delete a session
    bool deleteSession(const std::string& name);

    // Export session cards to .apkg (Anki-compatible)
    std::string exportToApkg(const std::string& sessionName);

    // Get data directory
    std::string dataDir() const { return dataDir_; }

private:
    std::string dataDir_;
    std::string sessionsDir_;
    std::string decksDir_;

    std::string sessionFilePath(const std::string& name) const;
    std::string sm2DataFilePath(const std::string& sessionName) const;
};

} // namespace sanki

#endif // SESSION_MANAGER_HPP
