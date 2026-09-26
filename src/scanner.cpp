namespace scanner
{
	// Helper function to safely extract a world-space coordinate from a specific actor bone
	bool GetActorBonePosition(RE::Actor* a_actor, const std::string_view& a_boneName, RE::NiPoint3& out_position)
	{
		if (!a_actor || !a_actor->Get3D())
			return false;

		// Search the actor's 3D skeleton tree for the specific node name
		RE::NiAVObject* boneNode = a_actor->Get3D()->GetObjectByName(a_boneName);
		if (boneNode) {
			// world.translate contains the actual runtime x, y, z coordinate in the game world
			out_position = boneNode->world.translate;
			return true;
		}
		return false;
	}

#if 0
	bool IsPathBlockedByStaticsOrTerrain(RE::Actor* a_viewer, RE::Actor* a_target)
	{
		if (!a_viewer || !a_target)
			return false;

		RE::NiPoint3 rayStart, rayEnd;

		if (!GetActorBonePosition(a_viewer, "NPC Head [Head]", rayStart)) {
			logger::debug("Didn't find 'NPC Head [Head]'");
			RE::NiPoint3 dummyDirection;
			a_viewer->GetEyeVector(rayStart, dummyDirection, true);
		}

		if (!GetActorBonePosition(a_target, "NPC Spine2 [Spn2]", rayEnd)) {
			logger::debug("Didn't find 'NPC Spine2 [Spn2]'");
			rayEnd = a_target->GetPosition();
		}

		auto playerCell = a_viewer->GetParentCell();
		if (!playerCell)
			return false;

		auto havokWorld = playerCell->GetbhkWorld();
		if (!havokWorld)
			return false;

		// === NEW LOCK METHOD ===
		// Safely acquire a shared read lock on the worldLock member variable.
		// The lock automatically unlocks when this variable goes out of scope at the end of the function.
		RE::BSReadLockGuard lock(havokWorld->worldLock);

		RE::hkpWorld* hkWorld = havokWorld->GetWorld1();
		if (!hkWorld)
			return false;

		// 1. Scale down your 3D vectors
		RE::NiPoint3 scaledFrom = rayStart * RE::bhkWorld::GetWorldScale();
		RE::NiPoint3 scaledTo = rayEnd * RE::bhkWorld::GetWorldScale();

		// 2. Initialize hkVector4 with clean 4D formatting (X, Y, Z, W)
		// For positional coordinates, W must always be explicitly set to 1.0f
		RE::hkpWorldRayCastInput raycastInput;
		logger::debug("setting 'from' to ({}, {}, {}, {}) = ", scaledFrom.x, scaledFrom.y, scaledFrom.z, 1.0f);
		logger::debug("setting 'to' to ({}, {}, {}, {}) = ", scaledTo.x, scaledTo.y, scaledTo.z, 1.0f);
		raycastInput.from = RE::hkVector4(scaledFrom.x, scaledFrom.y, scaledFrom.z, 1.0f);
		raycastInput.to = RE::hkVector4(scaledTo.x, scaledTo.y, scaledTo.z, 1.0f);

		//raycastInput.filterInfo = 0x00000010; // (Statics)
		raycastInput.filterInfo = RE::bhkCollisionFilter::GetSingleton()->GetCollisionFilterInfo(RE::COL_LAYER::kStatic);

		/* if (hkWorld->collisionFilter) {
			auto groupFilter = static_cast<RE::hkpGroupFilter*>(hkWorld->collisionFilter);

			// Generates the perfect bitmask for Layer 1 (L_STATIC) conforming to Skyrim's rules
			raycastInput.filterInfo = groupFilter->GetNewFilterInfo(1);
		} else {
			// Fallback if the filter is missing
			raycastInput.filterInfo = 0x00000001;
		}*/

		RE::hkpWorldRayCastOutput raycastOutput;
		raycastOutput.Reset();
		hkWorld->CastRay(raycastInput, raycastOutput);

		bool result = raycastOutput.HasHit();
		logger::debug("raycastOutput.HasHit() = {}", result);
		return result;
	}


#include <RE/H/hkpAllRayHitCollector.h>
#include <RE/H/hkpWorldRayCastInput.h>
#include <RE/H/hkpWorldRayCastOutput.h>
#include <RE/H/hkpRigidBody.h>
#endif

	/*
	bool PerformEnvironmentRaycast(RE::NiPoint3 rayStart, RE::NiPoint3 rayEnd, RE::TESObjectCELL* playerCell)
	{
		auto havokWorld = playerCell->GetbhkWorld();
		if (!havokWorld) return false;

		// Use the framework's lock mechanics
		RE::BSReadLockGuard lock(havokWorld->worldLock);

		RE::hkpWorld* hkWorld = havokWorld->GetWorld1();
		if (!hkWorld) return false;

		// 1. Vector transformations using the clean modern scale references
		float worldScale = RE::bhkWorld::GetWorldScale();

		RE::hkpWorldRayCastInput raycastInput;
		raycastInput.from = RE::hkVector4(rayStart.x * worldScale, rayStart.y * worldScale, rayStart.z * worldScale, 1.0f);
		raycastInput.to = RE::hkVector4(rayEnd.x * worldScale, rayEnd.y * worldScale, rayEnd.z * worldScale, 1.0f);

		// Set to 0 to bypass Skyrim's single-layer assignment collision limitations
		raycastInput.filterInfo = 0;

		// 2. Instantiate the dynamic hit collector exposed in the new headers
		RE::hkpAllRayHitCollector collector;

		// Execute the Havok ray trace
		hkWorld->CastRay(raycastInput, collector);

		bool hitStaticObstacle = false;
		float closestFraction = 1.0f;

		if (collector.HasHit()) {
			// Safe iteration over the modern hkArray layout
			for (std::uint32_t i = 0; i < collector.hits.size(); ++i) {
				const auto& hit = collector.hits[i];

				// Ignore immediate self-clipping at the actor's pivot point origin
				if (hit.hitFraction < 0.001f) {
					continue;
				}

				if (hit.rootCollidable) {
					// Extract the specific Collision Layer using the 6-bit mask definition
					std::uint32_t filterInfo = hit.rootCollidable->broadPhaseHandle.collisionFilterInfo;
					std::uint32_t hitLayer = filterInfo & 0x3F;

					// 1 = L_STATIC, 2 = L_ANIMSTATIC, 32 = L_TERRAIN
					if (hitLayer == 1 || hitLayer == 2 || hitLayer == 32) {
						if (hit.hitFraction < closestFraction) {
							closestFraction = hit.hitFraction;
							hitStaticObstacle = true;
						}
					}
				}
			}
		}

		return hitStaticObstacle;
	}
	*/

	bool PerformEnvironmentRaycast(RE::NiPoint3 rayStart, RE::NiPoint3 rayEnd, RE::TESObjectCELL* playerCell)
	{
		auto havokWorld = playerCell->GetbhkWorld();
		if (!havokWorld) return false;

		RE::BSReadLockGuard lock(havokWorld->worldLock);

		RE::hkpWorld* hkWorld = havokWorld->GetWorld1();
		if (!hkWorld) return false;

		float worldScale = RE::bhkWorld::GetWorldScale();

		// Havok-Vektoren für Start und Ende vorbereiten
		RE::hkVector4 fromVec(rayStart.x * worldScale, rayStart.y * worldScale, rayStart.z * worldScale, 1.0f);
		RE::hkVector4 toVec(rayEnd.x * worldScale, rayEnd.y * worldScale, rayEnd.z * worldScale, 1.0f);

		bool hitStaticObstacle = false;
		const std::uint32_t MAX_STEPS = 16; // Sicherheitsgrenze gegen Endlosschleifen

		for (std::uint32_t step = 0; step < MAX_STEPS; ++step) {
			RE::hkpWorldRayCastInput raycastInput;
			raycastInput.from = fromVec;
			raycastInput.to = toVec;
			raycastInput.filterInfo.filter = 0; // Wir lassen alles treffen und filtern selbst

			RE::hkpWorldRayCastOutput raycastOutput;
			hkWorld->CastRay(raycastInput, raycastOutput);

			// Wenn überhaupt nichts mehr getroffen wird, sind wir im leeren Raum -> Abbruch
			if (!raycastOutput.HasHit()) {
				break;
			}

			if (raycastOutput.rootCollidable) {
				RE::COL_LAYER hitLayer = raycastOutput.rootCollidable->broadPhaseHandle.collisionFilterInfo.GetCollisionLayer();

				// 2. Vergleiche direkt mit den offiziellen RE::COL_LAYER Enums
				//    kStatic = 1, kAnimStatic = 2, kTerrain = 32
				if (hitLayer == RE::COL_LAYER::kStatic ||
					hitLayer == RE::COL_LAYER::kAnimStatic ||
					hitLayer == RE::COL_LAYER::kTerrain)
				{
					hitStaticObstacle = true;
					break;
				}
			}		
		
			// --- CHAINING MECHANIK ---
			float hitFraction = raycastOutput.hitFraction;

			if (hitFraction < 0.0001f) {
				hitFraction = 0.0001f;
			}

			// 1. Richtungsvektor IMMER basierend auf dem aktuellen Zustand berechnen
			RE::hkVector4 currentDirection = toVec - fromVec;

			// 2. An den Aufprallpunkt springen + ein winziges Stück (0.001) weiter nach vorne schubsen
			RE::hkVector4 offset = currentDirection * RE::hkVector4(hitFraction + 0.001f);
			fromVec = fromVec + offset;

			// Wenn der Strahl das Ende erreicht hat, abbrechen
			if (hitFraction >= 1.0f) {
				break;
			}
		}

		return hitStaticObstacle;
	}

	bool IsPathBlockedByStaticsOrTerrain(RE::Actor* a_viewer, RE::Actor* a_target)
	{
		if (!a_viewer || !a_target)
			return false;

		RE::NiPoint3 rayStart, rayEnd;

		if (!GetActorBonePosition(a_viewer, "NPC Head [Head]", rayStart)) {
			logger::debug("Didn't find 'NPC Head [Head]'");
			RE::NiPoint3 dummyDirection;
			a_viewer->GetEyeVector(rayStart, dummyDirection, true);
		}

		if (!GetActorBonePosition(a_target, "NPC Spine2 [Spn2]", rayEnd)) {
			logger::debug("Didn't find 'NPC Spine2 [Spn2]'");
			rayEnd = a_target->GetPosition();
		}
	
		auto playerCell = a_viewer->GetParentCell();
		if (!playerCell)
			return false;

		return PerformEnvironmentRaycast(rayStart, rayEnd, playerCell);
	}



	bool scan_actors(float a_radius, size_t a_nearest_males, size_t a_nearest_females,
		std::vector<RE::Actor*>& a_male_actors, std::vector<bool>& a_male_see_player,
		std::vector<RE::Actor*>& a_female_actors, std::vector<bool>& a_female_see_player)
	{
		static const auto ActorTypeNPC = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("ActorTypeNPC");

		std::vector<std::pair<float, std::pair<RE::Actor*, bool>>> males;
		std::vector<std::pair<float, std::pair<RE::Actor*, bool>>> females;

		RE::Actor* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			logger::info("Player character not found.");
			return false;
		}

		const auto processLists = RE::ProcessLists::GetSingleton();
		if (!processLists) {
			return false;
		}

		auto squared_max_dist = a_radius * a_radius;

		for (auto& target_handle : processLists->highActorHandles) {
			const auto actor_ptr = target_handle.get();
			if (!actor_ptr) {
				continue;
			}
			const auto actor = actor_ptr.get();
			logger::debug("Processing actor = {}", (void*)actor);
			if (!actor || actor->IsDead() || !actor->Is3DLoaded() || actor == player) {
				continue;
			}

			// npc's only
			const auto base = actor_ptr->GetActorBase();
			if (!base) {
				continue;
			}

			if (base->GetFormType() != RE::FormType::NPC) {
				continue;
			}

			const auto race = base->GetRace();
			if (!race) {
				continue;
			}
			if (!race->HasKeyword(ActorTypeNPC)) {
				continue;
			}

			auto sex = actor->GetActorBase()->GetSex();
			if (sex < 0 || sex > 1) {
				continue;
			}

			// distance to player
			auto player_pos = player->GetPosition();
			auto squared_dist = player_pos.GetSquaredDistance(actor->GetPosition());
			logger::debug("squared_dist: {}, squared_max_dist: {}", squared_dist, squared_max_dist);
			if (squared_dist > squared_max_dist) {
				continue;
			}

			// can see player
			bool can_see_player = true;
			const float squared_auto_visible_dist = 400.f * 400.f;
			logger::debug("squared_dist = {}, squared_auto_visible_dist = {}", squared_dist, squared_auto_visible_dist);
			if (squared_dist >= squared_auto_visible_dist) {
				logger::debug("squared_dist {} >= squared_auto_visible_dist {}, checking line of sight", squared_dist, squared_auto_visible_dist);
				//can_see_player = can_see_target(actor, player, false);
				//can_see_player = !IsPhysicalPathBlocked(actor, player);
				can_see_player = !IsPathBlockedByStaticsOrTerrain(actor, player);
			}

			if (sex == 0) {
				males.push_back(std::pair(squared_dist, std::pair(actor, can_see_player)));
				logger::debug("added actor {} to males array, males array size = {}", (void*)actor, males.size());
			}
			else {
				females.push_back(std::pair(squared_dist, std::pair(actor, can_see_player)));
			}
		}

		int nth_males = std::min(a_nearest_males, males.size());
		logger::debug("nth_males = {}", nth_males);
		if (nth_males > 0) {
			std::nth_element(males.begin(), males.begin() + nth_males - 1, males.end(),
				[](const auto& a, const auto& b) { return a.first < b.first; });
		}
		logger::debug("males size after nth_element = {}", males.size());

		int nth_females = std::min(a_nearest_females, females.size());
		if (nth_females > 0) {
			std::nth_element(females.begin(), females.begin() + nth_females - 1, females.end(),
				[](const auto& a, const auto& b) { return a.first < b.first; });
		}

		// copy to result arrays
		for (auto& elem : males) {
			a_male_actors.push_back(elem.second.first);
			a_male_see_player.push_back(elem.second.second);
		}

		for (auto& elem : females) {
			a_female_actors.push_back(elem.second.first);
			a_female_see_player.push_back(elem.second.second);
		}

		return true;
	}
}

