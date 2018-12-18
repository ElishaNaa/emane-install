#include "queuemappingsingalton.h"

EMANE::Models::TDMA::queuemappingsingalton::queuemappingsingalton()
{}

std::map<std::uint8_t, std::string> 
EMANE::Models::TDMA::queuemappingsingalton::getQueueMapping()
{
	return queueMapping_;
}

void 
EMANE::Models::TDMA::queuemappingsingalton::setQueueMapping(std::map<std::uint8_t, std::string> q)
{
	queueMapping_ = q;
}

EMANE::Models::TDMA::queuemappingsingalton * EMANE::Models::TDMA::queuemappingsingalton::instance()
{

	if (!s_instance)
		s_instance = new queuemappingsingalton;
	return s_instance;
}