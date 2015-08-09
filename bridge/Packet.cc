#include "Packet.h"
#include "ServiceConfiguration.h"
#include "Types.h"
#include <cstdio>

using namespace SBA;

//TODO: Delete or Modify (already exists)
Packet_type_t SBA::getType(Header_t header){
	return (Packet_type_t) header.at(0);
}

//TODO: Delete (already exists)
Header_t SBA::getHeader(Packet_t packet) {
	Header_t header;
	header.push_back(packet.at(0));
	header.push_back(packet.at(1));
	header.push_back(packet.at(2));
	return  header;
}

//TODO: Delete or Modify (already exists)
Length_t SBA::getLength(Header_t header){
	return (Length_t) header.at(1); // 2nd word in Header contains the length
}

//TODO: Delete (already exists)
Word SBA::getReturn_as(const Header_t header) { // Returns the size of the data array (for D_RESP only)
	return header[2];
}

//TODO: Delete or Modify (already exists)
Header_t SBA::mkHeader(Word packet_type, Word length, Word return_as) { //returns_as: size of data array
	Header_t wl;
	wl.push_back(packet_type);
	wl.push_back(length);
	wl.push_back(return_as);
return wl;
}

Packet_t SBA::mkPacket(Header_t& header,Payload_t& payload) {
	Packet_t packet;
	for(uint i=0;i<=HEADER_SZ-1 ;i++) {
		packet.push_back(header[i]);
	}
	for(auto iter_=payload.begin();iter_!=payload.end();iter_++) {
		Word w=*iter_;
		packet.push_back(w);
	}
	return packet;
}

Packet_t SBA::mkPacket_new(Header_t& header,Word payload) {
	Packet_t packet;
	for(uint i=0;i<=HEADER_SZ-1 ;i++) {
		packet.push_back(header[i]);
	}
	packet.push_back(payload);
	return packet;
}
