package io.github.maureranton.sanki.ui.screens

import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SyncScreen(
    onBack: () -> Unit
) {
    var serverAddress by remember { mutableStateOf("192.168.1.1:8766") }
    var isSyncing by remember { mutableStateOf(false) }
    var syncProgress by remember { mutableStateOf(0f) }
    var statusText by remember { mutableStateOf("Ready to sync") }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Sync") },
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
                .padding(32.dp),
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.Center
        ) {
            Icon(
                Icons.Default.Sync,
                contentDescription = null,
                modifier = Modifier.size(72.dp),
                tint = if (isSyncing)
                    MaterialTheme.colorScheme.primary
                else
                    MaterialTheme.colorScheme.onSurface.copy(alpha = 0.4f)
            )

            Spacer(modifier = Modifier.height(24.dp))
            Text(
                "Anki Sync",
                style = MaterialTheme.typography.headlineMedium,
                fontWeight = FontWeight.Bold
            )
            Spacer(modifier = Modifier.height(8.dp))
            Text(
                "Sync with sanki-sync server on your PC",
                style = MaterialTheme.typography.bodyMedium,
                color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.6f)
            )

            Spacer(modifier = Modifier.height(32.dp))

            OutlinedTextField(
                value = serverAddress,
                onValueChange = { serverAddress = it },
                label = { Text("Server address") },
                placeholder = { Text("ip:port") },
                modifier = Modifier.fillMaxWidth(),
                singleLine = true,
                enabled = !isSyncing
            )

            Spacer(modifier = Modifier.height(16.dp))

            if (isSyncing) {
                LinearProgressIndicator(
                    progress = { syncProgress },
                    modifier = Modifier.fillMaxWidth()
                )
                Spacer(modifier = Modifier.height(8.dp))
                Text(statusText, style = MaterialTheme.typography.bodySmall)
            }

            Spacer(modifier = Modifier.height(24.dp))

            Button(
                onClick = {
                    if (!isSyncing) {
                        isSyncing = true
                        syncProgress = 0f
                        statusText = "Connecting..."
                        // In a full implementation, this would connect to the sync server
                        // For now, simulate
                    } else {
                        isSyncing = false
                        statusText = "Sync cancelled"
                    }
                },
                modifier = Modifier
                    .fillMaxWidth()
                    .height(52.dp)
            ) {
                if (isSyncing) {
                    CircularProgressIndicator(
                        modifier = Modifier.size(24.dp),
                        color = MaterialTheme.colorScheme.onPrimary
                    )
                } else {
                    Icon(Icons.Default.Sync, null)
                }
                Spacer(modifier = Modifier.width(8.dp))
                Text(if (isSyncing) "Syncing..." else "Start Sync")
            }

            Spacer(modifier = Modifier.height(16.dp))

            Text(
                "Setup: Run sanki-sync on your PC first.\nInstall AnkiConnect addon in Anki desktop.",
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.4f),
                textAlign = androidx.compose.ui.text.style.TextAlign.Center
            )
        }
    }
}
