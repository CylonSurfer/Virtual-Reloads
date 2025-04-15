#pragma once
#include "Reload.h"


namespace Reload {

	enum WeaponAmmoType
	{
		kAmmoType_Unknown,
		kAmmoType_Magazine,
		kAmmoType_Shell,
		kAmmoType_MultiBullet,
		kAmmoType_Projectile,
		kAmmoType_FusionCell
	};

	enum WeaponReloadType
	{
		kReloadType_Unknown,
		kReloadType_Bolt,
		kReloadType_BreakAction,
		KReloadType_Cylinder,
		KReloadType_DirectInsert,
		kReloadType_Laser,
		kReloadType_LeverAction
	};

	class CurrentWeapon {
	public:

		WeaponAmmoType ammoType; // type of ammo the equipped weapon uses for reloads (Magazine, Shells etc)
		WeaponReloadType ReloadType; // type of system use to complete the reload sequence (Bolt, Break Action etc)
		TESForm* baseForm = nullptr; // base form of the eqipped weapon
		TESObjectWEAP* baseWeapon = nullptr; // base weapon type of the equipped weapon 
		TESAmmo* ammoTypeForm = nullptr; // equipped weapons ammo type
		NiNode* magazineNode = nullptr; // equipped weapons magazine node
		NiNode* magazineNode2 = nullptr;
		NiNode* clonedMagazineNode = nullptr; // clone of equipped weapons magazine node
		NiNode* clonedMagazineNode2 = nullptr;
		NiNode* boltNode = nullptr; // equipped weapons bolt node
		NiNode* clonedBoltNode = nullptr; // clone of equipped weapons bolt node
		NiNode* breakActionNode = nullptr;
		NiNode* clonedBreakActionNode = nullptr;
		NiNode* cylinderNode = nullptr;
		NiNode* clonedCylinderNode = nullptr;
		NiNode* clonedCylinderNodeBKUP = nullptr;
		NiNode* breakActionAmmo01 = nullptr;
		NiNode* breakActionAmmo02 = nullptr;
		NiNode* weaponNode = nullptr; // players current weapon node
		NiNode* breakActionShellNode[5] = { nullptr, nullptr, nullptr, nullptr, nullptr };
		NiNode* clonedBreakActionShellNode[5] = { nullptr, nullptr, nullptr, nullptr, nullptr };
		std::string breakActionShellName[5] = { "WeaponMagazineChild1", "WeaponMagazineChild2" , "WeaponMagazineChild3" , "WeaponMagazineChild4" , "WeaponMagazineChild5" };
		std::string clonedBreakActionShellName[5] = { "ClonedMagazine1", "ClonedMagazine2" , "ClonedMagazine3" , "ClonedMagazine4" , "ClonedMagazine5" };
		float breakActionShellScale[5] = {};
		NiNode* cylinderBulletNode[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
		NiNode* clonedCylinderBulletNode[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
		std::string cylinderBulletName[6] = { "BulletNode01", "BulletNode02" , "BulletNode03" , "BulletNode04" , "BulletNode05", "BulletNode06" };
		std::string clonedCylinderBulletName[6] = { "ClonedBulletNode01", "ClonedBulletNode02" , "ClonedBulletNode03" , "ClonedBulletNode04" , "ClonedBulletNode05", "ClonedBulletNode06" };
		NiAVObject* latchNode = nullptr;
		NiAVObject* clonedLatchNode = nullptr;
		NiTransform savedWeaponData;
		NiTransform originalWeaponData;
		float originalMagScale = 1.0;
		float originalBoltScale = 1.0;
		float originalBoltPosition; // Current Weapons original bolt position (if it has one) so we can restore it on letting the bolt go
		float maxBoltPosition; // the maximum value we want to move the bolt before registering its reload as completed
		float boltPosDifference; 
		float reloadSlidePos; 
		int magCapacity; // the maximum capacity of the current weapons magazine
		int clipCount; // the current clips count
		int reserveAmmo;
		int chamberNum;
		std::string equippedFullName;  // equipped weapons full name
		BGSEquipIndex equipIndex; // equipped weapons equip index
		BGSObjectInstance* weaponInstance = nullptr; // equipped weapons instance
		TBO_InstanceData* weaponTBOInstanceData = nullptr; // equipped weapons TBO_InstanceData (if there is any)
		TESObjectWEAP::InstanceData* weaponInstanceData = nullptr; // equipped weapons InstanceData (if there is any)
		ExtraDataList* weaponExtraDataList = nullptr;
		void initWeapon(BGSKeyword* keyword);
		int GetReserveAmmo();
		float GetClipAmmoCount();
		void getWeaponNodes();

	private:
		void ExtractWeaponDataFromInventory();

	};
}