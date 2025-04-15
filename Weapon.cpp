#include "weapon.h"


namespace Reload {

	void CurrentWeapon::initWeapon(BGSKeyword* keyword) {
		ExtractWeaponDataFromInventory();
		auto lookup = g_weaponOffsets->getOffset(equippedFullName, normal);
		if (lookup.has_value()) {
			savedWeaponData = lookup.value();
			maxBoltPosition = savedWeaponData.pos.x;
			_MESSAGE(std::to_string(maxBoltPosition).c_str());
			ReloadType = static_cast<WeaponReloadType>(savedWeaponData.pos.y);
			ammoType = static_cast<WeaponAmmoType>(savedWeaponData.pos.z);
			Offsets::TESObjectREFR_AddKeyword(*g_player, keyword);
		}
		else {
			ReloadType = kReloadType_Unknown;
			return;
		}
		getWeaponNodes();
	}

	int CurrentWeapon::GetReserveAmmo() {
		GFxValue aCount;
		int ammoCount = 0;
		BSFixedString hudMenu("WSPrimaryWandHUD");
		IMenu* menu = (*g_ui)->GetMenu(hudMenu);
		if ((*g_ui)->IsMenuOpen(hudMenu)) {
			GFxMovieRoot* root = menu->movie->movieRoot;
			if (root) {
				if (root->GetVariable(&aCount, "root.tacticalHUD_mc.AmmoCount_mc.ReserveCount_tf.text")) {
					const char* acTemp = aCount.GetString();
					ammoCount = atoi(acTemp);
				}
			}
		}
		return ammoCount;
	}

	float CurrentWeapon::GetClipAmmoCount() {
		clipCount = ammoCapactity * Offsets::Actor_GetAmmoClipPercentage(*g_player, equipIndex);
		return clipCount;
	}

	void CurrentWeapon::ExtractWeaponDataFromInventory() {
		std::string weaponName;
		TESObjectWEAP* equippedItem = Offsets::GetEquippedWeapon((*g_gameVM)->m_virtualMachine, 0, *g_player, 0);
		if (equippedItem) {
			weaponName = equippedItem->GetFullName();
		}
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
					if (stack && weaponName == weapName) { // Only Visit Weapons with Matching Name to Eqipped
						int stackPos = 0;
						stack->Visit([&](BGSInventoryItem::Stack* element) {  //Start of Sub Stacks For this Weapon
							if (element) {
								ExtraDataList* stackDataList = element->extraData;
								if (stackDataList) {
									if (element->flags && element->kFlagEquipped && !isWeapEquipped) {
										ExtraInstanceData* currentEID = DYNAMIC_CAST(stackDataList->GetByType(kExtraData_InstanceData), BSExtraData, ExtraInstanceData);
										if (currentEID) {
											weaponInstanceData = (TESObjectWEAP::InstanceData*)currentEID->instanceData;
											weaponTBOInstanceData = currentEID->instanceData;
											ammoTypeForm = weaponInstanceData->ammo;
											magCapacity = weaponInstanceData->ammoCapacity;
											weaponInstance = new BGSObjectInstance(nullptr, nullptr);
											Offsets::Actor_GetWeaponEquipIndex(*g_player, &equipIndex, weaponInstance);
											baseForm = form;
											baseWeapon = weapon;
											equippedFullName = weaponName;
											weaponExtraDataList = stackDataList;
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
												equippedFullName = weaponName;
												baseForm = form;
												baseWeapon = weapon;
												magCapacity = weapon->weapData.ammoCapacity;
												ammoTypeForm = weapon->weapData.ammo;
												weaponInstance = new BGSObjectInstance(nullptr, nullptr);
												weaponExtraDataList = stackDataList;
												Offsets::Actor_GetWeaponEquipIndex(*g_player, &equipIndex, weaponInstance);
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
	}

	void CurrentWeapon::getWeaponNodes() {
		NiNode* wpnNode = getChildNode("Weapon", (*g_player)->firstPersonSkeleton->GetAsNiNode());
		if (wpnNode) {
			weaponNode = wpnNode;
			if (ReloadType == kReloadType_Bolt) {
				NiNode* recNode = getChildNode("P-Receiver", wpnNode);
				if (recNode) {
					NiNode* magNode = getChildNode("P-Mag", wpnNode);
					if (magNode) {
						magazineNode = magNode;
						clonedMagazineNode = CloneThisNode(magNode);
						if (clonedMagazineNode) {
							clonedMagazineNode->m_name = "ClonedMagazine";
						}
						originalMagScale = magNode->m_localTransform.scale;
					}
				}
				NiNode* _boltNode = getChildNode("WeaponBolt", wpnNode);
				if (_boltNode) {
					boltNode = _boltNode;
					originalBoltPosition = _boltNode->m_localTransform.pos.y;
					clonedBoltNode = CloneThisNode(_boltNode);
					if (clonedBoltNode) {
						clonedBoltNode->m_name = "ClonedBolt";
						originalBoltScale = _boltNode->m_localTransform.scale;
					}
				}
				boltPosDifference = std::abs(originalBoltPosition - maxBoltPosition);
				reloadSlidePos = originalBoltPosition - (boltPosDifference * 0.70);
			}
			if (ReloadType == kReloadType_BreakAction) {
				NiNode* _BreakActionNode = getChildNode("WeaponMagazine", wpnNode);
				if (_BreakActionNode) {
					breakActionNode = _BreakActionNode;
					originalBoltScale = breakActionNode->m_localTransform.scale;
					for (int i = 0; i < magCapacity; i++) {
						breakActionShellNode[i] = getChildNode(breakActionShellName[i].c_str(), _BreakActionNode);
						if (breakActionShellNode[i]) {
							if (i == 0) {
								originalMagScale = breakActionShellNode[i]->m_localTransform.scale;
							}
							breakActionShellScale[i] = breakActionShellNode[i]->m_localTransform.scale;
							breakActionShellNode[i]->m_localTransform.scale = 0.0;
							breakActionShellNode[i]->flags |= 0x1;
						}
					}
					clonedBreakActionNode = CloneThisNode(_BreakActionNode);
					if (clonedBreakActionNode) {
						clonedBreakActionNode->m_name = "ClonedBreakActionNode";
					}
					if (magCapacity <= 5) {
						for (int i = 0; i < magCapacity; i++) {
							if (breakActionShellNode[i]) {
								breakActionShellNode[i]->m_localTransform.scale = breakActionShellScale[i];
								breakActionShellNode[i]->flags &= 0xfffffffffffffffe;
								clonedBreakActionShellNode[i] = CloneThisNode(breakActionShellNode[i]);
								if (clonedBreakActionShellNode[i]) {
									clonedBreakActionShellNode[i]->m_localTransform.scale = 0.0;
									clonedBreakActionShellNode[i]->flags |= 0x1;
									clonedBreakActionShellNode[i]->m_name = clonedBreakActionShellName[i].c_str();
								}
							}
						}
					}
				}
			}
			if (ReloadType == KReloadType_Cylinder) {
				NiNode* _cylinderNode = getChildNode("WeaponMagazine", wpnNode);
				if (_cylinderNode) {
					cylinderNode = _cylinderNode;
					originalBoltScale = cylinderNode->m_localTransform.scale;
					for (int i = 0; i < magCapacity; i++) {
						cylinderBulletNode[i] = getChildNode(cylinderBulletName[i].c_str(), _cylinderNode);
						if (cylinderBulletNode[i]) {
							if (i == 0) {
								originalMagScale = cylinderBulletNode[i]->m_localTransform.scale;
							}
							cylinderBulletNode[i]->m_localTransform.scale = 0.0;
							cylinderBulletNode[i]->flags |= 0x1;
						}
					}
					clonedCylinderNode = CloneThisNode(_cylinderNode);
					clonedCylinderNodeBKUP = CloneThisNode(_cylinderNode);
					if (clonedCylinderNode) {
						clonedCylinderNode->m_name = "ClonedCylinderNode";
					}
					if (magCapacity <= 6) {
						for (int i = 0; i < magCapacity; i++) {
							if (cylinderBulletNode[i]) {
								cylinderBulletNode[i]->m_localTransform.scale = originalMagScale;
								cylinderBulletNode[i]->flags &= 0xfffffffffffffffe;
								clonedCylinderBulletNode[i] = CloneThisNode(cylinderBulletNode[i]);
								if (clonedCylinderBulletNode[i]) {
									clonedCylinderBulletNode[i]->m_localTransform.scale = 0.0;
									clonedCylinderBulletNode[i]->flags |= 0x1;
									clonedCylinderBulletNode[i]->m_name = clonedCylinderBulletName[i].c_str();
								}
							}
						}
					}
					savedWeaponData.rot.data[0][0] = -0.05906226858496666;
					savedWeaponData.rot.data[0][1] = -0.004241322632879019;
					savedWeaponData.rot.data[0][2] = 0.998248279094696;
					savedWeaponData.rot.data[0][3] = 0.0;
					savedWeaponData.rot.data[1][0] = -0.006301680579781532;
					savedWeaponData.rot.data[1][1] = 0.9999726414680481;
					savedWeaponData.rot.data[1][2] = 0.00387580250389874;
					savedWeaponData.rot.data[1][3] = 0.0;
					savedWeaponData.rot.data[2][0] = -0.9982383847236633;
					savedWeaponData.rot.data[2][1] = -0.006061736959964037;
					savedWeaponData.rot.data[2][2] = -0.059087324887514114;
					savedWeaponData.rot.data[2][3] = 0.0;
				}
			}
			if (ReloadType == kReloadType_Laser) {
				magazineNode = getChildNode("WeaponMagazine", wpnNode);
				if (magazineNode) {
					originalMagScale = magazineNode->m_localTransform.scale;
					clonedMagazineNode = CloneThisNode(magazineNode);
					if (clonedMagazineNode) {
						clonedMagazineNode->m_name = "ClonedMagazine";
					}
				}
				NiNode* wpnExtra = getChildNode("WeaponExtra1", wpnNode); 
				if (wpnExtra) {
					latchNode = FindMesh(wpnExtra, "LaserRifleLatch");
					if (latchNode) {
						originalWeaponData = latchNode->m_localTransform;
					}
				}
			}
			if (ReloadType == kReloadType_LeverAction) {
				NiNode* wpnExtra = getChildNode("WeaponExtra3", wpnNode);
				if (wpnExtra) {
					latchNode = FindMesh(wpnExtra, "LeverActionGaurd");
					if (!latchNode) {
						return; // this is where we'll look for third party weapon lever meshes to confirm type.
					}
					else {
						clonedBreakActionNode = CloneThisNode(wpnExtra);
						breakActionNode = wpnExtra;
						if (clonedBreakActionNode) {
							clonedBreakActionNode->m_name = "ClonedLeverActionNode";
						}
					}
				}
				NiNode* recNode = getChildNode("P-Receiver", wpnNode);
				if (recNode) {
					NiNode* magNode = getChildNode("P-Mag", wpnNode);
					if (magNode) {
						magazineNode = getChildNode("WeaponMagazineChild2", magNode);
						if (magazineNode) {
							_MESSAGE("FOUND LEVER MAG (3rd PARTY)");
							clonedMagazineNode = CloneThisNode(magNode);
							if (clonedMagazineNode) {
								clonedMagazineNode->m_name = "ClonedMagazine";
							}
							originalMagScale = magNode->m_localTransform.scale;
						}
					}
					else {
						magNode = getChildNode("WeaponMagazine", wpnNode);
						if (magNode) {
							magazineNode = getChildNode("WeaponMagazineChild2", magNode);
							if (magazineNode) {
								_MESSAGE("FOUND LEVER MAG (BETH)");
								clonedMagazineNode = CloneThisNode(magazineNode);
								if (clonedMagazineNode) {
									clonedMagazineNode->m_name = "ClonedMagazine";
								}
								originalMagScale = magNode->m_localTransform.scale;
							}
						}
					}
				}
			}
		}
	}
}