#ifndef SYSTEM_H
#define SYSTEM_H

#include "Base/System.h"
#include "Tile.h"
#include <vector>
#include <unordered_map>
#include "Types.h"

#ifdef BRIDGE
#include "mpi.h"
#include "pthread.h"
#include "Bridge.h"
#endif // BRIDGE

#ifdef VERBOSE
#include <sstream>
#include <iostream>
#endif //VERBOSE

class System: public Base::System {
	public:

		std::unordered_map<Service,Tile*> nodes;
#ifdef BRIDGE
		std::vector<Bridge*> bridge_list; // the list of bridges that have been created by this System instance
		MPI_Comm *comm_ptr; // pointer to the communicator
	#ifndef MPI_TOPOLOGY_OPT
		MPI_Comm comm; // it is assigned MPI_COMM_WORLD so that comm_ptr can later point to that
	#endif // MPI_TOPOLOGY_OPT

		pthread_spinlock_t bridge_selector_lock; // used for safely selecting a bridge from bridge_list
		pthread_spinlock_t killed_threads_lock; // used for updating the number of killed receiving threads, used in destructor

#ifdef VERBOSE
		stringstream ss; //used for printing messages
#endif // VERBOSE

#endif //BRIDGE
		
#ifdef BRIDGE
		//Constructor when BRIDGE is defined
		System(int r_, int c_): rows(r_), cols(c_), bridge_pos(-1), active(true), killed_threads(0){


			// Initialise MPI and define thread support level
			int tsl; // provided thread suppport level
			MPI_Init_thread(nullptr, nullptr, MPI_THREAD_SERIALIZED, &tsl);	

// Create a new communicator if MPI_TOPOLOGY_OPT was defined, or MPI_COMM_WORLD otherwise
#ifdef MPI_TOPOLOGY_OPT
			comm_ptr = System::create_communicator(rows, cols);
			MPI_Comm_rank(*comm_ptr, &rank);
			MPI_Comm_size(*comm_ptr, &size);
	#ifdef VERBOSE
			if (rank == 0 ) {
				cout << "Using optimised communicator (Cartesian topology)\n";
			}
	#endif // VERBOSE
#else // MPI_TOPOLOGY_OPT
	#ifdef VERBOSE
			comm = MPI_COMM_WORLD;
			comm_ptr = &comm;
			MPI_Comm_rank(*comm_ptr, &rank);
			MPI_Comm_size(*comm_ptr, &size);
			if (rank == 0 ) {
				cout << "Using standard communicator\n";
			}
	#endif // VERBOSE
#endif // MPI_TOPOLOGY_OPT


// Print the thread suppport level for each function (MPI does not ensure it will be the same for all processes)
#ifdef VERBOSE
			// Detect thread support level 
			ss << "Rank " << rank << ": Thread support level is ";
			switch (tsl){
				case MPI_THREAD_SINGLE: // A single thread will execute
					ss << "MPI_THREAD_SINGLE\n";
					break;
				case MPI_THREAD_FUNNELED: // If multiple threads exist, only the one that called MPI_Init() will be able to make MPI calls
					ss << "MPI_THREAD_FUNNELED\n";
					break;
				case MPI_THREAD_SERIALIZED: // If multiple threads exist, only one will be able to make MPI library calls at a time
					ss << "MPI_THREAD_SERIALIZED\n";
					break;
				case MPI_THREAD_MULTIPLE: // No restrictions on threads and MPI library calls (WARNING: "lightly tested")
					ss << "MPI_THREAD_MULTIPLE\n";
					break;
				default:
					ss << "???\n"; // Unidentified thread support level, this should never be printed
					break;
			}
			cout << ss.str();
#endif //VERBOSE

			initialise_process_table(); // create a 2D table of the MPI processes
			find_neighbours(); // find the processes that surround the current MPI node in the aforementioned table

			for (Service node_id_ = rank*NSERVICES + 1; node_id_ <= (Service) (rank+1)*NSERVICES; node_id_++) {
				ServiceAddress service_address=node_id_;
				Service node_id = node_id_;
				if  (service_address != 0) {
					
					if (nodes.count(node_id) == 0) {
						printf("RANK %d: ADDING NODE %d\n", rank, (int)node_id);
						nodes[node_id]=new Tile(this, node_id, service_address, rank);
					}
				}
			}

			/* Create a number of bridges */
			for (int i = 0; i < NBRIDGES; i++){
				bridge_list.push_back(new Bridge(this, rank));
			}

			pthread_spin_init(&bridge_selector_lock, PTHREAD_PROCESS_SHARED); 
			pthread_spin_init(&killed_threads_lock, PTHREAD_PROCESS_SHARED); 

		};
#endif // BRIDGE // should add a #else here for the original contructors making it the only constructor when MPI is used

		~System(){
			for (Service node_id_=1;node_id_<=NSERVICES;node_id_++) {
				delete nodes[node_id_];
			}
#ifdef BRIDGE		
			active = false;
	#ifdef VERBOSE
			ss.str("");
			ss << "Rank " << rank << ": killing receiving thread(s)...\n";
			cout << ss.str();
	#endif // VERBOSE
			while ((unsigned int) killed_threads < bridge_list.size()) {} // wait for threads to get killed
	#ifdef VERBOSE
			ss.str("");
			ss << "Rank " << rank << ": killed all " << bridge_list.size() << " receiving thread(s)...\n";
			cout << ss.str();
	#endif // VERBOSE
			pthread_spin_destroy(&killed_threads_lock); // no longer needed
			pthread_spin_destroy(&bridge_selector_lock); // no longer needed
			for (Bridge *bridge_ptr: bridge_list){
				delete bridge_ptr;
			}
	#ifdef MPI_TOPOLOGY_OPT
			delete comm_ptr; // created on the heap only when MPI_TOPOLOGY_OPT is defined (MPI_COMM_WORLD is used otherwise)
	#endif // MPI_TOPOLOGY_OPT

	/* Allow sometime for sending/receiving processes to finish their work */
	//for (int i = 0; i < 2000000; i++){}

	MPI_Finalize();
	#ifdef VERBOSE
			ss.str("");
			ss << "Rank " << rank << ": MPI_Finalize() was executed successfully...\n";
			cout << ss.str();
	#endif // VERBOSE
#endif // BRIDGE
		}

#ifdef BRIDGE
		void print_process_table();
		void find_neighbours(); // find the processes that surround the current MPI node in the process table
		void print_neighbours();
		std::vector<int> get_neighbours();
		int get_rank();
		int get_size() {
			return size;
		}

		void send(Packet_t packet);
		void stencil_operation(std::vector<Packet_t> packet_list);
		void neighboursreduce_operation(std::vector<Packet_t> packet_list);

		/**
		 * Returns the status of the System, which is true until the destructor of the System instance is called
		 * @return the value of active
		 */
		bool is_active();

		/**
		 * Thread-safe way of incrementing killed_threads, called when a receiving thread is about to exit 
		 */
		void kill_thread(); 

	#ifdef MPI_TOPOLOGY_OPT
		static MPI_Comm* create_communicator(int rows, int cols);
	#endif // MPI_TOPOLOGY_OPT

	private:
		/* Dimensions of process_tbl */
		int rows, cols; 

		/* Indicates the index of the next bridge to be used, cycles from 0 to bridge_list.size()-1 */
		int bridge_pos; 

		/* Indicates whether the System instance is active or not, used for killing receiving threads created by Bridge */
		bool active; 

		/* Indicates the number of threads killed during the destruction process, used to ensure all receivers have been killed */
		int killed_threads; 

		/* A vector of vectors representing a table of MPI processes TODO: move to find_neighbours after deployment */
		std::vector< std::vector<int> > process_tbl; 

		/* The neighbouring MPI processes of the current process */
		std::vector<int> neighbours; 

		/* MPI rank of current process */
		int rank;

		/* number of active MPI processes */
		int size;
		
		/** 
		 * Create a 2D table of the MPI processes, which is stored in process_tbl
		 */
		void initialise_process_table(); 

		/**
		 * Increment the member variable that indicates the index of the next bridge to be used 
		 */
		void increment_bridge_pos();


#endif // BRIDGE

};

#endif // SYSTEM_H
