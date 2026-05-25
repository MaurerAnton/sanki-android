// sanki-android: Session manager implementation

#include "session_manager.hpp"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <ctime>
#include <sys/stat.h>

namespace fs = std::filesystem;

namespace sanki {

// Minimal JSON writing helpers (avoid external deps for this size)
static std::string jsonEscape(const std::string& s) {
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
}

static std::string jsonUnescape(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            switch (s[i + 1]) {
                case '"': out += '"'; i++; break;
                case '\\': out += '\\'; i++; break;
                case 'n': out += '\n'; i++; break;
                case 'r': out += '\r'; i++; break;
                case 't': out += '\t'; i++; break;
                default: out += s[i];
            }
        } else {
            out += s[i];
        }
    }
    return out;
}

SessionManager::SessionManager(const std::string& dataDir)
    : dataDir_(dataDir) {
    sessionsDir_ = dataDir_ + "/sessions";
    decksDir_ = dataDir_ + "/decks";

    fs::create_directories(sessionsDir_);
    fs::create_directories(decksDir_);
}

std::string SessionManager::sessionFilePath(const std::string& name) const {
    return sessionsDir_ + "/" + name + ".json";
}

std::string SessionManager::sm2DataFilePath(const std::string& sessionName) const {
    return sessionsDir_ + "/" + sessionName + "_sm2.json";
}

Session SessionManager::createSession(const std::string& name,
                                       const std::vector<std::string>& deckPaths,
                                       const std::vector<Card>& cards,
                                       DeckMode mode) {
    Session s;
    s.name = name;
    s.mode = mode;
    s.deckPaths = deckPaths;
    s.cards = cards;
    s.created = time(nullptr);
    s.lastUsed = time(nullptr);
    s.playedMs = 0;
    s.playedCount = 0;
    return s;
}

bool SessionManager::saveSession(const Session& session) {
    std::string path = sessionFilePath(session.name);

    // Manual JSON serialization
    std::ostringstream json;
    json << "{\n";
    json << "  \"name\": \"" << jsonEscape(session.name) << "\",\n";
    json << "  \"mode\": " << static_cast<int>(session.mode) << ",\n";
    json << "  \"created\": " << session.created << ",\n";
    json << "  \"lastUsed\": " << session.lastUsed << ",\n";
    json << "  \"playedMs\": " << session.playedMs << ",\n";
    json << "  \"playedCount\": " << session.playedCount << ",\n";
    json << "  \"inverted\": " << (session.inverted ? "true" : "false") << ",\n";

    json << "  \"deckPaths\": [";
    for (size_t i = 0; i < session.deckPaths.size(); i++) {
        if (i > 0) json << ", ";
        json << "\"" << jsonEscape(session.deckPaths[i]) << "\"";
    }
    json << "],\n";

    json << "  \"cards\": [";
    for (size_t i = 0; i < session.cards.size(); i++) {
        if (i > 0) json << ", ";
        const auto& c = session.cards[i];
        json << "{\"id\":" << c.id
             << ",\"deckId\":" << c.deckId
             << ",\"count\":" << c.count << "}";
    }
    json << "]\n}\n";

    std::ofstream file(path);
    if (!file) return false;
    file << json.str();
    return true;
}

bool SessionManager::saveSm2Data(const std::string& sessionName,
                                  const std::map<std::string, Sm2CardData>& data) {
    std::string path = sm2DataFilePath(sessionName);
    std::ostringstream json;
    json << "{\n";
    json << "  \"sessionName\": \"" << jsonEscape(sessionName) << "\",\n";
    json << "  \"cards\": [\n";

    size_t count = 0;
    for (const auto& kv : data) {
        if (count++ > 0) json << ",\n";
        const auto& d = kv.second;
        json << "    {\"key\":\"" << jsonEscape(kv.first) << "\""
             << ",\"noteId\":" << d.noteId
             << ",\"deckId\":" << d.deckId
             << ",\"easeFactor\":" << d.easeFactor
             << ",\"interval\":" << d.interval
             << ",\"reps\":" << d.reps
             << ",\"lapses\":" << d.lapses
             << ",\"state\":" << static_cast<int>(d.state)
             << ",\"stepIndex\":" << d.stepIndex
             << ",\"leeched\":" << (d.leeched ? "true" : "false")
             << ",\"suspended\":" << (d.suspended ? "true" : "false")
             << ",\"flagged\":" << (d.flagged ? "true" : "false")
             << ",\"due\":" << d.due
             << ",\"lastReview\":" << d.lastReview
             << "}";
    }
    json << "\n  ]\n}\n";

    std::ofstream file(path);
    if (!file) return false;
    file << json.str();
    return true;
}

std::map<std::string, Sm2CardData> SessionManager::loadSm2Data(
    const std::string& sessionName) {
    std::map<std::string, Sm2CardData> result;
    std::string path = sm2DataFilePath(sessionName);

    std::ifstream file(path);
    if (!file) return result;

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

    // Very basic JSON parser for our known structure
    size_t pos = content.find("\"cards\": [");
    if (pos == std::string::npos) return result;
    pos = content.find("{", pos);

    while (pos != std::string::npos) {
        size_t end = content.find("}", pos);
        if (end == std::string::npos) break;

        std::string obj = content.substr(pos, end - pos + 1);
        Sm2CardData d;

        auto getInt = [&](const std::string& key) -> int64_t {
            size_t kp = obj.find("\"" + key + "\":");
            if (kp == std::string::npos) return 0;
            kp += key.length() + 3;
            while (kp < obj.size() && obj[kp] == ' ') kp++;
            return std::stoll(obj.substr(kp));
        };

        auto getDouble = [&](const std::string& key) -> double {
            size_t kp = obj.find("\"" + key + "\":");
            if (kp == std::string::npos) return 0.0;
            kp += key.length() + 3;
            while (kp < obj.size() && obj[kp] == ' ') kp++;
            return std::stod(obj.substr(kp));
        };

        auto getStr = [&](const std::string& key) -> std::string {
            size_t kp = obj.find("\"" + key + "\":\"");
            if (kp == std::string::npos) return "";
            kp += key.length() + 4;
            size_t ke = obj.find("\"", kp);
            if (ke == std::string::npos) return "";
            return jsonUnescape(obj.substr(kp, ke - kp));
        };

        auto getBool = [&](const std::string& key) -> bool {
            size_t kp = obj.find("\"" + key + "\":");
            if (kp == std::string::npos) return false;
            kp += key.length() + 3;
            while (kp < obj.size() && obj[kp] == ' ') kp++;
            return obj.substr(kp, 4) == "true";
        };

        std::string key = getStr("key");
        d.noteId = getInt("noteId");
        d.deckId = static_cast<uint32_t>(getInt("deckId"));
        d.easeFactor = getDouble("easeFactor");
        d.interval = getInt("interval");
        d.reps = static_cast<int32_t>(getInt("reps"));
        d.lapses = static_cast<int32_t>(getInt("lapses"));
        d.state = static_cast<Sm2State>(getInt("state"));
        d.stepIndex = static_cast<int32_t>(getInt("stepIndex"));
        d.leeched = getBool("leeched");
        d.suspended = getBool("suspended");
        d.flagged = getBool("flagged");
        d.due = getInt("due");
        d.lastReview = getInt("lastReview");

        result[key] = d;

        pos = content.find("{", end + 1);
    }

    return result;
}

std::vector<std::string> SessionManager::listSessions() {
    std::vector<std::string> names;
    if (!fs::exists(sessionsDir_)) return names;

    for (const auto& entry : fs::directory_iterator(sessionsDir_)) {
        std::string name = entry.path().filename().string();
        // Only plain session files, not _sm2 data
        if (name.find("_sm2") == std::string::npos && name.ends_with(".json")) {
            names.push_back(name.substr(0, name.size() - 5));
        }
    }
    std::sort(names.begin(), names.end());
    return names;
}

bool SessionManager::loadSession(const std::string& name, Session& session) {
    // For now, sessions are loaded implicitly by the scheduler.
    // Full session loading from JSON is complex — we rely on the card data
    // being reloaded via loadSm2Data + deck loading.
    (void)name;
    (void)session;
    return true;
}

bool SessionManager::deleteSession(const std::string& name) {
    std::string sp = sessionFilePath(name);
    std::string dp = sm2DataFilePath(name);
    bool ok = true;
    if (fs::exists(sp)) ok = fs::remove(sp) && ok;
    if (fs::exists(dp)) ok = fs::remove(dp) && ok;
    return ok;
}

std::string SessionManager::exportToApkg(const std::string& sessionName) {
    // Stub: export session as .apkg for Anki compatibility
    return "";
}

} // namespace sanki
