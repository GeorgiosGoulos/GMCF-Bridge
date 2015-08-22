#include "System.h"
#include "Types.h"
#include <string> //stoi()
#include "mpi.h"
#include <vector>
#include "ServiceConfiguration.h"
#include "Packet.h"

#ifdef VERBOSE
#include <sstream>
#include <iostream>
#endif //VERBOSE

#define SENDER 0 // rank of sender
#define D_SIZE 2 // size of data array


/** 
 * Test the transfer of GMCF packets of type P_DRESP to other MPI processes
 * @param sba_system the System instance
 */ 
void send_packet_dresp(System& sba_system);


/** 
 * Test the transfer of GMCF packets of a type other than P_DRESP to other MPI processes
 * @param sba_system the System instance
 */ 
void send_packet_no_dresp(System& sba_system);

/** 
 * Test the Bridge methods used for a stencil operation
 * @param sba_system the System instance
 */ 
void stencil_operation(System& sba_system);

//void neighboursreduce_operation(System& sba_system); // TODO: Use or Remove?

int main(int argc, char *argv[]){

#ifdef VERBOSE
	stringstream ss;
#endif // VERBOSE

	/* Dimensions of the logical MPI process grid */
	int rows, cols;

	if (argc < 3) { // TODO: REMOVE
			std::cerr << "Not enough arguments. Needs at least 2 (width and height)\n";
		MPI_Finalize();
		exit(1);
	} 

	rows=std::stoi(argv[1]);
	cols=std::stoi(argv[2]);

	System sba_system(rows, cols);


#ifdef VERBOSE
	MPI_Barrier(MPI_COMM_WORLD); //Waits for the table to be printed, not really needed otherwise
	if (sba_system.get_rank() == 0) sba_system.print_process_table();
	MPI_Barrier(MPI_COMM_WORLD); //Waits for the table to be printed, not really needed otherwise
#endif //VERBOSE

	/* TEST - neighboursreduce */


	/* TEST - mkHeader, mkPacket, P_DRESP */
	//send_packet_dresp(sba_system);

	/* TEST - mkHeader, mkPacket, no P_DRESP */	
	send_packet_no_dresp(sba_system);

	for (;;) {} // Keep the program running indefinitely
}

// Test the transfer of GMCF packets of type P_DRESP to other MPI processes
void send_packet_dresp(System& sba_system){
	if (sba_system.get_rank() == SENDER) {


		/* Create a float array. the GMCF packet will have a pointer to it */
		float *arr = new float[D_SIZE];
		for (int i = 0; i < D_SIZE; i++){
			*(arr + i) = 0.5 + i;
		}


		/* Add some Word elements to the payload */
		Payload_t payload;
		payload.push_back((Word)1);
		payload.push_back((Word)2);
		payload.push_back((Word)arr);

		/* node_id of the receiving tile */
		Word to_field = 29;

		/* node_id of sending tile */
		Word return_to_field = 6;

		/* Create the header of the GMCF packet. This function is part of the original GMCF code */
		Header_t header = mkHeader(P_DRESP, 2, 3, payload.size(), to_field, return_to_field, 7, D_SIZE);

		/* Create the GMCF packet. This function is part of the original GMCF code */
		Packet_t packet = mkPacket(header, payload);

		/* Create a second GMCF packet. These functions are part of the original GMCF code */
		header = mkHeader(P_DREQ, 2, 3, payload.size(), to_field, return_to_field, 7 ,D_SIZE);
		Packet_t packet2 = mkPacket(header, payload);

		/* Add the packets in the TX FIFO of the sending tile */
		sba_system.nodes[return_to_field]->transceiver->tx_fifo.push_back(packet);
		sba_system.nodes[return_to_field]->transceiver->tx_fifo.push_back(packet2);

		/* Transmit the GMCF packets in the TX FIFO of the return_to_field node */
		sba_system.nodes[return_to_field]->transceiver->transmit_packets();
	}

}

// Test the transfer of GMCF packets of a type other than P_DRESP to other MPI processes
void send_packet_no_dresp(System& sba_system){
	if (sba_system.get_rank() == SENDER) {

		/* Add some Word elements to the payload */
		Payload_t payload;
		payload.push_back((Word)1);
		payload.push_back((Word)2);
		payload.push_back((Word)0);

		/* node_id of the receiving tile */
		Word to_field = 29;
		
		/* node_id of sending tile */
		Word return_to_field = 6;

		/* Create the header of the GMCF packet. This function is part of the original GMCF code */
		Header_t header = mkHeader(P_DREQ, 2, 3, payload.size(), to_field, return_to_field, 7 ,D_SIZE);

		/* Create the GMCF packet. This function is part of the original GMCF code */
		Packet_t packet = mkPacket(header, payload);

		/* Create a second GMCF packet. These functions are part of the original GMCF code */
		header = mkHeader(P_DREQ, 2, 3, payload.size(), to_field, return_to_field, 7 ,D_SIZE);
		Packet_t packet2 = mkPacket(header, payload);

		/* Add the packets in the TX FIFO of the sending tile */
		sba_system.nodes[return_to_field]->transceiver->tx_fifo.push_back(packet);
		sba_system.nodes[return_to_field]->transceiver->tx_fifo.push_back(packet2);

		/* Transmit the GMCF packets in the TX FIFO of the return_to_field node */
		sba_system.nodes[return_to_field]->transceiver->transmit_packets();
	}
}

void stencil_operation(System& sba_system) {
	/*if (rank == SENDER) {
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
}

void neighboursreduce_operation(System& sba_system) {
	/*if (rank == SENDER) {
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
	}*/
}
