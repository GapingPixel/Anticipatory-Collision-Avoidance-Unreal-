// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

/**
 * To avoid the creation of unnecessary log categories,
 * this log category should be used across all modules of the OpenUnrealUtilities plugin.
 * Theoretically it's also possible to use this log category in other plugins or in a game module,
 * but it's strongly recommended not to use it outside of the plugin!
 */
BEANSTESTUTILITIES_API DECLARE_LOG_CATEGORY_EXTERN(LogOpenUnrealUtilities, Log, All);
