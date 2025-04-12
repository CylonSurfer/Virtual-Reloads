#include "hook.h"
#include "xbyak/xbyak.h"

RelocAddr<uintptr_t> hookMainLoopFunc(0xd83e39);

typedef void(*_hookedMainLoop)();
RelocAddr<_hookedMainLoop> hookedMainLoop(0x8b3320);

void hookMain() {
	g_branchTrampoline.Write5Call(hookMainLoopFunc.GetUIntPtr(), (uintptr_t)updateCounter);
}


void updateCounter() {
	//Holsters::MainLoop();
	_MESSAGE("HOOKED");
	hookedMainLoop();
}
