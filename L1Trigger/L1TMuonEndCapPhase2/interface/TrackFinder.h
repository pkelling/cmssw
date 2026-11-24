#ifndef L1Trigger_L1TMuonEndCapPhase2_TrackFinder_h
#define L1Trigger_L1TMuonEndCapPhase2_TrackFinder_h

#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFfwd.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFTypes.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFContext.h"

namespace emtf::phase2 {

  class TrackFinder {
  public:
    explicit TrackFinder(const edm::ParameterSet&, edm::ConsumesCollector&&);

    ~TrackFinder();

    void process(
        // Input
        const edm::Event&,
        const edm::EventSetup&,
        // Output
        EMTFHitCollection&,
        EMTFTrackCollection&,
        EMTFInputCollection&);

    void onJobBegin();

    void onJobEnd();

  private:
    void printDebuggingTPs(const TPCollection&,
                           const std::map<int, std::map<int, BXEMTFChamberTPCMap>>&) const;

    EMTFContext context_;

    std::vector<std::unique_ptr<TPCollector>> tp_collectors_;
    std::map<SubsystemType, std::unique_ptr<TPSelector>> tp_selectors_;
    // Sector processors are organized by [endcap][sector].
    std::map<int, std::map<int, std::unique_ptr<SectorProcessor>>> sector_processors_;
  };
}  // namespace emtf::phase2

#endif
