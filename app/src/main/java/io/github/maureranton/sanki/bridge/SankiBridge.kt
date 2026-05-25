package io.github.maureranton.sanki.bridge

import org.json.JSONArray
import org.json.JSONObject

/**
 * JNI bridge to the native sanki C++ core.
 * All native methods are implemented in jni_bridge.cpp.
 */
object SankiBridge {
    init {
        System.loadLibrary("sanki-core")
    }

    // Initialize the core with data directory
    @JvmStatic external fun nativeInit(dataDir: String): Boolean

    // Deck management
    @JvmStatic external fun nativeListDecks(): String
    @JvmStatic external fun nativeImportApkg(apkgPath: String): Boolean
    @JvmStatic external fun nativeCreateSession(name: String, deckPaths: Array<String>): Boolean

    // Session management
    @JvmStatic external fun nativeListSessions(): String
    @JvmStatic external fun nativeDeleteSession(name: String): Boolean
    @JvmStatic external fun nativeLoadSession(name: String): Boolean

    // Study
    @JvmStatic external fun nativeGetNextCard(): String
    @JvmStatic external fun nativeAnswerCard(quality: Int): Boolean
    @JvmStatic external fun nativeCanUndo(): Boolean
    @JvmStatic external fun nativeUndo(): Boolean

    // Stats
    @JvmStatic external fun nativeGetStats(): String

    // Persistence
    @JvmStatic external fun nativeSaveState()

    // Cram mode
    @JvmStatic external fun nativeSetCramMode(cram: Boolean)

    // Config
    @JvmStatic external fun nativeGetConfig(): String
    @JvmStatic external fun nativeSetConfig(configJson: String)

    // FSRS-5 Scheduler
    @JvmStatic external fun nativeInitFsrs()
    @JvmStatic external fun nativeFsrsGetNextCard(): String
    @JvmStatic external fun nativeFsrsAnswerCard(rating: Int)
    @JvmStatic external fun nativeFsrsGetStats(): String

    // FTS5 Search
    @JvmStatic external fun nativeInitSearch()
    @JvmStatic external fun nativeIndexDeck(deckPath: String)
    @JvmStatic external fun nativeSearch(query: String): String

    // Statistics
    @JvmStatic external fun nativeGetAllStats(): String

    // --- Kotlin-friendly wrappers ---

    data class DeckInfo(val path: String, val name: String, val cardCount: Int)

    data class CardData(
        val id: Long,
        val deckId: Int,
        val count: Int,
        val isReversed: Boolean,
        val frontText: String,
        val backText: String,
        val state: String = "",
        val interval: Long = 0,
        val easeFactor: Double = 0.0,
        val reps: Int = 0,
        val lapses: Int = 0
    )

    data class StudyStats(
        val new: Int, val learning: Int, val review: Int, val due: Int
    )

    data class SessionSummary(
        val new: Int, val learning: Int, val review: Int,
        val due: Int, val again: Int, val hard: Int,
        val good: Int, val easy: Int, val leeched: Int,
        val reviewsToday: Int, val newToday: Int
    )

    fun getDecks(): List<DeckInfo> {
        val json = nativeListDecks()
        val result = mutableListOf<DeckInfo>()
        try {
            val arr = JSONArray(json)
            for (i in 0 until arr.length()) {
                val obj = arr.getJSONObject(i)
                result.add(DeckInfo(
                    obj.getString("path"),
                    obj.getString("name"),
                    obj.getInt("cardCount")
                ))
            }
        } catch (_: Exception) {}
        return result
    }

    fun getSessions(): List<String> {
        val result = mutableListOf<String>()
        try {
            val arr = JSONArray(nativeListSessions())
            for (i in 0 until arr.length()) {
                result.add(arr.getString(i))
            }
        } catch (_: Exception) {}
        return result
    }

    fun getNextCard(): CardData? {
        val json = nativeGetNextCard()
        if (json.isEmpty()) return null
        try {
            val obj = JSONObject(json)
            val stats = obj.getJSONObject("stats")
            return CardData(
                id = obj.getLong("id"),
                deckId = obj.getInt("deckId"),
                count = obj.getInt("count"),
                isReversed = obj.optBoolean("isReversed", false),
                frontText = obj.getString("frontText"),
                backText = obj.getString("backText"),
                state = obj.optString("state", ""),
                interval = obj.optLong("interval", 0),
                easeFactor = obj.optDouble("easeFactor", 0.0),
                reps = obj.optInt("reps", 0),
                lapses = obj.optInt("lapses", 0)
            )
        } catch (_: Exception) {
            return null
        }
    }

    fun getSessionSummary(): SessionSummary {
        try {
            val obj = JSONObject(nativeGetStats())
            return SessionSummary(
                new = obj.getInt("new"),
                learning = obj.getInt("learning"),
                review = obj.getInt("review"),
                due = obj.getInt("due"),
                again = obj.getInt("again"),
                hard = obj.getInt("hard"),
                good = obj.getInt("good"),
                easy = obj.getInt("easy"),
                leeched = obj.getInt("leeched"),
                reviewsToday = obj.getInt("reviewsToday"),
                newToday = obj.getInt("newToday")
            )
        } catch (_: Exception) {
            return SessionSummary(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        }
    }
}
