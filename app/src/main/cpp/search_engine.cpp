// sanki-android: FTS5 search engine implementation

#include "search_engine.hpp"
#include <cstring>
#include <sstream>
#include <algorithm>

namespace sanki {

SearchEngine::SearchEngine() {
    // Create in-memory FTS database
    sqlite3_open(":memory:", &ftsDb_);
    createFtsTables();
}

SearchEngine::~SearchEngine() {
    if (ftsDb_) sqlite3_close(ftsDb_);
}

bool SearchEngine::createFtsTables() {
    if (!ftsDb_) return false;

    const char* sql = R"(
        CREATE VIRTUAL TABLE IF NOT EXISTS cards_fts USING fts5(
            deck_name,
            front_text,
            back_text,
            tags,
            tokenize='unicode61'
        );
    )";

    char* err = nullptr;
    int rc = sqlite3_exec(ftsDb_, sql, nullptr, nullptr, &err);
    if (rc != SQLITE_OK && err) {
        sqlite3_free(err);
        return false;
    }
    return true;
}

bool SearchEngine::indexDeck(const std::string& deckPath, sqlite3* deckDb) {
    if (!ftsDb_ || !deckDb) return false;

    // Extract deck name from path
    std::string deckName = deckPath;
    size_t pos = deckName.rfind('/');
    if (pos != std::string::npos) deckName = deckName.substr(pos + 1);

    // Read all notes and insert into FTS
    const char* sql = "SELECT id, flds, tags FROM notes";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(deckDb, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    // Begin transaction for speed
    sqlite3_exec(ftsDb_, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);

    const char* insertSql = R"(
        INSERT INTO cards_fts (deck_name, front_text, back_text, tags)
        VALUES (?, ?, ?, ?)
    )";
    sqlite3_stmt* insertStmt;
    sqlite3_prepare_v2(ftsDb_, insertSql, -1, &insertStmt, nullptr);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int64_t noteId = sqlite3_column_int64(stmt, 0);
        const char* flds = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* tags = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));

        std::string front, back;
        if (flds) {
            std::string f(flds);
            size_t sep = f.find('\x1F');
            if (sep != std::string::npos) {
                front = f.substr(0, sep);
                back = f.substr(sep + 1);
            } else {
                front = f;
            }
        }

        sqlite3_bind_text(insertStmt, 1, deckName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insertStmt, 2, front.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insertStmt, 3, back.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insertStmt, 4, tags ? tags : "", -1, SQLITE_TRANSIENT);

        sqlite3_step(insertStmt);
        sqlite3_reset(insertStmt);
    }

    sqlite3_finalize(insertStmt);
    sqlite3_finalize(stmt);
    sqlite3_exec(ftsDb_, "COMMIT", nullptr, nullptr, nullptr);

    return true;
}

std::vector<SearchResult> SearchEngine::search(const std::string& query, int limit) {
    std::vector<SearchResult> results;
    if (!ftsDb_ || query.empty()) return results;

    // FTS5 query with snippet
    const char* sql = R"(
        SELECT deck_name, snippet(cards_fts, 1, '<b>', '</b>', '...', 32),
               snippet(cards_fts, 2, '<b>', '</b>', '...', 32),
               rank
        FROM cards_fts
        WHERE cards_fts MATCH ?
        ORDER BY rank
        LIMIT ?
    )";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(ftsDb_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return results;

    sqlite3_bind_text(stmt, 1, query.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SearchResult r;
        const char* deck = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char* frontSnip = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* backSnip = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        r.score = sqlite3_column_double(stmt, 3);

        if (deck) r.deckName = deck;
        if (frontSnip) r.frontSnippet = frontSnip;
        if (backSnip) r.backSnippet = backSnip;

        results.push_back(r);
    }
    sqlite3_finalize(stmt);
    return results;
}

void SearchEngine::clearIndexes() {
    if (ftsDb_) {
        sqlite3_exec(ftsDb_, "DELETE FROM cards_fts", nullptr, nullptr, nullptr);
    }
}

int SearchEngine::indexedCardCount() const {
    if (!ftsDb_) return 0;
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(ftsDb_, "SELECT COUNT(*) FROM cards_fts", -1, &stmt, nullptr);
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return count;
}

} // namespace sanki
