// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#define CALL_NATIVE_EVENT_CONDITIONAL(bDisableBlueprintEvent, ObjectPtr, Event, ...)\
  ((bDisableBlueprintEvent) ? ObjectPtr->Event##_Implementation(__VA_ARGS__) : ObjectPtr->Event(__VA_ARGS__))

#define CALL_NATIVE_EVENT_ALWAYS(ObjectPtr, Event, ...)\
  (ObjectPtr->Event(__VA_ARGS__))

#define CALL_NATIVE_EVENT_NEVER(ObjectPtr, Event, ...)\
  (ObjectPtr->Event##_Implementation(__VA_ARGS__))
