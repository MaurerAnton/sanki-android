package io.github.maureranton.sanki.ui.theme

import android.os.Build
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext

// Sanki brand colors — warm, paper-like tones
val SankiOrange = Color(0xFFE67E22)
val SankiOrangeDark = Color(0xFFD35400)
val SankiWarm = Color(0xFFFDF6E3)
val SankiWarmDark = Color(0xFF2C2820)
val SankiPaper = Color(0xFFFFF8EE)
val SankiPaperDark = Color(0xFF1E1B16)
val SankiGreen = Color(0xFF27AE60)
val SankiRed = Color(0xFFE74C3C)
val SankiBlue = Color(0xFF2980B9)

private val LightColorScheme = lightColorScheme(
    primary = SankiOrange,
    onPrimary = Color.White,
    primaryContainer = Color(0xFFFFDCC0),
    secondary = SankiBlue,
    onSecondary = Color.White,
    tertiary = SankiGreen,
    background = SankiPaper,
    surface = SankiWarm,
    surfaceVariant = Color(0xFFF5EDE0),
    error = SankiRed,
)

private val DarkColorScheme = darkColorScheme(
    primary = SankiOrange,
    onPrimary = Color.Black,
    primaryContainer = SankiOrangeDark,
    secondary = Color(0xFF6BB5E0),
    onSecondary = Color.Black,
    tertiary = Color(0xFF5DD39E),
    background = SankiPaperDark,
    surface = SankiWarmDark,
    surfaceVariant = Color(0xFF2D2820),
    error = Color(0xFFFF6B5E),
)

@Composable
fun SankiTheme(
    darkTheme: Boolean = isSystemInDarkTheme(),
    dynamicColor: Boolean = true,
    content: @Composable () -> Unit
) {
    val colorScheme = when {
        dynamicColor && Build.VERSION.SDK_INT >= Build.VERSION_CODES.S -> {
            val context = LocalContext.current
            if (darkTheme) dynamicDarkColorScheme(context) else dynamicLightColorScheme(context)
        }
        darkTheme -> DarkColorScheme
        else -> LightColorScheme
    }

    MaterialTheme(
        colorScheme = colorScheme,
        typography = Typography(),
        content = content
    )
}
