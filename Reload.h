#pragma once
#include "Utils.h"
#include "Offsets.h"
#include "Weapon.h"
#include "weaponOffset.h"
#include "ConfigMode.h"
#include "Matrix.h"
#include "f4se/GameData.h"
#include "f4se/GameReferences.h"
#include "f4se/GameForms.h"
#include "f4se/GameExtraData.h"
#include "f4se/GameRTTI.h"
#include "f4se/PapyrusVM.h"
#include "f4se/GameMenus.h"
#include "f4se/ScaleformValue.h"
#include "f4se/ScaleformCallbacks.h"
#include "f4se/NiNodes.h"
#include "f4se/NiObjects.h"
#include <iostream>
#include <sstream>
#include "VRHookAPI.h"
#include "VRHook.h"
#include "BSFlattenedBoneTree.h"
#include <thread>
#include <cmath>

extern UInt16 ammoCapactity;
extern OpenVRHookManagerAPI* vrHook;
extern std::map<std::string, int> boneTreeMap;
extern std::vector<std::string> boneTreeVec;
namespace Reload {

	class MyAmmoPack {
	public:
		MyAmmoPack() {
			radius = 0;
			bone = nullptr;
			stickyRight = false;
			stickyLeft = false;
			turnOnDebugSpheres = false;
			offset.x = 0;
			offset.y = 0;
			offset.z = 0;
			debugSphere = nullptr;
			MeshPath = "path";
			MeshName = "name";
		}

		MyAmmoPack(float a_radius, NiNode* a_bone, NiPoint3 a_offset, BSFixedString a_mesh, BSFixedString a_meshN) : radius(a_radius), bone(a_bone), offset(a_offset), MeshPath(a_mesh), MeshName(a_meshN) {
			stickyRight = false;
			stickyLeft = false;
			turnOnDebugSpheres = false;
			debugSphere = nullptr;
		}
		float radius;
		NiNode* bone;
		NiPoint3 offset;
		NiPoint3 offset2;
		bool stickyRight;
		bool stickyLeft;
		bool turnOnDebugSpheres;
		NiNode* debugSphere;
		BSFixedString MeshPath;
		BSFixedString MeshName;
	};

	bool initBoneTree();
	void mainUpdate();
	void MainInit();
	void setupWeapon();
	void VRButtonsMain();
	void startReload();
	void waitFor3D(TESObjectREFR* object);
	void playDryFire();
	void dryPositionBolt();
	void grabAmmmo();
	void dropAmmo();
	void insertAmmo();
	void deleteSpentClip(TESObjectREFR* object);
	bool isAmmoInHand();
	bool gunRequiresMag();
	void detectMagNode();
	bool requiresRacking();
	void grabBolt();
	void detectBolt();
	void moveBolt();
	void retoreBolt();
	void completeSlideReload();
	void retoreBolt2();
	void rotateBreakAction();
	void detectBreakAction();
	void gripBreakAction();
	void rotateWithBreakAction();
	void rotateCylinder();
	void detectCylinder();
	void rotateWithCylinder();
	void gripCylinder();
	void rotateLaserLatch();
	void gripLaserLatch();
	void detectLaserLatch();
	void rotateWithLaserLatch();
	void detectLever();
	void gripLever();
	void rotateWithLever();
	void showAmmoPouch();
	void scaleAmmoPouch();
	void destroyAmmoPouch();
	void detectAmmoSphere();
	void displayAmmoSphere();
	void registerAmmoSphere(float radius, BSFixedString bone, NiPoint3 pos, BSFixedString MeshPath, BSFixedString MeshName);
	void createAmmoSpheres();

	void UITile01Function();

	void UITile02Function();

	void UITile03Function();


	inline NiNode* loadNifFromFile(char* path) {
		uint64_t flags[2];
		flags[0] = 0x0;
		flags[1] = 0xed | 0x2d;
		uint64_t mem = 0;
		int ret = Offsets::loadNif((uint64_t) & (*path), (uint64_t)&mem, (uint64_t)&flags);

		return (NiNode*)mem;
	}
}