package io.github.maureranton.sanki.ui.screens

import android.net.Uri
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import io.github.maureranton.sanki.bridge.SankiBridge
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun DeckListScreen(
    onStartStudy: (deckPaths: List<String>) -> Unit,
    onViewStats: () -> Unit,
    onOpenSettings: () -> Unit
) {
    var decks by remember { mutableStateOf(SankiBridge.getDecks()) }
    var selectedDecks by remember { mutableStateOf(setOf<String>()) }
    var showImportDialog by remember { mutableStateOf(false) }
    val scope = rememberCoroutineScope()
    val context = LocalContext.current

    val apkgLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.GetContent()
    ) { uri: Uri? ->
        uri?.let {
            scope.launch {
                withContext(Dispatchers.IO) {
                    // Copy to internal storage first
                    val inputStream = context.contentResolver.openInputStream(it)
                    val destFile = File(context.cacheDir, "import.apkg")
                    inputStream?.use { input ->
                        destFile.outputStream().use { output ->
                            input.copyTo(output)
                        }
                    }
                    SankiBridge.nativeImportApkg(destFile.absolutePath)
                    destFile.delete()
                }
                decks = SankiBridge.getDecks()
            }
        }
    }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("sanki", fontWeight = FontWeight.Bold) },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = MaterialTheme.colorScheme.primary,
                    titleContentColor = MaterialTheme.colorScheme.onPrimary
                ),
                actions = {
                    IconButton(onClick = onViewStats) {
                        Icon(Icons.Default.BarChart, "Stats",
                            tint = MaterialTheme.colorScheme.onPrimary)
                    }
                    IconButton(onClick = onOpenSettings) {
                        Icon(Icons.Default.Settings, "Settings",
                            tint = MaterialTheme.colorScheme.onPrimary)
                    }
                }
            )
        },
        floatingActionButton = {
            Column(horizontalAlignment = Alignment.End) {
                if (selectedDecks.isNotEmpty()) {
                    FloatingActionButton(
                        onClick = { onStartStudy(selectedDecks.toList()) },
                        containerColor = MaterialTheme.colorScheme.tertiary
                    ) {
                        Icon(Icons.Default.PlayArrow, "Study")
                    }
                    Spacer(modifier = Modifier.height(16.dp))
                }
                FloatingActionButton(
                    onClick = { apkgLauncher.launch("application/zip") },
                    containerColor = MaterialTheme.colorScheme.primary
                ) {
                    Icon(Icons.Default.Add, "Import")
                }
            }
        }
    ) { padding ->
        if (decks.isEmpty()) {
            Box(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(padding),
                contentAlignment = Alignment.Center
            ) {
                Column(horizontalAlignment = Alignment.CenterHorizontally) {
                    Icon(
                        Icons.Default.ImportContacts,
                        contentDescription = null,
                        modifier = Modifier.size(64.dp),
                        tint = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.4f)
                    )
                    Spacer(modifier = Modifier.height(16.dp))
                    Text(
                        "No decks yet",
                        style = MaterialTheme.typography.headlineSmall,
                        color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.6f)
                    )
                    Spacer(modifier = Modifier.height(8.dp))
                    Text(
                        "Tap + to import an .apkg file",
                        style = MaterialTheme.typography.bodyMedium,
                        color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.4f)
                    )
                }
            }
        } else {
            LazyColumn(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(padding),
                contentPadding = PaddingValues(16.dp),
                verticalArrangement = Arrangement.spacedBy(12.dp)
            ) {
                items(decks) { deck ->
                    val isSelected = deck.path in selectedDecks
                    Card(
                        modifier = Modifier
                            .fillMaxWidth()
                            .clickable {
                                selectedDecks = if (isSelected) {
                                    selectedDecks - deck.path
                                } else {
                                    selectedDecks + deck.path
                                }
                            },
                        colors = CardDefaults.cardColors(
                            containerColor = if (isSelected)
                                MaterialTheme.colorScheme.primaryContainer
                            else
                                MaterialTheme.colorScheme.surface
                        )
                    ) {
                        Row(
                            modifier = Modifier
                                .fillMaxWidth()
                                .padding(16.dp),
                            verticalAlignment = Alignment.CenterVertically
                        ) {
                            Icon(
                                Icons.Default.Folder,
                                contentDescription = null,
                                tint = if (isSelected)
                                    MaterialTheme.colorScheme.primary
                                else
                                    MaterialTheme.colorScheme.onSurface.copy(alpha = 0.6f)
                            )
                            Spacer(modifier = Modifier.width(16.dp))
                            Column(modifier = Modifier.weight(1f)) {
                                Text(
                                    deck.name,
                                    style = MaterialTheme.typography.titleMedium,
                                    fontWeight = FontWeight.SemiBold
                                )
                                Text(
                                    "${deck.cardCount} cards",
                                    style = MaterialTheme.typography.bodySmall,
                                    color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.6f)
                                )
                            }
                            Checkbox(
                                checked = isSelected,
                                onCheckedChange = { checked ->
                                    selectedDecks = if (checked) {
                                        selectedDecks + deck.path
                                    } else {
                                        selectedDecks - deck.path
                                    }
                                }
                            )
                        }
                    }
                }
            }
        }
    }
}
