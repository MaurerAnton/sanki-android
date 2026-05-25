package io.github.maureranton.sanki.ui.screens

import android.annotation.SuppressLint
import android.content.Context
import android.webkit.URLUtil
import android.webkit.WebResourceRequest
import android.webkit.WebView
import android.webkit.WebViewClient
import android.widget.Toast
import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.viewinterop.AndroidView
import io.github.maureranton.sanki.bridge.SankiBridge
import kotlinx.coroutines.*
import java.io.File
import java.io.FileOutputStream
import java.net.HttpURLConnection
import java.net.URL

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SharedDecksScreen(
    onBack: () -> Unit,
    onDeckImported: () -> Unit
) {
    val context = LocalContext.current
    val scope = rememberCoroutineScope()
    var isLoading by remember { mutableStateOf(true) }
    var webView by remember { mutableStateOf<WebView?>(null) }
    var downloadProgress by remember { mutableStateOf("") }
    var showProgress by remember { mutableStateOf(false) }
    var progressPercent by remember { mutableStateOf(0f) }

    // Search state
    var searchQuery by remember { mutableStateOf("") }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Shared Decks", fontWeight = FontWeight.Bold) },
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.Default.ArrowBack, "Back")
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = MaterialTheme.colorScheme.primary,
                    titleContentColor = MaterialTheme.colorScheme.onPrimary
                )
            )
        }
    ) { padding ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding)
        ) {
            // Search bar
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(horizontal = 12.dp, vertical = 8.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                OutlinedTextField(
                    value = searchQuery,
                    onValueChange = { searchQuery = it },
                    modifier = Modifier.weight(1f),
                    placeholder = { Text("Search AnkiWeb decks...") },
                    singleLine = true,
                    trailingIcon = {
                        if (searchQuery.isNotEmpty()) {
                            IconButton(onClick = { searchQuery = "" }) {
                                Icon(Icons.Default.Clear, "Clear")
                            }
                        }
                    }
                )
                Spacer(modifier = Modifier.width(8.dp))
                IconButton(onClick = {
                    if (searchQuery.isNotBlank()) {
                        val encoded = java.net.URLEncoder.encode(searchQuery, "UTF-8")
                        webView?.loadUrl("https://ankiweb.net/shared/decks?search=$encoded")
                    }
                }) {
                    Icon(Icons.Default.Search, "Search")
                }
            }

            // Download progress bar
            if (showProgress) {
                Column(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(horizontal = 16.dp, vertical = 4.dp)
                ) {
                    LinearProgressIndicator(
                        progress = { progressPercent },
                        modifier = Modifier.fillMaxWidth()
                    )
                    Text(
                        downloadProgress,
                        style = MaterialTheme.typography.bodySmall,
                        color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.7f)
                    )
                }
            }

            // WebView
            Box(modifier = Modifier.weight(1f)) {
                AndroidView(
                    factory = { ctx ->
                        WebView(ctx).apply {
                            @SuppressLint("SetJavaScriptEnabled")
                            // Full browser capabilities for AnkiWeb login
                            settings.javaScriptEnabled = true
                            settings.domStorageEnabled = true
                            settings.databaseEnabled = true
                            settings.loadWithOverviewMode = true
                            settings.useWideViewPort = true
                            @Suppress("DEPRECATION")
                            settings.mixedContentMode =
                                android.webkit.WebSettings.MIXED_CONTENT_ALWAYS_ALLOW

                            // Standard Chrome Android user-agent (avoids bot detection)
                            settings.userAgentString =
                                "Mozilla/5.0 (Linux; Android 14) AppleWebKit/537.36 " +
                                "(KHTML, like Gecko) Chrome/125.0.6422.165 Mobile Safari/537.36"

                            // Enable cookies — required for AnkiWeb login
                            val wv = this
                            android.webkit.CookieManager.getInstance().apply {
                                setAcceptCookie(true)
                                setAcceptThirdPartyCookies(wv, true)
                            }

                            // Proper download interception via DownloadListener
                            setDownloadListener { downloadUrl, _, _, _, _ ->
                                scope.launch {
                                    downloadAndImport(
                                        context, downloadUrl,
                                        onProgress = { msg, pct ->
                                            showProgress = true
                                            downloadProgress = msg
                                            progressPercent = pct
                                        },
                                        onDone = { deckName ->
                                            showProgress = false
                                            Toast.makeText(
                                                context,
                                                "Imported: $deckName",
                                                Toast.LENGTH_SHORT
                                            ).show()
                                            onDeckImported()
                                        },
                                        onError = { msg ->
                                            showProgress = false
                                            Toast.makeText(context, msg, Toast.LENGTH_LONG).show()
                                        }
                                    )
                                }
                            }

                            webViewClient = object : WebViewClient() {
                                override fun onPageFinished(view: WebView?, url: String?) {
                                    isLoading = false
                                }

                                override fun shouldOverrideUrlLoading(
                                    view: WebView?,
                                    request: WebResourceRequest?
                                ): Boolean {
                                    val url = request?.url?.toString() ?: return false
                                    // Keep browsing within AnkiWeb, block external links
                                    return !url.contains("ankiweb.net")
                                }
                            }

                            loadUrl("https://ankiweb.net/shared/decks/")
                            webView = this
                        }
                    },
                    modifier = Modifier.fillMaxSize()
                )

                // Loading indicator
                if (isLoading) {
                    CircularProgressIndicator(
                        modifier = Modifier.align(Alignment.Center)
                    )
                }
            }

            // Quick actions
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(horizontal = 16.dp, vertical = 8.dp),
                horizontalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                FilterChip(
                    selected = false,
                    onClick = {
                        webView?.loadUrl("https://ankiweb.net/shared/decks/")
                    },
                    label = { Text("Popular") },
                    leadingIcon = { Icon(Icons.Default.TrendingUp, null, Modifier.size(16.dp)) }
                )
                FilterChip(
                    selected = false,
                    onClick = {
                        webView?.loadUrl("https://ankiweb.net/shared/decks/?page=1&sort=rating")
                    },
                    label = { Text("Top Rated") },
                    leadingIcon = { Icon(Icons.Default.Star, null, Modifier.size(16.dp)) }
                )
                FilterChip(
                    selected = false,
                    onClick = {
                        webView?.loadUrl("https://ankiweb.net/shared/decks/?page=1&sort=newest")
                    },
                    label = { Text("Newest") },
                    leadingIcon = { Icon(Icons.Default.NewReleases, null, Modifier.size(16.dp)) }
                )
            }
        }
    }
}

private suspend fun downloadAndImport(
    context: Context,
    url: String,
    onProgress: (String, Float) -> Unit,
    onDone: (String) -> Unit,
    onError: (String) -> Unit
) {
    try {
        withContext(Dispatchers.Main) {
            onProgress("Downloading deck...", 0.1f)
        }

        val fileName = URLUtil.guessFileName(url, null, null)
            ?: "shared_deck.apkg"
        val destFile = File(context.cacheDir, fileName)

        // All network I/O in one IO block — no Main thread calls
        val downloadOk = withContext(Dispatchers.IO) {
            try {
                val connection = URL(url).openConnection() as HttpURLConnection
                connection.connectTimeout = 30000
                connection.readTimeout = 60000
                connection.instanceFollowRedirects = true
                connection.connect()

                val contentLength = connection.contentLengthLong

                connection.inputStream.use { input ->
                    FileOutputStream(destFile).use { output ->
                        val buffer = ByteArray(8192)
                        var bytesRead: Int
                        var totalRead = 0L

                        while (input.read(buffer).also { bytesRead = it } != -1) {
                            output.write(buffer, 0, bytesRead)
                            totalRead += bytesRead

                            if (totalRead % (512 * 1024) == 0L && contentLength > 0) {
                                val pct = totalRead.toFloat() / contentLength.toFloat()
                                val mb = totalRead / (1024.0 * 1024.0)
                                val totalMb = contentLength / (1024.0 * 1024.0)
                                withContext(Dispatchers.Main) {
                                    onProgress(
                                        "Downloading... %.1f / %.1f MB".format(mb, totalMb),
                                        pct.coerceIn(0f, 0.95f)
                                    )
                                }
                            }
                        }
                    }
                }
                connection.disconnect()
                true
            } catch (e: Exception) {
                destFile.delete()
                throw e
            }
        }

        if (!downloadOk) {
            withContext(Dispatchers.Main) {
                onError("Download incomplete")
            }
            return
        }

        withContext(Dispatchers.Main) {
            onProgress("Importing deck...", 0.96f)
        }

        val ok = withContext(Dispatchers.IO) {
            SankiBridge.nativeImportApkg(destFile.absolutePath)
        }
        destFile.delete()

        withContext(Dispatchers.Main) {
            if (ok) {
                val deckName = fileName.removeSuffix(".apkg")
                onProgress("Done!", 1f)
                onDone(deckName)
            } else {
                onError("Failed to import deck. It may be corrupted or in an unsupported format.")
            }
        }
    } catch (e: Exception) {
        val msg = when {
            e.message != null -> e.message!!
            e is java.net.UnknownHostException -> "No internet connection"
            e is java.net.SocketTimeoutException -> "Connection timed out"
            e is java.io.IOException -> "Network error: ${e.javaClass.simpleName}"
            else -> "Download failed: ${e.javaClass.simpleName}"
        }
        withContext(Dispatchers.Main) {
            onError(msg)
        }
    }
}
