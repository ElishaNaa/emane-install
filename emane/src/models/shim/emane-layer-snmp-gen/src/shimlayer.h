/*
* Copyright (c) 2018 -Elisha Namman
*/


#ifndef EMANEMODEL_SNMPLAYER_SHIMLAYER_HEADER_
#define EMANEMODEL_SNMPLAYER_SHIMLAYER_HEADER_

#include "emane/shimlayerimpl.h"
#include "snmpmodemservice.h"


namespace EMANE
{
	namespace R2RI
	{
		// namespace SNMP
		// {
		/**
		* @class ShimLayer
		*
		* @brief Shim class that produces latency numbers from one Shim Layer to another
		*/

		class ShimLayer : public ShimLayerImplementor
		{
		public:
			ShimLayer(NEMId id,
				PlatformServiceProvider *pPlatformService,
				RadioServiceProvider * pRadioServiceProvider);

			~ShimLayer();

			void initialize(Registrar & registrar) override;

			void configure(const ConfigurationUpdate & update) override;

			void start() override;

			void stop() override;

			void destroy() throw() override;

			void processUpstreamControl(const ControlMessages & msgs) override;

			void processDownstreamControl(const ControlMessages & msgs) override;

			void processUpstreamPacket(UpstreamPacket & pkt,
				const ControlMessages & msgs) override;

			void processDownstreamPacket(DownstreamPacket & pkt,
				const ControlMessages & msgs) override;


		private:
			ModemService snmpModemService_;
			INETAddr addressRedis_;
			std::uint64_t timeToUpdateRedis_;
			double upperBound_; 
			double lowerBound_; 
		};
		// }
	}
}

#endif