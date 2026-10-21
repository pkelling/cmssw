#include "FWCore/MessageLogger/interface/MessageLogger.h"

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
    : context_(context), endcap_(0), sector_(0) {}

void GE0TPSelector::select(const TPEntry& entry, SectorTPCMap& sector_tpc_map) const {
  emtf_assert(entry.tp_.subsystem() == L1TMuon::kME0);
  const auto& info = entry.info_;

  // Native Sector
  auto native_info = info;
  native_info.selection = TPSelection::kNative;
  native_info.ilink = calcInputLink(info.csc_id, TPSelection::kNative);
  sector_tpc_map[info.endcap][info.sector].emplace_back(entry.tp_, native_info);

  // Neighbor Sector
  const int neighbor_sector = (info.sector == kMaxTrigSector) ? kMinTrigSector : info.sector + 1;
  if (context_.config_.include_neighbor_en_ &&
      info.csc_id == 3) {
    auto neighbor_info = info;
    neighbor_info.selection = TPSelection::kNeighbor;
    neighbor_info.ilink = calcInputLink(info.csc_id, TPSelection::kNeighbor);
    sector_tpc_map[info.endcap][neighbor_sector].emplace_back(entry.tp_, neighbor_info);
  }
}

GE0TPSelector::GE0TPSelector(const EMTFContext& context, const int& endcap, const int& sector)
    : context_(context), endcap_(endcap), sector_(sector) {}

void GE0TPSelector::select(const TriggerPrimitive& tp, TPInfo tp_info, ILinkTPCMap& ilink_tpc_map) const {
  emtf_assert(tp.subsystem() == L1TMuon::kME0);

  // Map GE0 trigger primitives to input links
  int ilink = getInputLink(tp_info);  // Returns GE0 "link" index

  // Short-Circuit: Link not found (ilink = -1)
  if (ilink < 0) {
    return;
  }

  ilink_tpc_map[ilink].emplace_back(tp, tp_info);
}

// ===========================================================================
// Utils
// ===========================================================================
int GE0TPSelector::getInputLink(TPInfo& tp_info) const {
  int ilink = -1;

  // Unpack detector info
  const int tp_endcap = tp_info.endcap;
  const int tp_sector = tp_info.sector;
  const int tp_csc_id = tp_info.csc_id;

  // Find selection type
  auto tp_selection = TPSelection::kNone;

  if (csc::isTPInSector(endcap_, sector_, tp_endcap, tp_sector)) {
    tp_selection = TPSelection::kNative;
  } else if (this->context_.config_.include_neighbor_en_ && tp_endcap == endcap_ &&
             tp_sector == ((sector_ == 1) ? 6 : sector_ - 1) && tp_csc_id == 3) {
    tp_selection = TPSelection::kNeighbor;
  } else {  // Short-Circuit: tp_selection = TPSelection::kNone
    return ilink;
  }

  // Get chamber input link for this sector processor
  ilink = calcInputLink(tp_csc_id, tp_selection);

  // Add selection info
  tp_info.ilink = ilink;
  tp_info.selection = tp_selection;

  return ilink;
}

int GE0TPSelector::calcInputLink(const int& tp_csc_id, const TPSelection& tp_selection) const {
  // Three physical GE0 chambers per sector, plus one from the preceding sector.
  if (tp_selection == TPSelection::kNative) {
    const int ilink = 108 + tp_csc_id;
    emtf_assert((109 <= ilink) && (ilink <= 111));
    return ilink;
  }

  const int ilink = 112;
  emtf_assert(tp_selection == TPSelection::kNeighbor);
  return ilink;
}
