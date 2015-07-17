#include "System.h"
#include <string>
#include <vector>

void System::initialise_process_table(){
	process_tbl.resize(rows);
	for (int i = 0; i < rows; i++) {
		std::vector<int> row(cols);
		for (int j = 0; j < cols; j++){
			row.at(j) = i*cols + j;
		}
		process_tbl.at(i) = (row);
	}
}

void System::find_neighbours(){
	int rows = this->process_tbl.size();
	int cols = this->process_tbl.at(0).size();
	int rank = this->node_id; // Subject to change
	int row = rank/cols; // row on which the rank is located
	int col = rank - row*cols; //column on which the rank is located 
	
	//printf("RANK %d: row:%d col:%d vec_size:%d\n", rank, row, col, neighbours.size());

	if ((row-1 >= 0) && (col-1 >= 0)) {									// top left	
		this->neighbours.push_back(this->process_tbl.at(row-1).at(col-1));
	}
	if (row-1 >= 0) {													// top
		this->neighbours.push_back(this->process_tbl.at(row-1).at(col));
	}
	if ((row-1 >= 0) && (col+1 < cols )) {								// top right
		this->neighbours.push_back(this->process_tbl.at(row-1).at(col+1));
	}
	if (col-1 >= 0) {													// left 
		this->neighbours.push_back(this->process_tbl.at(row).at(col-1)); 
	}				
	neighbours.push_back(this->process_tbl.at(row).at(col));			// center (always in)
	if (col+1 < cols) { 												// right
		this->neighbours.push_back(this->process_tbl.at(row).at(col+1));
	}
	if ((row+1 < rows) && (col-1 >= 0)) {								//bottom left
		this->neighbours.push_back(this->process_tbl.at(row+1).at(col-1));
	}
	if (row+1 < rows) { 												// bottom
		this->neighbours.push_back(this->process_tbl.at(row+1).at(col));
	}
	if ((row+1 < rows) && (col+1 < cols)) {								//bottom right
		this->neighbours.push_back(this->process_tbl.at(row+1).at(col+1));
	}
}

void System::print_neighbours(){
	std::cout << "Neighbours of " << this->node_id << ": ";
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