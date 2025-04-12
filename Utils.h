#pragma once
#include "f4se/GameData.h"
#include "Reload.h"

namespace Reload {
	float vec3_len(NiPoint3 v1);
	NiPoint3 vec3_norm(NiPoint3 v1);
	float vec3_dot(NiPoint3 v1, NiPoint3 v2);
	NiPoint3 vec3_cross(NiPoint3 v1, NiPoint3 v2);
	float degrees_to_rads(float deg);
	float rads_to_degrees(float deg);
	TESForm* GetFormFromFile(UInt32 formID, const char* pluginName);
	bool isBallisticWeaponEquipped();
	int GetEquippedData();
	NiNode* CloneThisNode(NiNode* node);
	NiNode* getChildNode(const char* nodeName, NiNode* nde);
	NiNode* get1stChildNode(const char* nodeName, NiNode* nde);
	NiNode* FindNode(BSFixedString nodename);
	NiNode* FindNode1stp(BSFixedString nodename);
	bool matchSubString(const char* w1, const char* w2);
	void SetINIFloat(BSFixedString name, float value);
	void PlaySoundDescriptor(BGSSoundDescriptorForm* sound);
	void setFingerPositionScalar(bool isLeft, float thumb, float index, float middle, float ring, float pinky);
	void restoreFingerPoseControl(bool isLeft);
}