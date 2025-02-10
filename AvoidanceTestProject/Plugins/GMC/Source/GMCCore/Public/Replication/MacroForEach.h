// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#define EXP_1(...) __VA_ARGS__
#define EXP_2(...) EXP_1(EXP_1(EXP_1(__VA_ARGS__)))
#define EXP_3(...) EXP_2(EXP_2(EXP_2(__VA_ARGS__)))
#define EXP_4(...) EXP_3(EXP_3(EXP_3(__VA_ARGS__)))
#define EXP_5(...) EXP_4(EXP_4(EXP_4(__VA_ARGS__)))
#define EXP_6(...) EXP_5(EXP_5(EXP_5(__VA_ARGS__)))
#define EXP(...) EXP_6(EXP_6(EXP_6(__VA_ARGS__)))
#define END(...)
#define GET_END_2() , END
#define GET_END_1(...) GET_END_2
#define GET_END(...) GET_END_1
#define EMP()
#define DEF(X) X EMP()
#define OUT
#define NXT_0(T, N, ...) N OUT
#define NXT_1(T, N) DEF (NXT_0) (T, N)
#define NXT(T, N) NXT_1(GET_END T, N)
#define FE_0(F, X, P, ...) F(X) DEF (NXT(P, FE_1)) (F, P, __VA_ARGS__)
#define FE_1(F, X, P, ...) F(X) DEF (NXT(P, FE_0)) (F, P, __VA_ARGS__)

#define FOR_EACH(F, ...) EXP(FE_1(F, __VA_ARGS__, ()()()))
