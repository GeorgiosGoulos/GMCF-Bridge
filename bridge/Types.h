#ifndef TYPES_H
#define TYPES_H

#include <vector>


//TODO: Remove?
#include <cstdint>
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint64_t u_int64_t;

typedef u_int64_t Uint64;
typedef Uint64 Word;
typedef std::vector<Word> Packet_t;
typedef std::vector<Word> Header_t;
typedef std::vector<Word> Payload_t;
typedef unsigned int Service; // 4 bytes
typedef unsigned int MemAddress; // typically fits into Subtask
typedef MemAddress ServiceAddress;

typedef std::vector<Packet_t> Packet_Fifo; //Actually it's typedef Fifo<Packet_t> Packet_Fifo;
typedef Packet_Fifo TX_Packet_Fifo;
typedef Packet_Fifo RX_Packet_Fifo; // Actually RX_Packet_Fifo is a class containing a deque (Types.h)


enum MPI_Send_Type {
	tag_other = 0,
	tag_stencil_scatter = 1,
	tag_stencil_reduce = 2,
	tag_neighboursreduce_scatter = 3,
	tag_neighboursreduce_reduce = 4,
	tag_neighboursreduce_bcast = 5,
	tag_test = 9 // TODO: Remove?
};

#define TMP_RANK 0 // debugging, used in System.h/System.cc
#define NSERVICES 4 // tmp, used in System.h/System.cc
#define NBRIDGES 2 // tmp, used in System.h/System.cc

#endif // TYPES_H
