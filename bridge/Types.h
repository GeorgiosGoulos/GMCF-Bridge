#ifndef TYPES_H
#define TYPES_H

#include <vector>
#include "mpi.h"

typedef u_int64_t Uint64;
typedef Uint64 Word;
typedef std::vector<Word> Packet_t;
typedef std::vector<Word> Header_t;
typedef unsigned int Service; // 4 bytes
typedef unsigned int MemAddress; // typically fits into Subtask
typedef MemAddress ServiceAddress;

enum MPI_Send_Type {
	tag_other = 0,
	tag_stencil_scatter = 1,
	tag_stencil_reduce = 2,
	tag_allreduce_scatter = 3,
	tag_allreduce_reduce = 4,
	tag_allreduce_bcast = 5,
	tag_test = 9 // TODO: Remove?
};

#endif // TYPES_H
