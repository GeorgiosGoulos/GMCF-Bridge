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

int main(int argc, char *argv[]){

#ifdef VERBOSE
 stringstream ss;
#endif // VERBOSE


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
	if (sba_system.get_rank() == 0) sba_system.print_process_table();
	MPI_Barrier(MPI_COMM_WORLD); //Waits for the table to be printed, not really needed otherwise
#endif //VERBOSE

	/* TEST - Stencil */
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

	/* TEST - neighboursreduce */
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

	/* TEST - mkHeader,mkPacket, P_DRESP */
	if (sba_system.get_rank() == SENDER) {

		float *arr = new float[D_SIZE];
		for (int i = 0; i < D_SIZE; i++){
			*(arr + i) = 0.5 + i;
		}

		Payload_t payload;
		payload.push_back((Word)1);
		payload.push_back((Word)2);
		payload.push_back((Word)arr);


		Word to_field = 29;

		Header_t header = mkHeader(P_DRESP, 2, 3, payload.size(), to_field, 6, 7 ,D_SIZE);
		Packet_t packet = mkPacket(header, payload);

		header = mkHeader(P_DREQ, 2, 3, payload.size(), to_field, 6, 7 ,D_SIZE);
		Packet_t packet2 = mkPacket(header, payload);
		/*cout << "Packet_type: " << (int) getPacket_type(header) << endl;
		cout << "Prio/Ctrl: " << (int) getCtrl(header) << endl;
		cout << "Redir: " << (int) getRedir(header) << endl;
		cout << "Length: " << (int) getLength(header) << endl;
		cout << "To: " << (int) getTo(header) << endl;
		cout << "Return_to: " << (int) getReturn_to(header) << endl;*/


		//sba_system.bridge_list.at(0)->send(packet);
		sba_system.nodes[6]->transceiver->tx_fifo.push_back(packet);
		sba_system.nodes[6]->transceiver->tx_fifo.push_back(packet2);
		sba_system.nodes[6]->transceiver->transmit_packets();
	}

	/* TEST - mkHeader,mkPacket, non-P_DRESP */
	/*if (rank == SENDER) {

		float *arr = new float[D_SIZE];
		for (int i = 0; i < D_SIZE; i++){
			*(arr + i) = 0.5 + i;
		}

		Payload_t payload;
		payload.push_back((Word)1);
		payload.push_back((Word)2);
		payload.push_back((Word)arr);

		Header_t header = mkHeader(P_DREQ, 2, 3, payload.size(), 5, 6, 7 ,D_SIZE);
		cout << "Packet_type: " << (int) getPacket_type(header) << endl;
		cout << "Prio/Ctrl: " << (int) getCtrl(header) << endl;
		cout << "Redir: " << (int) getRedir(header) << endl;
		cout << "Length: " << (int) getLength(header) << endl;
		cout << "To: " << (int) getTo(header) << endl;
		cout << "To_rank: " << (int) getTo_rank(header) << endl;
		cout << "Return_to: " << (int) getReturn_to(header) << endl;
		cout << "Return_to_rank: " << (int) getReturn_to_rank(header) << endl;

		header = setTo_rank(header, (Word)1);
		header = setReturn_to_rank(header, (Word) SENDER);
		cout << "To_rank: " << (int) getTo_rank(header) << endl;
		cout << "Return_to_rank: " << (int) getReturn_to_rank(header) << endl;

		Packet_t packet = mkPacket(header, payload);

		//sba_system.bridge_list.at(0)->send(packet);
		sba_system.send(packet);
	}*/

	for (;;) {} // Keep the program running indefinitely
}
