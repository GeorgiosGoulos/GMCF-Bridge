#ifndef BRIDGE_H
#define BRIDGE_H

#include "Base/System.h"
#include <iostream>
#include <cstdio>
#include <vector>
#include <pthread.h>
#include "Types.h"
#include "Packet.h"

void *wait_recv_any_th(void*); // executed in a different thread, used to listen for receiving messages, runs indefinitely

class Bridge {
	public:
		Base::System* sba_system_ptr;
		std::vector<int> neighbours;
		int rank;
	
		//thread-related  
		int rc = 0;
		pthread_t thread;

		// constructors
		Bridge(Base::System *sba_s_, int r_): sba_system_ptr(sba_s_), rank(r_){	
			neighbours = sba_system_ptr->get_neighbours();

			/* new thread - wait_recv_any */
			rc = pthread_create(&thread, NULL, wait_recv_any_th, (void *) this);
			if (rc) {
				printf("Rank %d: Error during creation of receiving thread: Code %d\n", rank, rc);
				exit(1);
			}
			else {
				//printf("Rank %d: Creation ofreceiving thread was completed successfully %d\n", rank, rc);
			}
		};
		
		std::vector<int> get_neighbours();
		void send(int target, Packet_t packet, int tag=tag_other);

		//Methods for packing and unpacking DRESP data 
		Packet_t packDRespPacket(Packet_t packet);
		Packet_t unpackDRespPacket(Packet_t packet);
		
		// Implementation using p2p communication
		//void bcast_to_neighbours(Packet_t packet);	// Not threaded
		//void scatter_to_neighbours(std::vector<Packet_t> packet_list);	// Not threaded
		void stencil(std::vector<Packet_t> packet_list);	// Threaded	
		void neighboursreduce(std::vector<Packet_t> packet_list);	// Threaded	

};

#endif // BRIDGE_H
