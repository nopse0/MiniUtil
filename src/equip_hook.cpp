#include "equip_hook.h"

namespace equip {

	// Check via NEIF didn't work (I don't understand this), so we check via TESDataHandler instead
	bool IsSurvivalModItem_ViaDataHandler(RE::TESForm* a_form)
	{
		if (!a_form) return false;

		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler) return false;

		// 1. Extrahiere den Mod-Index aus der FormID (oberstes Byte)
		// Bei einer ID wie 0x1204103d schiebt das den Wert um 24 Bits nach rechts -> 0x12
		uint8_t modIndex = static_cast<uint8_t>((a_form->formID & 0xFF000000) >> 24);

		// 2. Spezialfall: Dynamische Laufzeit-Objekte (z.B. In-Game geschmiedet) beginnen mit 0xFF
		if (modIndex == 0xFF) return false;

		// 3. Spezialfall: Light-Plugins (FE:XXX). Falls Campfire/Hunterborn als ESL laufen würden:
		if (modIndex == 0xFE) {
			// Bei ESLs sind die nächsten 12 Bits (Bits 12-23) der Light-Mod-Index
			uint16_t lightModIndex = static_cast<uint16_t>((a_form->formID & 0x00FFF000) >> 12);

			// Prüfe, ob dieser Index zu Campfire oder Hunterborn gehört
			auto campfireIndex = dataHandler->GetLoadedLightModIndex("Campfire.esm");
			if (campfireIndex && *campfireIndex == lightModIndex) return true;

			auto hunterbornIndex = dataHandler->GetLoadedLightModIndex("Hunterborn.esp");
			if (hunterbornIndex && *hunterbornIndex == lightModIndex) return true;

			return false;
		}

		// 4. Standardfall für normale ESM/ESP (wie deine 0x12):
		// Hole die Indizes, an denen Campfire und Hunterborn in DIESEM Savegame geladen sind
		auto campfireIndex = dataHandler->GetLoadedModIndex("Campfire.esm");
		if (campfireIndex && *campfireIndex == modIndex) {
			return true;
		}

		auto hunterbornIndex = dataHandler->GetLoadedModIndex("Hunterborn.esp");
		if (hunterbornIndex && *hunterbornIndex == modIndex) {
			return true;
		}

		return false;
	}

	bool PlayerCanCraftAmmo(RE::TESBoundObject* a_ammo)
	{
		if (!a_ammo || !a_ammo->IsAmmo()) return false;

		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler) return false;

		auto* player = RE::PlayerCharacter::GetSingleton();

		// Wir holen uns das Array als generische TESForm-Pointer, um den Linker zu umgehen
		// BGSConstructibleObject hat den nativer FormType 47 (kCOBJ)
		const auto& genericForms = dataHandler->GetFormArray(RE::FormType::ConstructibleObject);

		for (auto* form : genericForms) {
			if (!form) continue;

			// Wir casten den Pointer manuell (ohne dynamic_cast, um vtable-Lookups zu vermeiden)
			// Da wir vorher nach FormType::ConstructibleObject gefiltert haben, ist das absolut sicher.
			auto* recipe = reinterpret_cast<RE::BGSConstructibleObject*>(form);

			if (recipe && recipe->createdItem == a_ammo) {

				// Wenn das Rezept keine Bedingungen hat, darf es jeder herstellen
				// Wir greifen direkt auf das struct-Mitglied "conditions" zu
				if (!recipe->conditions.head) {
					return true;
				}

				// Um den Linker-Fehler bei conditions.IsTrue(...) zu umgehen, 
				// nutzen wir den manuellen Weg, den IsTrue intern aufruft, 
				// oder weichen (falls vorhanden) auf den rohen Node-Check aus.
				// Falls das immer noch linkt, evaluieren wir die Nodes direkt:
				auto* currentNode = recipe->conditions.head;
				bool conditionsMet = true;
				//RE::TESConditionItem
				while (currentNode) {
					// Wir rufen die Node-eigene vtable-Funktion auf, die fast immer exportiert ist,
					// oder lassen die Engine die Auswertung machen.
					// Falls du auf Nummer sicher gehen willst, rufen wir hier die native Funktion auf:
					RE::ConditionCheckParams parms(player, player);
					if (!currentNode->IsTrue(parms)) {
						conditionsMet = false;
						break;
					}
					currentNode = currentNode->next;
				}

				if (conditionsMet) {
					return true;
				}
			}
		}

		return false;
	}

	RE::BSEventNotifyControl equip_event_listener::ProcessEvent(const RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>*)
	{
		logger::info("EquipEventListener called");
		if (!a_event || !a_event->actor || !a_event->actor->IsPlayerRef()) {
			return RE::BSEventNotifyControl::kContinue;
		}

		// a_event->equipped ist true, wenn das Item GERADE angelegt wurde
		if (a_event->equipped) {
			auto* player = RE::PlayerCharacter::GetSingleton();
			auto* boundObj = RE::TESForm::LookupByID<RE::TESBoundObject>(a_event->baseObject);

			// Only allow ammo the player can craft itself
			if (boundObj->IsAmmo()) {
				if (!PlayerCanCraftAmmo(boundObj)) {
					auto* equipManager = RE::ActorEquipManager::GetSingleton();
					if (equipManager) {
						equipManager->UnequipObject(player, boundObj, nullptr, 1, nullptr, true, false, false, false);
						RE::SendHUDMessage::ShowHUDMessage("You are too unexperienced to use these ammo!");
					}
				}
			}
			else if (boundObj && (boundObj->IsWeapon() || boundObj->IsArmor() || boundObj->Is(RE::FormType::AlchemyItem))) {

				// Hier greift deine Heuristik! 
				// ACHTUNG: Bei diesem Event müssen wir die ExtraData über das Inventar des Players holen,
				// da das Event selbst leider keine ExtraDataList mitliefert:
				bool isAllowed = false;
				auto inventory = player->GetInventory();

				for (const auto& [item, data] : inventory) {
					if (item != boundObj)
						continue;

					auto* entryData = data.second.get();

					// 1. Check für die Alchemie (Tränke/Gifte) greift direkt über das Objekt (keine ExtraData nötig)
					if (boundObj->Is(RE::FormType::AlchemyItem)) {
						std::uint32_t formID = boundObj->GetFormID();
						if ((formID & 0xFF000000) == 0xFF000000) {
							isAllowed = true; // Selbst gebrauter Trank (Runtime ID)
						}
					}

					// 2. Check für Waffen und Rüstungen ohne ExtraDataList
					if (entryData && !entryData->extraLists) {
						// Kleidung ohne Rüstungswert immer zulassen, sofern ohne Basis Verzauberung (nur Spieler Verzauberungen zulässig)
						auto* armor = boundObj->As<RE::TESObjectARMO>();
						if (armor && armor->GetArmorRating() == 0 && armor->formEnchanting == nullptr) {
							isAllowed = true;
						}
					}

					// 2. Check für Waffen und Rüstungen über die ExtraDataList
					if (entryData && entryData->extraLists) {

						for (auto* extraList : *entryData->extraLists) {
							if (!extraList) continue;

							// A) Schleifstein / Werktisch (Bereits erfolgreich getestet!)
							if (extraList->HasType(RE::ExtraDataType::kHealth)) {
								auto* extraHealth = extraList->GetByType<RE::ExtraHealth>();
								if (extraHealth && extraHealth->health > 1.0f) {
									isAllowed = true;
									break;
								}
							}

							// B) Arkaner Verzauberer (Spieler-Verzauberung)
							if (extraList->HasType(RE::ExtraDataType::kEnchantment)) {
								auto* extraEnchant = extraList->GetByType<RE::ExtraEnchantment>();
								if (extraEnchant && extraEnchant->enchantment) {
									std::uint32_t enchantFormID = extraEnchant->enchantment->GetFormID();

									// Wenn die Verzauberung selbst vom Spieler erstellt wurde, beginnt ihre ID mit 0xFF
									if ((enchantFormID & 0xFF000000) == 0xFF000000) {
										isAllowed = true;
										break;
									}
								}
							}
							else {
								// Kleidung ohne Rüstungswert immer zulassen, sofern ohne Basis Verzauberung (nur Spieler Verzauberungen zulässig)
								auto* armor = boundObj->As<RE::TESObjectARMO>();
								if (armor && armor->GetArmorRating() == 0 && armor->formEnchanting == nullptr) {
									isAllowed = true;
								}
							}

							// C) Frisch geschmiedet (Deine kTextDisplayData-Heuristik)
							if (extraList->HasType(RE::ExtraDataType::kTextDisplayData)) {
								auto* extraText = extraList->GetByType<RE::ExtraTextDisplayData>();
								if (extraText) {
									if (extraText->customNameLength > 0 ||
										extraText->displayName == nullptr ||
										std::strcmp(extraText->displayName.c_str(), boundObj->GetName()) == 0) {
										isAllowed = true;
										break;
									}
								}
							}
						}
					}
					
					// Check for Campfire or Hunterborn items based on Editor ID
					logger::info("Checking for Campfire or Hunterborn item");
					if (IsSurvivalModItem_ViaDataHandler(boundObj)) {
						logger::info("Detected Campfire or Hunterborn item");
						isAllowed = true;
					}
					
					break; // was item == boundObj, can stop loop
				}

				// Wenn nicht erlaubt, ziehen wir es dem Spieler sofort wieder aus!
				if (!isAllowed) {
					auto* equipManager = RE::ActorEquipManager::GetSingleton();
					if (equipManager) {
						// Unequip-Befehl erzwingen
						equipManager->UnequipObject(player, boundObj, nullptr, 1, nullptr, true, false, false, false);
						RE::SendHUDMessage::ShowHUDMessage("You don't know how to use this!");
					}
				}
			}
		}

		return RE::BSEventNotifyControl::kContinue;
	}

	void equip_event_listener::Register()
	{
		auto* scriptEventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
		if (scriptEventSourceHolder) {
			scriptEventSourceHolder->AddEventSink<RE::TESEquipEvent>(&get_instance());
		}
	}



}
