// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "Modules/ModuleManager.h"

class FGMCSteamModule : public IModuleInterface
{
public:

  virtual void StartupModule() override;
  virtual void ShutdownModule() override;
};
