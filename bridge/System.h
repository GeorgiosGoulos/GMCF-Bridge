#ifndef SYSTEM_H
#define SYSTEM_H

#include "Base/System.h"
#include "Tile.h"
#include <string>
#include <vector>
#include <unordered_map>
#include "Types.h"
#include "mpi.h"

class System: public Base::System {
	public:
		//Tile *sba_tile_ptr;
		int rank;
		int rows, cols; // dimensions of process_tbl
		int selected_node; // ID of next node to receive a packet, used for iterating over nodes TODO: here or static in Bridge?
		// selected_node in {0, n-1}, so ALWAYS use selected_node + 1 (in {1, n})
		std::vector< std::vector<int> > process_tbl; //TODO: move to find_neighbours after deployment
		std::vector<int> neighbours;
		Bridge *bridge; // not needed?
		std::unordered_map<int,Tile*> nodes;
		std::vector<Bridge*> bridge_list;
		int bridge_pos = 0; // cycles from 0 to bridge_list.size()-1. The instance itself changes it => no need for sync
		MPI_Comm *comm_ptr;

		//locks
		pthread_spinlock_t _nodes_lock;
		

		System(int rank_, int r_, int c_, MPI_Comm *c_ptr_): rank(rank_), rows(r_), cols(c_), selected_node(0), comm_ptr(c_ptr_) {
			initialise_process_table();
			find_neighbours();
			if (rank==0) print_neighbours();

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
			
			//locks
			pthread_spin_init(&_nodes_lock, PTHREAD_PROCESS_SHARED); // PTHREAD_PROCESS_PRIVATE
		};

		~System(){
			//locks
			pthread_spin_destroy(&_nodes_lock);
		}

		void initialise_process_table();
		void print_process_table();
		void find_neighbours();
		void print_neighbours();
		std::vector<int> get_neighbours();

		void bcast_to_neighbours(Packet_t packet);
		void stencil_operation(std::vector<Packet_t> packet_list);
		void allreduce_operation(std::vector<Packet_t> packet_list);

#ifdef MPI_TOPOLOGY_OPT
		static MPI_Comm create_communicator(int rows, int cols);
#endif // MPI_TOPOLOGY_OPT
		//TODO: add scatter

		Tile* get_node(); // used by threads for sending packets to tiles
};

#endif // SYSTEM_H
