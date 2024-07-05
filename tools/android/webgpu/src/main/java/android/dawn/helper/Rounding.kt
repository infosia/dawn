package android.dawn.helper

fun Long.roundDownToNearestMultipleOf(boundary: Int) =
  (((this - (if (this < 0 && (this % boundary != 0L)) boundary else 0)) / boundary) * boundary)
fun Int.roundDownToNearestMultipleOf(boundary: Int) =
  (((this - (if (this < 0 && (this % boundary != 0)) boundary else 0)) / boundary) * boundary)
fun Long.roundUpToNearestMultipleOf(boundary: Int) =
  (((this + (if (this > 0 && (this % boundary != 0L)) boundary else 0)) / boundary) * boundary)
fun Int.roundUpToNearestMultipleOf(boundary: Int) =
  (((this + (if (this > 0 && (this % boundary != 0)) boundary else 0)) / boundary) * boundary)
