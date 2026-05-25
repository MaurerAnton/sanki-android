package io.github.maureranton.sanki.ui.screens

import androidx.compose.animation.*
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.layout.RowScope
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import android.text.Html
import androidx.compose.ui.text.SpanStyle
import androidx.compose.ui.text.buildAnnotatedString
import androidx.compose.ui.text.withStyle
import io.github.maureranton.sanki.bridge.SankiBridge
import kotlinx.coroutines.launch

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun StudyScreen(
    deckPaths: List<String>,
    onFinish: () -> Unit,
    onSessionSummary: (summary: SankiBridge.SessionSummary) -> Unit
) {
    var card by remember { mutableStateOf<SankiBridge.CardData?>(null) }
    var showBack by remember { mutableStateOf(false) }
    var sessionFinished by remember { mutableStateOf(false) }
    var stats by remember { mutableStateOf(SankiBridge.getSessionSummary()) }
    var canUndo by remember { mutableStateOf(false) }
    var showUndo by remember { mutableStateOf(false) }
    val scope = rememberCoroutineScope()

    // Create session if needed
    LaunchedEffect(deckPaths) {
        val sessionName = deckPaths.map { it.split("/").last() }.joinToString("_")
        SankiBridge.nativeCreateSession(sessionName, deckPaths.toTypedArray())
        card = SankiBridge.getNextCard()
    }

    suspend fun loadNext() {
        SankiBridge.nativeSaveState()
        card = SankiBridge.getNextCard()
        showBack = false
        stats = SankiBridge.getSessionSummary()
        canUndo = SankiBridge.nativeCanUndo()
        if (card == null) {
            sessionFinished = true
        }
    }

    fun answer(quality: Int) {
        SankiBridge.nativeAnswerCard(quality)
        scope.launch { loadNext() }
    }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Study") },
                navigationIcon = {
                    IconButton(onClick = {
                        SankiBridge.nativeSaveState()
                        onFinish()
                    }) {
                        Icon(Icons.Default.ArrowBack, "Back")
                    }
                },
                actions = {
                    if (canUndo) {
                        IconButton(onClick = {
                            SankiBridge.nativeUndo()
                            scope.launch { loadNext() }
                        }) {
                            Icon(Icons.Default.Undo, "Undo")
                        }
                    }
                }
            )
        }
    ) { padding ->
        if (sessionFinished) {
            // Session complete
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(padding)
                    .padding(32.dp),
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = Arrangement.Center
            ) {
                Icon(
                    Icons.Default.CheckCircle,
                    contentDescription = null,
                    modifier = Modifier.size(80.dp),
                    tint = MaterialTheme.colorScheme.tertiary
                )
                Spacer(modifier = Modifier.height(24.dp))
                Text(
                    "Session Complete!",
                    style = MaterialTheme.typography.headlineLarge,
                    fontWeight = FontWeight.Bold
                )
                Spacer(modifier = Modifier.height(8.dp))
                Text(
                    "${stats.reviewsToday + stats.newToday} cards reviewed",
                    style = MaterialTheme.typography.titleMedium,
                    color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.7f)
                )
                Spacer(modifier = Modifier.height(24.dp))

                // Stats summary
                Card(modifier = Modifier.fillMaxWidth()) {
                    Column(modifier = Modifier.padding(16.dp)) {
                        Text("This Session", fontWeight = FontWeight.Bold)
                        Spacer(modifier = Modifier.height(8.dp))
                        StatRow("Again", stats.again, MaterialTheme.colorScheme.error)
                        StatRow("Hard", stats.hard, androidx.compose.ui.graphics.Color.Unspecified)
                        StatRow("Good", stats.good, MaterialTheme.colorScheme.tertiary)
                        StatRow("Easy", stats.easy, MaterialTheme.colorScheme.primary)
                        if (stats.leeched > 0) StatRow("Leeched", stats.leeched, MaterialTheme.colorScheme.error)
                    }
                }

                Spacer(modifier = Modifier.height(24.dp))
                Button(onClick = onFinish) {
                    Text("Back to Decks")
                }
            }
        } else if (card != null) {
            val c = card!!
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(padding)
            ) {
                // Progress indicator
                LinearProgressIndicator(
                    progress = {
                        val total = stats.new + stats.learning + stats.review
                        val done = stats.reviewsToday + stats.newToday
                        if (total > 0) done.toFloat() / total.toFloat() else 0f
                    },
                    modifier = Modifier.fillMaxWidth()
                )

                // Stats bar
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(horizontal = 16.dp, vertical = 4.dp),
                    horizontalArrangement = Arrangement.SpaceEvenly
                ) {
                    Chip("New: ${stats.new}")
                    Chip("Learn: ${stats.learning}")
                    Chip("Review: ${stats.review}")
                    Chip("Due: ${stats.due}")
                }

                // Card content area
                Card(
                    modifier = Modifier
                        .weight(1f)
                        .fillMaxWidth()
                        .padding(16.dp),
                    colors = CardDefaults.cardColors(
                        containerColor = MaterialTheme.colorScheme.surfaceVariant
                    )
                ) {
                    Column(
                        modifier = Modifier
                            .fillMaxSize()
                            .verticalScroll(rememberScrollState())
                            .padding(24.dp),
                        horizontalAlignment = Alignment.CenterHorizontally,
                        verticalArrangement = Arrangement.Center
                    ) {
                        // Card state badge
                        if (c.state.isNotEmpty()) {
                            AssistChip(
                                onClick = {},
                                label = { Text(c.state) },
                                modifier = Modifier.padding(bottom = 12.dp)
                            )
                        }

                        // Card text (render HTML simply)
                        val text = if (showBack) c.backText else c.frontText
                        val plainText = if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.N) {
                            Html.fromHtml(text, Html.FROM_HTML_MODE_LEGACY).toString()
                        } else {
                            @Suppress("DEPRECATION")
                            Html.fromHtml(text).toString()
                        }

                        Text(
                            text = plainText,
                            style = MaterialTheme.typography.headlineMedium,
                            textAlign = TextAlign.Center,
                            fontWeight = FontWeight.Medium
                        )

                        if (c.isReversed) {
                            Spacer(modifier = Modifier.height(8.dp))
                            Text(
                                "(reversed)",
                                style = MaterialTheme.typography.labelMedium,
                                color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.5f)
                            )
                        }

                        // Card metadata
                        if (showBack) {
                            Spacer(modifier = Modifier.height(16.dp))
                            Row(horizontalArrangement = Arrangement.spacedBy(16.dp)) {
                                if (c.interval > 0) {
                                    val days = c.interval / 86400
                                    val text = if (days > 0) "${days}d" else "${c.interval / 3600}h"
                                    AssistChip(
                                        onClick = {},
                                        label = { Text(text) }
                                    )
                                }
                                AssistChip(
                                    onClick = {},
                                    label = { Text("${c.reps} reps") }
                                )
                            }
                        }
                    }
                }

                // Answer buttons
                if (showBack) {
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(horizontal = 12.dp, vertical = 8.dp),
                        horizontalArrangement = Arrangement.spacedBy(8.dp)
                    ) {
                        AnswerButton("Again", 1, MaterialTheme.colorScheme.error) { answer(1) }
                        AnswerButton("Hard", 2, MaterialTheme.colorScheme.secondary) { answer(2) }
                        AnswerButton("Good", 3, MaterialTheme.colorScheme.tertiary) { answer(3) }
                        AnswerButton("Easy", 4, MaterialTheme.colorScheme.primary) { answer(4) }
                    }
                } else {
                    // Show answer button
                    Button(
                        onClick = { showBack = true },
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(horizontal = 40.dp, vertical = 16.dp)
                            .height(56.dp),
                        colors = ButtonDefaults.buttonColors(
                            containerColor = MaterialTheme.colorScheme.primary
                        )
                    ) {
                        Icon(Icons.Default.Visibility, contentDescription = null)
                        Spacer(modifier = Modifier.width(8.dp))
                        Text("Show Answer", style = MaterialTheme.typography.titleMedium)
                    }
                }
            }
        }
    }
}

@Composable
private fun Chip(text: String) {
    Surface(
        shape = MaterialTheme.shapes.small,
        color = MaterialTheme.colorScheme.secondaryContainer
    ) {
        Text(
            text,
            modifier = Modifier.padding(horizontal = 8.dp, vertical = 4.dp),
            style = MaterialTheme.typography.labelSmall
        )
    }
}

@Composable
private fun StatRow(label: String, value: Int, color: androidx.compose.ui.graphics.Color) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 4.dp),
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Text(label)
        Text("$value", fontWeight = FontWeight.Bold, color = color)
    }
}

@Composable
private fun RowScope.AnswerButton(
    label: String,
    answer: Int,
    color: androidx.compose.ui.graphics.Color,
    onClick: () -> Unit
) {
    Button(
        onClick = onClick,
        modifier = Modifier
            .weight(1f)
            .height(52.dp),
        colors = ButtonDefaults.buttonColors(containerColor = color)
    ) {
        Text(label, fontWeight = FontWeight.Bold)
    }
}
