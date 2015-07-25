#ifndef BRIDGE_H
#define BRIDGE_H

#include "Base/System.h"
#include <iostream>
#include <cstdio>
#include <vector>
#include <pthread.h>
#include "Types.h"

void *wait_recv_any_th(void*); // executed in a different thread, used to listen for receiving messages, runs indefinitely

class Bridge {
	public:
		Base::System* sba_system_ptr;
		std::vector<int> neighbours;
		int rank;
	
		//thread-related  
		int rc = 0;
		pthread_t thread;
		pthread_attr_t attr;

		// constructors
		Bridge(Base::System *sba_s_, int r_): sba_system_ptr(sba_s_), rank(r_){	
			neighbours = sba_system_ptr->get_neighbours();

			/* new thread - wait_recv_any */
			rc = pthread_create(&thread, NULL, wait_recv_any_th, (void *) this);
			if (rc) {
				printf("RANK %d: ERROR CREATING RECEIVING THREAD: CODE %d\n", rank, rc);
				exit(1);
			}
			else {
				//printf("RANK %d: CREATED RECEIVING THREAD SUCCESSFULLY: CODE %d\n", rank, rc);
			}
		};
		
		// methods
		std::vector<int> get_neighbours();
		void send(int target, Packet_t packet, int tag=tag_other);
		//void scatter(int source);
		
		// Implementation using p2p communication
		void bcast_to_neighbours(Packet_t packet); // Not threaded
		void scatter_to_neighbours(std::vector<Packet_t> packet_list); // Not threaded
		//threaded	
		void stencil(std::vector<Packet_t> packet_list);
		void allreduce(std::vector<Packet_t> packet_list);
};

#endif // BRIDGE_H
