#ifndef EMANEQUEUEMAPPINGSINGALTON_HEADER_
#define EMANEQUEUEMAPPINGSINGALTON_HEADER_

#include "emane/types.h"

#include <sstream>
#include <vector>
#include <map>

namespace EMANE
{
	namespace Models
	{
		namespace TDMA
		{
			class queuemappingsingalton
			{
			private:
				static queuemappingsingalton* s_instance;
				std::map<std::uint8_t, std::string> queueMapping_;
				queuemappingsingalton();
			public:
				std::map<std::uint8_t, std::string> getQueueMapping();
				void setQueueMapping(std::map<std::uint8_t, std::string> q);
				static queuemappingsingalton *instance();
			};

		}
	}
}

#endif // EMANEQUEUEMAPPINGSINGALTON_HEADER_
