plugins {
    id("com.android.application")
}

android {
    namespace = "com.jnmaini.overlay"
    compileSdk = 35
    ndkVersion = "28.2.13676358"

    defaultConfig {
        applicationId = "com.jnmaini.overlay"
        minSdk = 28
        targetSdk = 35
        versionCode = 1
        versionName = "1.0"
        ndk { abiFilters += setOf("arm64-v8a") }
    }

    buildTypes {
        getByName("debug") { isMinifyEnabled = false }
        getByName("release") {
            isMinifyEnabled = false
            isDebuggable = false
            signingConfig = signingConfigs.getByName("debug")
            ndk { debugSymbolLevel = "NONE" }
        }
    }

    externalNativeBuild {
        ndkBuild {
            path = file("../jni/Android.mk")
        }
    }

    packaging {
        jniLibs { useLegacyPackaging = true }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
}
