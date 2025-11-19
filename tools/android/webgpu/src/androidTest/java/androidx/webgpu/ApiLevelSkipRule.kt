package androidx.webgpu

import android.os.Build
import org.junit.AssumptionViolatedException
import org.junit.rules.MethodRule
import org.junit.runners.model.FrameworkMethod
import org.junit.runners.model.Statement

class ApiLevelSkipRule : MethodRule {

  override fun apply(base: Statement, method: FrameworkMethod, target: Any): Statement {
    return object : Statement() {
      override fun evaluate() {
        val requirement = method.getAnnotation(ApiRequirement::class.java)
        println("WGP: apply: requirement:$requirement")
        if (requirement != null) {
          val currentApi = Build.VERSION.SDK_INT
          val minApi = requirement.minApi
          println("WGP: apply: minApi:$minApi, isEmulator:${EmulatorUtils.inEmulator}, currentApi:$currentApi, onlySkipOnEmulator:${requirement.onlySkipOnEmulator}")

          // Check if the API level condition is met (API < 35)
          val isBelowMinApi = currentApi < minApi

          // Check if the skip condition is active
          val shouldSkip =
            isBelowMinApi && EmulatorUtils.inEmulator && requirement.onlySkipOnEmulator

          if (shouldSkip) {
            // Skip only if all conditions are met:
            // API < 35 AND It is an emulator AND The annotation is configured to skip on emulator
            val reason = "Skipping on emulator (API $currentApi): Test requires API $minApi+."
            println("SKIPPING TEST: ${method.name} - $reason")
            throw AssumptionViolatedException(reason)
          }
        }

        // Run the test if no annotation is present, or if skip condition is not met (e.g., it's a real device)
        base.evaluate()
      }
    }
  }
}