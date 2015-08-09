#ifndef SERVICE_MGR_CONF_H_
#define SERVICE_MGR_CONF_H_

///#include "Base/Types.h" //
//#include "./Base/Types.h" // WHY?

//using namespace SBA;

namespace SBA {

	#define HEADER_SZ 3

	// For compatibility with streaming data, we either add a Mode field (stream or 'datagram') or we use a bit in the Type field
	// We use the MSB to indicate the mode.
	enum Packet_Type { // >7, so needs at least 4 bits. We have 8 bits.
		    P_error=0, // Payload could be error message. In general not Symbols
		    P_subtask=1, // list of Symbols. At least 2 elts: (S ...); Can be stream
		    P_code=2, // list of Symbols; Can be stream
		    P_reference=3, // 1-elt list of Symbols; Can be stream
		    P_request=4, // 1-elt list of Symbols; Can be stream
		    P_data=5, // preferred; Can be stream
		    P_DREQ=6, // list of Symbols. Usually 1-elt
		    P_TREQ=7,
		    P_DRESP=8, // "what is the address of service n?", with n a number => 1-elt list of uint64
		    P_TRESP=9, //
		    P_FIN=10, // "my address is n", with n a number => 1-elt list of uint64
		    P_DACK=11, // data ack, to signal it's OK to de-allocate memory
		    P_RRDY=12 // register ready for access by another thread
		};



}

#endif // SERVICE_MGR_CONF_H_
