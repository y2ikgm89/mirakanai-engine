import org.gradle.api.DefaultTask
import org.gradle.api.file.DirectoryProperty
import org.gradle.api.file.RegularFileProperty
import org.gradle.api.tasks.InputDirectory
import org.gradle.api.tasks.InputFile
import org.gradle.api.tasks.Optional
import org.gradle.api.tasks.OutputDirectory
import org.gradle.api.tasks.PathSensitive
import org.gradle.api.tasks.PathSensitivity
import org.gradle.api.tasks.TaskAction
import org.gradle.jvm.toolchain.JavaLanguageVersion

plugins {
    id("com.android.application")
}

java {
    toolchain {
        languageVersion = JavaLanguageVersion.of(17)
    }
}

val mirakanaiGameName = providers.gradleProperty("mirakanai.game").orElse("sample_headless").get()
val mirakanaiRoot = file("../../..").canonicalFile
val mirakanaiGameRoot = mirakanaiRoot.resolve("games/$mirakanaiGameName")
val mirakanaiGameManifest = mirakanaiGameRoot.resolve("game.agent.json")
val mirakanaiGameAssets = mirakanaiGameRoot.resolve("assets")
val generatedGameAssets = layout.buildDirectory.dir("generated/mirakanai/assets")

fun environmentValue(name: String): String? =
    providers.environmentVariable(name).orNull?.takeIf { it.isNotBlank() }

val mkAndroidKeystore = environmentValue("MK_ANDROID_KEYSTORE")
val mkAndroidKeystorePassword = environmentValue("MK_ANDROID_KEYSTORE_PASSWORD")
val mkAndroidKeyAlias = environmentValue("MK_ANDROID_KEY_ALIAS")
val mkAndroidKeyPassword = environmentValue("MK_ANDROID_KEY_PASSWORD")
val mkAndroidReleaseSigningReady = listOf(
    mkAndroidKeystore,
    mkAndroidKeystorePassword,
    mkAndroidKeyAlias,
    mkAndroidKeyPassword,
).all { it != null }

abstract class PrepareGameAssetsTask : DefaultTask() {
    @get:InputFile
    @get:PathSensitive(PathSensitivity.RELATIVE)
    abstract val manifestFile: RegularFileProperty

    @get:Optional
    @get:InputDirectory
    @get:PathSensitive(PathSensitivity.RELATIVE)
    abstract val assetsDirectory: DirectoryProperty

    @get:OutputDirectory
    abstract val outputDirectory: DirectoryProperty

    @TaskAction
    fun prepare() {
        val output = outputDirectory.get().asFile
        project.delete(output)
        project.copy {
            from(manifestFile)
            into(output.resolve("game"))
            rename { "game.agent.json" }
        }

        if (assetsDirectory.isPresent) {
            project.copy {
                from(assetsDirectory)
                into(output.resolve("assets"))
            }
        }
    }
}

val prepareGameAssets by tasks.registering(PrepareGameAssetsTask::class) {
    manifestFile.set(layout.file(providers.provider { mirakanaiGameManifest }))
    if (mirakanaiGameAssets.isDirectory) {
        assetsDirectory.set(layout.dir(providers.provider { mirakanaiGameAssets }))
    }
    outputDirectory.set(generatedGameAssets)
}

android {
    namespace = "dev.mirakanai.android"
    compileSdk = 36
    compileSdkMinor = 1

    defaultConfig {
        applicationId = "dev.mirakanai.android"
        minSdk = 26
        targetSdk = 36
        versionCode = 1
        versionName = "0.1.0"

        externalNativeBuild {
            cmake {
                arguments.add("-DMK_ANDROID_GAME_NAME=$mirakanaiGameName")
                arguments.add("-DMK_ANDROID_GAME_MANIFEST=${mirakanaiGameManifest.invariantSeparatorsPath}")
                arguments.add("-DMK_ANDROID_MIRAKANAI_ROOT=${mirakanaiRoot.invariantSeparatorsPath}")
                cppFlags.add("-std=c++23")
                targets.add("mirakanai_android")
            }
        }

        ndk {
            abiFilters.add("arm64-v8a")
        }
    }

    signingConfigs {
        if (mkAndroidReleaseSigningReady) {
            create("releaseFromEnvironment") {
                storeFile = file(mkAndroidKeystore!!)
                storePassword = mkAndroidKeystorePassword
                keyAlias = mkAndroidKeyAlias
                keyPassword = mkAndroidKeyPassword
            }
        }
    }

    buildTypes {
        getByName("release") {
            if (mkAndroidReleaseSigningReady) {
                signingConfig = signingConfigs.getByName("releaseFromEnvironment")
            }
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "4.1.2"
        }
    }

    buildFeatures {
        prefab = true
    }

    packaging {
        jniLibs {
            useLegacyPackaging = false
        }
    }
}

dependencies {
    implementation("androidx.appcompat:appcompat:1.7.1")
    implementation("androidx.core:core:1.18.0")
    implementation("androidx.games:games-activity:3.0.4")
}

androidComponents {
    onVariants { variant ->
        val assets = requireNotNull(variant.sources.assets) {
            "Android assets source directories are unavailable for ${variant.name}"
        }
        assets.addGeneratedSourceDirectory(prepareGameAssets) { it.outputDirectory }
    }
}
