#ifndef SYSTEM_H
#define SYSTEM_H

#include "Base/System.h"
#include "Tile.h"
#include <string>
#include <vector>

#define TMP_RANK 0 // used for debugging

class System: public Base::System {
	public:
		Tile *sba_tile_ptr;
		int node_id;	// for now it's the rank of the process
		std::vector< std::vector<int> > process_tbl;
		std::vector<int> neighbours;
		int rows, cols; // dimensions of process_tbl

		System(int n_, int r_, int c_): node_id(n_), rows(r_), cols(c_) {
			initialise_process_table();
			find_neighbours();
			sba_tile_ptr = new Tile(this, node_id);
		};

		void initialise_process_table();
		void print_process_table();
		void find_neighbours();
		void print_neighbours();
		std::vector<int> get_neighbours();
};

#endif // SYSTEM_H
