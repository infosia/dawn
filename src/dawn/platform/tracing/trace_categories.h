// Copyright 2020 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DAWN_TRACING_TRACE_CATEGORIES_H_
#define DAWN_TRACING_TRACE_CATEGORIES_H_

#if defined(DAWN_USE_PERFETTO)

#include "dawn/platform/dawn_platform_export.h"
#include "perfetto/tracing/track_event.h"

// List of categories used by built-in V8 trace events.
PERFETTO_DEFINE_CATEGORIES_IN_NAMESPACE_WITH_ATTRS(dawn,
                                                   DAWN_PLATFORM_EXPORT,
                                                   perfetto::Category("gpu.dawn"),
                                                   perfetto::Category("gpu.dawn.validation"),
                                                   perfetto::Category("gpu.dawn.recording"),
                                                   perfetto::Category("gpu.dawn.gpu_work"));

PERFETTO_USE_CATEGORIES_FROM_NAMESPACE(dawn);

#endif  // defined(DAWN_USE_PERFETTO)

#endif  // DAWN_TRACING_TRACE_CATEGORIES_H_