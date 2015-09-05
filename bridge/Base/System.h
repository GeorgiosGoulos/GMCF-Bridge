/* File taken from the original GMCF code. The get_neighbours() virtual method was added so that the Bridge constructor can invoke it */

#ifndef SYSTEM_BASE_H_
#define SYSTEM_BASE_H_

#ifdef BRIDGE
#include <vector>
#endif // BRIDGE

namespace SBA {
	namespace Base {
		class System {
			public:
#ifdef BRIDGE
				virtual std::vector<int> get_neighbours()=0;		
#endif // BRIDGE
		};
	}
}
#endif /*SYSTEM_BASE_H_*/
