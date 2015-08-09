#include <iostream>
#include "System.h"
#include "Types.h"
#include <string> //stoi()
#include "mpi.h"
#include <vector>
#include "ServiceConfiguration.h"

#define SENDER 1 // rank of sender
#define D_SIZE 4 // size of data array

/*void findAndSetLength(Packet_t& packet){
	packet.at(1) = packet.size() - getHeader(packet).size();
}*/

int main(int argc, char *argv[]){

	// Define thread support level
	int tsl;
	MPI_Init_thread(&argc, &argv, MPI_THREAD_SERIALIZED, &tsl);	
	
	int rank, new_rank, size;
	int rows, cols;

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	if (argc < 3) {
		if (rank == 0) {
			std::cerr << "Not enough arguments. Needs at least 2 (width and height)\n";
		}
		MPI_Finalize();
		exit(0);
	} 

	if ((rows=std::stoi(argv[1])) * (cols=std::stoi(argv[2])) != size) {
		if (rank==0) {
			printf("parameters passed don't match the number of processes created\n");
			printf("(%d * %d != %d)\n", rows, cols, size);
		}
		MPI_Finalize();
		exit(0);
	}


	//--------------------------------------------------------------------
	// Create new (and potentially optimised) communicator
	MPI_Comm comm, *comm_ptr;

#ifdef MPI_TOPOLOGY_OPT
	comm = System::create_communicator(rows, cols);
	if (rank == 0 ) printf("Using new communicator.\n");
#else // MPI_TOPOLOGY_OPT
	if (rank == 0 ) printf("Using standard communicator.\n");
	comm = MPI_COMM_WORLD;
#endif // MPI_TOPOLOGY_OPT

	MPI_Comm_rank(comm, &new_rank); // TODO: Remove before deployment, keep only rank and reassign
#ifdef MPI_TOPOLOGY_OPT
	printf("Rank %d: new rank is %d\n", rank, new_rank);
#endif // MPI_TOPOLOGY_OPT
	comm_ptr = &comm;

	if (new_rank == 0) printf("Running with %d processes (%dx%d)\n", size, rows, cols);

	/* Detect thread support level */
	if (new_rank == 0){
		printf("Rank %d: THREAD SUPPORT LEVEL: ", new_rank);
		switch (tsl){
			case MPI_THREAD_SINGLE: // A single thread will execute
				printf("MPI_THREAD_SINGLE\n");
				break;
			case MPI_THREAD_FUNNELED: // If multiple threads exist, only the one that called MPI_Init() will be able to make MPI calls
				printf("MPI_THREAD_FUNNELED\n");
				break;
			case MPI_THREAD_SERIALIZED: // If multiple threads exist, only one will be able to make MPI library calls at a time
				printf("MPI_THREAD_SERIALIZED\n");
				break;
			case MPI_THREAD_MULTIPLE: // No restrictions on threads and MPI library calls (WARNING: "lightly tested")
				printf("MPI_THREAD_MULTIPLE\n");
				break;
			default:
				printf("MPI_???\n");
				break;
		}
	}

	MPI_Barrier(comm); //Waits for the above messages to be printed, not really needed otherwise

	System sba_system(new_rank, rows, cols, comm_ptr);

	
	/* TESTING */
	if (new_rank == 0) sba_system.print_process_table();
	MPI_Barrier(MPI_COMM_WORLD); //Waits for the table to be printed, not really needed otherwise


	if (SENDER >= size) {
		if (new_rank == 0) {
			printf("There is no node with SENDER rank\n");
		}
	}

	/* TEST - Broadcast */
	/*Packet_t packet;
	if (new_rank == SENDER) {
		packet.push_back(4);
		packet.push_back(42);
		packet.push_back(7);
		
		// sba_system.bcast_to_neighbours(packet);
		// sba_system.bcast_to_neighbours(packet);
		// sba_system.bcast_to_neighbours(packet);
	}*/

	/* TEST - Scatter */
	/*Packet_t packet;
	if (new_rank == SENDER) {
		int num_neighbours = sba_system.get_neighbours().size();
		std::vector<Packet_t> packet_list;
		for (int i = 0; i < num_neighbours;i++){
			Packet_t packet;
			packet.push_back(i+1);
			packet.push_back(i+2);
			packet_list.push_back(packet);
		}
		sba_system.bridge->scatter_to_neighbours(packet_list);
	}*/

	/* TEST - Stencil */
	/*if (new_rank == SENDER) {
		int num_neighbours = sba_system.get_neighbours().size();
		std::vector<Packet_t> packet_list;
		for (int i = 0; i < num_neighbours - 1; i++){
			Packet_t packet;
			packet.push_back(i+1);
			packet.push_back(i+2);
			packet_list.push_back(packet);
		}
		sba_system.stencil_operation(packet_list);
		printf("Non-blocking stencil computation has started\n");
	}*/

	/* TEST - neighboursreduce */
	
	if (new_rank == SENDER) {
		int num_neighbours = sba_system.get_neighbours().size();
		std::vector<Packet_t> packet_list;
		for (int i = 0; i < num_neighbours - 1; i++){
			Packet_t packet;

			float *arr = new float[D_SIZE];
			for (int i = 0 ; i < D_SIZE; i++) {
				arr[i] = i*2 + 10;
				//printf("arr[%d] = %f\n");
			}

			// Create header
			Header_t header = mkHeader(P_DRESP, 12345, D_SIZE);

			// Create payload
			Payload_t payload;
			payload.push_back(i+1);
			payload.push_back(i+2);
			payload.push_back(i+4);
			payload.push_back(i+5);

			void* fvp = (void*) arr;
			int64_t fivp = (int64_t) fvp;
			payload.push_back(fivp);

			packet = mkPacket(header, payload);

			packet.at(1) = packet.size() - getHeader(packet).size(); // Modify packet length manually // TODO: remove
			printf("PACKET LENGTH: %llu\n", packet.at(1));


			packet_list.push_back(packet);
		}
		sba_system.neighboursreduce_operation(packet_list);
		printf("Non-blocking neighboursreduce computation has started\n");
	}

	for (;;) {} // Keep the program running indefinitely

	MPI_Finalize();
}
