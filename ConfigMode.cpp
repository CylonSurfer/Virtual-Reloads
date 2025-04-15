#include "ConfigMode.h"

namespace Reload {

	char* mUI_Nodes[3] = { "Data/Meshes/VRR/UI-NODE01_ReloadType.nif", "Data/Meshes/VRR/UI-NODE02_AmmoType.nif", "Data/Meshes/VRR/UI-NODE03_Save.nif" };
	char* mUI_Tiles[3] = { "Data/Meshes/VRR/UI-TILE01_ReloadType.nif", "Data/Meshes/VRR/UI-TILE02_AmmoType.nif", "Data/Meshes/VRR/UI-TILE03_Save.nif" };
	char* mUI_Info_Nodes[7] = { "Data/Meshes/VRR/UI-INFO_MODE00.nif", "Data/Meshes/VRR/UI-INFO_MODE01.nif", "Data/Meshes/VRR/UI-INFO_MODE02.nif", "Data/Meshes/VRR/UI-INFO_MODE03.nif", "Data/Meshes/VRR/UI-INFO_MODE04.nif", "Data/Meshes/VRR/UI-INFO_MODE05.nif", "Data/Meshes/VRR/UI-INFO_MODE06.nif" };
	char* nUI_Info_Nodes[7] = { "UI-INFO_MODE00", "UI-INFO_MODE01", "UI-INFO_MODE02", "UI-INFO_MODE03", "UI-INFO_MODE04", "UI-INFO_MODE05", "UI-INFO_MODE06" };
	char* nUI_Nodes[3] = { "UI-NODE01_ReloadType", "UI-NODE02_AmmoType", "UI-NODE03_Save" };
	char* nUI_Tiles[3] = { "UI-TILE01_ReloadType", "UI-TILE02_AmmoType", "UI-TILE03_Save" };
	void (*funcs[3])() = { UITile01Function, UITile02Function, UITile03Function };
	std::string cloneToCheck[2] = { "ClonedBolt", "ClonedBreakActionNode" };
	std::string nodeToRestore[2] = { "WeaponBolt", "WeaponMagazine" };

	void ConfigMode::StartConfigMode(int ReloadType) {
		BSFixedString nodename = "primaryUIAttachNode";
		NiNode* boneNode = nullptr;
		boneNode = FindNode(nodename);
		if (!boneNode) {
			return;
		}
		primaryAttachNode = boneNode;
		cPrimaryAttachNode = primaryAttachNode;
		// Load and Display Main UI
		BSFixedString NameofMesh = "VRRHudMain";
		NiNode* retNode = loadNifFromFile("Data/Meshes/VRR/UI-MAIN_TITLE.nif");
		UIMain = CloneThisNode(retNode);
		UIMain->m_name = NameofMesh;
		boneNode->AttachChild((NiAVObject*)UIMain, true);
		cUIMain = UIMain;
		//gInPowerArmor ? UIMain->m_localTransform.scale = 0.85 : UIMain->m_localTransform.scale = 0.7;
		UIMain->m_localTransform.scale = 0.7;
		UIMain->flags &= 0xfffffffffffffffe;
		for (int i = 0; i < 3; i++) {
			BSFixedString UI_ITEMNAME = (nUI_Nodes[i]);
			retNode = loadNifFromFile(mUI_Nodes[i]);
			NiNode* UIItem = CloneThisNode(retNode);
			UIItem->m_name = BSFixedString(UI_ITEMNAME);
			UIMain->AttachChild((NiAVObject*)UIItem, true);
			UIItem->flags &= 0xfffffffffffffffe;
			niUI_Nodes[i] = UIItem;
			BSFixedString UI_ITEMNAME2 = (nUI_Tiles[i]);
			retNode = loadNifFromFile(mUI_Tiles[i]);
			NiNode* UIItem2 = CloneThisNode(retNode);
			UIItem2->m_name = BSFixedString(UI_ITEMNAME2);
			UIItem->AttachChild((NiAVObject*)UIItem2, true);
			UIItem2->flags &= 0xfffffffffffffffe;
			niUI_Tiles[i] = UIItem2;
		}
		TESObjectWEAP* equippedItem = Offsets::GetEquippedWeapon((*g_gameVM)->m_virtualMachine, 0, *g_player, 0);
		if (equippedItem) {
			weaponName = equippedItem->GetFullName();
		}
		//cReloadType = kcReloadType_Bolt;
		cWeaponReloadType rlt = static_cast<cWeaponReloadType>(ReloadType);
		pullWeaponNodes(rlt);
		SetINIFloat("fDirectionalDeadzone:Controls", 1.0);
		setFingerPositionScalar(true, 0.0, 1.0, 0.0, 0.0, 0.0);
		_isConfigModeActive = true;
	}

	void ConfigMode::ExitConfigMode() {
		_isConfigModeActive = false;
		SetINIFloat("fDirectionalDeadzone:Controls", 0.5);
		restoreFingerPoseControl(true);
		if (cWeaponNode) {
			if (restoreNode) {
				restoreNode->m_localTransform.scale = cOriginalBoltScale;
				restoreNode->flags &= 0xfffffffffffffffe;
			}
			if (removeNode) {
				cWeaponNode->RemoveChild(removeNode);
			}
		}
		cClonedBoltNode = nullptr;
		cClonedBreakActionNode = nullptr;
		cBreakActionNode = nullptr;
		cBoltNode = nullptr;
		cWeaponNode = nullptr;
		restoreNode = nullptr;
		removeNode = nullptr;
		cClonedCylinderNode = nullptr;
		cCylinderNode = nullptr;
		if (cPrimaryAttachNode && cUIMain) {
			cPrimaryAttachNode->RemoveChild(cUIMain);
			cPrimaryAttachNode = nullptr;
			cUIMain = nullptr;
		}
	}

	void ConfigMode::updateUITiles() {
		BSFlattenedBoneTree* rt = (BSFlattenedBoneTree*)(*g_player)->unkF0->rootNode->m_children.m_data[0]->GetAsNiNode();
		NiPoint3 lFinger;
		lFinger = rt->transforms[boneTreeMap["LArm_Finger23"]].world.pos;
		for (int i = 0; i < 3; i++) {
			if (niUI_Nodes[i] && niUI_Tiles[i]) {
				float distance = vec3_len(lFinger - niUI_Tiles[i]->m_worldTransform.pos);
				if (distance > 2.0) {
					niUI_Nodes[i]->m_localTransform.pos.y = 0.0;
					bUITilePressed[i] = false;
				}
				else if (distance <= 2.0) {
					float fz = (2.0 - distance);
					if (fz > 0.0 && fz < 1.2) {
						niUI_Nodes[i]->m_localTransform.pos.y = (fz);
					}
					if ((niUI_Nodes[i]->m_localTransform.pos.y > 1.0) && !bUITilePressed[i]) {
						bUITilePressed[i] = true;
						vrHook->StartHaptics(1, 0.05, 0.3);
						funcs[i]();
					}
				}
			}
		}
		for (int i = 0; i < 7; i++) {
			int rlm = static_cast<int>(cReloadType);
			if (rlm != i) {
				if (niUI_ModeNodes[i] != nullptr) {
					UIMain->RemoveChild(niUI_ModeNodes[i]);
					niUI_ModeNodes[i] = nullptr;
				}
			}
			if (rlm == i) {
				if (niUI_ModeNodes[i] == nullptr) {
					NiNode* retNode = loadNifFromFile(mUI_Info_Nodes[i]);
					niUI_ModeNodes[i] = CloneThisNode(retNode);
					niUI_ModeNodes[i]->m_name = nUI_Info_Nodes[i];
					UIMain->AttachChild((NiAVObject*)niUI_ModeNodes[i], true);
				}
			}
		}
	}

	void ConfigMode::pullWeaponNodes(cWeaponReloadType ReloadType) {
		
		NiNode* wpnNode = getChildNode("Weapon", (*g_player)->firstPersonSkeleton->GetAsNiNode());
		if (wpnNode) {
			cWeaponNode = wpnNode;
			// remove any existing clones and restore originals
			if (removeNode) {
				wpnNode->RemoveChild(removeNode);
				removeNode = nullptr;
			}
			if (restoreNode) {
				restoreNode->m_localTransform.scale = cOriginalBoltScale;
				restoreNode->flags &= 0xfffffffffffffffe;
			}
			if (cReloadType == kConfReloadType_Bolt) {
				cAmmoType = kConfAmmoType_Magazine;
				NiNode* _boltNode = getChildNode("WeaponBolt", wpnNode);
				if (_boltNode) {
					cBoltNode = _boltNode;
					cOriginalBoltPosition = _boltNode->m_localTransform.pos.y;
					cClonedBoltNode = CloneThisNode(_boltNode);
					if (cClonedBoltNode) {
						cClonedBoltNode->m_name = "ClonedBolt";
						cOriginalBoltScale = _boltNode->m_localTransform.scale;
						cMaxBoltPosition = _boltNode->m_localTransform.pos.y;
						_boltNode->m_localTransform.scale = 0.0;
						_boltNode->flags |= 0x1;
						wpnNode->AttachChild(cClonedBoltNode, true);
						removeNode = cClonedBoltNode;
						restoreNode = cBoltNode;
					}
				}
			}
			if (cReloadType == kConfReloadType_BreakAction) {
				cAmmoType = kConfAmmoType_Shell;
				NiNode* _BreakActionNode = getChildNode("WeaponMagazine", wpnNode);
				if (_BreakActionNode) {
					cBreakActionNode = _BreakActionNode;
					cBreakActionNodeRot.rot = cBreakActionNode->m_localTransform.rot;
					cOriginalBoltScale = cBreakActionNode->m_localTransform.scale;
					cClonedBreakActionNode = CloneThisNode(_BreakActionNode);
					if (cClonedBreakActionNode) {
						cClonedBreakActionNode->m_name = "ClonedBreakActionNode";
						cBreakActionNode->m_localTransform.scale = 0.0;
						cBreakActionNode->flags |= 0x1;
						wpnNode->AttachChild(cClonedBreakActionNode, true);
						removeNode = cClonedBreakActionNode;
						restoreNode = cBreakActionNode;
					}
				}
			}
			if (cReloadType == KConfReloadType_Cylinder) {
				cAmmoType = kConfAmmoType_MultiBullet;
				_cCustomTransform.pos.x = cOriginalBoltPosition;
				_cCustomTransform.pos.y = 3.0;
				_cCustomTransform.pos.z = 3.0;
				_cCustomTransform.scale = 0.0;
			}
			if (cReloadType == kConfReloadType_Laser) {
				cAmmoType = kConfAmmoType_FusionCell;
				NiNode* wpnExtra = getChildNode("WeaponExtra1", wpnNode);
				if (wpnExtra) {
					cLatchNode = FindMesh(wpnExtra, "LaserRifleLatch");
					/*wpnExtra->m_name.c_str();
					wpnExtra->m_children.m_emptyRunStart;
					if (wpnExtra->m_children.m_emptyRunStart != 0) {
						for (auto i = 0; i < wpnExtra->m_children.m_emptyRunStart; ++i) {
							auto nextNode = wpnExtra->m_children.m_data[i];
							if (nextNode) {
								BSTriShape* NodeTri = nextNode->GetAsBSTriShape();
								if (NodeTri) {
									BSFixedString TriName = NodeTri->m_name;
									if (matchSubString(TriName.c_str(), "LaserRifleLatch")) {
										cLatchNode = nextNode;
									}
								}
							}
						}
					}*/

				}
				_cCustomTransform.pos.y = 5.0;
				_cCustomTransform.pos.z = 5.0;
				_cCustomTransform.scale = 0.0;
			}
			if (cReloadType == kConfReloadType_LeverAction) {
				NiNode* wpnExtra = getChildNode("WeaponExtra3", wpnNode);
				if (wpnExtra) {
					NiAVObject* leverMesh = FindMesh(wpnExtra, "LeverActionGaurd");
					if (!leverMesh) {
						return; // this is where we'll look for third party weapon lever meshes to confirm type.
					}
					else {
						removeNode =  CloneThisNode(wpnExtra);
						restoreNode = wpnExtra;
						if (removeNode) {
							removeNode->m_name = "ClonedLeverActionNode";
							restoreNode->m_localTransform.scale = 0.0;
							restoreNode->flags |= 0x1;
							wpnNode->AttachChild(removeNode, true);
						}
					}
				}
			}
		}
	}
}