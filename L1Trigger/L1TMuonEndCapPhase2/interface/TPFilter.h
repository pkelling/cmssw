#ifndef L1Trigger_L1TMuonEndCapPhase2_TPFilter_h
#define L1Trigger_L1TMuonEndCapPhase2_TPFilter_h

#include <cstdint>

#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFTypes.h"

namespace emtf::phase2 {

  class TPFilter {
  public:
    explicit TPFilter(const EMTFContext&);

    ILinkTPCMap filter(const ILinkTPCMap&) const;

  private:
    const EMTFContext& context_;

    // Valid layer-2 GEM candidates, grouped by chamber for the current BX.
    typedef std::map<uint32_t, std::vector<const TPEntry*>> GEMCoincidenceMap;

    bool acceptCSC(const TPEntry&) const;
    bool acceptRPC(const TPEntry&) const;
    bool acceptGEM(TPEntry&, const GEMCoincidenceMap&) const;
    bool acceptME0(const TPEntry&) const;
    bool acceptGE0(const TPEntry&) const;
  };

}  // namespace emtf::phase2

#endif  // L1Trigger_L1TMuonEndCapPhase2_TPFilter_h
