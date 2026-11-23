#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"

#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFConfiguration.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFConstants.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFContext.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/GEMTPCollector.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/SubsystemTags.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/TPrimitives.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/Utils/DebugUtils.h"

using namespace emtf::phase2;

GEMTPCollector::GEMTPCollector(const EMTFContext& context, edm::ConsumesCollector& i_consumes_collector)
    : context_(context),
      input_token_(i_consumes_collector.consumes<GEMTag::collection_type>(context.config_.gem_input_)) {}

void GEMTPCollector::collect(const edm::Event& i_event, TPCollection& tpc) const {
  edm::Handle<GEMTag::collection_type> gem_digis;
  i_event.getByToken(input_token_, gem_digis);

  for (auto chamber = gem_digis->begin(); chamber != gem_digis->end(); ++chamber) {
    for (auto digi = (*chamber).second.first; digi != (*chamber).second.second; ++digi) {
      // Reject invalid digis before TriggerPrimitive construction
      if (!digi->isValid()) {
        continue;
      }

      tpc.emplace_back((*chamber).first, *digi);
      auto& info = tpc.back().info_;
      const auto& det_id = tpc.back().tp_.detId<GEMDetId>();
      const auto& data = tpc.back().tp_.getGEMData();

      const int region = det_id.region();
      info.bx = data.bx + context_.config_.gem_bx_shift_;
      info.endcap = (region == -1) ? 2 : region;
      info.endcap_pm = (info.endcap == 2) ? -1 : info.endcap;
      info.station = det_id.station();
      info.ring = det_id.ring();
      info.roll = det_id.roll();
      info.layer = det_id.layer();
      info.chamber = det_id.chamber();
      info.sector = csc::getTriggerSector(info.station, info.ring, info.chamber);
      info.subsector = csc::getTriggerSubsector(info.station, info.chamber);
      info.csc_id = csc::getId(info.station, info.ring, info.chamber);
      info.csc_facing = csc::getFaceDirection(info.station, info.ring, info.chamber);

#ifdef EMTF_ALLOW_ASSERT
      emtf_assert((*digi).isValid());
      const int pad = (data.pad_low + data.pad_hi) / 2;
      emtf_assert(kMinEndcap <= info.endcap && info.endcap <= kMaxEndcap);
      emtf_assert(kMinTrigSector <= info.sector && info.sector <= kMaxTrigSector);
      emtf_assert(0 <= info.subsector && info.subsector <= 2);
      emtf_assert(1 <= info.station && info.station <= 2);
      emtf_assert(info.ring == 1);
      emtf_assert(1 <= info.chamber && info.chamber <= 36);
      emtf_assert(1 <= info.csc_id && info.csc_id <= 3);
      emtf_assert(info.station == 1 || (1 <= info.roll && info.roll <= 16));
      emtf_assert(info.station != 1 || (1 <= info.roll && info.roll <= 8));
      emtf_assert(1 <= info.layer && info.layer <= 2);
      emtf_assert((info.station == 1 && 0 <= pad && pad <= 191) || info.station != 1);
      emtf_assert((info.station == 2 && 0 <= pad && pad <= 383) || info.station != 2);
#endif
    }
  }
}
