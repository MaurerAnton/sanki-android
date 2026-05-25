// sanki-android: JNI bridge between Java/Kotlin and C++ core

#include <jni.h>
#include <string>
#include <cstring>
#include <ctime>
#include <android/log.h>

#include "sanki_core.hpp"
#include "fsrs_scheduler.hpp"
#include "search_engine.hpp"
#include "stats_engine.hpp"

#define TAG "sanki-native"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static sanki::SankiCore* g_core = nullptr;
static sanki::FsrsScheduler* g_fsrs = nullptr;
static sanki::SearchEngine* g_search = nullptr;
static sanki::StatsEngine* g_stats = nullptr;

// Helper: convert jstring to std::string
static std::string jstringToString(JNIEnv* env, jstring jstr) {
    if (!jstr) return "";
    const char* chars = env->GetStringUTFChars(jstr, nullptr);
    std::string result(chars);
    env->ReleaseStringUTFChars(jstr, chars);
    return result;
}

// Helper: convert std::string to jstring
static jstring stringToJstring(JNIEnv* env, const std::string& str) {
    return env->NewStringUTF(str.c_str());
}

extern "C" {

JNIEXPORT jboolean JNICALL
Java_io_github_maureranton_sanki_bridge_SankiBridge_nativeInit(
    JNIEnv* env, jclass clazz, jstring dataDir) {
    try {
        std::string dir = jstringToString(env, dataDir);
        LOGI("Initializing sanki core at: %s", dir.c_str());
        if (g_core) delete g_core;
        g_core = new sanki::SankiCore(dir);
        return JNI_TRUE;
    } catch (const std::exception& e) {
        LOGE("nativeInit failed: %s", e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT jstring JNICALL
Java_io_github_maureranton_sanki_bridge_SankiBridge_nativeListDecks(
    JNIEnv* env, jclass clazz) {
    if (!g_core) return stringToJstring(env, "[]");

    try {
        auto decks = g_core->listDecks();
        std::string json = "[";
        for (size_t i = 0; i < decks.size(); i++) {
            if (i > 0) json += ",";
            json += "{\"path\":\"" + decks[i].path
                  + "\",\"name\":\"" + decks[i].name
                  + "\",\"cardCount\":" + std::to_string(decks[i].cardCount)
                  + "}";
        }
        json += "]";
        return stringToJstring(env, json);
    } catch (const std::exception& e) {
        LOGE("listDecks failed: %s", e.what());
        return stringToJstring(env, "[]");
    }
}

JNIEXPORT jboolean JNICALL
Java_io_github_maureranton_sanki_bridge_SankiBridge_nativeImportApkg(
    JNIEnv* env, jclass clazz, jstring apkgPath) {
    if (!g_core) return JNI_FALSE;
    try {
        std::string path = jstringToString(env, apkgPath);
        bool ok = g_core->importApkg(path);
        return ok ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        LOGE("importApkg failed: %s", e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT jboolean JNICALL
Java_io_github_maureranton_sanki_bridge_SankiBridge_nativeCreateSession(
    JNIEnv* env, jclass clazz, jstring name, jobjectArray deckPaths) {
    if (!g_core) return JNI_FALSE;

    try {
        std::string sname = jstringToString(env, name);
        std::vector<std::string> paths;
        jsize len = env->GetArrayLength(deckPaths);
        for (jsize i = 0; i < len; i++) {
            jstring jp = (jstring)env->GetObjectArrayElement(deckPaths, i);
            paths.push_back(jstringToString(env, jp));
            env->DeleteLocalRef(jp);
        }

        bool ok = g_core->createSession(sname, paths, sanki::DeckMode::SM2);
        return ok ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        LOGE("createSession failed: %s", e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT jstring JNICALL
Java_io_github_maureranton_sanki_bridge_SankiBridge_nativeListSessions(
    JNIEnv* env, jclass clazz) {
    if (!g_core) return stringToJstring(env, "[]");

    try {
        auto sessions = g_core->listSessions();
        std::string json = "[";
        for (size_t i = 0; i < sessions.size(); i++) {
            if (i > 0) json += ",";
            json += "\"" + sessions[i] + "\"";
        }
        json += "]";
        return stringToJstring(env, json);
    } catch (const std::exception& e) {
        LOGE("listSessions failed: %s", e.what());
        return stringToJstring(env, "[]");
    }
}

JNIEXPORT jboolean JNICALL
Java_io_github_maureranton_sanki_bridge_SankiBridge_nativeDeleteSession(
    JNIEnv* env, jclass clazz, jstring name) {
    if (!g_core) return JNI_FALSE;
    try {
        return g_core->deleteSession(jstringToString(env, name)) ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        LOGE("deleteSession failed: %s", e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT jboolean JNICALL
Java_io_github_maureranton_sanki_bridge_SankiBridge_nativeLoadSession(
    JNIEnv* env, jclass clazz, jstring name) {
    if (!g_core) return JNI_FALSE;
    try {
        return g_core->loadSession(jstringToString(env, name)) ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        LOGE("loadSession failed: %s", e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT jstring JNICALL
Java_io_github_maureranton_sanki_bridge_SankiBridge_nativeGetNextCard(
    JNIEnv* env, jclass clazz) {
    if (!g_core) return stringToJstring(env, "");
    try {
        std::string json = g_core->getNextCard();
        return stringToJstring(env, json);
    } catch (const std::exception& e) {
        LOGE("getNextCard failed: %s", e.what());
        return stringToJstring(env, "");
    }
}

JNIEXPORT jboolean JNICALL
Java_io_github_maureranton_sanki_bridge_SankiBridge_nativeAnswerCard(
    JNIEnv* env, jclass clazz, jint quality) {
    if (!g_core) return JNI_FALSE;
    try {
        return g_core->answerCard(quality) ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        LOGE("answerCard failed: %s", e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT jboolean JNICALL
Java_io_github_maureranton_sanki_bridge_SankiBridge_nativeCanUndo(
    JNIEnv* env, jclass clazz) {
    return g_core && g_core->canUndo() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_io_github_maureranton_sanki_bridge_SankiBridge_nativeUndo(
    JNIEnv* env, jclass clazz) {
    if (!g_core) return JNI_FALSE;
    try {
        return g_core->undo() ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        LOGE("undo failed: %s", e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT jstring JNICALL
Java_io_github_maureranton_sanki_bridge_SankiBridge_nativeGetStats(
    JNIEnv* env, jclass clazz) {
    if (!g_core) return stringToJstring(env, "{}");
    try {
        return stringToJstring(env, g_core->getSessionSummary());
    } catch (const std::exception& e) {
        LOGE("getStats failed: %s", e.what());
        return stringToJstring(env, "{}");
    }
}

JNIEXPORT void JNICALL
Java_io_github_maureranton_sanki_bridge_SankiBridge_nativeSaveState(
    JNIEnv* env, jclass clazz) {
    if (g_core) {
        try {
            g_core->saveState();
        } catch (const std::exception& e) {
            LOGE("saveState failed: %s", e.what());
        }
    }
}

JNIEXPORT void JNICALL
Java_io_github_maureranton_sanki_bridge_SankiBridge_nativeSetCramMode(
    JNIEnv* env, jclass clazz, jboolean cram) {
    if (g_core) g_core->setCramMode(cram == JNI_TRUE);
}

JNIEXPORT jstring JNICALL
Java_io_github_maureranton_sanki_bridge_SankiBridge_nativeGetConfig(
    JNIEnv* env, jclass clazz) {
    if (!g_core) return stringToJstring(env, "{}");

    auto& cfg = g_core->getSm2Config();
    std::string json = "{";
    json += "\"startingEase\":" + std::to_string(cfg.startingEase);
    json += ",\"easyBonus\":" + std::to_string(cfg.easyBonus);
    json += ",\"intervalModifier\":" + std::to_string(cfg.intervalModifier);
    json += ",\"hardIntervalMultiplier\":" + std::to_string(cfg.hardIntervalMultiplier);
    json += ",\"maxReviewsPerDay\":" + std::to_string(cfg.maxReviewsPerDay);
    json += ",\"leechThreshold\":" + std::to_string(cfg.leechThreshold);
    json += ",\"maxInterval\":" + std::to_string(cfg.maxInterval);
    json += ",\"graduatingInterval\":" + std::to_string(cfg.graduatingInterval);
    json += ",\"easyInterval\":" + std::to_string(cfg.easyInterval);
    json += ",\"reversedMode\":" + std::string(cfg.reversedMode ? "true" : "false");
    json += ",\"twoButtonMode\":" + std::string(cfg.twoButtonMode ? "true" : "false");

    // Learning steps
    json += ",\"learningSteps\":[";
    for (size_t i = 0; i < cfg.learningSteps.size(); i++) {
        if (i > 0) json += ",";
        json += std::to_string(cfg.learningSteps[i]);
    }
    json += "]";

    json += "}";
    return stringToJstring(env, json);
}

JNIEXPORT void JNICALL
Java_io_github_maureranton_sanki_bridge_SankiBridge_nativeSetConfig(
    JNIEnv* env, jclass clazz, jstring configJson) {
    if (!g_core) return;
    (void)configJson;
}

// ===== FSRS-5 Scheduler =====

JNIEXPORT void JNICALL
Java_io_github_maureranton_sanki_bridge_SankiBridge_nativeInitFsrs(
    JNIEnv* env, jclass clazz) {
    if (g_fsrs) delete g_fsrs;
    g_fsrs = new sanki::FsrsScheduler();
    if (!g_stats) g_stats = new sanki::StatsEngine();
    LOGI("FSRS-5 scheduler initialized");
}

JNIEXPORT jstring JNICALL
Java_io_github_maureranton_sanki_bridge_SankiBridge_nativeFsrsGetNextCard(
    JNIEnv* env, jclass clazz) {
    if (!g_fsrs) return stringToJstring(env, "");
    return stringToJstring(env, "{}");
    // Full implementation would return JSON card data
}

JNIEXPORT void JNICALL
Java_io_github_maureranton_sanki_bridge_SankiBridge_nativeFsrsAnswerCard(
    JNIEnv* env, jclass clazz, jint rating) {
    if (!g_fsrs) return;
    g_fsrs->answerCard(rating);
    // Record in stats
    if (g_stats) {
        auto* card = g_fsrs->currentCard();
        auto* state = g_fsrs->currentState();
        if (card && state) {
            g_stats->recordReview(card->id, rating, state->stability, time(nullptr));
        }
    }
}

JNIEXPORT jstring JNICALL
Java_io_github_maureranton_sanki_bridge_SankiBridge_nativeFsrsGetStats(
    JNIEnv* env, jclass clazz) {
    if (!g_fsrs) return stringToJstring(env, "{}");
    char buf[256];
    snprintf(buf, sizeof(buf),
        "{\"due\":%d,\"new\":%d,\"review\":%d,\"learning\":%d}",
        g_fsrs->dueCount(), g_fsrs->newCount(),
        g_fsrs->reviewCount(), g_fsrs->learningCount());
    return stringToJstring(env, buf);
}

// ===== FTS5 Search =====

JNIEXPORT void JNICALL
Java_io_github_maureranton_sanki_bridge_SankiBridge_nativeInitSearch(
    JNIEnv* env, jclass clazz) {
    if (g_search) delete g_search;
    g_search = new sanki::SearchEngine();
    LOGI("FTS5 search engine initialized");
}

JNIEXPORT void JNICALL
Java_io_github_maureranton_sanki_bridge_SankiBridge_nativeIndexDeck(
    JNIEnv* env, jclass clazz, jstring deckPath) {
    if (!g_search) return;
    std::string path = jstringToString(env, deckPath);
    sanki::DeckLoader loader;
    if (loader.openDeck(path)) {
        // sqlite3_db_handle would be the internal handle — we need to access it
        // For now, reopen the DB directly
        std::string dbPath = path + "/collection.anki21";
        sqlite3* db = nullptr;
        if (sqlite3_open_v2(dbPath.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) == SQLITE_OK) {
            g_search->indexDeck(path, db);
            sqlite3_close(db);
            LOGI("Indexed deck: %s", path.c_str());
        }
    }
}

JNIEXPORT jstring JNICALL
Java_io_github_maureranton_sanki_bridge_SankiBridge_nativeSearch(
    JNIEnv* env, jclass clazz, jstring query) {
    if (!g_search) return stringToJstring(env, "[]");
    std::string q = jstringToString(env, query);
    auto results = g_search->search(q);

    std::string json = "[";
    for (size_t i = 0; i < results.size(); i++) {
        if (i > 0) json += ",";
        json += "{\"deck\":\"" + results[i].deckName
              + "\",\"front\":\"" + results[i].frontSnippet
              + "\",\"back\":\"" + results[i].backSnippet
              + "\",\"score\":" + std::to_string(results[i].score)
              + "}";
    }
    json += "]";
    return stringToJstring(env, json);
}

// ===== Statistics =====

JNIEXPORT jstring JNICALL
Java_io_github_maureranton_sanki_bridge_SankiBridge_nativeGetAllStats(
    JNIEnv* env, jclass clazz) {
    if (!g_stats) return stringToJstring(env, "{}");
    return stringToJstring(env, g_stats->toJson());
}

} // extern "C"
