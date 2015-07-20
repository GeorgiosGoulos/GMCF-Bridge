#include <iostream>
#include "System.h"
#include "Types.h"
#include <string> //stoi()
#include "mpi.h"
#include <vector>

#define SENDER 3

int main(int argc, char *argv[]){

	//MPI::Init(argc, argv);
	int tsl = MPI::Init_thread(argc, argv, MPI::THREAD_SERIALIZED);	//Thread support level
	
	int rank = MPI::COMM_WORLD.Get_rank();
	int size = MPI::COMM_WORLD.Get_size();
	int rows, cols;

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

	if (rank == 0) printf("Running with %d processes (%dx%d)\n", size, rows, cols);

	/* Detect thread support level */
	if (rank == 0){
		printf("THREAD SUPPORT LEVEL: ");
		switch (tsl){
			case MPI::THREAD_SINGLE: // A single thread will execute
				printf("%d: MPI::THREAD_SINGLE\n", rank);
				break;
			case MPI::THREAD_FUNNELED: // If multiple threads exist, only the one that called MPI_Init() will be able to make MPI calls
				printf("%d: MPI::THREAD_FUNNELED\n", rank);
				break;
			case MPI::THREAD_SERIALIZED: // If multiple threads exist, only one will be able to make MPI library calls at a time
				printf("MPI::THREAD_SERIALIZED\n", rank);
				break;
			case MPI::THREAD_MULTIPLE: // No restrictions on threads and MPI library calls (WARNING: "lightly tested")
				printf("%d: MPI::THREAD_MULTIPLE\n", rank);
				break;
			default:
				printf("%d: MPI::???\n", rank);
				break;
		}
	}

	MPI_Barrier(MPI_COMM_WORLD); //Waits for the above messages to be printed, not really needed otherwise

	System sba_system(rank, rows, cols);

	/* TESTING */

	int source = 4;
	int target = 1;

	if (rank == 0) sba_system.print_process_table();
	MPI_Barrier(MPI_COMM_WORLD); //Waits for the table to be printed, not really needed otherwise

	/* TEST - Broadcast */
	/*Packet_t packet;
	if (rank == SENDER) {
		packet.push_back(4);
		packet.push_back(42);
		packet.push_back(7);
		
		// sba_system.bcast_to_neighbours(packet);
		// sba_system.bcast_to_neighbours(packet);
		// sba_system.bcast_to_neighbours(packet);
	}*/

	/* TEST - Scatter */
	/*Packet_t packet;
	if (rank == SENDER) {
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
	if (rank == SENDER) {
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
	}

	/* TEST - AllReduce */
	/*if (rank == SENDER) {
		int num_neighbours = sba_system.get_neighbours().size();
		std::vector<Packet_t> packet_list;
		for (int i = 0; i < num_neighbours - 1; i++){
			Packet_t packet;
			packet.push_back(i+1);
			packet.push_back(i+2);
			packet_list.push_back(packet);
		}
		sba_system.allreduce_operation(packet_list);
		printf("Non-blocking allreduce computation has started\n");
	}*/

	for (;;) {} // So that the program doesn't exit

	MPI::Finalize();
}
