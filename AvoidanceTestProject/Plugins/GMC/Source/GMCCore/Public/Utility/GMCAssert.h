// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

// Activates additional check macros in the GMC module for more exhaustive debugging. Add
//   PrivateDefinitions.Add("GMC_DO_CHECK");
// to the Build.cs to enable. Only works in build configurations that have DO_CHECK enabled as well.

#if DO_CHECK && defined GMC_DO_CHECK

#define gmc_ck(expr) check(expr)
#define gmc_ckne() checkNoEntry()
#define gmc_ckc(Code) checkCode(Code)

#else

#define gmc_ck(expr)
#define gmc_ckne()
#define gmc_ckc(Code)

#endif
