#pragma once
#include "Reload.h"
#include "f4se/BSGeometry.h"

namespace Reload {
	enum cWeaponAmmoType
	{
		kConfAmmoType_Unknown,
		kConfAmmoType_Magazine,
		kConfAmmoType_Shell,
		kConfAmmoType_MultiBullet,
		kConfAmmoType_Projectile,
		kConfAmmoType_FusionCell
	};

	enum cWeaponReloadType
	{
		kConfReloadType_Unknown,
		kConfReloadType_Bolt,
		kConfReloadType_BreakAction,
		KConfReloadType_Cylinder,
		KConfReloadType_DirectInsert,
		kConfReloadType_Laser
	};

	class ConfigMode
	{
	public:

		void StartConfigMode(int ReloadType);
		void ExitConfigMode();
		void updateUITiles();
		void pullWeaponNodes(cWeaponReloadType ReloadType);
		bool _isConfigModeActive = false;
		cWeaponReloadType cReloadType;
		cWeaponAmmoType cAmmoType;
		NiNode* cWeaponNode = nullptr;
		NiNode* cBoltNode = nullptr;
		NiAVObject* cLatchNode = nullptr;
		NiAVObject* cClonedLatchNode = nullptr;
		NiNode* cClonedBoltNode = nullptr;
		NiNode* cPrimaryAttachNode = nullptr;
		NiNode* cUIMain = nullptr;
		NiNode* cBreakActionNode = nullptr;
		NiNode* cClonedBreakActionNode = nullptr;
		NiNode* cCylinderNode = nullptr;
		NiNode* cClonedCylinderNode = nullptr;
		NiNode* removeNode = nullptr;
		NiNode* restoreNode = nullptr;
		NiTransform cBreakActionNodeRot;
		NiTransform cCylinderNodeRot;
		float cOriginalBoltScale;
		float cOriginalBoltPosition;
		float cMaxBoltPosition;
		NiTransform _cCustomTransform;
		std::string weaponName;

	private:

		bool _MCTouchbuttons[3] = { false, false, false };
		bool _isChangeReloadTypeButtonPressed = false;
		bool _isChangeAmmoButtonPressed = false;
		bool _isSaveConfigButtonPressed = false;
		NiNode* UIMain = nullptr;
		NiNode* primaryAttachNode = nullptr;
		NiNode* niUIButton_Nodes[3] = {};
		NiNode* niUI_Nodes[3] = {};
		NiNode* niUI_Tiles[3] = {};
		NiNode* niUI_ModeNodes[6] = {};
		bool bUITilePressed[3] = {};

	};

}