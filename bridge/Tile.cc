#include "Tile.h"
#include "Packet.h"

void Tile::add_to_tx_fifo(Packet_t packet){ //TODO: Solve this problem that stop the execution of the program
	/*
	//TODO: remove
	pthread_spin_lock(&_lock);

	printf("Rank %d: Added packet to tx_fifo of node %d:\n", rank, node_id);
	tx_fifo.push_back(packet); // For testing purposes only
	
	//TODO: Remove
	pthread_spin_unlock(&_lock);
	*/
}
void Tile::add_to_rx_fifo(Packet_t packet){ //TODO: Solve this problem that stop the execution of the program
	/*
	//TODO: Remove
	pthread_spin_lock(&_lock);

	printf("Rank %d: Added packet to rx_fifo of node %d: packet.at(0)= %llu\n", rank, node_id, packet.at(0));
	rx_fifo.push_back(packet); // For testing purposes only

	//TODO: Remove
	pthread_spin_unlock(&_lock);	
	*/
}
