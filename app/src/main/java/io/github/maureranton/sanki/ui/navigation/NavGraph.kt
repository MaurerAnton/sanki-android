package io.github.maureranton.sanki.ui.navigation

import androidx.compose.runtime.Composable
import androidx.navigation.NavHostController
import androidx.navigation.NavType
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.navArgument
import io.github.maureranton.sanki.ui.screens.*

sealed class Screen(val route: String) {
    object DeckList : Screen("decks")
    object Study : Screen("study/{deckPaths}") {
        fun createRoute(deckPaths: List<String>): String {
            return "study/${deckPaths.joinToString("|")}"
        }
    }
    object Stats : Screen("stats")
    object Settings : Screen("settings")
    object Sync : Screen("sync")
    object SharedDecks : Screen("shared")
}

@Composable
fun SankiNavGraph(navController: NavHostController) {
    NavHost(navController = navController, startDestination = Screen.DeckList.route) {
        composable(Screen.DeckList.route) {
            DeckListScreen(
                onStartStudy = { paths ->
                    navController.navigate(Screen.Study.createRoute(paths))
                },
                onViewStats = {
                    navController.navigate(Screen.Stats.route)
                },
                onOpenSettings = {
                    navController.navigate(Screen.Settings.route)
                },
                onOpenSharedDecks = {
                    navController.navigate(Screen.SharedDecks.route)
                }
            )
        }

        composable(
            route = Screen.Study.route,
            arguments = listOf(navArgument("deckPaths") { type = NavType.StringType })
        ) { backStackEntry ->
            val pathsStr = backStackEntry.arguments?.getString("deckPaths") ?: ""
            val paths = pathsStr.split("|").filter { it.isNotEmpty() }
            StudyScreen(
                deckPaths = paths,
                onFinish = { navController.popBackStack() },
                onSessionSummary = { /* handled internally */ }
            )
        }

        composable(Screen.Stats.route) {
            StatsScreen(onBack = { navController.popBackStack() })
        }

        composable(Screen.Settings.route) {
            SettingsScreen(onBack = { navController.popBackStack() })
        }

        composable(Screen.Sync.route) {
            SyncScreen(onBack = { navController.popBackStack() })
        }

        composable(Screen.SharedDecks.route) {
            SharedDecksScreen(
                onBack = { navController.popBackStack() },
                onDeckImported = { navController.popBackStack() }
            )
        }
    }
}
