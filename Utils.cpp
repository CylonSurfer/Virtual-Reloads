#include "Utils.h"
#include <math.h>
#define PI 3.14159265358979323846
#include "f4se/GameRTTI.h"
#include "f4se/GameForms.h"
#include "f4se/GameFormComponents.h"
#include "f4se/PapyrusEvents.h"
#include <Windows.h>
#include <vector>
#include <regex>

namespace Reload {
	std::string equippedFullName;

	float vec3_len(NiPoint3 v1) {

		return sqrt(v1.x * v1.x + v1.y * v1.y + v1.z * v1.z);
	}

	NiPoint3 vec3_norm(NiPoint3 v1) {
		double mag = vec3_len(v1);
		if (mag < 0.000001) {
			float maxX = abs(v1.x);
			float maxY = abs(v1.y);
			float maxZ = abs(v1.z);
			if (maxX >= maxY && maxX >= maxZ) {
				return (v1.x >= 0 ? NiPoint3(1, 0, 0) : NiPoint3(-1, 0, 0));
			}
			else if (maxY > maxZ) {
				return (v1.y >= 0 ? NiPoint3(0, 1, 0) : NiPoint3(0, -1, 0));
			}
			return (v1.z >= 0 ? NiPoint3(0, 0, 1) : NiPoint3(0, 0, -1));
		}
		v1.x /= mag;
		v1.y /= mag;
		v1.z /= mag;
		return v1;
	}

	float vec3_dot(NiPoint3 v1, NiPoint3 v2) {
		return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
	}

	NiPoint3 vec3_cross(NiPoint3 v1, NiPoint3 v2) {
		NiPoint3 crossP;
		crossP.x = v1.y * v2.z - v1.z * v2.y;
		crossP.y = v1.z * v2.x - v1.x * v2.z;
		crossP.z = v1.x * v2.y - v1.y * v2.x;
		return crossP;
	}

	float degrees_to_rads(float deg) {
		return (deg * PI) / 180;
	}

	float rads_to_degrees(float rad) {
		return (rad * 180) / PI;
	}

	TESForm* GetFormFromFile(UInt32 formID, const char* pluginName)
	{
		auto mod = (*g_dataHandler)->LookupLoadedModByName(pluginName);
		if (!mod) // No loaded mod by this name
			return nullptr;

		formID |= ((UInt32)mod->modIndex) << 24;

		return LookupFormByID(formID);
	}

	bool isBallisticWeaponEquipped() {
		TESObjectWEAP* equippedItem = Offsets::GetEquippedWeapon((*g_gameVM)->m_virtualMachine, 0, *g_player, 0);
		if (equippedItem) {
			equippedFullName = equippedItem->GetFullName();
			UInt32 meleeKWFormId[2] = { 0x4A0A4, 0x4A0A45 };
			for (int index = 0; index < 2; index++) {
				for (UInt32 i = 0; i < equippedItem->keyword.numKeywords; i++)
				{
					if (equippedItem->keyword.keywords[i])
					{
						if (equippedItem->keyword.keywords[i]->formID == meleeKWFormId[index])
						{
							return false; 
						}
					}
				}
			}
			return true;
		}
		else {
			return false;
		}
	}

	int GetEquippedData() {
		int clipAmount = 0;
		BGSInventoryList* playerINV = (*g_player)->inventoryList;
		playerINV->inventoryLock.LockForRead();
		bool isWeapEquipped = false;
		for (size_t invPos = 0; invPos < playerINV->items.count; invPos++) {
			BGSInventoryItem item = playerINV->items[invPos];
			TESForm* form = item.form;
			BGSInventoryItem::Stack* stack = item.stack;
			if (form) {  //Only Visit FORMS 
				if (form->formType == kFormType_WEAP) {  //Only Visit WEAPON Items
					TESObjectWEAP* weapon = static_cast<TESObjectWEAP*>(form);
					std::string weapName = weapon->GetFullName();
					if (stack && equippedFullName == weapName) { // Only Visit Weapons with Matching Name to Eqipped
						int stackPos = 0;
						stack->Visit([&](BGSInventoryItem::Stack* element) {  //Start of Sub Stacks For this Weapon
							if (element) {
								ExtraDataList* stackDataList = element->extraData;
								if (stackDataList) {
									if (element->flags && element->kFlagEquipped && !isWeapEquipped) {
										ExtraInstanceData* currentEID = DYNAMIC_CAST(stackDataList->GetByType(kExtraData_InstanceData), BSExtraData, ExtraInstanceData);
										if (currentEID) {
											TESObjectWEAP::InstanceData* weapData = (TESObjectWEAP::InstanceData*)currentEID->instanceData;
											TBO_InstanceData* TBOweapData = currentEID->instanceData;
											TESAmmo* ammoType = weapData->ammo;
											ammoCapactity = weapData->ammoCapacity;
											BGSObjectInstance* instance = new BGSObjectInstance(nullptr, nullptr);
											BGSEquipIndex idx;
											Offsets::Actor_GetWeaponEquipIndex(*g_player, &idx, instance);
											SInt64 ac = Offsets::ExtraDataList_GetAmmoCount(stackDataList);  //returns 0.
											float ap = Offsets::Actor_GetAmmoClipPercentage(*g_player, idx);
											clipAmount = ap * ammoCapactity;
										}
										else {
											bool isThrowable = true;
											for (int slotIndex = 32; slotIndex < 44; slotIndex++) {
												TESForm* equippedItem = (*g_player)->equipData->slots[slotIndex].item;
												if (equippedItem && isThrowable) {
													UInt32 equipSlot = weapon->weapData.equipSlot->formID;
													if (equipSlot != 289452) { // ensure equipped weapon is not using 'Grenade' slot. After dropping items equipped throwables sometimes appear in slot index 41 as well as the main drawn weapon.
														isThrowable = false;
														break;
													}
												}
											}
											if (!isThrowable) {
												ammoCapactity = weapon->weapData.ammoCapacity;
												BGSObjectInstance* instance = new BGSObjectInstance(nullptr, nullptr);
												BGSEquipIndex idx;
												Offsets::Actor_GetWeaponEquipIndex(*g_player, &idx, instance);
												float ap = Offsets::Actor_GetAmmoClipPercentage(*g_player, idx);
												clipAmount = ap * ammoCapactity;
											}
										}
									}
								}
							}
							return true;
							});
					}
				}
			}
		}
		playerINV->inventoryLock.Unlock();
		return clipAmount;
	}

	NiNode* CloneThisNode(NiNode* node) {
		NiCloneProcess proc;
		proc.unk18 = Offsets::cloneAddr1;
		proc.unk48 = Offsets::cloneAddr2;
		NiNode* clonedNode = Offsets::cloneNode(node, &proc);
		if (clonedNode) {
			return clonedNode;
		}
		else {
			return nullptr;
		}
	}

	NiNode* FindNode(BSFixedString nodename) {
		NiNode* boneNode = (NiNode*)getChildNode(nodename, (*g_player)->unkF0->rootNode);
		if (!boneNode) {
			auto n = (*g_player)->unkF0->rootNode->GetAsNiNode();
			while (n->m_parent) {
				n = n->m_parent->GetAsNiNode();
			}
			boneNode = getChildNode(nodename, n);
			if (!boneNode) {
				boneNode = nullptr;
				return boneNode;
			}
		}
		return boneNode;
	}

	NiAVObject* FindMesh(NiNode* node, BSFixedString nameLike) {
		if (node) {
			node->m_name.c_str();
			node->m_children.m_emptyRunStart;
			if (node->m_children.m_emptyRunStart != 0) {
				for (auto i = 0; i < node->m_children.m_emptyRunStart; ++i) {
					auto nextNode = node->m_children.m_data[i];
					if (nextNode) {
						BSTriShape* NodeTri = nextNode->GetAsBSTriShape();
						if (NodeTri) {
							BSFixedString TriName = NodeTri->m_name;
							if (matchSubString(TriName.c_str(), nameLike)) {
								return nextNode;
							}
						}
						else {
							BSSubIndexTriShape* NodeSITri = nextNode->GetAsBSSubIndexTriShape();
							if (NodeSITri) {
								BSFixedString TriName = NodeSITri->m_name;
								if (matchSubString(TriName.c_str(), nameLike)) {
									return nextNode;
								}
							}
						}
					}
				}
			}
			return nullptr;
		}
		return nullptr;
	}

	NiNode* FindNode1stp(BSFixedString nodename) {
		NiNode* boneNode = (NiNode*)getChildNode(nodename, (*g_player)->firstPersonSkeleton);
		if (!boneNode) {
			auto n = (*g_player)->firstPersonSkeleton->GetAsNiNode();
			while (n->m_parent) {
				n = n->m_parent->GetAsNiNode();
			}
			boneNode = getChildNode(nodename, n);
			if (!boneNode) {
				boneNode = nullptr;
				return boneNode;
			}
		}
		return boneNode;
	}

	NiNode* getChildNode(const char* nodeName, NiNode* nde) {
		if (!nde->m_name) {
			return nullptr;
		}
		if (!_stricmp(nodeName, nde->m_name.c_str())) {
			return nde;
		}
		NiNode* ret = nullptr;
		for (auto i = 0; i < nde->m_children.m_emptyRunStart; ++i) {
			auto nextNode = nde->m_children.m_data[i] ? nde->m_children.m_data[i]->GetAsNiNode() : nullptr;
			if (nextNode) {
				//_MESSAGE(nextNode->m_name);
				ret = getChildNode(nodeName, nextNode);
				if (ret) {
					return ret;
				}
			}
		}
		return nullptr;
	}

	NiNode* get1stChildNode(const char* nodeName, NiNode* nde) {
		for (auto i = 0; i < nde->m_children.m_emptyRunStart; ++i) {
			auto nextNode = nde->m_children.m_data[i] ? nde->m_children.m_data[i]->GetAsNiNode() : nullptr;
			if (nextNode) {
				if (!_stricmp(nodeName, nextNode->m_name.c_str())) {
					return nextNode;
				}
			}
		}
		return nullptr;
	}

	bool matchSubString(const char* w1, const char* w2) {
		const char* result = strstr(w1, w2);
		if (result) {
			return true;
		}
		else {
			return false;
		}
	}

	void SetINIFloat(BSFixedString name, float value) {
		CallGlobalFunctionNoWait2<BSFixedString, float>("Utility", "SetINIFloat", BSFixedString(name.c_str()), value);
	}

	void PlaySoundDescriptor(BGSSoundDescriptorForm* sound) {
		CallGlobalFunctionNoWait1<BGSSoundDescriptorForm*>("Sound", "Play", sound);
	}

	void setFingerPositionScalar(bool isLeft, float thumb,float index, float middle, float ring, float pinky) {
		CallGlobalFunctionNoWait6<bool, float, float, float, float, float>("FRIK:FRIK", "setFingerPositionScalar", isLeft, thumb, index, middle, ring, pinky);
	}

	void restoreFingerPoseControl(bool isLeft) {
		CallGlobalFunctionNoWait1<bool>("FRIK:FRIK", "restoreFingerPoseControl", isLeft);
	}

}