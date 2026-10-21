#ifndef L1Trigger_L1TMuonEndCapPhase2_SectorProcessor_h
#define L1Trigger_L1TMuonEndCapPhase2_SectorProcessor_h

#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFfwd.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFTypes.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/TPFilter.h"

namespace emtf::phase2 {

  class SectorProcessor {
  public:
    SectorProcessor(const EMTFContext&, const int&, const int&);

    ~SectorProcessor();

    void configureEvent(const edm::Event&);

    void process(const BXILinkTPCMap&, EMTFHitCollection&, EMTFTrackCollection&, EMTFInputCollection&);

  private:
    const EMTFContext& context_;

    int endcap_, sector_;
    TPFilter tp_filter_;
    std::map<SubsystemType, std::unique_ptr<TPConverter>> tp_converters_;

    // Event
    const edm::Event* event_;

    // Helper functions
    void convertTP(const int&, const ILinkTPCMap&, EMTFHitCollection&);

    void populateSegments(int, const EMTFHitCollection&, std::map<int, int>&, segment_collection_t&);

    void buildTracks(int, const std::map<int, int>&, const segment_collection_t&, const bool&, EMTFTrackCollection&);
  };

}  // namespace emtf::phase2

#endif  // L1Trigger_L1TMuonEndCapPhase2_SectorProcessor_h
