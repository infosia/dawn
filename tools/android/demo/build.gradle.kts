plugins {
  id("com.android.application")
  id("org.jetbrains.kotlin.android")
  id("org.jetbrains.kotlin.plugin.compose")
}

android {
  namespace = "com.webgpu.demo"
  compileSdk = 36

  defaultConfig {
    applicationId = "com.webgpu.demo"
    minSdk = 26
    targetSdk = 34
    versionCode = 1
    versionName = "1.0"

    testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
  }

  buildTypes {
    release {
      isMinifyEnabled = false
      proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
    }
  }
  compileOptions {
    sourceCompatibility = JavaVersion.VERSION_11
    targetCompatibility = JavaVersion.VERSION_11
  }
  kotlinOptions {
    jvmTarget = "11"
  }
  buildFeatures {
    compose = true
  }
  packaging {
    jniLibs {
      pickFirsts.add("lib/**/libc++_shared.so")
    }
  }
}

dependencies {
  implementation("androidx.core:core-ktx:1.18.0")
  implementation("androidx.lifecycle:lifecycle-runtime-ktx:2.10.0")
  implementation("androidx.activity:activity-compose:1.13.0")
  implementation(platform("androidx.compose:compose-bom:2024.09.00"))
  implementation("androidx.compose.ui:ui")
  implementation("androidx.compose.ui:ui-graphics")
  implementation("androidx.compose.ui:ui-tooling-preview")
  implementation("androidx.compose.material3:material3")
  implementation("io.github.kotlin-graphics:glm:0.9.9.1-12")
  implementation(project(":webgpu"))
  implementation("androidx.compose.ui:ui-graphics")
  implementation("androidx.compose.ui:ui-android-stubs")
}