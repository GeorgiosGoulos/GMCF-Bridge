#ifndef TILE_H
#define TILE_H

#include "Base/Tile.h"
#include "Base/System.h"
#include "Bridge.h"
#include "Types.h"

//TODO: Remove
#include <pthread.h>

class Tile: public Base::Tile {
	public:
		
		Base::System *sba_system_ptr;
		int node_id;
		int service_address;
		int rank;
		RX_Packet_Fifo rx_fifo;
		TX_Packet_Fifo tx_fifo;

		//TODO:Remove
		pthread_spinlock_t _lock;

		Tile(Base::System *sba_s_, int n_, int s_, int r_): sba_system_ptr(sba_s_), node_id(n_), service_address(s_), rank(r_){
		//TODO: Remove
		pthread_spin_init(&_lock, PTHREAD_PROCESS_SHARED);
		};

		~Tile() {
			//TODO: Remove
			pthread_spin_destroy(&_lock);
		}


		void add_to_tx_fifo(Packet_t packet); // GPRM already uses spinlock for this
		void add_to_rx_fifo(Packet_t packet); // No thread safety(probably because it's one thread that reads/modifies it)
											  //TODO: Should thread safety be added now?
};

#endif // TILE_H
