#ifndef XINPUT1_3_PROVIDED_SYMBOLS_H
#define XINPUT1_3_PROVIDED_SYMBOLS_H
#include "DestructorGenerator.h"
#include "SymbolResolver.h"

void hookRequiredSymbols(SymbolResolver& provider);

void* generateDummySymbol(const char* mangledName, DummyFunctionCallHandler CallHandler);
void* provideSymbolImplementation(const char* mangledName);

#endif //XINPUT1_3_PROVIDED_SYMBOLS_H
