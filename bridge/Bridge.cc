#include <iostream>
#include <cstdio>
#include "Bridge.h"
#include "mpi.h"
#include <vector>
#include <algorithm> //find()

using namespace std;

#define TAG 0 // for debugging purposes

/*void Bridge::scatter(int source) {
	int size = MPI::COMM_WORLD.Get_size();
	int sendcount = 2;
	int sendbuff[size*sendcount];
	int recvbuff[sendcount];
	
	if (node_id == source) for (int i = 0; i < size*sendcount; i++) sendbuff[i] = i;
	MPI::COMM_WORLD.Scatter(sendbuff, sendcount, MPI::INT, recvbuff, sendcount, MPI::INT, source);

	int sum = 0;
	for (int i=0; i < sendcount; i++) sum += recvbuff[i];
	printf("Scatter: rank %d got %d and %d\n", node_id, recvbuff[0], recvbuff[1]);
	printf("Scatter: rank %d calculated a sum of %d\n", node_id, sum);
}*/

void Bridge::send(int target, const Packet_t packet) {

	if (!(std::find(neighbours.begin(), neighbours.end(), target) != neighbours.end())){
		printf("ERROR: Rank %d tried to send a message to a rank that's not a neighbour\n", node_id);
		return;
	}
	//printf("Rank %d: Ready to send...\n", node_id);

	MPI::Request req;

	req = MPI::COMM_WORLD.Isend(&packet.front(), packet.size(), MPI_UINT64_T, target, TAG); //TODO: Change TAG
	// while (!req.Test()) {} // wait for the message to be sent
	printf("Rank %d: Sent a packet to %d (packet.at(0)=%d)\n", node_id, target, packet.at(0));
}

std::vector<int> Bridge::get_neighbours(){
	this->neighbours = sba_system_ptr->get_neighbours();
	vector<int> tmp;
	return tmp;
}

void* wait_recv_any_th(void *arg){
	int node_id = (int) arg;
	int num;
	MPI::Status status;

	for(;;){

		while (!MPI::COMM_WORLD.Iprobe(MPI::ANY_SOURCE, MPI::ANY_TAG, status)) { // waits for a msg but doesn't receive
			// printf("Rank %d: Waiting for msg\n", node_id);
		}

		Packet_t packet;
		/*find size of packer */
		int recv_size = status.Get_count(MPI_UINT64_T);
		packet.resize(recv_size);

		MPI::Request request = MPI::COMM_WORLD.Irecv(&packet.front(), recv_size, MPI_UINT64_T,
													status.Get_source(), status.Get_tag());
		while (!request.Test()){ // waits until the whole message is received
			// printf("Rank %d: Waiting for request to be completed\n", node_id);
		}
		printf("Rank %d: Received a packet from source %d successfully!(packet.at(0)=%d)\n", node_id, status.Get_source(), packet.at(0));
	}
}

/*void Bridge::bcast_to_neighbours(int num) {
	for (std::vector<int>::iterator it = neighbours.begin(); it < neighbours.end(); it++){
		this->send(*it, num);
	}
}*/

void Bridge::bcast_to_neighbours(Packet_t packet) {
	for (std::vector<int>::iterator it = neighbours.begin(); it < neighbours.end(); it++){
		this->send(*it, packet);
	}
}
