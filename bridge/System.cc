#include "System.h"
#include <string>
#include <vector>
#include "mpi.h"
#include <algorithm> //find()

void System::initialise_process_table(){ // create a 2D table of the MPI processes
	process_tbl.resize(rows); // Each element of this vector is a row
	for (int i = 0; i < rows; i++) {
		std::vector<int> row(cols);
		for (int j = 0; j < cols; j++){
			row.at(j) = rows*j + i;
		}
		process_tbl.at(i) = row; // Stores rows
	}
}

#ifdef MPI_TOPOLOGY_OPT
void System::find_neighbours(){ //This method can be used for a 2D topology ONLY, modifications are required for other dimensions
	int ndims = 2; //number of dimensions //TODO: define it as a macro?
	int coords[ndims]; // coord[0]:col, coord[1]: row
	MPI_Cart_coords(*comm_ptr,rank, ndims, coords);
	
	int new_coords[2];
	int neighbour;
	for (int col = -1; col <= 1; col++){
		for (int row = -1; row <= 1; row++) {
			new_coords[0] = coords[0] + col;
			new_coords[1] = coords[1] + row;
			MPI_Cart_rank(*comm_ptr,new_coords,&neighbour);
			if (!(std::find(neighbours.begin(), neighbours.end(), neighbour) != neighbours.end())) {
				this->neighbours.push_back(neighbour);
			}
		}	
	}
	std::sort(neighbours.begin(), neighbours.end());
}
#else // ifndef MPI_TOPOLOGY_OPT 
void System::find_neighbours(){
	int rows = this->process_tbl.size();
	int cols = this->process_tbl.at(0).size();
	int col = rank/rows; // row on which the rank is located
	int row = rank - col*rows; //column on which the rank is located 
	
	//printf("RANK %d: row:%d col:%d vec_size:%d\n", rank, row, col, neighbours.size());

	if ((row-1 >= 0) && (col-1 >= 0)) {									// top left	
		this->neighbours.push_back(this->process_tbl.at(row-1).at(col-1));
	}
	if (col-1 >= 0) {													// left 
		this->neighbours.push_back(this->process_tbl.at(row).at(col-1)); 
	}
	if ((row+1 < rows) && (col-1 >= 0)) {								//bottom left
		this->neighbours.push_back(this->process_tbl.at(row+1).at(col-1));
	}
	if (row-1 >= 0) {													// top
		this->neighbours.push_back(this->process_tbl.at(row-1).at(col));
	}
	neighbours.push_back(this->process_tbl.at(row).at(col));			// center (always in)
	if (row+1 < rows) { 												// bottom
		this->neighbours.push_back(this->process_tbl.at(row+1).at(col));
	}
	if ((row-1 >= 0) && (col+1 < cols )) {								// top right
		this->neighbours.push_back(this->process_tbl.at(row-1).at(col+1));
	}				
	if (col+1 < cols) { 												// right
		this->neighbours.push_back(this->process_tbl.at(row).at(col+1));
	}
	if ((row+1 < rows) && (col+1 < cols)) {								//bottom right
		this->neighbours.push_back(this->process_tbl.at(row+1).at(col+1));
	}
}
#endif //MPI_TOPOLOGY_OPT

void System::print_neighbours(){
	std::cout << "Neighbours of " << rank << ": ";
	for (std::vector<int>::iterator it = this->neighbours.begin(); it < this->neighbours.end(); it++){
		std::cout << *it << " ";
	}
	std::cout << std::endl;
}

void System::print_process_table(){
	std::cout << "=== PROCESS TABLE ===" << std::endl;
		for (auto row1: process_tbl){
			printf("\t");
			for (std::vector<int>::iterator it = row1.begin(); it < row1.end(); it++){
				std::cout << *it << " ";
			}
			printf("\n");
		}
		printf("=====================\n");
}


std::vector<int> System::get_neighbours(){
	return this->neighbours;
}

int System::get_rank(){
	return rank;
}


void System::send(Packet_t packet){
	increment_bridge_pos();
	printf("Rank %d: Bridge %d(0-%d) was selected to send a message \n", rank, bridge_pos, bridge_list.size()-1);
	this->bridge_list.at(bridge_pos)->send(packet);
}

void System::stencil_operation(std::vector<Packet_t> packet_list){
	increment_bridge_pos();
	printf("Rank %d: Bridge %d(0-%d) was selected to initiate a stencil operation\n", rank, bridge_pos, bridge_list.size()-1);
	this->bridge_list.at(bridge_pos)->stencil(packet_list);
}

void System::neighboursreduce_operation(std::vector<Packet_t> packet_list){
	increment_bridge_pos();
	printf("Rank %d: Bridge %d(0-%d) was selected to initiate a neighboursreduce operation\n", rank, bridge_pos, bridge_list.size()-1);
	this->bridge_list.at(bridge_pos)->neighboursreduce(packet_list);
}

void System::increment_bridge_pos(){ // TODO: make thread safe
	pthread_spin_lock(&bridge_selector_lock);
	bridge_pos = (bridge_pos+1) % bridge_list.size();
	pthread_spin_unlock(&bridge_selector_lock);
}

#ifdef MPI_TOPOLOGY_OPT
MPI_Comm* System::create_communicator(int rows, int cols){
	MPI_Comm comm, *comm_ptr;

	int ndims = 2;	// number of dimensions
	int dim[2];
	int period[2]; //state with dimension cycles
	int reorder; // state whether reordering of ranks is accepted, REQUIRED FOR POTENTIAL OPTIMISED LOGICAL TOPOLOGY
	
	dim[0] = rows;
	dim[1] = cols;
	period[0] = true;
	period[1] = true;
	reorder = true;

	MPI_Cart_create(MPI_COMM_WORLD, ndims, dim, period, reorder, &comm);
	comm_ptr = new MPI_Comm(comm);
	return comm_ptr;
}

#endif // MPI_TOPOLOGY_OPT

bool System::is_active(){
	return active;
}

void System::kill_thread(){ // thread-safe way of incrementing killed_threads, called when a receiving thread is about to exit
	pthread_spin_lock(&killed_threads_lock);
	killed_threads++;
	pthread_spin_unlock(&killed_threads_lock);
}
