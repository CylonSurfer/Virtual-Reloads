#include "Offsets.h"

namespace Offsets {
	RelocAddr<_loadNif> loadNif(0x1d0dee0);
	RelocAddr<_cloneNode> cloneNode(0x1c13ff0);
	RelocAddr<UInt64*> cloneAddr1(0x36ff560);
	RelocAddr<UInt64*> cloneAddr2(0x36ff564);
	RelocAddr<_TESObjectREFR_AddKeyword> TESObjectREFR_AddKeyword(0x3df2e0);
	RelocAddr<_TESObjectREFR_RemoveKeyword> TESObjectREFR_RemoveKeyword(0x3df380);
	RelocAddr<_TESObjectREFR_HasKeyword> TESObjectREFR_HasKeyword(0x59d8e0);
	RelocAddr<_GetEquippedWeapon> GetEquippedWeapon(0x140d790);
	RelocAddr<_Actor_GetCurrentAmmoCount> Actor_GetCurrentAmmoCount(0xddf690);
	RelocAddr<_Actor_SetCurrentAmmoCount> Actor_SetCurrentAmmoCount(0xddf790);
	RelocAddr<_Actor_GetWeaponEquipIndex> Actor_GetWeaponEquipIndex(0xe50e70);
	RelocAddr<_ExtraDataList_GetAmmoCount> ExtraDataList_GetAmmoCount(0x0980a0);
	RelocAddr<_Actor_GetAmmoClipPercentage> Actor_GetAmmoClipPercentage(0xddf6c0);
	RelocAddr<_PlaySoundAtActor> PlaySoundAtActor(0x30f9f0);
	RelocAddr<_TESObjectREFR_GetWorldSpace> TESObjectREFR_GetWorldSpace(0x3f75a0);
	RelocAddr<_TESDataHandler_CreateReferenceAtLocation> TESDataHandler_CreateReferenceAtLocation(0x11bd80);
	RelocAddr<_BSPointerHandleManagerInterface_GetSmartPointer> BSPointerHandleManagerInterface_GetSmartPointer(0xab60);
	RelocAddr<_MemoryManager_Allocate> MemoryManager_Allocate(0x1b91950);
	RelocAddr<_ExtraDataList_ExtraDataList> ExtraDataList_ExtraDataList(0x81360);
	RelocAddr<_ExtraDataList_setCount> ExtraDataList_setCount(0x88fe0);
	RelocAddr<_DeleteRef> DeleteRef(0x14808f0);
	RelocAddr<_PlaySoundAtRef> PlaySoundAtRef(0x3f8bb0);
}