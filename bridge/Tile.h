#ifndef TILE_H
#define TILE_H

#include "Base/Tile.h"
#include "Base/System.h"
#include "Bridge.h"

class Tile: public Base::Tile {
	public:
		
		Base::System *sba_system_ptr;
		int node_id;
		Bridge *bridge;

		Tile(Base::System *sba_s_, int n_): sba_system_ptr(sba_s_), node_id(n_){
			bridge = new Bridge(this, sba_system_ptr, node_id);
		};
};

#endif // TILE_H
