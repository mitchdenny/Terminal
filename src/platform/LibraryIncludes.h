// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
// Linux replacement for LibraryIncludes.h
// On Windows, this pulls in WIL, GSL, and Chromium safe math.
// On Linux, we provide compatible replacements.

#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

// C
#include <climits>
#include <cwchar>
#include <cwctype>

// STL
#include <algorithm>
#include <atomic>
#include <cmath>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <list>
#include <map>
#include <memory_resource>
#include <memory>
#include <mutex>
#include <new>
#include <numeric>
#include <optional>
#include <queue>
#include <regex>
#include <set>
#include <shared_mutex>
#include <span>
#include <stdexcept>
#include <string_view>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// Platform compatibility (replaces windows.h, WIL, SAL, etc.)
#include "../../platform/windows_compat.h"
#include "../../platform/wil_compat.h"

// GSL
#include <gsl/gsl_util>
#include <gsl/pointers>
#include <gsl/narrow>

// fmt
#include <fmt/format.h>
#include <fmt/compile.h>
#include <fmt/xchar.h>

// Chromium Numerics (safe math)
#include <base/numerics/safe_math.h>

// TIL (Terminal Infrastructure Library)
#include <til.h>

// IntervalTree
#define USE_INTERVAL_TREE_NAMESPACE
#include <IntervalTree.h>

#pragma GCC diagnostic pop
