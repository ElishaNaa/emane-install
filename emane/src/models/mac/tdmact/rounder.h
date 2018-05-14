
#ifndef ROUNDER_HEADER_
#define ROUNDER_HEADER_
// Rounder
// Simple class for rounding times

#include "emane/types.h"


namespace EMANE
{
  namespace Models
  {
    namespace TDMACT
    {
      class Rounder {
        uint64_t u64Rounding_ = 1; // minimal time step
        public:
         void setRounding(std::uint64_t &u64Rounding);
         void round_microSeconds(std::uint64_t &microSeconds);
         void round_EMANE_microSeconds(EMANE::Microseconds &microSeconds);
         void round(EMANE::TimePoint &time);
         std::uint64_t get_rounding_us();
      };
    }
  }
}

#include "rounder.inl"



#endif // ROUNDER_HEADER_
