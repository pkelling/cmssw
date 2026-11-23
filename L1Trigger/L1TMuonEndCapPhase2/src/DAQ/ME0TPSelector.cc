#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFConfiguration.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFConstants.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFContext.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/SubsystemTags.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/TPrimitives.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/Utils/DebugUtils.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/Utils/CSCUtils.h"

#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/ME0TPSelector.h"

using namespace emtf::phase2;

ME0TPSelector::ME0TPSelector(const EMTFContext& context)
    : context_(context) {}

void ME0TPSelector::select(const TPEntry& entry, SectorTPCMap& sector_tpc_map) const {
  emtf_assert(entry.tp_.subsystem() == L1TMuon::kME0);
  const auto& info = entry.info_;

  // Native Sector
  auto native_info = info;
  native_info.selection = TPSelection::kNative;
  native_info.ilink = calcInputLink(info.subsector, info.csc_id, TPSelection::kNative);
  sector_tpc_map[info.endcap][info.sector].emplace_back(entry.tp_, native_info);

  // Neighbor Sector
  const int neighbor_sector = (info.sector == kMaxTrigSector) ? kMinTrigSector : info.sector + 1;
  if (context_.config_.include_neighbor_en_ &&
      csc::isTPInNeighborSector(info.endcap, neighbor_sector, info.endcap, info.sector,
                               info.subsector, info.station, info.csc_id)) {
    auto neighbor_info = info;
    neighbor_info.selection = TPSelection::kNeighbor;
    neighbor_info.ilink = calcInputLink(info.subsector, info.csc_id, TPSelection::kNeighbor);
    sector_tpc_map[info.endcap][neighbor_sector].emplace_back(entry.tp_, neighbor_info);
  }
}

int ME0TPSelector::calcInputLink(const int& tp_subsector, const int& tp_csc_id, const TPSelection& tp_selection) const {
  int ilink = -1;

  // Links
  // ME0                  : 108..113
  // ME0 (N)              : 114

  if (tp_selection == TPSelection::kNative) {
    const int ilink_offset = 108;

    ilink = ilink_offset + ((tp_subsector - 1) * 3) + (tp_csc_id - 1);

    emtf_assert((108 <= ilink) && (ilink < 114));
  } else {
    const int ilink_offset = 114;

    ilink = ilink_offset;

    emtf_assert(ilink == ilink_offset);
  }

  return ilink;
}
