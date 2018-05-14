inline void EMANE::Models::TDMACT::Rounder::setRounding(std::uint64_t &u64Rounding) {
    u64Rounding_ = u64Rounding;
}

inline void EMANE::Models::TDMACT::Rounder::round_microSeconds(std::uint64_t &microSeconds) {
    if ((microSeconds % u64Rounding_) >= u64Rounding_/2) microSeconds = (microSeconds/u64Rounding_ + 1)*u64Rounding_;
    else microSeconds = (microSeconds/u64Rounding_)*u64Rounding_;
}

inline std::uint64_t EMANE::Models::TDMACT::Rounder::get_rounding_us() {
    return u64Rounding_;
}

inline void EMANE::Models::TDMACT::Rounder::round(EMANE::TimePoint &time) {
    EMANE::Microseconds fromEpoch = std::chrono::duration_cast<EMANE::Microseconds>((time).time_since_epoch());
    std::uint64_t elapsedMicro = fromEpoch.count();
    round_microSeconds(elapsedMicro);
    EMANE::Microseconds rounded{elapsedMicro};
    time = EMANE::TimePoint{rounded};
}

inline void EMANE::Models::TDMACT::Rounder::round_EMANE_microSeconds(EMANE::Microseconds &microSeconds) {
    std::uint64_t u64_microSeconds = microSeconds.count();
    round_microSeconds(u64_microSeconds);
    microSeconds = EMANE::Microseconds{u64_microSeconds};
}
