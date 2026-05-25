// sanki-android: FTS5 full-text search engine
// Uses SQLite FTS5 already compiled into sanki-core.so

#ifndef SEARCH_ENGINE_HPP
#define SEARCH_ENGINE_HPP

#include "card_model.hpp"
#include <string>
#include <vector>
#include <sqlite3.h>

namespace sanki {

struct SearchResult {
    uint64_t noteId = 0;
    std::string frontSnippet;  // highlighted excerpt from front
    std::string backSnippet;   // highlighted excerpt from back
    std::string deckName;
    double score = 0.0;
};

class SearchEngine {
public:
    SearchEngine();
    ~SearchEngine();

    // Initialize FTS5 index for a deck
    bool indexDeck(const std::string& deckPath, sqlite3* deckDb);

    // Search all indexed decks. Returns results sorted by relevance.
    std::vector<SearchResult> search(const std::string& query, int limit = 50);

    // Clear all indexes
    void clearIndexes();

    // Number of indexed cards
    int indexedCardCount() const;

private:
    sqlite3* ftsDb_ = nullptr;
    bool createFtsTables();
};

} // namespace sanki

#endif // SEARCH_ENGINE_HPP
