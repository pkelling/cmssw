#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"

#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFConfiguration.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFConstants.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFContext.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/SubsystemTags.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/TPrimitives.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/Utils/CSCUtils.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/Utils/DebugUtils.h"

#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/GE0TPCollector.h"

using namespace emtf::phase2;

GE0TPCollector::GE0TPCollector(const EMTFContext& context, edm::ConsumesCollector& i_consumes_collector)
    : context_(context),
      input_token_(i_consumes_collector.consumes<GE0Tag::collection_type>(context.config_.ge0_input_)) {}

void GE0TPCollector::collect(const edm::Event& i_event, BXTPCMap& bx_tpc_map) const {
  static const int me0_max_partition = 9;

  // Read GE0 digis
  TPCollection tpc;

  edm::Handle<GE0Tag::collection_type> me0_digis;
  i_event.getByToken(input_token_, me0_digis);

  auto chamber = me0_digis->begin();
  auto chend = me0_digis->end();

  for (; chamber != chend; ++chamber) {
    auto digi = (*chamber).second.first;
    auto dend = (*chamber).second.second;

    for (; digi != dend; ++digi) {
      tpc.emplace_back((*chamber).first, *digi);
    }
  }

  // Map to BX
  for (auto& tp_entry : tpc) {
    const auto& tp_det_id = tp_entry.tp_.detId<GEMDetId>();
    const ME0Data& tp_data = tp_entry.tp_.getME0Data();

    const int tp_region = tp_det_id.region();                    // 0: barrel, +/-1: endcap
    const int tp_endcap = (tp_region == -1) ? 2 : tp_region;     // 1: +endcap, 2: -endcap
    const int tp_endcap_pm = (tp_endcap == 2) ? -1 : tp_endcap;  // 1: +endcap, -1: -endcap
    const int tp_station = 1;
    const int tp_ring = 4;
    const int tp_layer = tp_det_id.layer();
    const int tp_roll = tp_det_id.roll();
    const int tp_chamber = tp_det_id.chamber();

    const int tp_pad = tp_data.phiposition;
    const int tp_partition = tp_data.partition;
    const int tp_bx = tp_data.bx + this->context_.config_.me0_bx_shift_;

    // Reject if outside eta of 2.4
    if (tp_partition > me0_max_partition) {
      continue;
    }

    // GE0 chamber 1 starts at 0 degrees. Use 20-degree CSC numbering for routing,
    // while retaining the GE0 station, ring, and physical chamber in TPInfo.
    const int tp_sector = csc::getTriggerSector(2, 1, tp_chamber);
    const int tp_subsector = 0;
    const int tp_csc_id = csc::getId(2, 1, tp_chamber);
    const auto tp_csc_facing = csc::Facing::kNone;

    // Assertion checks
    emtf_assert(kMinEndcap <= tp_endcap && tp_endcap <= kMaxEndcap);
    emtf_assert(kMinTrigSector <= tp_sector && tp_sector <= kMaxTrigSector);
    emtf_assert((1 <= tp_subsector) and (tp_subsector <= 2));
    emtf_assert(tp_station == 1);
    emtf_assert(tp_ring == 4);
    emtf_assert((1 <= tp_chamber) and (tp_chamber <= 36));
    emtf_assert(1 <= tp_csc_id && tp_csc_id <= 3);
    emtf_assert(0 <= tp_pad && tp_pad <= 767);
    emtf_assert(0 <= tp_partition && tp_partition <= 15);

    // Add info
    tp_entry.info_.bx = tp_bx;

    tp_entry.info_.endcap = tp_endcap;
    tp_entry.info_.endcap_pm = tp_endcap_pm;
    tp_entry.info_.sector = tp_sector;
    tp_entry.info_.subsector = tp_subsector;
    tp_entry.info_.station = tp_station;
    tp_entry.info_.ring = tp_ring;
    tp_entry.info_.layer = tp_layer;
    tp_entry.info_.roll = tp_roll;
    tp_entry.info_.chamber = tp_chamber;

    tp_entry.info_.csc_id = tp_csc_id;
    tp_entry.info_.csc_facing = tp_csc_facing;

    bx_tpc_map[tp_bx].push_back(tp_entry);
  }
}

void GE0TPCollector::collect(const edm::Event& i_event, TPCollection& tpc) const {
  edm::Handle<GE0Tag::collection_type> ge0_digis;
  i_event.getByToken(input_token_, ge0_digis);

  for (auto chamber = ge0_digis->begin(); chamber != ge0_digis->end(); ++chamber) {
    for (auto digi = (*chamber).second.first; digi != (*chamber).second.second; ++digi) {
      tpc.emplace_back((*chamber).first, *digi);
      auto& info = tpc.back().info_;
      const auto& det_id = tpc.back().tp_.detId<GEMDetId>();
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

      // GE0 uses the same sector assignment as a 20-degree CSC chamber (Station 2, ring 1)
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
