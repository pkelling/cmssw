#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFConfiguration.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFConstants.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFContext.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/SubsystemTags.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/TPrimitives.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/Utils/DebugUtils.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/Utils/CSCUtils.h"

#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/CSCTPSelector.h"

using namespace emtf::phase2;

CSCTPSelector::CSCTPSelector(const EMTFContext& context)
    : context_(context) {}

void CSCTPSelector::select(const TPEntry& entry, SectorTPCMap& sector_tpc_map) const {
  emtf_assert(entry.tp_.subsystem() == L1TMuon::kCSC);
  const auto& info = entry.info_;

  // Native Sector
  auto native_info = info;
  native_info.selection = TPSelection::kNative;
  native_info.emtf_chamber = calcEMTFChamber(info.subsector, info.station, info.ring, info.csc_id, TPSelection::kNative);
  sector_tpc_map[info.endcap][info.sector].emplace_back(entry.tp_, native_info);

  // Neighbor Sector
  const int neighbor_sector = (info.sector == kMaxTrigSector) ? kMinTrigSector : info.sector + 1;
  if (context_.config_.include_neighbor_en_ &&
      csc::isTPInNeighborSector(info.endcap, neighbor_sector, info.endcap, info.sector,
                               info.subsector, info.station, info.csc_id)) {
    auto neighbor_info = info;
    neighbor_info.selection = TPSelection::kNeighbor;
    neighbor_info.emtf_chamber = calcEMTFChamber(info.subsector, info.station, info.ring, info.csc_id, TPSelection::kNeighbor);
    sector_tpc_map[info.endcap][neighbor_sector].emplace_back(entry.tp_, neighbor_info);
  }
}

// Returns the sector-relative EMTF chamber.
int CSCTPSelector::calcEMTFChamber(const int& tp_subsector,
                                 const int& tp_station,
                                 const int& tp_ring,
                                 const int& tp_csc_id,
                                 const TPSelection& tp_selection) const {
  int emtf_chamber = -1;

  // EMTF chamber indices - based on CSC TMB numbering
  // ME1,2,3,4        : 0..17,  18..26, 27..35, 36..44
  // ME1,2,3,4 (N)    : 45..47, 48..49, 50..51, 52..53


  if (tp_selection == TPSelection::kNative) {
    const int emtf_chamber_offset = 0;

    if (tp_station == 1) {
      emtf_chamber = emtf_chamber_offset + (tp_subsector - 1) * 9 + (tp_csc_id - 1);
    } else {
      emtf_chamber = emtf_chamber_offset + tp_station * 9 + (tp_csc_id - 1);
    }

    emtf_assert((0 <= emtf_chamber) && (emtf_chamber < 45));
  } else {
    const int emtf_chamber_offset = 45;

    if (tp_station == 1) {
      emtf_chamber = emtf_chamber_offset + ((tp_station - 1) * 2) + (tp_csc_id - 1) / 3;
    } else if (tp_ring == 1) {
      emtf_chamber = emtf_chamber_offset + ((tp_station - 1) * 2) + 1;
    } else {
      emtf_chamber = emtf_chamber_offset + ((tp_station - 1) * 2) + 2;
    }

    emtf_assert((45 <= emtf_chamber) && (emtf_chamber < 54));
  }

  return emtf_chamber;
}
