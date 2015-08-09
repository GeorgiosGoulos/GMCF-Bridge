#include <iostream>
#include <cstdio>
#include "Bridge.h"
#include "mpi.h"
#include <vector>
#include <algorithm> //find()
#include <pthread.h>
#include "Packet.h"

#include "System.h" // cast System references and passing received packets to nodes

using namespace std;
using namespace SBA;

#define MAX_ITERATIONS 2000 // Temporary solution for bridges not getting stuck in wait_recv_any_th() TODO: find a better alternative

struct bridge_packets{ //Used for passing parameters to threads TODO: find a better name
	Bridge* bridge;
	std::vector<Packet_t> packet_list;
};

std::vector<int> Bridge::get_neighbours(){
	return sba_system_ptr->get_neighbours();
}

//TODO: Delete (For testing purposes only)
void findAndSetLength(Packet_t& packet){
	packet.at(1) = packet.size() - getHeader(packet).size();
}

uint64_t float2Word(float x) {
    Word *y = reinterpret_cast<Word*>(&x); // endianness should be taken into consideration if different machines are used
    return *y;
}

float Word2float(Word x) {
    float *y = reinterpret_cast<float*>(&x); // endianness should be taken into consideration if different machines are used
    return *y;
}

Packet_t Bridge::packDRespPacket(Packet_t packet){
	if (getType(getHeader(packet)) != P_DRESP) return packet; //failsafe, TODO: Remove?

	int data_size = getReturn_as(getHeader(packet));
	cout << "packP - Data size: " << data_size << endl;

	void* ptr = (void *) packet.back();
	float *arr = (float*)ptr;

	for (int i = 0; i < data_size; i++){
		cout << "Pack: Adding " << *(arr+i) << endl;
		packet.push_back(float2Word(*(arr+i)));
	}

	return packet;
}

Packet_t Bridge::unpackDRespPacket(Packet_t packet) {

	if (getType(getHeader(packet)) != P_DRESP) return packet;

	int original_size = getLength(getHeader(packet));
	int data_size = getReturn_as(getHeader(packet));

	Packet_t new_packet;
	float *arr = new float[data_size];

	for (vector<Word>::iterator it = packet.begin(); it < packet.begin() + getHeader(packet).size() + original_size - 1; it++){
		new_packet.push_back(*it);
	}
	new_packet.push_back((Word) arr);

	int counter = 0;

	for (vector<Word>::iterator it = packet.begin() + getHeader(packet).size() + original_size; it < packet.end(); it++){
		cout << "Unpack: Adding " << Word2float(*it) << endl;
		arr[counter++] = Word2float(*it);
	}

	return new_packet;
}

void* stencil_operation_th(void *args);
void* neighboursreduce_operation_th(void *args);

void Bridge::send(int target, Packet_t packet, int tag) { //TODO: make packet const?

	System& sba_system = *((System*) this->sba_system_ptr);
	MPI_Comm *comm_ptr = sba_system.comm_ptr;


	if (getType(getHeader(packet)) == P_DRESP) {
		printf ("Rank %d (Send): P_DRESP packet was detected\n", rank);

		float *arr = (float*)packet.back(); //TODO: Perhaps add that to packDRespPacket()?
		packet = packDRespPacket(packet);
		delete arr;
	}

	if (!(std::find(neighbours.begin(), neighbours.end(), target) != neighbours.end())){
		printf("ERROR: Rank %d tried to send a message to a rank that's not a neighbour\n", rank);
		return;
	}
	//printf("Rank %d: Ready to send...\n", rank);

	MPI_Request req;

	MPI_Isend(&packet.front(), packet.size(), MPI_UINT64_T, target, tag, *comm_ptr, &req);
	// while (!req.Test()) {} // wait for the message to be sent
	printf("Rank %d: Sent a packet to %d (packet.at(0)=%llu)\n", rank, target, packet.at(0));
}

void* wait_recv_any_th(void *arg){
	Bridge *bridge = (Bridge*) arg;
	//int rank = (int) bridge->rank;
	MPI_Status status;

	System& sba_system = *((System*)bridge->sba_system_ptr);
	MPI_Comm *comm_ptr = sba_system.comm_ptr;

	int flag;

	for(;;){
		do { // Test for a message, but don't receive
			MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, *comm_ptr, &flag, &status);
		}while (!flag);

		int tag = status.MPI_TAG;
		if (tag == tag_stencil_reduce) {
			continue; // Let the stencil_operation_th() function do this work
		}
		if (tag == tag_neighboursreduce_reduce) {
			continue; // Let the neighboursreduce_operation_th() function do this work
		}

		Packet_t packet;
		/*find size of packet */
		int recv_size;
		MPI_Get_count(&status, MPI_UINT64_T, &recv_size);
		packet.resize(recv_size);

		MPI_Request req;
		MPI_Irecv(&packet.front(), recv_size, MPI_UINT64_T, status.MPI_SOURCE, status.MPI_TAG,
					*comm_ptr, &req);


		// waits until the whole message is received TODO: make it so it doesn't block when 
		// multiple bridges (same system) reach this point
		int counter = 0;
		do {
			if (++counter == MAX_ITERATIONS){
					break;
			} 
			MPI_Test(&req, &flag, &status);
		} while (!flag);

		if (counter > MAX_ITERATIONS){			
			printf("=======Rank %d: bridge was stuck testing a Irecv request\n", bridge->rank);
			continue; // Test for a new message (MPI_Iprobe())
		}

		if (getType(getHeader(packet)) == P_DRESP) {
			printf("Rank %d (Recv): P_DRESP packet was detected\n", bridge->rank);
			packet = bridge->unpackDRespPacket(packet);
		}

		//sba_system.nodes[sba_system.selected_node + 1]->add_to_rx_fifo(packet); // thread safety - SOLVED
		//sba_system.selected_node = (sba_system.selected_node + 1) % NSERVICES; // thread safety - SOLVED
		Tile* tile_ptr = sba_system.get_node();
		tile_ptr->transceiver->add_to_rx_fifo(packet); //TODO: Solve this, it stops the execution of the program

		//	printf("Rank %d: Received a packet from source %d successfully!(packet.at(0)=%d)\n", rank, status.Get_source(), packet.at(0));
		if (tag == tag_stencil_scatter) { // For now just create a packet and send it back TODO: Modify this
			Packet_t new_packet;
			new_packet.push_back(7);
			bridge->send(status.MPI_SOURCE, new_packet, tag_stencil_reduce); // TODO: Test SOURCE
		}
		if (tag == tag_neighboursreduce_scatter) { // For now just create a packet and send it back TODO: Modify this
			Packet_t new_packet;
			new_packet.push_back(8);
			//bridge->send(status.MPI_SOURCE, new_packet, tag_neighboursreduce_reduce); //TODO: Test SOURCE
		}
	}

	return nullptr; // Does it matter what is returned since it runs indefinitely? // TODO: pthread_exit()
}

/*void Bridge::bcast_to_neighbours(Packet_t packet) {
	for (std::vector<int>::iterator it = neighbours.begin(); it < neighbours.end(); it++){
		this->send(*it, packet);
	}
}*/

/*void Bridge::scatter_to_neighbours(std::vector<Packet_t> packet_list){
	if (packet_list.size() != this->neighbours.size()) {
		printf("---ERROR (rank %d): Scattering %d packets to %d neighbours\n", rank, packet_list.size(), this->neighbours.size());
		exit(1);
	}
	for (unsigned int i = 0; i < neighbours.size() ; i++){
		this->send(neighbours.at(i), packet_list.at(i));
	}
}*/

void Bridge::stencil(std::vector<Packet_t> packet_list){

	/* Ensure Number of packets == Number of neighbours */
	/*if (packet_list.size() != this->neighbours.size()) {
		printf("---ERROR (rank %d): Scattering %d packets to %d neighbours\n", rank, packet_list.size(), this->neighbours.size());
		exit(1);
	}*/

	printf("Rank %d: Scattering %d packets among %d neighbours (STENCIL COMPUTATION)\n", rank, packet_list.size(), neighbours.size()); 


	pthread_t thread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	struct bridge_packets parameters_ = {
		this,
		packet_list
	};
	struct bridge_packets *parameters = new struct bridge_packets(parameters_);
	int rc = pthread_create(&thread, &attr, stencil_operation_th, (void *) parameters);
	if (rc) {
		printf("Rank %d: stencil operation thread could not be created. Exiting program...\n", rank);
		exit(1);
	}
	pthread_attr_destroy(&attr);	
	//void *th_status;
	// rc = pthread_join(thread, &th_status); // No need to wait as long as it works
}

void* stencil_operation_th(void *args){

	//Timeit
	double start_time = MPI_Wtime();
	struct bridge_packets *parameters;
	parameters = (struct bridge_packets *) args;
	std::vector<Packet_t> packet_list = parameters->packet_list;
	Bridge *bridge = parameters->bridge;
	printf("Rank %d: starting stencil operation\n", bridge->rank);

	int sum=0;
	MPI_Status status;
	int flag;

	System& sba_system = *((System*) bridge->sba_system_ptr);
	MPI_Comm *comm_ptr = sba_system.comm_ptr;

	for (unsigned int i = 0; i < packet_list.size() ; i++){
		bridge->send(bridge->neighbours.at(i%bridge->neighbours.size()), packet_list.at(i), tag_stencil_scatter);
		//printf("RANK %d: send a packet to %d (STENCIL)\n", bridge->rank, bridge->neighbours.at(i));
	}

	for (unsigned int i = 0; i < packet_list.size(); i++){
		do { // waits for a msg but doesn't receive
			MPI_Iprobe(bridge->neighbours.at(i%bridge->neighbours.size()), tag_stencil_reduce,
			*comm_ptr, &flag, &status);
		}
		while (!flag);

		Packet_t packet;
		//find size of packet
		int recv_size;
		MPI_Get_count(&status, MPI_UINT64_T, &recv_size);
		packet.resize(recv_size);

		MPI_Request request;
		MPI_Irecv(&packet.front(), recv_size, MPI_UINT64_T, status.MPI_SOURCE, tag_stencil_reduce,
					*comm_ptr, &request);

		do { // waits until the whole message is received
			MPI_Test(&request, &flag, &status);
		} while (!flag);
		printf("RANK %d: received tag_stencil_reduce from %d\n", bridge->rank, status.MPI_SOURCE);
		sum += packet.at(0); //TODO: What to do with the packets received?
	}
	printf("RANK %d: sum = %d\n", bridge->rank, sum);
	printf("Rank %d: STENCIL Thread exiting... (%fs)\n", bridge->rank, MPI_Wtime() - start_time);
	delete parameters; // Free the space allocated on the heap
	pthread_exit(NULL);
}

void Bridge::neighboursreduce(std::vector<Packet_t> packet_list) {
	/* Ensure Numberof packets == Number of neighbours */
	/*if (packet_list.size() != this->neighbours.size()) {
		printf("---ERROR (rank %d): Scattering %d packets to %d neighbours\n", rank, packet_list.size(), this->neighbours.size());
		exit(1);
	}*/
	printf("Rank %d: Scattering %d packets among %d neighbours (neighboursreduce COMPUTATION)\n", rank, packet_list.size(), neighbours.size()); 

	pthread_t thread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	struct bridge_packets parameters_ = {
		this,
		packet_list
	};
	struct bridge_packets *parameters = new struct bridge_packets(parameters_);
	int rc = pthread_create(&thread, &attr, neighboursreduce_operation_th, (void *) parameters);
	if (rc) {
		printf("Rank %d: neighboursreduce thread could not be created. Exiting program...\n", rank);
	}
	pthread_attr_destroy(&attr);	
	//void *th_status;
	//rc = pthread_join(thread, &th_status); // No need to wait AS LONG AS IT WORKS
}

void *neighboursreduce_operation_th(void *args){

	//Timeit
	double start_time = MPI_Wtime();

	struct bridge_packets *parameters;
	parameters = (struct bridge_packets *) args;
	std::vector<Packet_t> packet_list = parameters->packet_list;
	Bridge *bridge = parameters->bridge;
	//printf("Rank %d: starting neighboursreduce operation\n", bridge->rank);

	System& sba_system = *((System*) bridge->sba_system_ptr);
	MPI_Comm *comm_ptr = sba_system.comm_ptr;

	int sum=0;
	MPI_Status status;
	int flag;

	for (unsigned int i = 0; i < packet_list.size() ; i++){
		bridge->send(bridge->neighbours.at(i%bridge->neighbours.size()), packet_list.at(i), tag_neighboursreduce_scatter);
		//printf("RANK %d: send a packet to %d (neighboursreduce)\n", bridge->rank, bridge->neighbours.at(i));
	}

	for (unsigned int i = 0; i < packet_list.size(); i++){ // TODO: Maybe change this so it receives any tag_neighboursreduce_reduce packet regardless of its source
		do { // waits for a msg but doesn't receive
			MPI_Iprobe(bridge->neighbours.at(i%bridge->neighbours.size()), tag_neighboursreduce_reduce,
			*comm_ptr, &flag, &status);
		}
		while (!flag);

		Packet_t packet;
		//find size of packet
		int recv_size;
		MPI_Get_count(&status, MPI_UINT64_T, &recv_size);
		packet.resize(recv_size);

		MPI_Request request;
		MPI_Irecv(&packet.front(), recv_size, MPI_UINT64_T, status.MPI_SOURCE, tag_neighboursreduce_reduce,
					*comm_ptr, &request);

		do { // waits until the whole message is received
			MPI_Test(&request, &flag, &status);
		} while (!flag);

		printf("RANK %d: received tag_neighboursreduce_reduce from %d\n", bridge->rank, status.MPI_SOURCE);
		sum += packet.at(0); //TODO: Use an operation for the reduce part of the neighboursreduce
	}
	printf("RANK %d: sum = %d\n", bridge->rank, sum);

	Packet_t bpacket;
	bpacket.push_back(sum);
	for (unsigned int i = 0; i < (packet_list.size() > bridge->neighbours.size()? bridge->neighbours.size(): packet_list.size()); i++){
		bridge->send(bridge->neighbours.at(i), bpacket, tag_neighboursreduce_bcast);
	}
	printf("Rank %d: neighboursreduce Thread exiting... (%fs)\n", bridge->rank, MPI_Wtime() - start_time);
	delete parameters; // Free the space allocated on the heap
	pthread_exit(NULL);
}
