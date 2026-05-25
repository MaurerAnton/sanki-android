package io.github.maureranton.sanki.ui.screens

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.foundation.clickable
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import io.github.maureranton.sanki.bridge.SankiBridge

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SettingsScreen(
    onBack: () -> Unit
) {
    var newCardsPerDay by remember { mutableStateOf("20") }
    var maxReviewsPerDay by remember { mutableStateOf("200") }
    var leechThreshold by remember { mutableStateOf("8") }
    var darkTheme by remember { mutableStateOf(true) }
    var showSyncDialog by remember { mutableStateOf(false) }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Settings") },
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.Default.ArrowBack, "Back")
                    }
                }
            )
        }
    ) { padding ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding)
                .verticalScroll(androidx.compose.foundation.rememberScrollState()),
            verticalArrangement = Arrangement.spacedBy(4.dp)
        ) {
            // SM-2 Settings
            Text(
                "SM-2 Scheduler",
                style = MaterialTheme.typography.titleSmall,
                fontWeight = FontWeight.Bold,
                color = MaterialTheme.colorScheme.primary,
                modifier = Modifier.padding(start = 16.dp, top = 16.dp, bottom = 8.dp)
            )

            OutlinedTextField(
                value = newCardsPerDay,
                onValueChange = { newCardsPerDay = it },
                label = { Text("New cards per day") },
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(horizontal = 16.dp),
                singleLine = true
            )

            OutlinedTextField(
                value = maxReviewsPerDay,
                onValueChange = { maxReviewsPerDay = it },
                label = { Text("Max reviews per day") },
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(horizontal = 16.dp),
                singleLine = true
            )

            OutlinedTextField(
                value = leechThreshold,
                onValueChange = { leechThreshold = it },
                label = { Text("Leech threshold (lapses)") },
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(horizontal = 16.dp),
                singleLine = true
            )

            Spacer(modifier = Modifier.height(16.dp))

            // Appearance
            Text(
                "Appearance",
                style = MaterialTheme.typography.titleSmall,
                fontWeight = FontWeight.Bold,
                color = MaterialTheme.colorScheme.primary,
                modifier = Modifier.padding(start = 16.dp, top = 8.dp, bottom = 8.dp)
            )

            ListItem(
                headlineContent = { Text("Dark theme") },
                trailingContent = {
                    Switch(checked = darkTheme, onCheckedChange = { darkTheme = it })
                }
            )

            Spacer(modifier = Modifier.height(16.dp))

            // Sync
            Text(
                "Sync",
                style = MaterialTheme.typography.titleSmall,
                fontWeight = FontWeight.Bold,
                color = MaterialTheme.colorScheme.primary,
                modifier = Modifier.padding(start = 16.dp, top = 8.dp, bottom = 8.dp)
            )

            ListItem(
                headlineContent = { Text("Sync with Anki") },
                supportingContent = { Text("Through sanki-sync server") },
                leadingContent = { Icon(Icons.Default.Sync, null) },
                modifier = Modifier.clickable { showSyncDialog = true }
            )

            Spacer(modifier = Modifier.height(16.dp))

            // About
            Text(
                "About",
                style = MaterialTheme.typography.titleSmall,
                fontWeight = FontWeight.Bold,
                color = MaterialTheme.colorScheme.primary,
                modifier = Modifier.padding(start = 16.dp, top = 8.dp, bottom = 8.dp)
            )

            ListItem(
                headlineContent = { Text("sanki-enhanced") },
                supportingContent = { Text("Version 0.3.0\nC++ core • Jetpack Compose UI\nSM-2 Scheduler • Anki compatible") },
                leadingContent = { Icon(Icons.Default.Info, null) }
            )
        }
    }
}

// Using androidx.compose.foundation.clickable directly
