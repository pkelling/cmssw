#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFConfiguration.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFConstants.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFContext.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/SubsystemTags.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/TPrimitives.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/Utils/DebugUtils.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/Utils/CSCUtils.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/Utils/RPCUtils.h"

#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/RPCTPSelector.h"

using namespace emtf::phase2;

RPCTPSelector::RPCTPSelector(const EMTFContext& context)
    : context_(context) {}

void RPCTPSelector::select(const TPEntry& entry, SectorTPCMap& sector_tpc_map) const {
  emtf_assert(entry.tp_.subsystem() == L1TMuon::kRPC);
  const auto& info = entry.info_;

  // Native Sector
  // CPPF supplies separate digis for each receiving board.
  const auto& data = entry.tp_.getRPCData();
  if (!data.isCPPF || data.emtf_sector == info.sector) {
    auto native_info = info;
    native_info.selection = TPSelection::kNative;
    native_info.emtf_chamber = calcEMTFChamber(info.subsector, info.station, info.ring, info.csc_id, TPSelection::kNative);
    sector_tpc_map[info.endcap][info.sector].emplace_back(entry.tp_, native_info);
  }

  // Neighbor Sector
  const int neighbor_sector = (info.sector == kMaxTrigSector) ? kMinTrigSector : info.sector + 1;
  if (context_.config_.include_neighbor_en_ &&
      csc::isTPInNeighborSector(info.endcap, neighbor_sector, info.endcap, info.sector,
                               info.subsector, info.station, info.csc_id) &&
      (!data.isCPPF || data.emtf_sector == neighbor_sector)) {
    auto neighbor_info = info;
    neighbor_info.selection = TPSelection::kNeighbor;
    neighbor_info.emtf_chamber = calcEMTFChamber(info.subsector, info.station, info.ring, info.csc_id, TPSelection::kNeighbor);
    sector_tpc_map[info.endcap][neighbor_sector].emplace_back(entry.tp_, neighbor_info);
  }
}

int RPCTPSelector::calcEMTFChamber(const int& tp_subsector,
                                 const int& tp_station,
                                 const int& tp_ring,
                                 const int& tp_csc_id,
                                 const TPSelection& tp_selection) const {
  int emtf_chamber = -1;

  // EMTF chamber indices
  // RE1,2,3,4 + GE1,2        : 54..71, 72..80, 81..89, 90..98
  // RE1,2,3,4 + GE1,2 (N)    : 99..101, 102..103, 104..105, 106..107

  if (tp_selection == TPSelection::kNative) {
    const int emtf_chamber_offset = 54;

    if (tp_station == 1) {
      emtf_chamber = emtf_chamber_offset + (tp_subsector - 1) * 9 + (tp_csc_id - 1);
    } else {
      emtf_chamber = emtf_chamber_offset + tp_station * 9 + (tp_csc_id - 1);
    }

    emtf_assert((54 <= emtf_chamber) && (emtf_chamber < 99));
  } else {
    const int emtf_chamber_offset = 99;

    if (tp_station == 1) {
      emtf_chamber = emtf_chamber_offset + ((tp_station - 1) * 2) + ((tp_csc_id - 1) / 3);
    } else if (tp_ring == 1) {
      emtf_chamber = emtf_chamber_offset + ((tp_station - 1) * 2) + 1;
    } else {
      emtf_chamber = emtf_chamber_offset + ((tp_station - 1) * 2) + 2;
    }

    emtf_assert((99 <= emtf_chamber) && (emtf_chamber < 108));
  }

  return emtf_chamber;
}
