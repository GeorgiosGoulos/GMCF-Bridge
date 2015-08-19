#include <iostream>
#include <cstdio>
#include "Bridge.h"
#include "mpi.h"
#include <vector>
#include <algorithm> //find()
#include <pthread.h>
#include "Packet.h"

#include "System.h"

using namespace std;
using namespace SBA;

/* Temporary solution for threads not getting stuck in wait_recv_any_th() */
#define MAX_ITERATIONS 2000 

/**
 * Used for passing parameters to threads 
 */
struct bridge_packets{ 
	/* A pointer to the bridge that created the thread */
	Bridge* bridge;

	/* A list of GPRM packets to be scattered among neighbours of the current MPI node */
	std::vector<Packet_t> packet_list;
};

/**
 * Used for passing parameters to threads 
 */
struct bridge_packet_tag{
	/* A pointer to the bridge that created the thread */
	Bridge* bridge;

	/* A GPRM packet to be sent using a MPI message */
	Packet_t packet;

	/* The tag of the MPI message to be sent */
	int tag;
};

/**
 * A helper function that "converts" floating-point numbers to elements of type Word
 * @param x the floating-point number to be converted
 * @return the element of type Word
 */
uint64_t float2Word(float x) {
    Word *y = reinterpret_cast<Word*>(&x); // endianness should be taken into consideration if different machines are used
    return *y;
}

/**
 * A helper function that "converts" elements of type Word to floating-point numbers
 * @param x the element of type Word to be converted
 * @return the floating-point number
 */
float Word2float(Word x) {
    float *y = reinterpret_cast<float*>(&x); // endianness should be taken into consideration if different machines are used
    return *y;
}

/**
 * A helper function that packages the contents of floating-point arrays together with the contents of GPRM packets
 * of type P_DRESP so that they can be sent as one message using MPI routines
 * @param packet the GPRM packet
 * @return The packaged contents
 */
Packet_t packDRespPacket(Packet_t packet){
	Header_t header = getHeader(packet);

	if (getPacket_type(header) != P_DRESP) return packet; // Failsafe

	int data_size = getReturn_as(header); 
	cout << "packP - Data size: " << data_size << endl; //TODO: Remove

	void* ptr = (void *) packet.back();
	float *arr = (float*)ptr;

	for (int i = 0; i < data_size; i++){
		cout << "Pack: Adding " << *(arr+i) << endl; //TODO: Remove
		packet.push_back(float2Word(*(arr+i)));
	}
	return packet;
}

/**
 * A helper function that unpackages the contents of MPI messages sent between processes
 * @param packet the MPI packet/message
 * @return The GPRM packet
 */
Packet_t unpackDRespPacket(Packet_t packet) {
	Header_t header = getHeader(packet);
	if (getPacket_type(header) != P_DRESP) return packet; //Failsafe

	int original_size = getLength(header); // size of the payload of the GPRM packet
	int data_size = getReturn_as(header); // size of the float array that the pointer in the GPRM packet points to

	Packet_t new_packet;
	float *arr = new float[data_size];

	// Copy the contents of the header and payload to a new packet
	for (vector<Word>::iterator it = packet.begin(); it < packet.begin() + getHeader(packet).size() + original_size - 1; it++){
		new_packet.push_back(*it);
	}
	new_packet.push_back((Word) arr);

	int counter = 0;

	for (vector<Word>::iterator it = packet.begin() + getHeader(packet).size() + original_size; it < packet.end(); it++){
		cout << "Unpack: Adding " << Word2float(*it) << endl; //TODO: Remove
		arr[counter++] = Word2float(*it);
	}

	return new_packet;
}


/**
 * A function that will be executed by a thread which will initiate a stencil operation
 * @param args a void pointer that points to a dynamically allocated bridge_packets structure
 */
void* stencil_operation_th(void *args);

/**
 * A function that will be executed by a thread which will initiate a neighboursreduce operation
 * @param args a void pointer that points to a dynamically allocated bridge_packets structure
 */
void* neighboursreduce_operation_th(void *args);

/**
 * A function that will be executed by a thread which will initiate a send operation
 * @param args a void pointer that points to a dynamically allocated bridge_packet_tag structure
 */
void* send_th(void *args);

void Bridge::send(int target, Packet_t packet, int tag) { //TODO: Remove

	/*System& sba_system = *((System*) this->sba_system_ptr);
	MPI_Request req;
	MPI_Comm *comm_ptr = sba_system.comm_ptr;

	Header_t header = getHeader(packet);
	if (getPacket_type(header) == P_DRESP) {
		printf ("Rank %d (Send): P_DRESP packet was detected\n", rank);

		float *arr = (float*)packet.back(); //TODO: Perhaps add that to packDRespPacket()?
		packet = packDRespPacket(packet);
		delete arr;
	}

	MPI_Isend(&packet.front(), packet.size(), MPI_UINT64_T, target, tag, *comm_ptr, &req);
	// while (!req.Test()) {} // wait for the message to be sent
	printf("Rank %d: Sent a packet to %d (packet.at(0)=%llu)\n", rank, target, packet.at(0));*/
}


// Used for sending GPRM packets through MPI messages to other MPI processes (used for debugging purposes)
void Bridge::send(Packet_t packet, int tag) {

	Header_t header = getHeader(packet);

	Header_t new_header = setReturn_to_rank(header, (Word) rank);

	packet = setHeader(packet, new_header);

	pthread_t thread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE); // TODO: Perhaps detached?

	struct bridge_packet_tag parameters_ = {
		this,
		packet,
		tag
	};
	struct bridge_packet_tag *parameters = new struct bridge_packet_tag(parameters_);

	int rc = pthread_create(&thread, &attr, send_th, (void *) parameters);
	if (rc) {
		printf("Rank %d: send thread could not be created. Exiting program...\n", rank);
		exit(1);
	}
	pthread_attr_destroy(&attr);	
}

// Returns the MPI neighbours of the current MPI process in  System::process_tbl
std::vector<int> Bridge::get_neighbours(){
	return sba_system_ptr->get_neighbours();
}

void Bridge::stencil(std::vector<Packet_t> packet_list){

#ifdef VERBOSE
	printf("Rank %d: Scattering %d packets among %d neighbours (STENCIL COMPUTATION)\n", rank, packet_list.size(), neighbours.size()); 
#endif // VERBOSE

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
}


void Bridge::neighboursreduce(std::vector<Packet_t> packet_list) {

#ifdef VERBOSE
	printf("Rank %d: Scattering %d packets among %d neighbours (neighboursreduce)\n", rank, packet_list.size(), neighbours.size()); 
#endif // VERBOSE

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

// The function executed by a different thread, used for listening for incoming MPI messages
// The thread is created in the constructor of Bridge
void* wait_recv_any_th(void *arg){
	Bridge *bridge = (Bridge*) arg;
	MPI_Status status;

	System& sba_system = *((System*)bridge->sba_system_ptr);
	MPI_Comm *comm_ptr = sba_system.comm_ptr;

	int flag;

	bool exit = false; // indicates whether the thread should be killed (System destructor was called)
	while(sba_system.is_active()){
		do { // Test for a message, but don't receive
			MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, *comm_ptr, &flag, &status);
			if (!sba_system.is_active()){
				exit = true;
				break;
			}
		}while (!flag);

		if (exit){
			break;
		}

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


		// waits until the whole message is received //TODO: Change to what WV suggested
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


#ifdef VERBOSE
		int from_rank = (int) getReturn_to_rank(getHeader(packet));
		printf("Rank %d: Received msg from rank %d\n", sba_system.get_rank(), from_rank);
#endif // VERBOSE
		Header_t header = getHeader(packet);
		if (getPacket_type(header) == P_DRESP) {
#ifdef VERBOSE
			printf("Rank %d (Recv): P_DRESP packet was detected\n", bridge->rank);
#endif // VERBOSE
			packet = unpackDRespPacket(packet);
		}

		Service service_id = getTo(getHeader(packet));
		ServiceAddress dest = service_id;
		dest = ((service_id-1) % NSERVICES) + 1; // Is this really needed?
		sba_system.nodes[dest]->transceiver->rx_fifo.push_back(packet);
#ifdef VERBOSE
		printf("Rank %d (Recv): Sent packet to dest %d\n", bridge->rank, dest);
#endif // VERBOSE

		if (tag == tag_stencil_scatter) { // For now just create a packet and send it back TODO: Modify this
			Packet_t new_packet;
			new_packet.push_back(7);
			//bridge->send(status.MPI_SOURCE, new_packet, tag_stencil_reduce); // TODO: Test SOURCE
		}
		if (tag == tag_neighboursreduce_scatter) { // For now just create a packet and send it back TODO: Modify this
			Packet_t new_packet;
			new_packet.push_back(8);
			//bridge->send(status.MPI_SOURCE, new_packet, tag_neighboursreduce_reduce); //TODO: Test SOURCE
		}
	}
	sba_system.kill_thread();
#ifdef VERBOSE
	printf(" %d: Recv thread is exiting...\n", sba_system.get_rank());
#endif // VERBOSE
	pthread_exit(NULL);
}

// A function that will be executed by a thread which will initiate a send operation
void* send_th(void *args) {
	
	/* Convert the void pointer *args to a bridge_packet_tag structure */
	struct bridge_packet_tag *parameters;
	parameters = (struct bridge_packet_tag *) args;

	/* Pointer to the bridge that created this thread */
	Bridge* bridge = parameters->bridge;

	/* Pointer to the System instance that created the aforementioned Bridge */
	System& sba_system = *((System*)bridge->sba_system_ptr);

	/* Pointer to the communicator to be used for this operation */
	MPI_Comm *comm_ptr = sba_system.comm_ptr;

	/* The GPRM packet to be sent */
	Packet_t packet = parameters->packet;

	/* The tag of the MPI message to be sent */
	int tag = parameters->tag;

	/* the header of the GPRM packet to be sent */
	Header_t header = getHeader(packet);

	/* The MPI rank of the process to receive the packet */
	int to_rank = (int) getTo_rank(header);

	/* If the GPRM packet is of type P_DRESP package the contents of the float array it points to along with the packet */
	if (getPacket_type(header) == P_DRESP) {
#ifdef VERBOSE
		printf("Rank %d (Send): P_DRESP packet was detected\n", sba_system.get_rank()); //TODO: Remove
#endif // VERBOSE
		float *arr = (float*)packet.back();
		packet = packDRespPacket(packet);
		
		/* Delete the dynamically allocated float array */
		delete arr;
	}

	/* A handle to a request object used for quering the status of the send operation */
	MPI_Request req;

	/* Send the MPI message */
	MPI_Isend(&packet.front(), packet.size(), MPI_UINT64_T, to_rank, tag, *comm_ptr, &req);

	/* Flag that indicates the status of the send operation */
	int flag;

	/* Wait until the whole message is received */
	do { 
		MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
	} while (!flag);

#ifdef VERBOSE
	printf("Rank %d (Sent): Sent a packet to %d\n", sba_system.get_rank(), to_rank);
#endif // VERBOSE

	/* Remove the dynamically allocated bridge_packet_tag structure that was passed as an argument to this function */
	delete parameters;

	pthread_exit(NULL);
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
#ifdef VERBOSE
		printf("RANK %d: send a packet to %d (STENCIL)\n", bridge->rank, getTo_rank(getHeader(packet_list.at(i))));
#endif //VERBOSE
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
