#ifndef SYSTEM_H
#define SYSTEM_H

#include "Base/System.h"
#include "Tile.h"
#include <string>
#include <vector>
#include <unordered_map>
#include "Types.h"

#define TMP_RANK 0 // debugging
#define NSERVICES 1 // tmp
#define NBRIDGES 1 // tmp

class System: public Base::System {
	public:
		//Tile *sba_tile_ptr;
		int rank;
		std::vector< std::vector<int> > process_tbl;
		std::vector<int> neighbours;
		int rows, cols; // dimensions of process_tbl
		Bridge *bridge;
		std::unordered_map<int,Tile*> nodes;
		std::vector<Bridge*> bridge_list;
		int bridge_pos = 0; // cycles from 0 to bridge_list.size()-1

		System(int rank_, int r_, int c_): rank(rank_), rows(r_), cols(c_) {
			initialise_process_table();
			find_neighbours();

			for (int node_id_=1;node_id_<=NSERVICES;node_id_++) {
				int service_address=node_id_;
				int node_id = node_id_;
				if  (service_address != 0) {
					if (nodes.count(node_id) == 0) {
						nodes[node_id]=new Tile(this, node_id, service_address, rank);
					}
				}
			}
			
			/* Create a number of bridges */
			for (int i = 0; i < NBRIDGES; i++){
				bridge_list.push_back(new Bridge(this, rank));
			}
		};

		void initialise_process_table();
		void print_process_table();
		void find_neighbours();
		void print_neighbours();
		std::vector<int> get_neighbours();

		/* TEST - Send msg using round robin to select a bridge */
		void bcast_to_neighbours(Packet_t packet);
		void stencil_operation(std::vector<Packet_t> packet_list);
		void allreduce_operation(std::vector<Packet_t> packet_list);
		//TODO: add scatter
};

#endif // SYSTEM_H
