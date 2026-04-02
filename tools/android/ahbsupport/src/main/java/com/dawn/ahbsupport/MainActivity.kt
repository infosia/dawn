package com.dawn.ahbsupport

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.webgpu.BackendType
import androidx.webgpu.FeatureLevel
import androidx.webgpu.GPURequestAdapterOptions
import androidx.webgpu.helper.initLibrary

class MainActivity : ComponentActivity() {
  init {
    initLibrary()
  }

  private val demos = listOf(
    DemoInfo("ZeroCopyRGBAImage") { modifier, options -> ZeroCopyRGBAImage(modifier, options) },
    DemoInfo("ZeroCopyRGBVideo") { modifier, options -> ZeroCopyRGBVideo(modifier, options) },
    DemoInfo("CopyRGBVideo") { modifier, options -> CopyRGBVideo(modifier, options) },
    DemoInfo("RenderTriangle") { modifier, options -> RenderTriangle(modifier, options) },
    DemoInfo("ZeroCopyVideoYuv") { modifier, options -> ZeroCopyVideoYuv(modifier, options) },
  )

  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)
    enableEdgeToEdge()
    setContent {
      val currentDemo = remember { mutableIntStateOf(0) }
      Scaffold { paddingValues ->
        Column(
          verticalArrangement = Arrangement.SpaceBetween,
          horizontalAlignment = Alignment.CenterHorizontally,
          modifier = Modifier.padding(paddingValues)
        ) {
          demos[currentDemo.intValue].renderer(
            Modifier
              .height(720.dp),
            Options(
              requestAdapterOptions = GPURequestAdapterOptions(
                featureLevel = FeatureLevel.Core,
                backendType = BackendType.Vulkan
              )
            )
          )
          Text(demos[currentDemo.intValue].label)
          Row(
            horizontalArrangement = Arrangement.SpaceAround,
            modifier = Modifier
              .fillMaxWidth()
              .padding(vertical = 10.dp)
          ) {
            Button(onClick = {
              currentDemo.intValue = (demos.size + currentDemo.intValue - 1) % demos.size
            }) {
              Text("Previous")
            }
            Button(onClick = {
              currentDemo.intValue = (currentDemo.intValue + 1) % demos.size
            }) {
              Text("Next")
            }
          }

        }

      }
    }
  }
}

data class DemoInfo(
  val label: String,
  val renderer: @Composable (Modifier, Options) -> Unit,
)