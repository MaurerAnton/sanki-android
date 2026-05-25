package io.github.maureranton.sanki

import android.app.Application
import io.github.maureranton.sanki.bridge.SankiBridge

class SankiApp : Application() {
    override fun onCreate() {
        super.onCreate()
        instance = this
        // Initialize native core with app's data directory
        val dataDir = filesDir.absolutePath + "/sanki"
        SankiBridge.nativeInit(dataDir)
    }

    companion object {
        lateinit var instance: SankiApp
            private set
    }
}
