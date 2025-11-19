package androidx.webgpu

import android.os.Build
import org.junit.Assume
import org.junit.rules.TestRule
import org.junit.runner.Description
import org.junit.runners.model.Statement

class ApiLevelSkipRule : TestRule {
  override fun apply(base: Statement, description: Description): Statement {
    val annotation = getAnnotation(description)
    if (annotation != null) {
      val minApi = annotation.minApi
      if (Build.VERSION.SDK_INT < minApi) {
        if (annotation.onlySkipOnEmulator && !EmulatorUtils.isEmulator) {
          // Don't skip if we're not on an emulator
        } else {
          return object : Statement() {
            override fun evaluate() {
              Assume.assumeTrue(
                "Skipping test due to API level requirement: " +
                  "minApi=${minApi}, deviceApi=${Build.VERSION.SDK_INT}",
                false,
              )
            }
          }
        }
      }
    }
    return base
  }

  private fun getAnnotation(description: Description): ApiRequirement? {
    return description.getAnnotation(ApiRequirement::class.java)
      ?: description.testClass?.getAnnotation(ApiRequirement::class.java)
  }
}
