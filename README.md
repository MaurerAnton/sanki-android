# sanki — Spaced Repetition Flashcards for Android

An Android flashcard app with a native C++ core for spaced repetition (SRS) scheduling, built with Jetpack Compose and JNI.

## Quick Start

Open in Android Studio, sync Gradle, and run on device/emulator.

```bash
./gradlew assembleDebug
```

## Features

- Spaced repetition scheduling (SM-2 algorithm via native C++ core)
- Create and manage flashcard decks
- Review cards with rating-based scheduling
- Jetpack Compose Material 3 UI
- Home screen widget for daily review count

## Architecture

- **Kotlin/Jetpack Compose** — UI layer
- **C++ via JNI** — Spaced repetition engine (native performance, cross-platform core)
- **Navigation Compose** — Screen routing

## Build

Requires: Android Studio, NDK 27+, SDK 35+
Minimum Android version: API 26 (Android 8.0)
