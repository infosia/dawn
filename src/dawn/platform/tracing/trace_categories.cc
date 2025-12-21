
// Copyright 2020 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "dawn/platform/tracing/trace_categories.h"

#if defined(DAWN_USE_PERFETTO)
PERFETTO_TRACK_EVENT_STATIC_STORAGE_IN_NAMESPACE_WITH_ATTRS(dawn, DAWN_PLATFORM_EXPORT);
#endif
