#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFConfiguration.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFConstants.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFContext.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/SubsystemTags.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/TPrimitives.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/Utils/DebugUtils.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/Utils/CSCUtils.h"

#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/GEMTPSelector.h"

using namespace emtf::phase2;

GEMTPSelector::GEMTPSelector(const EMTFContext& context)
    : context_(context) {}

void GEMTPSelector::select(const TPEntry& entry, SectorTPCMap& sector_tpc_map) const {
  emtf_assert(entry.tp_.subsystem() == L1TMuon::kGEM);
  const auto& info = entry.info_;

  // Native Sector
  auto native_info = info;
  native_info.selection = TPSelection::kNative;
  native_info.ilink = calcInputLink(info.subsector, info.station, info.ring, info.csc_id, TPSelection::kNative);
  sector_tpc_map[info.endcap][info.sector].emplace_back(entry.tp_, native_info);

  // Neighbor Sector
  const int neighbor_sector = (info.sector == kMaxTrigSector) ? kMinTrigSector : info.sector + 1;
  if (context_.config_.include_neighbor_en_ &&
      csc::isTPInNeighborSector(info.endcap, neighbor_sector, info.endcap, info.sector,
                               info.subsector, info.station, info.csc_id)) {
    auto neighbor_info = info;
    neighbor_info.selection = TPSelection::kNeighbor;
    neighbor_info.ilink = calcInputLink(info.subsector, info.station, info.ring, info.csc_id, TPSelection::kNeighbor);
    sector_tpc_map[info.endcap][neighbor_sector].emplace_back(entry.tp_, neighbor_info);
  }
}

int GEMTPSelector::calcInputLink(const int& tp_subsector,
                                 const int& tp_station,
                                 const int& tp_ring,
                                 const int& tp_csc_id,
                                 const TPSelection& tp_selection) const {
  int ilink = -1;

  // Links
  // RE1,2,3,4 + GE1,2        : 54..71, 72..80, 81..89, 90..98
  // RE1,2,3,4 + GE1,2 (N)    : 99..101, 102..103, 104..105, 106..107

  if (tp_selection == TPSelection::kNative) {
    const int ilink_offset = 54;

    if (tp_station == 1) {
      ilink = ilink_offset + (tp_subsector - 1) * 9 + (tp_csc_id - 1);
    } else {
      ilink = ilink_offset + tp_station * 9 + (tp_csc_id - 1);
    }

    emtf_assert((54 <= ilink) && (ilink < 99));
  } else {
    const int ilink_offset = 99;

    if (tp_station == 1) {
      ilink = ilink_offset + ((tp_station - 1) * 2) + ((tp_csc_id - 1) / 3);
    } else if (tp_ring == 1) {
      ilink = ilink_offset + ((tp_station - 1) * 2) + 1;
    } else {
      ilink = ilink_offset + ((tp_station - 1) * 2) + 2;
    }

    emtf_assert((99 <= ilink) && (ilink < 108));
  }

  return ilink;
}
