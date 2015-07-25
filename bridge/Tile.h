#ifndef TILE_H
#define TILE_H

#include "Base/Tile.h"
#include "Base/System.h"
#include "Bridge.h"
#include "Types.h"

class Tile: public Base::Tile {
	public:
		
		Base::System *sba_system_ptr;
		int node_id;
		int service_address;
		int rank;
		RX_Packet_Fifo rx_fifo;
		TX_Packet_Fifo tx_fifo ;

		Tile(Base::System *sba_s_, int n_, int s_, int r_): sba_system_ptr(sba_s_), node_id(n_), service_address(s_), rank(r_){};


		void add_to_tx_fifo(Packet_t packet);
		void add_to_rx_fifo(Packet_t packet);
};

#endif // TILE_H
