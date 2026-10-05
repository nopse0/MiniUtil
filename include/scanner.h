#pragma once

namespace scanner {

	// Returns male and female actors near the player, and their squared distance to the player
	bool scan_actors(
		float a_radius,
		std::vector< std::pair<float, RE::Actor*> >& a_males,
		std::vector< std::pair<float, RE::Actor*> >& a_females);

	// Puts the n nearest actors to the front of the vector
	size_t nth_nearest(size_t a_n_nearest, std::vector< std::pair<float, RE::Actor*> >& a_actors);
}
