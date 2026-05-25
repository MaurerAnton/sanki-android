// sanki-android: SQLite deck loader implementation

#include "deck_loader.hpp"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <algorithm>
#include <sys/stat.h>

namespace fs = std::filesystem;

namespace sanki {

DeckLoader::DeckLoader() {}

DeckLoader::~DeckLoader() {
    close();
}

void DeckLoader::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool DeckLoader::openDatabase(const std::string& dbPath) {
    close();
    int rc = sqlite3_open_v2(dbPath.c_str(), &db_,
        SQLITE_OPEN_READONLY, nullptr);
    return rc == SQLITE_OK && db_ != nullptr;
}

bool DeckLoader::openDeck(const std::string& path) {
    deckPath_ = path;

    // Extract name from path
    fs::path p(path);
    name_ = p.filename().string();

    // Find the Anki database file
    std::string dbPath;
    std::string collection21 = path + "/collection.anki21";
    std::string collection2 = path + "/collection.anki2";

    struct stat st;
    if (stat(collection21.c_str(), &st) == 0) {
        dbPath = collection21;
    } else if (stat(collection2.c_str(), &st) == 0) {
        dbPath = collection2;
    } else {
        return false;
    }

    if (!openDatabase(dbPath)) {
        return false;
    }

    // Set media path
    mediaPath_ = path + "/media";

    return true;
}

int DeckLoader::cardCount() const {
    if (!db_) return 0;

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_,
        "SELECT COUNT(id) FROM notes", -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return 0;

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return count;
}

std::vector<Card> DeckLoader::loadAllCards(uint32_t deckId) {
    std::vector<Card> cards;
    if (!db_) return cards;

    sqlite3_stmt* stmt;
    const char* sql =
        "SELECT n.id, n.flds FROM notes n "
        "JOIN cards c ON c.nid = n.id "
        "GROUP BY n.id "
        "ORDER BY n.id";

    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return cards;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Card card;
        card.id = sqlite3_column_int64(stmt, 0);
        card.deckId = deckId;

        const char* flds = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        if (flds) {
            std::string fields(flds);
            // Anki uses U+001F (Unit Separator) to split fields
            size_t sep = fields.find('\x1F');
            if (sep != std::string::npos) {
                card.frontField = fields.substr(0, sep);
                card.backField = fields.substr(sep + 1);
            } else {
                card.frontField = fields;
                card.backField = "";
            }
        }
        cards.push_back(card);
    }
    sqlite3_finalize(stmt);
    return cards;
}

bool DeckLoader::getCardFields(uint64_t noteId, std::string& flds) {
    if (!db_) return false;

    sqlite3_stmt* stmt;
    const char* sql = "SELECT flds FROM notes WHERE id = ?";
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_int64(stmt, 1, noteId);

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (text) {
            flds = text;
            found = true;
        }
    }
    sqlite3_finalize(stmt);
    return found;
}

std::string DeckLoader::extractApkg(const std::string& apkgPath,
                                     const std::string& destDir) {
    // .apkg is a zip file, use system unzip
    // Returns the path to the extracted deck directory

    fs::path apkg(apkgPath);
    std::string stem = apkg.stem().string();

    // Clean and create dest directory
    std::string deckDir = destDir + "/" + stem;

    // Remove existing if any
    fs::remove_all(deckDir);
    fs::create_directories(deckDir);

    // Unzip
    std::string cmd = "unzip -o -q \"" + apkgPath + "\" -d \"" + deckDir + "\"";
    int ret = system(cmd.c_str());

    if (ret == 0 && fs::exists(deckDir)) {
        return deckDir;
    }
    return "";
}

std::vector<std::string> DeckLoader::listDecks(const std::string& storageDir) {
    std::vector<std::string> decks;
    if (!fs::exists(storageDir)) return decks;

    for (const auto& entry : fs::directory_iterator(storageDir)) {
        if (entry.is_directory()) {
            decks.push_back(entry.path().string());
        }
    }
    std::sort(decks.begin(), decks.end());
    return decks;
}

} // namespace sanki
