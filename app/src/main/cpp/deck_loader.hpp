// sanki-android: SQLite-based deck loader (Qt-free)
// Reads Anki .apkg SQLite databases directly

#ifndef DECK_LOADER_HPP
#define DECK_LOADER_HPP

#include "card_model.hpp"
#include <string>
#include <vector>
#include <sqlite3.h>

namespace sanki {

class DeckLoader {
public:
    DeckLoader();
    ~DeckLoader();

    // Open a deck directory (extracted .apkg or Anki profile)
    bool openDeck(const std::string& path);

    // Get all cards from the deck as a list of Card structs
    std::vector<Card> loadAllCards(uint32_t deckId);

    // Get deck name
    std::string deckName() const { return name_; }

    // Get card count from DB
    int cardCount() const;

    // Get the raw fields (question/answer) for a card by note ID
    bool getCardFields(uint64_t noteId, std::string& flds);

    // Get the media file path for media references
    std::string mediaFilePath() const { return mediaPath_; }

    // Get the deck path
    std::string deckPath() const { return deckPath_; }

    // Check if deck is valid/open
    bool isOpen() const { return db_ != nullptr; }

    // Close the deck
    void close();

    // Static: extract .apkg to temp directory, return deck path
    static std::string extractApkg(const std::string& apkgPath, const std::string& destDir);

    // Static: list all deck directories in storage
    static std::vector<std::string> listDecks(const std::string& storageDir);

private:
    sqlite3* db_ = nullptr;
    std::string deckPath_;
    std::string name_;
    std::string mediaPath_;

    bool openDatabase(const std::string& dbPath);
};

} // namespace sanki

#endif // DECK_LOADER_HPP
