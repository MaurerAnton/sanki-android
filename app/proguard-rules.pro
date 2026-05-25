# sanki proguard rules
-keep class io.github.maureranton.sanki.bridge.SankiBridge { *; }
-keepclassmembers class io.github.maureranton.sanki.bridge.SankiBridge {
    native <methods>;
}
