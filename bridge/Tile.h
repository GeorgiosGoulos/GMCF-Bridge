#ifndef TILE_H
#define TILE_H

#include "Base/Tile.h"
#include "Base/System.h"
#include "Bridge.h"

class Tile: public Base::Tile {
	public:
		
		Base::System *sba_system_ptr;
		int node_id;
		int service_address;
		int rank;

		Tile(Base::System *sba_s_, int n_, int s_, int r_): sba_system_ptr(sba_s_), node_id(n_), service_address(s_), rank(r_){};
};

#endif // TILE_H
