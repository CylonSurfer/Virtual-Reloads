#pragma once
#include "f4sE_common/Relocation.h"
#include "F4SE_common/SafeWrite.h"
#include "f4se/GameReferences.h"
#include "f4se/GameObjects.h"
#include "MiscStructs.h"
#include "NiCloneProcess.h"
#include "f4se/GameData.h"

namespace Offsets {
	typedef int(*_loadNif)(uint64_t path, uint64_t mem, uint64_t flags);
	extern RelocAddr<_loadNif> loadNif;

	typedef NiNode* (*_cloneNode)(NiNode* node, NiCloneProcess* obj);
	extern RelocAddr<_cloneNode> cloneNode;

	extern RelocAddr<UInt64*> cloneAddr1;
	extern RelocAddr<UInt64*> cloneAddr2;

	typedef void(*_TESObjectREFR_AddKeyword)(TESObjectREFR* obj, BGSKeyword* keyword);
	extern RelocAddr< _TESObjectREFR_AddKeyword> TESObjectREFR_AddKeyword;

	typedef void(*_TESObjectREFR_RemoveKeyword)(TESObjectREFR* obj, BGSKeyword* keyword);
	extern RelocAddr< _TESObjectREFR_RemoveKeyword> TESObjectREFR_RemoveKeyword;

	typedef bool(*_TESObjectREFR_HasKeyword)(Actor* obj, BGSKeyword* keyword, TBO_InstanceData* data);
	extern RelocAddr< _TESObjectREFR_HasKeyword> TESObjectREFR_HasKeyword;

	typedef TESObjectWEAP* (*_GetEquippedWeapon)(VirtualMachine* registry, UInt64 stackID, Actor* actor, UInt64 aiEquipIndex);
	extern RelocAddr <_GetEquippedWeapon> GetEquippedWeapon;

	typedef void(*_Actor_GetWeaponEquipIndex)(Actor* a_actor, Reload::BGSEquipIndex* idx, Reload::BGSObjectInstance* instance);
	extern RelocAddr<_Actor_GetWeaponEquipIndex> Actor_GetWeaponEquipIndex;

	typedef float(*_Actor_GetCurrentAmmoCount)(Actor* a_actor, Reload::BGSEquipIndex a_idx);
	extern RelocAddr<_Actor_GetCurrentAmmoCount> Actor_GetCurrentAmmoCount;

	typedef float(*_Actor_SetCurrentAmmoCount)(Actor* a_actor, Reload::BGSEquipIndex a_idx, int a_count);
	extern RelocAddr<_Actor_SetCurrentAmmoCount> Actor_SetCurrentAmmoCount;

	typedef SInt64(*_ExtraDataList_GetAmmoCount)(ExtraDataList* list);
	extern RelocAddr<_ExtraDataList_GetAmmoCount> ExtraDataList_GetAmmoCount;

	typedef float(*_Actor_GetAmmoClipPercentage)(Actor* a_actor, Reload::BGSEquipIndex a_idx);
	extern RelocAddr<_Actor_GetAmmoClipPercentage> Actor_GetAmmoClipPercentage;

	typedef void(*_PlaySoundAtActor)(BGSSoundDescriptorForm* sound, TESObjectREFR* a_object);
	extern RelocAddr<_PlaySoundAtActor> PlaySoundAtActor;

	typedef TESWorldSpace* (*_TESObjectREFR_GetWorldSpace)(TESObjectREFR* a_refr);
	extern RelocAddr<_TESObjectREFR_GetWorldSpace> TESObjectREFR_GetWorldSpace;

	typedef void* (*_TESDataHandler_CreateReferenceAtLocation)(DataHandler* dataHandler, void* newRefr, Reload::NEW_REFR_DATA* refrData);
	extern RelocAddr<_TESDataHandler_CreateReferenceAtLocation> TESDataHandler_CreateReferenceAtLocation;

	typedef void(*_BSPointerHandleManagerInterface_GetSmartPointer)(void* a_handle, void* a_refr);
	extern RelocAddr<_BSPointerHandleManagerInterface_GetSmartPointer> BSPointerHandleManagerInterface_GetSmartPointer;

	typedef void(*_ExtraDataList_setCount)(ExtraDataList* a_list, int a_count);
	extern RelocAddr<_ExtraDataList_setCount> ExtraDataList_setCount;

	typedef void(*_ExtraDataList_ExtraDataList)(ExtraDataList* a_list);
	extern RelocAddr<_ExtraDataList_ExtraDataList> ExtraDataList_ExtraDataList;

	typedef void* (*_MemoryManager_Allocate)(Heap* manager, uint64_t size, uint32_t someint, bool somebool);
	extern RelocAddr<_MemoryManager_Allocate> MemoryManager_Allocate;

	typedef void(*_DeleteRef)(TESObjectREFR* object);
	extern RelocAddr<_DeleteRef> DeleteRef;

	typedef void(*_PlaySoundAtRef)(TESObjectREFR* object, BGSSoundDescriptorForm* sound);
	extern RelocAddr<_PlaySoundAtRef> PlaySoundAtRef;

}