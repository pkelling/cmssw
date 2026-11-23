#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"

#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFConfiguration.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFConstants.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFContext.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/TPrimitives.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/SubsystemTags.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/Utils/DebugUtils.h"

#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/CSCTPCollector.h"

using namespace emtf::phase2;

CSCTPCollector::CSCTPCollector(const EMTFContext& context, edm::ConsumesCollector& i_consumes_collector)
    : context_(context),
      input_token_(i_consumes_collector.consumes<CSCTag::collection_type>(context.config_.csc_input_)) {}

void CSCTPCollector::collect(const edm::Event& i_event, TPCollection& tpc) const {
  edm::Handle<CSCTag::collection_type> csc_digis;
  i_event.getByToken(input_token_, csc_digis);

  for (auto chamber = csc_digis->begin(); chamber != csc_digis->end(); ++chamber) {
    for (auto digi = (*chamber).second.first; digi != (*chamber).second.second; ++digi) {
      tpc.emplace_back((*chamber).first, *digi);
      auto& info = tpc.back().info_;
      const auto& det_id = tpc.back().tp_.detId<CSCDetId>();
      const auto& data = tpc.back().tp_.getCSCData();

      info.bx = data.bx + context_.config_.csc_bx_shift_;
      info.endcap = det_id.endcap();
      info.endcap_pm = (info.endcap == 2) ? -1 : info.endcap;
      info.sector = det_id.triggerSector();
      info.station = det_id.station();
      info.ring = det_id.ring();
      info.chamber = det_id.chamber();
      info.layer = det_id.layer();
      info.subsector = csc::getTriggerSubsector(info.station, info.chamber);
      info.csc_id = data.cscID;
      info.csc_facing = csc::getFaceDirection(info.station, info.ring, info.chamber);
      info.csc_first_wire = data.keywire;

#ifdef EMTF_ALLOW_ASSERT
      const auto& [max_strip, max_wire] = csc::getMaxStripAndWire(info.station, info.ring);
      const auto& [max_pattern, max_quality] = csc::getMaxPatternAndQuality(info.station, info.ring);
      emtf_assert(kMinEndcap <= info.endcap && info.endcap <= kMaxEndcap);
      emtf_assert(kMinTrigSector <= info.sector && info.sector <= kMaxTrigSector);
      emtf_assert(0 <= info.subsector && info.subsector <= 2);
      emtf_assert(1 <= info.station && info.station <= 4);
      emtf_assert(1 <= info.ring && info.ring <= 4);
      emtf_assert(1 <= info.chamber && info.chamber <= 36);
      emtf_assert(1 <= info.csc_id && info.csc_id <= 9);
      emtf_assert(data.strip < max_strip);
      emtf_assert(data.keywire < max_wire);
      emtf_assert(data.pattern < max_pattern);
      emtf_assert(0 < data.quality && data.quality < max_quality);
      emtf_assert(data.valid);
#endif
    }
  }
}
