#ifndef BRIDGE_H
#define BRIDGE_H

#include "Base/Tile.h"
#include "Base/System.h" // TODO: Change to "System.h" ?
#include <iostream>
#include <cstdio>
#include <vector>
#include <pthread.h>
#include "Types.h"

void *wait_recv_any_th(void*);

class Bridge {
	public:
		Base::System* sba_system_ptr;
		Base::Tile* sba_tile_ptr;
		std::vector<int> neighbours;
		int node_id;
	
		//thread-related  
		int rc = 0;
		pthread_t thread;
		pthread_attr_t attr;

		// constructors
		Bridge(Base::Tile *sba_t_, Base::System *sba_s_, int n_): sba_tile_ptr(sba_t_), sba_system_ptr(sba_s_), node_id(n_){	
			neighbours = sba_system_ptr->get_neighbours();	

			/* thread creation */
			rc = pthread_create(&thread, NULL, wait_recv_any_th, (void *) node_id);
			if (rc) {
				printf("RANK %d: ERROR CREATING THREAD: CODE %d\n", node_id, rc);
				exit(1);
			}
			else {
				//printf("RANK %d: CREATED THREAD SUCCESSFULLY: CODE %d\n", node_id, rc);
			}
		};
		
		// methods
		std::vector<int> get_neighbours();
		void send(int target, Packet_t packet);
		//void scatter(int source);
		
		// Implementation using p2p communication
		void wait_send_recv();
		void wait_recv_any();
		void bcast_to_neighbours(int num=42);
		void bcast_to_neighbours(Packet_t packet);

		//threaded	
		//void *wait_recv_any_th(void*);
};

#endif // BRIDGE_H
