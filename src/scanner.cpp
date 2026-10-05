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
					hitLayer == RE::COL_LAYER::kTerrain ||
					hitLayer == RE::COL_LAYER::kGround)
				{
					hitStaticObstacle = true;
					break;
				}
				logger::trace("Ignoring collision with collision layer {}", hitLayer);
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



	bool scan_actors_old(float a_radius, size_t a_nearest_males, size_t a_nearest_females,
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

	/**
	* Returns male and female actors near the player, and their squared distance to the player 
	*/
	bool scan_actors(
		float a_radius,
		std::vector< std::pair<float, RE::Actor*> >& a_males, 
		std::vector< std::pair<float, RE::Actor*> >& a_females)
	{
		static const auto ActorTypeNPC = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("ActorTypeNPC");

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
			/*bool can_see_player = true;
			const float squared_auto_visible_dist = 400.f * 400.f;
			logger::debug("squared_dist = {}, squared_auto_visible_dist = {}", squared_dist, squared_auto_visible_dist);
			if (squared_dist >= squared_auto_visible_dist) {
				logger::debug("squared_dist {} >= squared_auto_visible_dist {}, checking line of sight", squared_dist, squared_auto_visible_dist);
				//can_see_player = can_see_target(actor, player, false);
				//can_see_player = !IsPhysicalPathBlocked(actor, player);
				can_see_player = !IsPathBlockedByStaticsOrTerrain(actor, player);
			}*/

			if (sex == 0) {
				a_males.push_back(std::pair(squared_dist, actor));
				logger::debug("added actor {} to males array, males array size = {}", (void*)actor, a_males.size());
			}
			else {
				a_females.push_back(std::pair(squared_dist, actor));
			}
		}

		/*int nth_males = std::min(a_nearest_males, males.size());
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
		}*/

		return true;
	}

	// Puts the n nearest actors to the front of the vector
	size_t nth_nearest(size_t a_n_nearest, std::vector< std::pair<float, RE::Actor*> >& a_actors)
	{
		auto nth_actors = std::min(a_n_nearest, a_actors.size());
		logger::debug("nth_actors = {}", nth_actors);
		if (nth_actors > 0) {
			std::nth_element(a_actors.begin(), a_actors.begin() + nth_actors - 1, a_actors.end(),
				[](const auto& a, const auto& b) { return a.first < b.first; });
		}
		return nth_actors;
	}

}

