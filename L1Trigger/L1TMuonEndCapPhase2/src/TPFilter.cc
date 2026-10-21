#include "L1Trigger/L1TMuonEndCapPhase2/interface/TPFilter.h"

#include "DataFormats/MuonDetId/interface/GEMDetId.h"

#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFContext.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/TPrimitives.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/Utils/CSCUtils.h"

using namespace emtf::phase2;

TPFilter::TPFilter(const EMTFContext& context) : context_(context) {}

ILinkTPCMap TPFilter::filter(const ILinkTPCMap& ilink_tpc_map) const {
  GEMCoincidenceMap gem_coincidences;

  // Gather GEM coincidence candidates before filtering either layer.
  for (const auto& [ilink, tpc] : ilink_tpc_map) {
    for (const auto& entry : tpc) {
      if (entry.tp_.subsystem() != L1TMuon::kGEM || entry.info_.layer != 2) {
        continue;
      }

      const auto det_id = entry.tp_.detId<GEMDetId>();
      const GEMDetId module_id(det_id.region(), det_id.ring(), det_id.station(), 0, det_id.chamber(), 0);
      gem_coincidences[module_id.rawId()].push_back(&entry);
    }
  }

  ILinkTPCMap filtered_tps;

  // Preserve the original order, including subsystems sharing an input link.
  for (const auto& [ilink, tpc] : ilink_tpc_map) {
    for (const auto& entry : tpc) {
      auto filtered_entry = entry;
      bool accepted = true;

      switch (entry.tp_.subsystem()) {
        case L1TMuon::kCSC:
          accepted = acceptCSC(entry);
          break;
        case L1TMuon::kRPC:
          accepted = acceptRPC(entry);
          break;
        case L1TMuon::kGEM:
          accepted = acceptGEM(filtered_entry, gem_coincidences);
          break;
        case L1TMuon::kME0:
          // GE0 uses the ME0 subsystem tag and overrides ME0 conversion when enabled.
          accepted = context_.config_.ge0_en_ ? acceptGE0(entry) : acceptME0(entry);
          break;
        default:
          break;
      }

      if (accepted) {
        filtered_tps[ilink].push_back(filtered_entry);
      }
    }
  }

  return filtered_tps;
}

bool TPFilter::acceptCSC(const TPEntry& entry) const {
  const auto& info = entry.info_;
  const auto data = entry.tp_.getCSCData();
  const auto [max_strip, max_wire] = csc::getMaxStripAndWire(info.station, info.ring);
  const auto [max_pattern, max_quality] = csc::getMaxPatternAndQuality(info.station, info.ring);

  return data.valid && data.strip < max_strip && data.keywire < max_wire && data.pattern < max_pattern &&
         0 < data.quality && data.quality < max_quality;
}

bool TPFilter::acceptRPC(const TPEntry& entry) const {
  const auto& info = entry.info_;
  const auto data = entry.tp_.getRPCData();

  // Exclude overlap chambers RE1/3 and RE2/3.
  if (info.station <= 2 && info.ring == 3) {
    return false;
  }

  const int cluster_width = data.strip_hi - data.strip_low + 1;
  const int max_cluster_width = (info.rpc_type == rpc::Type::kiRPC) ? 6 : 4;
  return cluster_width <= max_cluster_width;
}

bool TPFilter::acceptGEM(TPEntry& entry, const GEMCoincidenceMap& gem_coincidences) const {
  const auto& info = entry.info_;

  // Layer 2 supplies coincidence candidates but produces no converted hits.
  if (info.layer != 1) {
    return false;
  }

  const auto det_id = entry.tp_.detId<GEMDetId>();
  const GEMDetId module_id(det_id.region(), det_id.ring(), det_id.station(), 0, det_id.chamber(), 0);
  const auto candidates_it = gem_coincidences.find(module_id.rawId());

  // Keep layer 1 as a substitute when the chamber has no valid layer-2 candidates.
  if (candidates_it == gem_coincidences.end()) {
    entry.info_.flag_substitute = true;
    return true;
  }

  const int max_delta_roll = 1;
  const int max_delta_pad = 4;  // Same tolerance for GE1/1 and GE2/1.
  const auto data = entry.tp_.getGEMData();

  for (const auto* candidate : candidates_it->second) {
    const auto candidate_data = candidate->tp_.getGEMData();
    const int roll_low = candidate->info_.roll - max_delta_roll;
    const int roll_high = candidate->info_.roll + max_delta_roll;
    const int pad_low = candidate_data.pad_low - max_delta_pad;
    const int pad_high = candidate_data.pad_hi + max_delta_pad;

    if (roll_low <= info.roll && info.roll <= roll_high && data.pad_low <= pad_high && data.pad_hi >= pad_low) {
      entry.info_.flag_substitute = false;
      return true;
    }
  }

  return false;
}

bool TPFilter::acceptME0(const TPEntry& entry) const {
  return entry.tp_.getME0Data().partition <= 9;
}

bool TPFilter::acceptGE0(const TPEntry& entry) const {
  return entry.tp_.getME0Data().partition <= 9;
}
