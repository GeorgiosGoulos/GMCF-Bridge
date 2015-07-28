#include "System.h"
#include <string>
#include <vector>
#include "mpi.h"
#include <algorithm> //find()

void System::initialise_process_table(){
	process_tbl.resize(rows); // Each element is a row
	for (int i = 0; i < rows; i++) {
		std::vector<int> row(cols);
		for (int j = 0; j < cols; j++){
			row.at(j) = rows*j + i;
		}
		process_tbl.at(i) = (row); // Stores rows
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
#else //MPI_TOPOLOGY_OPT 
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

/*void System::bcast_to_neighbours(Packet_t packet){
	printf("Rank %d: Bridge %d(%d) was selected to send a message\n", rank, bridge_pos, bridge_list.size());
	this->bridge_list.at(bridge_pos)->bcast_to_neighbours(packet);
	bridge_pos = (bridge_pos+1) % bridge_list.size();
}*/

void System::stencil_operation(std::vector<Packet_t> packet_list){
	printf("Rank %d: Bridge %d(%d) was selected to send a message (stencil)\n", rank, bridge_pos, bridge_list.size());
	this->bridge_list.at(bridge_pos)->stencil(packet_list);
	bridge_pos = (bridge_pos+1) % bridge_list.size();
}

void System::allreduce_operation(std::vector<Packet_t> packet_list){
	printf("Rank %d: Bridge %d(%d) was selected to send a message (allreduce)\n", rank, bridge_pos, bridge_list.size());
	this->bridge_list.at(bridge_pos)->allreduce(packet_list);
	bridge_pos = (bridge_pos+1) % bridge_list.size();
}

Tile* System::get_node() { // TODO: USE IT
	Tile* tile_ptr;
	pthread_spin_lock(&_nodes_lock);
	printf("Rank %d: entered lock\n", rank);
	tile_ptr = nodes[selected_node+1]; // value ranges from 1 to NSERVICES
	selected_node = selected_node + 1 % NSERVICES;
	printf("Rank %d: exiting lock\n", rank);
	pthread_spin_unlock(&_nodes_lock);
	return tile_ptr;
}

#ifdef MPI_TOPOLOGY_OPT
MPI_Comm System::create_communicator(int rows, int cols){
	MPI_Comm comm;

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
	return comm;
}

#endif // MPI_TOPOLOGY_OPT
