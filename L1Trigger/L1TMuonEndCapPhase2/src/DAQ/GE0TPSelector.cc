#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFConfiguration.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFConstants.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFContext.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/SubsystemTags.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/TPrimitives.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/Utils/DebugUtils.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/Utils/CSCUtils.h"

#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/GE0TPSelector.h"

using namespace emtf::phase2;

GE0TPSelector::GE0TPSelector(const EMTFContext& context)
    : context_(context) {}

void GE0TPSelector::select(const TPEntry& entry, SectorTPCMap& sector_tpc_map) const {
  emtf_assert(entry.tp_.subsystem() == L1TMuon::kME0);
  const auto& info = entry.info_;

  // Native Sector
  auto native_info = info;
  native_info.selection = TPSelection::kNative;
  native_info.emtf_chamber = calcEMTFChamber(info.csc_id, TPSelection::kNative);
  sector_tpc_map[info.endcap][info.sector].emplace_back(entry.tp_, native_info);

  // Neighbor Sector
  const int neighbor_sector = (info.sector == kMaxTrigSector) ? kMinTrigSector : info.sector + 1;
  if (context_.config_.include_neighbor_en_ &&
      info.csc_id == 3) {
    auto neighbor_info = info;
    neighbor_info.selection = TPSelection::kNeighbor;
    neighbor_info.emtf_chamber = calcEMTFChamber(info.csc_id, TPSelection::kNeighbor);
    sector_tpc_map[info.endcap][neighbor_sector].emplace_back(entry.tp_, neighbor_info);
  }
}

int GE0TPSelector::calcEMTFChamber(const int& tp_csc_id, const TPSelection& tp_selection) const {
  // Three GE0 chambers per sector, plus one from the preceding sector.
  if (tp_selection == TPSelection::kNative) {
    const int emtf_chamber = 108 + tp_csc_id;
    emtf_assert((109 <= emtf_chamber) && (emtf_chamber <= 111));
    return emtf_chamber;
  }

  const int emtf_chamber = 112;
  emtf_assert(tp_selection == TPSelection::kNeighbor);
  return emtf_chamber;
}
