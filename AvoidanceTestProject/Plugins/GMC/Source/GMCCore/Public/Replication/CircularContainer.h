// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GMCAssert.h"

template<typename T, int32 Size>
struct TGMC_CircularNumericBuffer
{
  TGMC_CircularNumericBuffer()
  {
    Buffer.AddDefaulted(Size);
    Buffer.Shrink();
    gmc_ck(Buffer.Num() == Size)
  }

  int32 GetNum() const
  {
    return Num;
  }

  int32 GetOffset() const
  {
    return Offset;
  }

  const TArray<T>& GetBuffer() const
  {
    return Buffer;
  }

  T GetNewestElement() const
  {
    if (Num == 0)
    {
      return 0;
    }

    if (Num == 1)
    {
      return Buffer[0];
    }

    return Buffer[(Offset + Size - 1) % Size];
  }

  T GetOldestElement() const
  {
    if (Num == 0)
    {
      return 0;
    }

    if (Num < Size)
    {
      return Buffer[0];
    }

    return Buffer[Offset];
  }

  T GetMean() const
  {
    if (Num == 0)
    {
      return 0;
    }

    T Sum = 0;
    for (int32 Index = 0; Index < Num; ++Index)
    {
      Sum += Buffer[(Offset + Size - Index - 1) % Size];
    }

    return Sum / Num;
  }

  T GetTrimmedMean(float TrimPercent) const
  {
    if (Num == 0)
    {
      return 0;
    }

    const int32 LowIndex = Num * TrimPercent;
    const int32 HighIndex = Num - LowIndex;
    const int32 TrimmedNum = Num - 2 * LowIndex;

    if (TrimmedNum <= 0)
    {
      return 0;
    }

    TArray<T> SortedValues{};
    SortedValues.Reserve(Num);
    for (int32 Index = 0; Index < Num; ++Index)
    {
      SortedValues.Add(Buffer[(Offset + Size - Index - 1) % Size]);
    }

    SortedValues.Sort();

    T Sum = 0;
    for (int32 Index = LowIndex; Index < HighIndex; ++Index)
    {
      Sum += SortedValues[Index];
    }

    return Sum / TrimmedNum;
  }

  void Add(T Value)
  {
    Buffer[Offset] = Value;

    if (++Offset == Size)
    {
      Offset = 0;
    }

    if (Num < Size)
    {
      Num++;
    }

    gmc_ck(Num < Size ? Num == Offset : true)
    gmc_ck(Buffer.Num() == Size)
  }

  void Reset()
  {
    Num = 0;
    Offset = 0;
    gmc_ck(Buffer.Num() == Size)
  }

private:

  int32 Num{0};
  int32 Offset{0};
  TArray<T> Buffer{};
};

template<typename T>
struct TGMC_CircularArray
{
  void Reset(int32 InMaxSize)
  {
    Offset = 0;
    MaxSize = InMaxSize;
    Container.Empty(MaxSize);
    gmc_ck(Container.Max() == MaxSize)
  }

  int32 Num() const
  {
    return Container.Num();
  }

  T& Last(int32 IndexFromEnd = 0)
  {
    gmc_ck(IndexFromEnd >= 0)
    gmc_ck(Container.Num() > IndexFromEnd)
    return (*this)[Container.Num() - 1 - IndexFromEnd];
  }

  const T& Last(int32 IndexFromEnd = 0) const
  {
    gmc_ck(IndexFromEnd >= 0)
    gmc_ck(Container.Num() > IndexFromEnd)
    return (*this)[Container.Num() - 1 - IndexFromEnd];
  }

  void Add(T&& Item)
  {
    gmc_ck(&Item < Container.GetData() || &Item >= Container.GetData() + MaxSize)
    Emplace(MoveTempIfPossible(Item));
  }

  void Add(const T& Item)
  {
    gmc_ck(&Item < Container.GetData() || &Item >= Container.GetData() + MaxSize)
    Emplace(Item);
  }

  T& operator[](int32 Index)
  {
    gmc_ck(Index >= 0)
    gmc_ck(Index < Container.Num())
    return Container[GetInternalIndex(Index)];
  }

  const T& operator[](int32 Index) const
  {
    gmc_ck(Index >= 0)
    gmc_ck(Index < Container.Num())
    return Container[GetInternalIndex(Index)];
  }

private:

  int32 GetInternalIndex(int32 Index) const
  {
    gmc_ck(Index >= 0)
    gmc_ck(Index < Container.Num())
    return Container.Num() < MaxSize ? Index : (Offset + Index) % MaxSize;
  }

  template <typename... U>
  void Emplace(U&&... Args)
  {
    if (Container.Num() < MaxSize)
    {
      Container.Emplace(Forward<U>(Args)...);
    }
    else
    {
      Container[Offset] = T(Forward<U>(Args)...);
      gmc_ck(Container.Num() == MaxSize)
    }

    if (++Offset == MaxSize)
    {
      gmc_ck(Container.Num() == MaxSize)
      gmc_ck(Container.Num() == Offset)
      Offset = 0;
    }

    gmc_ck(Container.Max() == MaxSize)
    gmc_ck(Container.Num() < MaxSize ? Container.Num() == Offset : true)
  }

  int32 Offset{0};
  int32 MaxSize{0};
  TArray<T> Container{};
};
