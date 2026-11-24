#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"

#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFConfiguration.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFConstants.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFContext.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/SubsystemTags.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/TPrimitives.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/Utils/CSCUtils.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/Utils/DebugUtils.h"

#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/ME0TPCollector.h"

using namespace emtf::phase2;

ME0TPCollector::ME0TPCollector(const EMTFContext& context, edm::ConsumesCollector& i_consumes_collector)
    : context_(context),
      input_token_(i_consumes_collector.consumes<ME0Tag::collection_type>(context.config_.me0_input_)) {}

void ME0TPCollector::collect(const edm::Event& i_event, TPCollection& tpc) const {
  edm::Handle<ME0Tag::collection_type> me0_digis;
  i_event.getByToken(input_token_, me0_digis);

  for (auto chamber = me0_digis->begin(); chamber != me0_digis->end(); ++chamber) {
    for (auto digi = (*chamber).second.first; digi != (*chamber).second.second; ++digi) {
      tpc.emplace_back((*chamber).first, *digi);
      auto& info = tpc.back().info_;
      const auto& det_id = tpc.back().tp_.detId<ME0DetId>();
      const auto& data = tpc.back().tp_.getME0Data();

      const int region = det_id.region();
      info.bx = data.bx + context_.config_.me0_bx_shift_;
      info.endcap = (region == -1) ? 2 : region;
      info.endcap_pm = (info.endcap == 2) ? -1 : info.endcap;
      info.station = 1;
      info.ring = 4;
      info.layer = det_id.layer();
      info.roll = det_id.roll();
      info.chamber = det_id.chamber();

      // ME0 uses the same sector assignment as a 20-degree CSC chamber (Station 2, ring 1)
      info.sector = csc::getTriggerSector(2, 1, info.chamber);
      info.subsector = 0;
      info.csc_id = csc::getId(2, 1, info.chamber);
      info.csc_facing = csc::Facing::kNone;

#ifdef EMTF_ALLOW_ASSERT
      emtf_assert(kMinEndcap <= info.endcap && info.endcap <= kMaxEndcap);
      emtf_assert(kMinTrigSector <= info.sector && info.sector <= kMaxTrigSector);
      emtf_assert(info.subsector == 0);
      emtf_assert(info.station == 1);
      emtf_assert(info.ring == 4);
      emtf_assert(1 <= info.chamber && info.chamber <= 18);
      emtf_assert(1 <= info.csc_id && info.csc_id <= 3);
      emtf_assert(0 <= data.phiposition && data.phiposition <= 767);
      emtf_assert(0 <= data.partition && data.partition <= 15);
#endif
    }
  }
}
