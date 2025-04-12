#include "Reload.h"
#include "NiCloneProcess.h"
#include "f4se/NiNodes.h"
#include "f4se/NiObjects.h"
#include "f4se/GameReferences.h"
#include "f4se/BSGeometry.h"
#include "f4se/BSCollision.h"


OpenVRHookManagerAPI* vrHook;

UInt16 ammoCapactity = 0;
std::map<std::string, int> boneTreeMap;
std::vector<std::string> boneTreeVec;


namespace Reload {

	bool firstrun = true;
	int c_reloadButtonID = vr::EVRButtonId::k_EButton_Grip;
	int c_grabAmmoButtonID = vr::EVRButtonId::k_EButton_Grip;
	int c_triggerButtonID = vr::EVRButtonId::k_EButton_SteamVR_Trigger;
	int c_configModeButtonID = vr::EVRButtonId::k_EButton_A;
	bool _isReloadPressed = false;
	bool _isTriggerPressed = false;
	bool _isGrabAmmoPressed = false;
	bool _isConfigModePressed = false;
	bool _isWithinMagInsertZone = false;
	bool _isMagInserting = false;
	bool _isInSlideZone = false;
	bool _isInBreakActionZone = false;
	bool _isInCylinderZone = false;
	bool _isGrabbingBolt = false;
	bool _maxSlideReached = false; 
	bool _minBreakActionReached = false;
	bool _minCylinderReached = false;
	bool _maxCylinderReached = true;
	bool isReloading = false;
	bool _isNewWeapon = false;
	bool removingPouch = false;
	bool addingPouch = false;
	bool boltsnap = false;
	float rAxisOffsetY;
	float distanceToGrip;
	float lastDistanceToGrip;
	std::map<UInt32, MyAmmoPack*> AmmoRegisteredObjects;
	bool initBoneTreeFlag = true;
	bool isAmmoIntersected = false;
	UInt32 curDevice;
	bool wpnChangeSticky = false;
	BGSKeyword* preventReload = nullptr;
	BGSKeyword* preventWPNFire = nullptr;
	BGSSoundDescriptorForm* dryFire = nullptr;
	BGSSoundDescriptorForm* magOut = nullptr;
	BGSSoundDescriptorForm* magIn = nullptr;
	BGSSoundDescriptorForm* ammoGrab = nullptr;
	BGSSoundDescriptorForm* boltOpen = nullptr;
	BGSSoundDescriptorForm* boltClose = nullptr;
	BGSSoundDescriptorForm* breakActionMagIn = nullptr;
	BGSSoundDescriptorForm* breakActionBoltOpen = nullptr;
	BGSSoundDescriptorForm* breakActionBoltClose = nullptr;
	BGSSoundDescriptorForm* cylinderMagIn = nullptr;
	BGSSoundDescriptorForm* cylinderBoltOpen = nullptr;
	BGSSoundDescriptorForm* cylinderBoltClose = nullptr;
	TESAmmo* emptyMag = nullptr;
	NiNode* hand = nullptr;
	NiPoint3 handpos;
	NiTransform GripPos;
	CurrentWeapon* thisWeapon = nullptr;
	ConfigMode* thisConfigMode = nullptr;
	Matrix44 minCylinderRot;
	NiNode* bkpCylinderNode = nullptr;

	bool initBoneTree() {
		BSFlattenedBoneTree* rt = (BSFlattenedBoneTree*)(*g_player)->unkF0->rootNode->m_children.m_data[0]->GetAsNiNode();
		if (!rt) {
			return false;
		}
		boneTreeMap.clear();
		boneTreeVec.clear();
		_MESSAGE("BoneTree Init -> Num Transforms = %d", rt->numTransforms);
		if (rt->numTransforms <= 0) {
			return false;
		}
		for (auto i = 0; i < rt->numTransforms; i++) {
			//_MESSAGE("BoneTree Init -> Push %s into position %d", rt->transforms[i].name.c_str(), i);
			boneTreeMap.insert({ rt->transforms[i].name.c_str(), i });
			boneTreeVec.push_back(rt->transforms[i].name.c_str());
		}
		_MESSAGE("BoneTree Init Completed");
		return true;
	}

	void mainUpdate() {
		if (firstrun) {
			MainInit();
		}
		if (isBallisticWeaponEquipped()) { //Only run if the player has a non-melee type weapon equipped.
			if (_isNewWeapon) {
				_isNewWeapon = false;
				setupWeapon();
				return;
			}
			VRButtonsMain();
			if (thisWeapon) {
				hand = getChildNode("LArm_Hand", (*g_player)->unkF0->rootNode);
			    wpnChangeSticky = false;
			    detectAmmoSphere();
				if (isAmmoInHand()) {
					detectMagNode();

				}
				if (requiresRacking() && !_isGrabbingBolt) {
					detectBolt();
				}
				if (thisWeapon->ReloadType == kReloadType_BreakAction && thisWeapon->clipCount > 0 && !_isGrabbingBolt && !isAmmoInHand()) {
					detectBreakAction();
				}
				if (thisWeapon->ReloadType == kReloadType_BreakAction && thisWeapon->clipCount > 0 && _isGrabbingBolt && !isAmmoInHand() && !_minBreakActionReached) {
					rotateWithBreakAction();
				}
				if (thisWeapon->ReloadType == KReloadType_Cylinder && thisWeapon->clipCount > 0 && !_isGrabbingBolt && !isAmmoInHand()) {
					detectCylinder();
				}
				if (thisWeapon->ReloadType == kReloadType_Laser && thisWeapon->clipCount > 0 && !_isGrabbingBolt && !isAmmoInHand()) {
					detectLaserLatch();
				}
				if (thisWeapon->ReloadType == KReloadType_Cylinder && thisWeapon->clipCount > 0 && _isGrabbingBolt && !isAmmoInHand() && !_minCylinderReached) {
					rotateWithCylinder();
				}
				if (thisWeapon->ReloadType == kReloadType_Laser && thisWeapon->clipCount > 0 && _isGrabbingBolt && !isAmmoInHand() && !_minCylinderReached) {
					rotateWithLaserLatch();
				}
				if (requiresRacking() && _isGrabbingBolt) {
					moveBolt();
				}
			}
			if (thisConfigMode != nullptr) {
				if (thisConfigMode->_isConfigModeActive) {
					thisConfigMode->updateUITiles();
				}
			}
		}
		else { //Restore everything ready for next weapon to be equipped.
			if (!wpnChangeSticky) {
				wpnChangeSticky = true;
				_isNewWeapon = true;
				isReloading = false;
				isAmmoIntersected = false;
				//_isReloadPressed = false;
				//_isTriggerPressed = false;
				_isGrabAmmoPressed = false;
				_isWithinMagInsertZone = false;
				_isInSlideZone = false;
				_isGrabbingBolt = false;
				_maxSlideReached = false;
				if (thisWeapon != nullptr) {
					if (isAmmoInHand()) {
						hand->RemoveChild(thisWeapon->clonedMagazineNode);
						thisWeapon->clonedMagazineNode = nullptr;
					}
				}
				removingPouch = true;
				destroyAmmoPouch();
				delete thisWeapon;
				thisWeapon = nullptr;
				Offsets::TESObjectREFR_RemoveKeyword(*g_player, preventReload);
			}
		}
	}

	void MainInit() {
		vrHook = RequestOpenVRHookManagerObject();
		if (VRHook::g_vrHook != nullptr) {
			VRHook::g_vrHook->setVRControllerState();
		}
		else {
			VRHook::InitVRSystem();
			VRHook::g_vrHook->setVRControllerState();
		}
		preventReload = DYNAMIC_CAST(GetFormFromFile(0x000F99, "VirtualReloads.esp"), TESForm, BGSKeyword);
		preventWPNFire = DYNAMIC_CAST(GetFormFromFile(0x001733, "VirtualReloads.esp"), TESForm, BGSKeyword);
		//dryFire = DYNAMIC_CAST(GetFormFromFile(0x01EA80, "Fallout4.esm"), TESForm, BGSSoundDescriptorForm);
		dryFire = DYNAMIC_CAST(GetFormFromFile(0x002E03, "VirtualReloads.esp"), TESForm, BGSSoundDescriptorForm);
		magOut = DYNAMIC_CAST(GetFormFromFile(0x002E05, "VirtualReloads.esp"), TESForm, BGSSoundDescriptorForm);
		magIn = DYNAMIC_CAST(GetFormFromFile(0x002E04, "VirtualReloads.esp"), TESForm, BGSSoundDescriptorForm);
		ammoGrab = DYNAMIC_CAST(GetFormFromFile(0x0347DF, "Fallout4.esm"), TESForm, BGSSoundDescriptorForm);
		boltOpen = DYNAMIC_CAST(GetFormFromFile(0x002E07, "VirtualReloads.esp"), TESForm, BGSSoundDescriptorForm);
		boltClose = DYNAMIC_CAST(GetFormFromFile(0x002E06, "VirtualReloads.esp"), TESForm, BGSSoundDescriptorForm);
		breakActionMagIn = DYNAMIC_CAST(GetFormFromFile(0x0035A1, "VirtualReloads.esp"), TESForm, BGSSoundDescriptorForm);
		breakActionBoltOpen = DYNAMIC_CAST(GetFormFromFile(0x0035A2, "VirtualReloads.esp"), TESForm, BGSSoundDescriptorForm);
		breakActionBoltClose = DYNAMIC_CAST(GetFormFromFile(0x0035A3, "VirtualReloads.esp"), TESForm, BGSSoundDescriptorForm);
		cylinderMagIn = DYNAMIC_CAST(GetFormFromFile(0x0044D6, "VirtualReloads.esp"), TESForm, BGSSoundDescriptorForm);
		cylinderBoltOpen = DYNAMIC_CAST(GetFormFromFile(0x0044D7, "VirtualReloads.esp"), TESForm, BGSSoundDescriptorForm);
		cylinderBoltClose = DYNAMIC_CAST(GetFormFromFile(0x0044D8, "VirtualReloads.esp"), TESForm, BGSSoundDescriptorForm);
		emptyMag = DYNAMIC_CAST(GetFormFromFile(0x001ECE, "VirtualReloads.esp"), TESForm, TESAmmo);
		
		if (initBoneTreeFlag) {
			if (!initBoneTree()) {
				return;
			}
			initBoneTreeFlag = false;
		}
		createAmmoSpheres();
		readOffsetJson();
		firstrun = false;
	}

	void setupWeapon() {
		thisWeapon = new CurrentWeapon; //Create new weapon class on equip / detection.
		thisWeapon->initWeapon(preventReload);
	}

	void VRButtonsMain() {
		if (VRHook::g_vrHook != nullptr) {
			VRHook::g_vrHook->setVRControllerState();
			uint64_t primaryHand = VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).ulButtonPressed;
			uint64_t secondaryHand = VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).ulButtonPressed;
			vr::VRControllerAxis_t doinantHandTrigger = VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).rAxis[3];
			const auto reloadButtonPressed = primaryHand & vr::ButtonMaskFromId((vr::EVRButtonId)c_reloadButtonID);
			const auto triggerButtonPressed = primaryHand & vr::ButtonMaskFromId((vr::EVRButtonId)c_triggerButtonID);
			const auto grabAmmoButtonPressed = secondaryHand & vr::ButtonMaskFromId((vr::EVRButtonId)c_grabAmmoButtonID);
			const auto configModeButtonPressed = secondaryHand & vr::ButtonMaskFromId((vr::EVRButtonId)c_configModeButtonID);
			if (configModeButtonPressed && !_isConfigModePressed) {
				_isConfigModePressed = true;
				thisConfigMode = new ConfigMode;
				int rlm;
				if (thisWeapon) {
					rlm = static_cast<int>(thisWeapon->ReloadType);
					thisConfigMode->cReloadType = static_cast<cWeaponReloadType>(rlm);
				}
				else {
					rlm = 0;
					thisConfigMode->cReloadType = kConfReloadType_Unknown;
				}
				thisConfigMode->StartConfigMode(rlm);
			}
			else if (!configModeButtonPressed) {
				_isConfigModePressed = false;
			}

			// Reload Button:
			if (reloadButtonPressed && !_isReloadPressed && !isReloading) {  //Needs changing to detect long / quick presses for grenade throws....
				_isReloadPressed = true;
				if (thisWeapon != nullptr) {
					isReloading = true;
					startReload();
				}
			}
			else if (!reloadButtonPressed) {
				_isReloadPressed = false;
			}
			//Trigger Button:
			if (triggerButtonPressed && !_isTriggerPressed) {
				_isTriggerPressed = true;
				if (thisWeapon != nullptr) {
					playDryFire();
				}
			}
			else if (!triggerButtonPressed) {
				_isTriggerPressed = false;
			}
			if (thisWeapon && isReloading) {
				//Secondary hand grip button:
				if (grabAmmoButtonPressed && !_isGrabAmmoPressed && isAmmoIntersected && !isAmmoInHand()) {
					_isGrabAmmoPressed = true;
					grabAmmmo();
				}
				else if (grabAmmoButtonPressed && !_isGrabAmmoPressed && requiresRacking() && _isInSlideZone && !_isGrabbingBolt) {
					_isGrabbingBolt = true;
					grabBolt();
				}
				else if (grabAmmoButtonPressed && thisWeapon->ReloadType == kReloadType_Bolt && _isInBreakActionZone && !_isGrabAmmoPressed && !_isGrabbingBolt && thisWeapon->clipCount > 0 && !isAmmoInHand()) {
					_isGrabbingBolt = true;
					gripBreakAction();
				}
				else if (grabAmmoButtonPressed && thisWeapon->ReloadType == KReloadType_Cylinder && _isInCylinderZone && !_isGrabAmmoPressed && !_isGrabbingBolt && thisWeapon->clipCount > 0 && !isAmmoInHand()) {
					_isGrabbingBolt = true;
					_maxCylinderReached = false;
					gripCylinder();
				}
				else if (grabAmmoButtonPressed && thisWeapon->ReloadType == kReloadType_Laser && _isInCylinderZone && !_isGrabAmmoPressed && !_isGrabbingBolt && thisWeapon->clipCount > 0 && !isAmmoInHand()) {
					_isGrabbingBolt = true;
					_maxCylinderReached = false;
					gripLaserLatch();
				}
				else if (!grabAmmoButtonPressed) {
					_isGrabAmmoPressed = false;
					_isGrabbingBolt = false;
					if (isAmmoInHand() && !_isWithinMagInsertZone) {
						dropAmmo();
					}
					else if (isAmmoInHand() && _isWithinMagInsertZone) {
						insertAmmo();
					}
					else if (thisWeapon->ReloadType == kReloadType_Bolt && !_maxSlideReached) {
						retoreBolt2();
					}
					else if (thisWeapon->ReloadType == kReloadType_Bolt && _maxSlideReached) {
						Offsets::PlaySoundAtActor(boltClose, *g_player);
						retoreBolt();
					}
					else if (thisWeapon->ReloadType == kReloadType_BreakAction && !_minBreakActionReached) {
						rotateBreakAction();
					}
					else if (thisWeapon->ReloadType == KReloadType_Cylinder && !_minCylinderReached && thisWeapon->clipCount > 0 && !_maxCylinderReached) {
						rotateCylinder();
					}
					else if (thisWeapon->ReloadType == kReloadType_Laser && !_minCylinderReached && thisWeapon->clipCount > 0 && !_maxCylinderReached) {
						rotateLaserLatch();
					}
				}
			}
			if (thisConfigMode!=nullptr) {
				if (thisConfigMode->_isConfigModeActive) {
					vr::VRControllerAxis_t doinantHandStick = (VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).rAxis[0]);
					if (thisConfigMode->cReloadType == kConfReloadType_Bolt && thisConfigMode->cClonedBoltNode != nullptr) {
						if (doinantHandStick.y > 0.50 || doinantHandStick.y < -0.50) {
						rAxisOffsetY = doinantHandStick.y / 10;
							//if ((thisConfigMode->cClonedBoltNode->m_localTransform.pos.y + rAxisOffsetY) > 15 || (thisConfigMode->cClonedBoltNode->m_localTransform.pos.y + rAxisOffsetY) < -15) {
								//return;
							//}
							thisConfigMode->cClonedBoltNode->m_localTransform.pos.y = (thisConfigMode->cClonedBoltNode->m_localTransform.pos.y - rAxisOffsetY);
							thisConfigMode->_cCustomTransform.pos.x = thisConfigMode->cClonedBoltNode->m_localTransform.pos.y;
							thisConfigMode->_cCustomTransform.pos.y = 1.0;
							thisConfigMode->_cCustomTransform.pos.z = 1.0;
						}
					}
					if (thisConfigMode->cReloadType == kConfReloadType_BreakAction && thisConfigMode->cClonedBreakActionNode != nullptr) {
						if (doinantHandStick.y > 0.50 || doinantHandStick.y < -0.50) {
							Matrix44 rot;
							rAxisOffsetY = doinantHandStick.y / 10;
							rot.setEulerAngles((degrees_to_rads(rAxisOffsetY)), 0, 0);
							thisConfigMode->cClonedBreakActionNode->m_localTransform.rot = rot.multiply43Left(thisConfigMode->cClonedBreakActionNode->m_localTransform.rot);
							rot.multiply43Left(thisConfigMode->cClonedBreakActionNode->m_localTransform.rot);
							thisConfigMode->_cCustomTransform.rot = thisConfigMode->cClonedBreakActionNode->m_localTransform.rot;
							thisConfigMode->_cCustomTransform.pos.x = thisConfigMode->cOriginalBoltPosition;
							thisConfigMode->_cCustomTransform.pos.y = 2.0;
							thisConfigMode->_cCustomTransform.pos.z = 2.0;
						}
					}
					if (thisConfigMode->cReloadType == kConfReloadType_Laser && thisConfigMode->cLatchNode != nullptr) {
						if (doinantHandStick.y > 0.50 || doinantHandStick.y < -0.50) {
							Matrix44 rot;
							rAxisOffsetY = doinantHandStick.y / 5;
							rot.setEulerAngles(0, (degrees_to_rads(rAxisOffsetY)), 0);
							thisConfigMode->cLatchNode->m_localTransform.rot = rot.multiply43Left(thisConfigMode->cLatchNode->m_localTransform.rot);
							rot.multiply43Left(thisConfigMode->cLatchNode->m_localTransform.rot);
							thisConfigMode->_cCustomTransform.rot = thisConfigMode->cLatchNode->m_localTransform.rot;
							//thisConfigMode->_cCustomTransform.pos.x = thisConfigMode->cOriginalBoltPosition;
							thisConfigMode->_cCustomTransform.pos.y = 5.0;
							thisConfigMode->_cCustomTransform.pos.z = 5.0;
						}
					}
				}
			}
		}
	}

	void startReload() {
		if (thisWeapon != nullptr) {
			if (thisWeapon->ReloadType != kReloadType_Unknown) {
				if (thisWeapon->ReloadType == kReloadType_Bolt) {
					if (thisWeapon->weaponNode && thisWeapon->clonedBoltNode && thisWeapon->boltNode && thisWeapon->magazineNode) {
						Offsets::Actor_SetCurrentAmmoCount(*g_player, thisWeapon->equipIndex, 0);
						thisWeapon->weaponNode->AttachChild(thisWeapon->clonedBoltNode, true);
						thisWeapon->boltNode->flags |= 0x1; // hide the original bolt node until weapon reload it completed
						thisWeapon->boltNode->m_localTransform.scale = 0.0;
						if (thisWeapon->ammoType == kAmmoType_Magazine) { // hide the weapons magazine - to do: simulate a magazine dropping to the floor. 
							thisWeapon->magazineNode->flags |= 0x1;
							thisWeapon->magazineNode->m_localTransform.scale = 0;
							Offsets::PlaySoundAtActor(magOut, *g_player);
							NEW_REFR_DATA* refrData = new NEW_REFR_DATA();
							refrData->location = thisWeapon->magazineNode->m_worldTransform.pos;
							refrData->direction = (*g_player)->rot;
							refrData->interior = (*g_player)->parentCell;
							refrData->world = Offsets::TESObjectREFR_GetWorldSpace(*g_player);
							ExtraDataList* extraData = (ExtraDataList*)Offsets::MemoryManager_Allocate(g_mainHeap, 0x28, 0, false);
							Offsets::ExtraDataList_ExtraDataList(extraData);
							extraData->m_refCount += 1;
							Offsets::ExtraDataList_setCount(extraData, 10);
							refrData->extra = extraData;
							refrData->object = emptyMag;
							void* ammoDrop = new std::size_t;
							void* newHandle = Offsets::TESDataHandler_CreateReferenceAtLocation(*g_dataHandler, ammoDrop, refrData);
							std::uintptr_t newRefr = 0x0;
							Offsets::BSPointerHandleManagerInterface_GetSmartPointer(newHandle, &newRefr);
							TESObjectREFR* currentRefr = (TESObjectREFR*)newRefr;
							if (currentRefr) {
								waitFor3D(currentRefr);
							}
						}
						showAmmoPouch();
					}
				}
				if (thisWeapon->ReloadType == kReloadType_BreakAction) {
					int currentAmmoCount = thisWeapon->magCapacity * Offsets::Actor_GetAmmoClipPercentage(*g_player, thisWeapon->equipIndex);
					std::string mes = "Mag Capacity: " + std::to_string(thisWeapon->magCapacity);
					_MESSAGE(mes.c_str());
					if (currentAmmoCount == thisWeapon->magCapacity) {
						isReloading = false;
						return; // cambers are full do nothing.
					}
					if (thisWeapon->weaponNode && thisWeapon->clonedBreakActionNode) {
						thisWeapon->weaponNode->AttachChild(thisWeapon->clonedBreakActionNode, true);
						for (int i = 0; i < thisWeapon->magCapacity; i++) {
							if (thisWeapon->clonedBreakActionShellNode[i]) {
								thisWeapon->clonedBreakActionNode->AttachChild(thisWeapon->clonedBreakActionShellNode[i], true);
								thisWeapon->clonedBreakActionShellNode[i]->m_localTransform.scale = 0.0;
								thisWeapon->clonedBreakActionShellNode[i]->flags |= 0x1;
							}
						}
					}
					if (thisWeapon->breakActionNode) {
						thisWeapon->breakActionNode->m_localTransform.scale = 0.0;
						thisWeapon->breakActionNode->flags |= 0x1;
					}
					int ammoToShow = thisWeapon->magCapacity - currentAmmoCount;
					if (ammoToShow != thisWeapon->magCapacity) {
						for (int i = 1; i < thisWeapon->magCapacity; i++) {
							if (thisWeapon->clonedBreakActionShellNode[i]) {
								thisWeapon->clonedBreakActionShellNode[i]->m_localTransform.scale = thisWeapon->breakActionShellScale[i];
								thisWeapon->clonedBreakActionShellNode[i]->flags &= 0xfffffffffffffffe;
							}
						}
					}
					if (thisWeapon->breakActionShellNode[0]) {
						thisWeapon->magazineNode = CloneThisNode(thisWeapon->breakActionShellNode[0]);
						if (thisWeapon->magazineNode && thisWeapon->clonedBreakActionNode) {
							thisWeapon->clonedBreakActionNode->AttachChild(thisWeapon->magazineNode, true);
							thisWeapon->magazineNode->m_localTransform.scale = 0.0;
							thisWeapon->magazineNode->flags |= 0x1;
						}
						thisWeapon->clonedMagazineNode = CloneThisNode(thisWeapon->breakActionShellNode[0]);
						if (thisWeapon->clonedMagazineNode) {
							thisWeapon->clonedMagazineNode->m_name = "ClonedMagazine";
						}
					}
					Offsets::PlaySoundAtActor(breakActionBoltOpen, *g_player);
					thisWeapon->reserveAmmo = thisWeapon->GetReserveAmmo();
					thisWeapon->chamberNum = 0;
					rotateBreakAction();
					showAmmoPouch();
					thisWeapon->clipCount = currentAmmoCount;
					Offsets::Actor_SetCurrentAmmoCount(*g_player, thisWeapon->equipIndex, 0);
				}
				if (thisWeapon->ReloadType == KReloadType_Cylinder) {
					int currentAmmoCount = thisWeapon->magCapacity * Offsets::Actor_GetAmmoClipPercentage(*g_player, thisWeapon->equipIndex);
					std::string mes = "Mag Capacity: " + std::to_string(thisWeapon->magCapacity);
					_MESSAGE(mes.c_str());
					if (currentAmmoCount == thisWeapon->magCapacity) {
						isReloading = false;
						_MESSAGE("MAG FULL DO NOTHING");
						return; // cambers are full do nothing.
					}
					if (thisWeapon->weaponNode && thisWeapon->clonedCylinderNode) {
						thisWeapon->weaponNode->AttachChild(thisWeapon->clonedCylinderNode, true);
						//bkpCylinderNode = CloneThisNode(thisWeapon->clonedCylinderNode);
						for (int i = 0; i < thisWeapon->magCapacity; i++) {
							if (thisWeapon->clonedCylinderBulletNode[i]) {
								thisWeapon->clonedCylinderNode->AttachChild(thisWeapon->clonedCylinderBulletNode[i], true);
								thisWeapon->clonedCylinderBulletNode[i]->m_localTransform.scale = 0.0;
								thisWeapon->clonedCylinderBulletNode[i]->flags |= 0x1;
							}
						}
					}
					if (thisWeapon->cylinderNode) {
						thisWeapon->cylinderNode->m_localTransform.scale = 0.0;
						thisWeapon->cylinderNode->flags |= 0x1;
					}
					int ammoToShow = thisWeapon->magCapacity - currentAmmoCount;
					std::string mes2 = "AMMO to SHOW: " + std::to_string(ammoToShow) + " Current Ammo Count: " + std::to_string(currentAmmoCount) + "Original Mag Size: " + std::to_string(thisWeapon->originalMagScale);
					_MESSAGE(mes2.c_str());
					if (ammoToShow != thisWeapon->magCapacity) {
						for (int i = ammoToShow; i < thisWeapon->magCapacity; i++) {
							if (thisWeapon->clonedCylinderBulletNode[i]) {
								thisWeapon->clonedCylinderBulletNode[i]->m_localTransform.scale = thisWeapon->originalMagScale;
								thisWeapon->clonedCylinderBulletNode[i]->flags &= 0xfffffffffffffffe;

							}
						}
					}
					if (thisWeapon->cylinderBulletNode[0]) {
						thisWeapon->magazineNode = CloneThisNode(thisWeapon->cylinderBulletNode[0]);
						if (thisWeapon->magazineNode && thisWeapon->clonedCylinderNode) {
							thisWeapon->clonedCylinderNode->AttachChild(thisWeapon->magazineNode, true);
							thisWeapon->magazineNode->m_localTransform.scale = 0.0;
							thisWeapon->magazineNode->flags |= 0x1;
						}
						thisWeapon->clonedMagazineNode = CloneThisNode(thisWeapon->cylinderBulletNode[0]);
						if (thisWeapon->clonedMagazineNode) {
							thisWeapon->clonedMagazineNode->m_name = "ClonedMagazine";
						}
					}
					Offsets::PlaySoundAtActor(cylinderBoltOpen, *g_player);
					thisWeapon->reserveAmmo = thisWeapon->GetReserveAmmo();
					thisWeapon->chamberNum = 0;
					minCylinderRot.makeTransformMatrix(thisWeapon->clonedCylinderNode->m_localTransform.rot, NiPoint3(0, 0, 0));
					Matrix44 maxRot;
					float rotMinX;
					float rotMinY;
					float rotMinZ;
					float rotMaxX;
					float rotMaxY;
					float rotMaxZ;
					minCylinderRot.getEulerAngles(&rotMinX, &rotMinY, &rotMinZ);
					maxRot.makeTransformMatrix(thisWeapon->savedWeaponData.rot, NiPoint3(0, 0, 0));
					maxRot.getEulerAngles(&rotMaxX, &rotMaxY, &rotMaxZ);
					std::string mes4 = "X:" + std::to_string(rotMaxX) + " Y:" + std::to_string(rotMaxY) + " Z:" + std::to_string(rotMaxZ);
					_MESSAGE(mes4.c_str());
					_maxCylinderReached = false;
					_minCylinderReached = false;
					rotateCylinder();
					showAmmoPouch();
					thisWeapon->clipCount = currentAmmoCount;
					Offsets::Actor_SetCurrentAmmoCount(*g_player, thisWeapon->equipIndex, 0);
				}
				if (thisWeapon->ReloadType == kReloadType_Laser) {
					if (thisWeapon->weaponNode && thisWeapon->magazineNode) {
						Offsets::Actor_SetCurrentAmmoCount(*g_player, thisWeapon->equipIndex, 0);
						_maxCylinderReached = false;
						thisWeapon->clipCount = 0;
						_minCylinderReached = false;
						rotateLaserLatch();
					}
				}

			}
		}
		else {
			isReloading = false;
		}

	}

	void waitFor3D(TESObjectREFR* object)
	{
		auto loadedData = object->unkF0;
		if (loadedData == nullptr) {
			std::thread t4(waitFor3D, object);
			t4.detach();
			return;
		}
		else {
			if (loadedData->rootNode == nullptr) {
				std::thread t5(waitFor3D, object);
				t5.detach();
				return;
			}
			else {
				if (thisWeapon->magazineNode) {
					NiNode* dropMag = CloneThisNode(thisWeapon->magazineNode);
					NiNode* magRoot = object->GetObjectRootNode()->GetAsNiNode();
					if (dropMag && magRoot) {
						//magRoot->m_localTransform.scale = 0;
						dropMag->flags &= 0xfffffffffffffffe;
						dropMag->m_localTransform.scale = thisWeapon->originalMagScale;
						magRoot->AttachChild(dropMag, true);
						//magRoot->m_spCollisionObject = nullptr;
					}
					//Offsets::DeleteRef(object);
					std::thread t6(deleteSpentClip, object);
					t6.detach();
					return;
				}
			}
		}
	}

	void deleteSpentClip(TESObjectREFR* object) {
			Sleep(5000);
			Offsets::DeleteRef(object);
			return;
	}

	void playDryFire() {
		if (thisWeapon) {
			int currentAmmoCount = thisWeapon->magCapacity * Offsets::Actor_GetAmmoClipPercentage(*g_player, thisWeapon->equipIndex);
			if (currentAmmoCount == 0) {
				Offsets::PlaySoundAtActor(dryFire, *g_player);
				vrHook->StartHaptics(2, 0.025, 0.3);
				return;
			}
			return;
		}
	}

	void grabAmmmo() {
		if (hand && thisWeapon->clonedMagazineNode) {
			thisWeapon->clonedMagazineNode->flags &= 0xfffffffffffffffe;
			thisWeapon->clonedMagazineNode->m_localTransform.scale = thisWeapon->originalMagScale;
			hand->AttachChild(thisWeapon->clonedMagazineNode, true);
			thisWeapon->clonedMagazineNode->m_localTransform.pos.y = thisWeapon->clonedMagazineNode->m_localTransform.pos.y - 3.5;
			thisWeapon->clonedMagazineNode->m_localTransform.pos.x = thisWeapon->clonedMagazineNode->m_localTransform.pos.x + 6.0;
			Offsets::PlaySoundAtActor(ammoGrab, *g_player);
			isAmmoIntersected = false;
			AmmoRegisteredObjects[0]->debugSphere->flags |= 0x1;
			AmmoRegisteredObjects[0]->debugSphere->m_localTransform.scale = 0;
			vrHook->StartHaptics(1, 0.05, 1.0);
		}
	}

	void dropAmmo() {
		if (hand && thisWeapon->clonedMagazineNode) {
			NEW_REFR_DATA* refrData = new NEW_REFR_DATA();
			refrData->location = thisWeapon->clonedMagazineNode->m_worldTransform.pos;
			if (hand) {
				hand->RemoveChild(thisWeapon->clonedMagazineNode);
			}
			thisWeapon->clonedMagazineNode = nullptr;
			thisWeapon->clonedMagazineNode = CloneThisNode(thisWeapon->magazineNode);
			if (thisWeapon->clonedMagazineNode) {
				thisWeapon->clonedMagazineNode->m_name = "ClonedMagazine";
			}
			refrData->direction = (*g_player)->rot;
			refrData->interior = (*g_player)->parentCell;
			refrData->world = Offsets::TESObjectREFR_GetWorldSpace(*g_player);
			ExtraDataList* extraData = (ExtraDataList*)Offsets::MemoryManager_Allocate(g_mainHeap, 0x28, 0, false);
			Offsets::ExtraDataList_ExtraDataList(extraData);
			extraData->m_refCount += 1;
			Offsets::ExtraDataList_setCount(extraData, 10);
			refrData->extra = extraData;
			refrData->object = emptyMag;
			void* ammoDrop = new std::size_t;
			void* newHandle = Offsets::TESDataHandler_CreateReferenceAtLocation(*g_dataHandler, ammoDrop, refrData);
			std::uintptr_t newRefr = 0x0;
			Offsets::BSPointerHandleManagerInterface_GetSmartPointer(newHandle, &newRefr);
			TESObjectREFR* currentRefr = (TESObjectREFR*)newRefr;
			if (currentRefr) {
				waitFor3D(currentRefr);
			}
		}
	}

	void insertAmmo() {
		if (thisWeapon->clonedMagazineNode && thisWeapon->magazineNode) {
			thisWeapon->clipCount++;
			thisWeapon->chamberNum++;
			thisWeapon->reserveAmmo = thisWeapon->reserveAmmo - 1;
			std::string mes = "reserve Ammo: " + std::to_string(thisWeapon->reserveAmmo);
			_MESSAGE(mes.c_str());
			if (hand) {
				hand->RemoveChild(thisWeapon->clonedMagazineNode);
			}
			thisWeapon->clonedMagazineNode = nullptr;
			thisWeapon->clonedMagazineNode = CloneThisNode(thisWeapon->magazineNode);
			thisWeapon->clonedMagazineNode->m_name = "ClonedMagazine";
			thisWeapon->magazineNode->flags &= 0xfffffffffffffffe;
			thisWeapon->magazineNode->m_localTransform.scale = thisWeapon->originalMagScale;
			if (thisWeapon->ReloadType == kReloadType_Bolt) {
				Offsets::PlaySoundAtActor(magIn, *g_player);
			}
			else if (thisWeapon->ReloadType == kReloadType_BreakAction) {
				Offsets::PlaySoundAtActor(breakActionMagIn, *g_player);
			}
			else if (thisWeapon->ReloadType == KReloadType_Cylinder) {
				Offsets::PlaySoundAtActor(cylinderMagIn, *g_player);
			}
			vrHook->StartHaptics(2, 0.05, 1.0);
			if (thisWeapon->ReloadType == kReloadType_BreakAction && thisWeapon->clipCount < thisWeapon->magCapacity && thisWeapon->reserveAmmo != 0 && thisWeapon->clonedBreakActionNode && thisWeapon->breakActionShellNode[thisWeapon->clipCount]) {
				thisWeapon->magazineNode = CloneThisNode(thisWeapon->breakActionShellNode[thisWeapon->chamberNum]);
				if (thisWeapon->magazineNode) {
					thisWeapon->magazineNode->m_localTransform.scale = 0.0;
					thisWeapon->magazineNode->flags |= 0x1;
					thisWeapon->clonedBreakActionNode->AttachChild(thisWeapon->magazineNode, true);
				}
			}
			else if (thisWeapon->ReloadType == KReloadType_Cylinder && thisWeapon->clipCount < thisWeapon->magCapacity && thisWeapon->reserveAmmo != 0 && thisWeapon->clonedCylinderNode && thisWeapon->cylinderBulletNode[thisWeapon->clipCount]) {
				thisWeapon->magazineNode = CloneThisNode(thisWeapon->cylinderBulletNode[thisWeapon->chamberNum]);
				if (thisWeapon->magazineNode) {
					thisWeapon->magazineNode->m_localTransform.scale = 0.0;
					thisWeapon->magazineNode->flags |= 0x1;
					thisWeapon->clonedCylinderNode->AttachChild(thisWeapon->magazineNode, true);
				}
			}
			else {
				removingPouch = true;
				destroyAmmoPouch();
			}
		}
	}

	bool isAmmoInHand() {
		if (hand) {
			NiNode* Ammo = getChildNode("ClonedMagazine", hand);
			if (Ammo) {
				return true;
			}
			else {
				return false;
			}
		}
		else {
			return false;
		}
	}

	bool gunRequiresMag() {
		if (thisWeapon->magazineNode) {
			//if (thisWeapon->magazineNode->m_localTransform.scale == 0.0) {
			if (thisWeapon->magazineNode->flags & 0x1) {
				_MESSAGE("Weapon needs Mag");
				return true;
			}
			else {
				_MESSAGE("Weapon has Mag");
				return false;
			}
		}
		return false;
	}

	void detectMagNode() {
		if (thisWeapon->clonedMagazineNode && thisWeapon->magazineNode) {
			float distance = vec3_len(thisWeapon->clonedMagazineNode->m_worldTransform.pos - thisWeapon->magazineNode->m_worldTransform.pos);
			if (distance < 10.0) {
				if (!_isWithinMagInsertZone) {
					vrHook->StartHaptics(1, 0.05, 0.3);
				}
				_isWithinMagInsertZone = true;
			}
			else {
				_isWithinMagInsertZone = false;
			}
		}
	}

	bool requiresRacking() {
		if (thisWeapon->magazineNode && thisWeapon->weaponNode) {
			NiNode* node = getChildNode("ClonedBolt", thisWeapon->weaponNode);
			if (node && thisWeapon->magazineNode->m_localTransform.scale != 0) {
				return true;
			}
			else {
				return false;
			}
		}
		return false;
	}

	void grabBolt() {
		if (hand) {
			handpos = hand->m_worldTransform.pos;
		}
		if (hand && thisWeapon) {
			NiNode* grip = nullptr;
			if (thisWeapon->weaponNode) {
				//grip = getChildNode("p-grip", thisWeapon->weaponNode);
				grip = getChildNode("Camera", (*g_player)->unkF0->rootNode);
			}
			if (grip) {
				lastDistanceToGrip = vec3_len(hand->m_worldTransform.pos - grip->m_worldTransform.pos);
			}
		}
		Offsets::PlaySoundAtActor(boltOpen, *g_player);
		vrHook->StartHaptics(2, 0.05, 0.25);
		vrHook->StartHaptics(1, 0.05, 0.25);
		boltsnap = true;	
	}

	void detectBolt() {
		float distance;
		if (hand && thisWeapon->boltNode) {
			distance = vec3_len(hand->m_worldTransform.pos - thisWeapon->boltNode->m_worldTransform.pos);
			if (distance < 15.0) {
				if (!_isInSlideZone) {
					vrHook->StartHaptics(1, 0.05, 0.3);
				}
				_isInSlideZone = true;
			}
			else {
				_isInSlideZone = false;
			}
		}
	}

	void moveBolt() {
		if (thisWeapon->clonedBoltNode && hand) {
			NiNode* grip = nullptr;
			if (thisWeapon->weaponNode) {
				//grip = getChildNode("p-grip", thisWeapon->weaponNode);
				grip = getChildNode("RArm_ForeArm2", (*g_player)->unkF0->rootNode);
			}
			float distance;
			distance = vec3_len(hand->m_worldTransform.pos - handpos);
			if (grip) {
				distanceToGrip = vec3_len(hand->m_worldTransform.pos - grip->m_worldTransform.pos);
			}
			if (hand && thisWeapon->clonedBoltNode) {
				float curPos = thisWeapon->clonedBoltNode->m_localTransform.pos.y;
				if (distanceToGrip < lastDistanceToGrip) {
					if (curPos > thisWeapon->maxBoltPosition) {
						thisWeapon->clonedBoltNode->m_localTransform.pos.y = thisWeapon->clonedBoltNode->m_localTransform.pos.y - distance;
					}
					else {
						if (!_maxSlideReached) {
							vrHook->StartHaptics(2, 0.05, 0.5);
						}
						_maxSlideReached = true;
					}
				}
				else if (distanceToGrip > lastDistanceToGrip) {
					if (curPos < thisWeapon->originalBoltPosition) {
						thisWeapon->clonedBoltNode->m_localTransform.pos.y = thisWeapon->clonedBoltNode->m_localTransform.pos.y + distance;
					}
				}
			}
			handpos = hand->m_worldTransform.pos;
			lastDistanceToGrip = distanceToGrip;
		}
	}

	void retoreBolt() {
		if (isReloading) {
			if (thisWeapon->weaponNode && thisWeapon->boltNode && thisWeapon->clonedBoltNode) {
				float curPos = thisWeapon->clonedBoltNode->m_localTransform.pos.y;
				if (thisWeapon->clonedBoltNode->m_localTransform.pos.y < thisWeapon->originalBoltPosition) {
					thisWeapon->clonedBoltNode->m_localTransform.pos.y = thisWeapon->clonedBoltNode->m_localTransform.pos.y + 0.02;
					std::thread t1(retoreBolt);
					t1.detach();
					return;
				}
				else {
					thisWeapon->weaponNode->RemoveChild(thisWeapon->clonedBoltNode);
					thisWeapon->boltNode->flags &= 0xfffffffffffffffe;
					thisWeapon->boltNode->m_localTransform.scale = thisWeapon->originalBoltScale;
					thisWeapon->clonedBoltNode = nullptr;
					thisWeapon->clonedBoltNode = CloneThisNode(thisWeapon->boltNode);
					thisWeapon->clonedBoltNode->m_name = "ClonedBolt";
					int reserveAmmo = thisWeapon->GetReserveAmmo();
					if (reserveAmmo < thisWeapon->magCapacity) {
						Offsets::Actor_SetCurrentAmmoCount(*g_player, thisWeapon->equipIndex, reserveAmmo);
					}
					else {
						Offsets::Actor_SetCurrentAmmoCount(*g_player, thisWeapon->equipIndex, thisWeapon->magCapacity);
					}
					_isReloadPressed = false;
					_isTriggerPressed = false;
					_isGrabAmmoPressed = false;
					_isWithinMagInsertZone = false;
					_isGrabbingBolt = false;
					_maxSlideReached = false;
					isReloading = false;
					vrHook->StartHaptics(2, 0.05, 1.0);
				}
			}
		}
	}

	void retoreBolt2() {
		if (isReloading) {
			if (thisWeapon->weaponNode && thisWeapon->boltNode && thisWeapon->clonedBoltNode) {
				float curPos = thisWeapon->clonedBoltNode->m_localTransform.pos.y;
				if (thisWeapon->clonedBoltNode->m_localTransform.pos.y < thisWeapon->originalBoltPosition) {
					thisWeapon->clonedBoltNode->m_localTransform.pos.y = thisWeapon->clonedBoltNode->m_localTransform.pos.y + 0.02;
					std::thread t7(retoreBolt2);
					t7.detach();
					return;
				}
				else {
					if (boltsnap) {
						vrHook->StartHaptics(2, 0.05, 5.0);
						Offsets::PlaySoundAtActor(boltOpen, *g_player);
						boltsnap = false;
					}
					return;
				}
			}
		}
	}

	void rotateBreakAction() {
		Matrix44 rot;
		Matrix44 rot2;
		if (thisWeapon->clonedBreakActionNode) {
			//_MESSAGE("ROTATING BREAK ACTION");
			_minBreakActionReached = false;
			rot.makeTransformMatrix(thisWeapon->clonedBreakActionNode->m_localTransform.rot, NiPoint3(0, 0, 0));
			rot2.makeTransformMatrix(thisWeapon->savedWeaponData.rot, NiPoint3(0, 0, 0));
			float rotx;
			float roty;
			float rotz;
			float rot2x;
			float rot2y;
			float rot2z;
			rot.getEulerAngles(&rotx, &roty, &rotz);
			rot2.getEulerAngles(&rot2x, &rot2y, &rot2z);
			if (rotx > rot2x) {
				rot.setEulerAngles((degrees_to_rads(0.10)), 0, 0);
				thisWeapon->clonedBreakActionNode->m_localTransform.rot = rot.multiply43Left(thisWeapon->clonedBreakActionNode->m_localTransform.rot);
				rot.multiply43Left(thisWeapon->clonedBreakActionNode->m_localTransform.rot);
				std::thread t8(rotateBreakAction);
				t8.detach();
				return;
			}
		}
	}

	void detectBreakAction() {
		float distance;
		if (hand && thisWeapon->breakActionNode) {
			distance = vec3_len(hand->m_worldTransform.pos - thisWeapon->breakActionNode->m_worldTransform.pos);
			if (distance < 15.0) {
				if (!_isInBreakActionZone) {
					vrHook->StartHaptics(1, 0.05, 0.3);
				}
				_isInBreakActionZone = true;
			}
			else {
				_isInBreakActionZone = false;
			}
		}
	}

	void gripBreakAction() {
		if (hand) {
			handpos = hand->m_worldTransform.pos;
		}
		if (hand && thisWeapon) {
			NiNode* grip = nullptr;
			if (thisWeapon->weaponNode) {
				grip = getChildNode("camera", (*g_player)->unkF0->rootNode);
			}
			if (grip) {
				lastDistanceToGrip = vec3_len(hand->m_worldTransform.pos - grip->m_worldTransform.pos);
			}
		}
		Offsets::PlaySoundAtActor(boltOpen, *g_player);
		vrHook->StartHaptics(2, 0.05, 0.25);
		vrHook->StartHaptics(1, 0.05, 0.25);
		boltsnap = true;
	}

	void restoreBreakAction() {

	}

	void restoreBreakAction2() {

	}

	void rotateWithBreakAction() {
		NiNode* grip = nullptr;
		if (thisWeapon->weaponNode) {
			grip = getChildNode("camera", (*g_player)->unkF0->rootNode);
		}
		float distance;
		distance = vec3_len(hand->m_worldTransform.pos - handpos);
		distance = distance * 10;
		if (grip) {
			distanceToGrip = vec3_len(hand->m_worldTransform.pos - grip->m_worldTransform.pos);
		}
		Matrix44 rot;
		Matrix44 maxRot;
		Matrix44 minRot;
		if (thisWeapon->clonedBreakActionNode) {
			rot.makeTransformMatrix(thisWeapon->clonedBreakActionNode->m_localTransform.rot, NiPoint3(0, 0, 0));
			maxRot.makeTransformMatrix(thisWeapon->savedWeaponData.rot, NiPoint3(0, 0, 0));
			minRot.makeTransformMatrix(thisWeapon->breakActionNode->m_localTransform.rot, NiPoint3(0, 0, 0));
			float rotx;
			float roty;
			float rotz;
			float rotMaxX;
			float rotMaxY;
			float rotMaxZ;
			float rotMinX;
			float rotMinY;
			float rotMinZ;
			rot.getEulerAngles(&rotx, &roty, &rotz);
			maxRot.getEulerAngles(&rotMaxX, &rotMaxY, &rotMaxZ);
			minRot.getEulerAngles(&rotMinX, &rotMinY, &rotMinZ);
			if (distanceToGrip > lastDistanceToGrip) {
				if (rotx > rotMaxX) {
					rot.setEulerAngles((degrees_to_rads(distance)), 0, 0);
					thisWeapon->clonedBreakActionNode->m_localTransform.rot = rot.multiply43Left(thisWeapon->clonedBreakActionNode->m_localTransform.rot);
					rot.multiply43Left(thisWeapon->clonedBreakActionNode->m_localTransform.rot);
				}
			}
			if (distanceToGrip < lastDistanceToGrip) {
				if (rotx < rotMinX) {
					distance = distance * -1;
					rot.setEulerAngles((degrees_to_rads(distance)), 0, 0);
					thisWeapon->clonedBreakActionNode->m_localTransform.rot = rot.multiply43Left(thisWeapon->clonedBreakActionNode->m_localTransform.rot);
					rot.multiply43Left(thisWeapon->clonedBreakActionNode->m_localTransform.rot);
				}
				else if (rotx >= rotMinX && !_minBreakActionReached) {
					_minBreakActionReached = true;
					vrHook->StartHaptics(2, 0.05, 0.5);
					Offsets::Actor_SetCurrentAmmoCount(*g_player, thisWeapon->equipIndex, thisWeapon->clipCount);
					thisWeapon->breakActionNode->m_localTransform.scale = thisWeapon->originalBoltScale;
					thisWeapon->breakActionNode->flags &= 0xfffffffffffffffe;
					thisWeapon->weaponNode->RemoveChild(thisWeapon->clonedBreakActionNode);
					thisWeapon->clonedBreakActionNode = nullptr;
					thisWeapon->getWeaponNodes();
					Offsets::PlaySoundAtActor(breakActionBoltClose, *g_player);
					removingPouch = true;
					destroyAmmoPouch();
					isReloading = false;

				}
			}
			handpos = hand->m_worldTransform.pos;
			lastDistanceToGrip = distanceToGrip;
		}

	}

	void rotateCylinder() {
		Matrix44 rot;
		Matrix44 rot2;
		if (thisWeapon->clonedCylinderNode) {
			rot.makeTransformMatrix(thisWeapon->clonedCylinderNode->m_localTransform.rot, NiPoint3(0, 0, 0));
			rot2.makeTransformMatrix(thisWeapon->savedWeaponData.rot, NiPoint3(0, 0, 0));
			float rotx;
			float roty;
			float rotz;
			float rot2x;
			float rot2y;
			float rot2z;
			rot.getEulerAngles(&rotx, &roty, &rotz);
			rot2.getEulerAngles(&rot2x, &rot2y, &rot2z);
			if (rot2y > 0) {
				rot2y = rot2y * -1;
			}
			//_MESSAGE(std::to_string(roty).c_str());
			if (roty > rot2y && !_maxCylinderReached) {
				rot.setEulerAngles(0, (degrees_to_rads(0.10)), 0);
				thisWeapon->clonedCylinderNode->m_localTransform.rot = rot.multiply43Right(thisWeapon->clonedCylinderNode->m_localTransform.rot);
				rot.multiply43Right(thisWeapon->clonedCylinderNode->m_localTransform.rot);
				std::thread t9(rotateCylinder);
				t9.detach();
				return;
			}
			else {
				_maxCylinderReached = true;
			}
		}
	}

	void detectCylinder() {
		float distance;
		if (hand && thisWeapon->cylinderNode) {
			distance = vec3_len(hand->m_worldTransform.pos - thisWeapon->cylinderNode->m_worldTransform.pos);
			if (distance < 15.0) {
				if (!_isInCylinderZone) {
					vrHook->StartHaptics(1, 0.05, 0.3);
				}
				_isInCylinderZone = true;
			}
			else {
				_isInCylinderZone = false;
			}
		}
	}

	void rotateWithCylinder() {
		NiNode* grip = nullptr;
		if (thisWeapon->weaponNode) {
			grip = getChildNode("camera", (*g_player)->unkF0->rootNode);
		}
		float distance;
		distance = vec3_len(hand->m_worldTransform.pos - handpos);
		distance = distance * 10;
		if (grip) {
			distanceToGrip = vec3_len(hand->m_worldTransform.pos - grip->m_worldTransform.pos);
		}
		Matrix44 rot;
		Matrix44 maxRot;
		Matrix44 minRot;
		if (thisWeapon->clonedCylinderNode) {
			rot.makeTransformMatrix(thisWeapon->clonedCylinderNode->m_localTransform.rot, NiPoint3(0, 0, 0));
			maxRot.makeTransformMatrix(thisWeapon->savedWeaponData.rot, NiPoint3(0, 0, 0));
			//minRot.makeTransformMatrix(thisWeapon->cylinderNode->m_localTransform.rot, NiPoint3(0, 0, 0));
			float rotx;
			float roty;
			float rotz;
			float rotMaxX;
			float rotMaxY;
			float rotMaxZ;
			float rotMinX;
			float rotMinY;
			float rotMinZ;
			//float rotyb;
			rot.getEulerAngles(&rotx, &roty, &rotz);
			maxRot.getEulerAngles(&rotMaxX, &rotMaxY, &rotMaxZ);
			if (rotMaxY > 0) {
				rotMaxY = rotMaxY * -1;
			}
			minCylinderRot.getEulerAngles(&rotMinX, &rotMinY, &rotMinZ);
			if (distanceToGrip > lastDistanceToGrip) {
				if (roty > rotMaxY) {
					//distance = distance * -1;
					rot.setEulerAngles(0, (degrees_to_rads(distance)), 0);
					thisWeapon->clonedCylinderNode->m_localTransform.rot = rot.multiply43Right(thisWeapon->clonedCylinderNode->m_localTransform.rot);
					rot.multiply43Right(thisWeapon->clonedCylinderNode->m_localTransform.rot);
				}
			}
			if (distanceToGrip < lastDistanceToGrip) {
				if (roty < rotMinY) {
					_maxCylinderReached = false;
					distance = distance * -1;
					rot.setEulerAngles(0, (degrees_to_rads(distance)), 0);
					thisWeapon->clonedCylinderNode->m_localTransform.rot = rot.multiply43Right(thisWeapon->clonedCylinderNode->m_localTransform.rot);
					rot.multiply43Right(thisWeapon->clonedCylinderNode->m_localTransform.rot);
				}
				else if (roty >= rotMinY && !_minCylinderReached) {
					std::string mes = "pos: " + std::to_string(roty) + " min pos: " + std::to_string(rotMinY);
					_MESSAGE(mes.c_str());
					_minCylinderReached = true;
					vrHook->StartHaptics(2, 0.05, 0.5);
					Offsets::Actor_SetCurrentAmmoCount(*g_player, thisWeapon->equipIndex, thisWeapon->clipCount);
					thisWeapon->cylinderNode->m_localTransform.scale = thisWeapon->originalBoltScale;
					thisWeapon->cylinderNode->flags &= 0xfffffffffffffffe;
					thisWeapon->weaponNode->RemoveChild(thisWeapon->clonedCylinderNode);
					thisWeapon->clonedCylinderNode = nullptr;
					if (thisWeapon->clonedCylinderNodeBKUP) {
						thisWeapon->clonedCylinderNode = CloneThisNode(thisWeapon->clonedCylinderNodeBKUP);
						for (int i = 0; i < thisWeapon->magCapacity; i++) {
							if (thisWeapon->cylinderBulletNode[i]) {
								thisWeapon->clonedCylinderBulletNode[i] = CloneThisNode(thisWeapon->cylinderBulletNode[i]);
								if (thisWeapon->clonedCylinderBulletNode[i]) {
									thisWeapon->clonedCylinderBulletNode[i]->m_localTransform.scale = 0.0;
									thisWeapon->clonedCylinderBulletNode[i]->flags |= 0x1;
									thisWeapon->clonedCylinderBulletNode[i]->m_name = thisWeapon->clonedCylinderBulletName[i].c_str();
								}
							}
						}
					}
					//thisWeapon->getWeaponNodes();
					Offsets::PlaySoundAtActor(cylinderBoltClose, *g_player);
					removingPouch = true;
					destroyAmmoPouch();
					isReloading = false;
				}
			}
			handpos = hand->m_worldTransform.pos;
			lastDistanceToGrip = distanceToGrip;
		}
	}

	void gripCylinder() {
		if (hand) {
			handpos = hand->m_worldTransform.pos;
		}
		if (hand && thisWeapon) {
			NiNode* grip = nullptr;
			if (thisWeapon->weaponNode) {
				grip = getChildNode("camera", (*g_player)->unkF0->rootNode);
			}
			if (grip) {
				lastDistanceToGrip = vec3_len(hand->m_worldTransform.pos - grip->m_worldTransform.pos);
			}
		}
		Offsets::PlaySoundAtActor(boltOpen, *g_player);
		vrHook->StartHaptics(2, 0.05, 0.25);
		vrHook->StartHaptics(1, 0.05, 0.25);
		boltsnap = true;
	}

	void rotateLaserLatch() {
		Matrix44 rot;
		Matrix44 rot2;
		if (thisWeapon->latchNode) {
			rot.makeTransformMatrix(thisWeapon->latchNode->m_localTransform.rot, NiPoint3(0, 0, 0));
			rot2.makeTransformMatrix(thisWeapon->savedWeaponData.rot, NiPoint3(0, 0, 0));
			float rotx;
			float roty;
			float rotz;
			float rot2x;
			float rot2y;
			float rot2z;
			rot.getEulerAngles(&rotx, &roty, &rotz);
			rot2.getEulerAngles(&rot2x, &rot2y, &rot2z);
			//if (rot2y > 0) {
				//rot2y = rot2y * -1;
			//}
			//_MESSAGE(std::to_string(roty).c_str());
			std::string mes = "current: " + std::to_string(roty) + " Target: " + std::to_string(rot2y);
			if (roty > rot2y && !_maxCylinderReached) {
				rot.setEulerAngles(0, (degrees_to_rads(0.10)), 0);
				thisWeapon->latchNode->m_localTransform.rot = rot.multiply43Right(thisWeapon->latchNode->m_localTransform.rot);
				rot.multiply43Right(thisWeapon->latchNode->m_localTransform.rot);
				std::thread t10(rotateLaserLatch);
				t10.detach();
				return;
			}
			else {
				_maxCylinderReached = true;
				if (thisWeapon->magazineNode && thisWeapon->clipCount == 0) {
					thisWeapon->magazineNode->m_localTransform.scale = 0.0;
					thisWeapon->magazineNode->flags |= 0x1;
					thisWeapon->weaponNode->AttachChild(thisWeapon->magazineNode, true);
					Offsets::PlaySoundAtActor(magOut, *g_player);
					NEW_REFR_DATA* refrData = new NEW_REFR_DATA();
					refrData->location = thisWeapon->magazineNode->m_worldTransform.pos;
					refrData->direction = (*g_player)->rot;
					refrData->interior = (*g_player)->parentCell;
					refrData->world = Offsets::TESObjectREFR_GetWorldSpace(*g_player);
					ExtraDataList* extraData = (ExtraDataList*)Offsets::MemoryManager_Allocate(g_mainHeap, 0x28, 0, false);
					Offsets::ExtraDataList_ExtraDataList(extraData);
					extraData->m_refCount += 1;
					Offsets::ExtraDataList_setCount(extraData, 10);
					refrData->extra = extraData;
					refrData->object = emptyMag;
					void* ammoDrop = new std::size_t;
					void* newHandle = Offsets::TESDataHandler_CreateReferenceAtLocation(*g_dataHandler, ammoDrop, refrData);
					std::uintptr_t newRefr = 0x0;
					Offsets::BSPointerHandleManagerInterface_GetSmartPointer(newHandle, &newRefr);
					TESObjectREFR* currentRefr = (TESObjectREFR*)newRefr;
					if (currentRefr) {
						waitFor3D(currentRefr);
					}
					showAmmoPouch();
				}
			}
		}
	}

	void gripLaserLatch() {
		if (hand) {
			handpos = hand->m_worldTransform.pos;
		}
		if (hand && thisWeapon) {
			NiNode* grip = nullptr;
			if (thisWeapon->weaponNode) {
				grip = getChildNode("camera", (*g_player)->unkF0->rootNode);
			}
			if (grip) {
				lastDistanceToGrip = vec3_len(hand->m_worldTransform.pos - grip->m_worldTransform.pos);
			}
		}
		Offsets::PlaySoundAtActor(boltOpen, *g_player);
		vrHook->StartHaptics(2, 0.05, 0.25);
		vrHook->StartHaptics(1, 0.05, 0.25);
		boltsnap = true;
	}

	void detectLaserLatch() {
		float distance;
		if (hand && thisWeapon->latchNode) {
			distance = vec3_len(hand->m_worldTransform.pos - thisWeapon->latchNode->m_worldTransform.pos);
			if (distance < 15.0) {
				if (!_isInCylinderZone) {
					vrHook->StartHaptics(1, 0.05, 0.3);
				}
				_isInCylinderZone = true;
			}
			else {
				_isInCylinderZone = false;
			}
		}
	}

	void rotateWithLaserLatch() {
		NiNode* grip = nullptr;
		if (thisWeapon->weaponNode) {
			grip = getChildNode("camera", (*g_player)->unkF0->rootNode);
		}
		float distance;
		distance = vec3_len(hand->m_worldTransform.pos - handpos);
		distance = distance * 10;
		if (grip) {
			distanceToGrip = vec3_len(hand->m_worldTransform.pos - grip->m_worldTransform.pos);
		}
		Matrix44 rot;
		Matrix44 maxRot;
		Matrix44 minRot;
		if (thisWeapon->latchNode) {
			rot.makeTransformMatrix(thisWeapon->latchNode->m_localTransform.rot, NiPoint3(0, 0, 0));
			maxRot.makeTransformMatrix(thisWeapon->savedWeaponData.rot, NiPoint3(0, 0, 0));
			minRot.makeTransformMatrix(thisWeapon->originalWeaponData.rot, NiPoint3(0, 0, 0));
			float rotx;
			float roty;
			float rotz;
			float rotMaxX;
			float rotMaxY;
			float rotMaxZ;
			float rotMinX;
			float rotMinY;
			float rotMinZ;
			rot.getEulerAngles(&rotx, &roty, &rotz);
			maxRot.getEulerAngles(&rotMaxX, &rotMaxY, &rotMaxZ);
			minRot.getEulerAngles(&rotMinX, &rotMinY, &rotMinZ);
			if (distanceToGrip > lastDistanceToGrip) {
				if (roty > rotMaxY) {
					rot.setEulerAngles(0, (degrees_to_rads(distance)), 0);
					thisWeapon->latchNode->m_localTransform.rot = rot.multiply43Right(thisWeapon->latchNode->m_localTransform.rot);
					rot.multiply43Right(thisWeapon->latchNode->m_localTransform.rot);
				}
			}
			if (distanceToGrip < lastDistanceToGrip) {
				if (roty < rotMinY) {
					_maxCylinderReached = false;
					distance = distance * -1;
					rot.setEulerAngles(0, (degrees_to_rads(distance)), 0);
					thisWeapon->latchNode->m_localTransform.rot = rot.multiply43Right(thisWeapon->latchNode->m_localTransform.rot);
					rot.multiply43Right(thisWeapon->latchNode->m_localTransform.rot);
				}
				else if (roty >= rotMinY && !_minCylinderReached) {
					std::string mes = "pos: " + std::to_string(roty) + " min pos: " + std::to_string(rotMinY);
					_MESSAGE(mes.c_str());
					_minCylinderReached = true;
					vrHook->StartHaptics(2, 0.05, 0.5);
					Offsets::Actor_SetCurrentAmmoCount(*g_player, thisWeapon->equipIndex, thisWeapon->magCapacity);
					Offsets::PlaySoundAtActor(cylinderBoltClose, *g_player);
					removingPouch = true;
					destroyAmmoPouch();
					isReloading = false;
				}
			}
			handpos = hand->m_worldTransform.pos;
			lastDistanceToGrip = distanceToGrip;
		}
	}

	void showAmmoPouch() {
		NiNode* retNode = loadNifFromFile("Data/Meshes/VRR/AmmoPouch.nif");
		NiNode* ammoPouch = CloneThisNode(retNode);
		if (ammoPouch) {
			ammoPouch->m_name = BSFixedString("AmmoPouch");
			NiNode* duplicate = getChildNode("AmmoPouch", (*g_player)->unkF0->rootNode);
			if (duplicate) {
				duplicate->flags |= 0x1;
				duplicate->m_localTransform.scale = 0;
				duplicate->m_parent->RemoveChild(duplicate);
			}
			NiNode* bone = getChildNode("Chest", (*g_player)->unkF0->rootNode);
			bone->AttachChild((NiAVObject*)ammoPouch, true);
			ammoPouch->m_localTransform.pos.x = (ammoPouch->m_localTransform.pos.x + -7.0);
			ammoPouch->m_localTransform.pos.y = (ammoPouch->m_localTransform.pos.y + 12.0);
			ammoPouch->m_localTransform.pos.z = (ammoPouch->m_localTransform.pos.z + 0.0);
			ammoPouch->m_localTransform.scale = 0;
			addingPouch = true;
			scaleAmmoPouch();
		}
	}

	void scaleAmmoPouch() {
		NiNode* ammoPouch = getChildNode("AmmoPouch", (*g_player)->unkF0->rootNode);
		if (ammoPouch) {
			if (ammoPouch->m_localTransform.scale < 1.0 && addingPouch) {
				ammoPouch->m_localTransform.scale = ammoPouch->m_localTransform.scale + 0.001;
				std::thread t3(scaleAmmoPouch);
				t3.detach();
			}
			else {
				addingPouch = false;
			}
		}
	}

	void destroyAmmoPouch() {

		NiNode* AmmoPouch = getChildNode("AmmoPouch", (*g_player)->unkF0->rootNode);
		if (AmmoPouch) {
			if (AmmoPouch->m_localTransform.scale > 0 && removingPouch) {
				AmmoPouch->m_localTransform.scale = AmmoPouch->m_localTransform.scale - 0.001;
				std::thread t2(destroyAmmoPouch);
				t2.detach();
			}
			else {
				AmmoPouch->flags |= 0x1;
				AmmoPouch->m_parent->RemoveChild(AmmoPouch);
				removingPouch = false;
			}
		}
	}

	void detectAmmoSphere() {
		if (!gunRequiresMag() || isAmmoInHand()) {
			std::string mes = "Requires Mag: " + std::to_string(gunRequiresMag()) + " Ammo in Hand: " + std::to_string(isAmmoInHand());
			//_MESSAGE(mes.c_str());
			return;
		}
		if ((*g_player)->firstPersonSkeleton == nullptr) {
			return;
		}
		if ((BSFadeNode*)(*g_player)->unkF0->rootNode->m_children.m_data[0]->GetAsNiNode()) {
			BSFlattenedBoneTree* rt = (BSFlattenedBoneTree*)(*g_player)->unkF0->rootNode->m_children.m_data[0]->GetAsNiNode();
			NiPoint3 lFinger;
			lFinger = rt->transforms[boneTreeMap["LArm_Finger23"]].world.pos;
			NiPoint3 offset;
			for (auto const& element : AmmoRegisteredObjects) {
				offset = element.second->bone->m_worldTransform.rot * element.second->offset;
				offset = element.second->bone->m_worldTransform.pos + offset;
				double dist = (double)vec3_len(lFinger - offset);
				if ((dist <= ((double)element.second->radius - 0.1)) && (isAmmoIntersected == false)) {
					if ((*g_player)->actorState.IsWeaponDrawn() && (AmmoRegisteredObjects[element.first])) {
						TESObjectWEAP* currentWeap = Offsets::GetEquippedWeapon((*g_gameVM)->m_virtualMachine, 0, *g_player, 0);
						if (!currentWeap) {   //prevents sphere block if "fists" are equipped as no weapon is returned.
							_MESSAGE("NO WEAPON");
							return;
						}
					}
					if (!element.second->stickyRight) {
						element.second->stickyRight = true;
						UInt32 handle = element.first;
						UInt32 device = 1;
						curDevice = device;
						vrHook->StartHaptics(1, 0.05, 0.3);
						isAmmoIntersected = true;
						//gCurHolster = handle;
						BSFixedString NameofMesh = element.second->MeshName;
						NiNode* holster = element.second->debugSphere;
						if (!holster) {
							return;
						}
						//holster->flags &= 0xfffffffffffffffe;
						//float f = 12.0;
						//holster->m_localTransform.scale = f;
						
					}
				}
				else if (dist >= ((double)element.second->radius + 0.1)) {
					if (element.second->stickyRight) {
						element.second->stickyRight = false;
						BSFixedString NameofMesh = element.second->MeshName;
						NiNode* holster = element.second->debugSphere;
						if (!holster) {
							return;
						}
						UInt32 handle = element.first;
						UInt32 device = 1;
						curDevice = 0;
						//holster->flags |= 0x1;
						//holster->m_localTransform.scale = 0;
						isAmmoIntersected = false;
					}
				}
			}
		}
	}

	void displayAmmoSphere() {
		if (AmmoRegisteredObjects[0]) {
			NiNode* bone = AmmoRegisteredObjects[0]->bone;
			NiNode* hol;
			BSFixedString NameofMesh = AmmoRegisteredObjects[0]->MeshName;
			NiNode* retNode = loadNifFromFile("Data/Meshes/VRR/1x1Sphere.nif");
			NiCloneProcess proc;
			proc.unk18 = Offsets::cloneAddr1;
			proc.unk48 = Offsets::cloneAddr2;
			hol = Offsets::cloneNode(retNode, &proc);
			if (!hol) {
				_MESSAGE("NIF NOT LOADED");
			}
			if (hol) {
				hol->m_name = BSFixedString(NameofMesh);
				if (hol->m_children.m_emptyRunStart > 0) {
					for (auto i = 0; i < hol->m_children.m_emptyRunStart; ++i) {
						auto nextNode = hol->m_children.m_data[i];
						if (nextNode) {
							BSTriShape* NodeTri = nextNode->GetAsBSTriShape();
							if (NodeTri) {
								std::string n = "AmmoMesh" + std::to_string(i);
								nextNode->m_name = n.c_str();
							}
						}
					}
				}
				NiNode* Weap2 = getChildNode(NameofMesh, (*g_player)->unkF0->rootNode); //Check for accidental spawned clones & delete
				if (Weap2) {
					Weap2->flags |= 0x1;
					Weap2->m_localTransform.scale = 0;
					Weap2->m_parent->RemoveChild(Weap2);
				}
				bone->AttachChild((NiAVObject*)hol, true);
				hol->m_localTransform.scale = (AmmoRegisteredObjects[0]->radius * 2);
				AmmoRegisteredObjects[0]->debugSphere = hol;
				hol->m_localTransform.pos.x = (hol->m_localTransform.pos.x + AmmoRegisteredObjects[0]->offset.x);
				hol->m_localTransform.pos.y = (hol->m_localTransform.pos.y + AmmoRegisteredObjects[0]->offset.y);
				hol->m_localTransform.pos.z = (hol->m_localTransform.pos.z + AmmoRegisteredObjects[0]->offset.z);
				hol->flags |= 0x1;
				hol->m_localTransform.scale = 0;
				_MESSAGE("SPHERE ATTACHED");
			}
		}

	}

	void registerAmmoSphere(float radius, BSFixedString bone, NiPoint3 pos, BSFixedString MeshPath, BSFixedString MeshName) {
		if (radius == 0.0) {
			return;
		}
		if (!(*g_player)->unkF0) {
			_MESSAGE("can't register yet as new game");
			return;
		}
		NiNode* boneNode = FindNode(bone);
		if (!boneNode) {
			return;
		}
		NiPoint3 offsetVec;
		MyAmmoPack* hol = new MyAmmoPack(radius, boneNode, pos, MeshPath, MeshName);
		AmmoRegisteredObjects[0] = hol;
		displayAmmoSphere();
		return;
	}

	void createAmmoSpheres() {
		BSFixedString bone = "chest";
		BSFixedString MeshPath = "AmmoPack";
		BSFixedString MeshName = "AmmoPack";
		NiPoint3 pos;
		pos.x = -7.0;
		pos.y = 10.0;
		pos.z = 0.0;
		float radius = 13.598394393920898;
		NiNode* boneNode = FindNode(bone);
		if (!boneNode) {
			return;
		}
		registerAmmoSphere(radius, bone, pos, MeshPath, MeshName);
	}

	void UITile01Function() { // switch reload types
		if (thisConfigMode != nullptr) {
			int rlm = static_cast<int>(thisConfigMode->cReloadType);
			if (rlm > 4) {
				rlm = 0;
			}
			else {
				rlm++;
			}
			thisConfigMode->cReloadType = static_cast<cWeaponReloadType>(rlm);
			thisConfigMode->pullWeaponNodes(thisConfigMode->cReloadType);
		}
	}

	void UITile02Function() {
		_MESSAGE("FUNCTION 2");
	}

	void UITile03Function() { //save config && exit
		if (thisConfigMode != nullptr) {
			if (thisConfigMode->cReloadType != kConfReloadType_Unknown) {
				g_weaponOffsets->addOffset(thisConfigMode->weaponName, thisConfigMode->_cCustomTransform, normal);
				writeOffsetJson();
				thisConfigMode->ExitConfigMode();
				delete thisConfigMode;
				thisConfigMode = nullptr;
			}
			else {
				thisConfigMode->ExitConfigMode();
				delete thisConfigMode;
				thisConfigMode = nullptr;
			}
		}
	}
}