// MathLibrary.h - Contains declarations of math functions
#pragma once

#ifdef BSM_EXPORTS
#define BSM_API __declspec(dllexport)
#else
#define BSM_API __declspec(dllimport)
#endif


extern "C" BSM_API void RVExtensionVersion(char* output, int outputSize);
extern "C" BSM_API void RVExtension(char* output, int outputSize, const char* function);
extern "C" BSM_API int RVExtensionArgs(char* output, int outputSize, const char* function, const char** args, int argsCnt);