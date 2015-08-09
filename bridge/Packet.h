#ifndef PACKET_H
#define PACKET_H

#include "Types.h"
#include "ServiceConfiguration.h"

using namespace std;
using namespace SBA;

namespace SBA {

	typedef unsigned int uint;

	typedef u_int64_t Uint64;
	typedef uint8 Packet_type_t; //3
	typedef uint16 Length_t; //16

	Packet_t packet_pointer_int(Packet_t packet);

	Header_t mkHeader(Word,Word,Word); // Actually it's mkHeader(Word,Word,Word,Word,Word,Word,Word,Word)
	Packet_t mkPacket(Header_t&,Payload_t&);
	Packet_t mkPacket_new(Header_t&,Word);


	Packet_type_t getType(Header_t header);
	Header_t getHeader(Packet_t packet);
	Length_t getLength(Header_t header);
	Word getReturn_as(Header_t header); 


} // namespace SBA


#endif // #ifndef PACKET_H
