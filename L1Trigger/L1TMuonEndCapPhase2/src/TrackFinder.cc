#include <iostream>
#include <sstream>
#include <string>

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFContext.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFConfiguration.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFConstants.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFTypes.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/SectorProcessor.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/TPrimitives.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/TPCollectors.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/CSCTPCollector.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/RPCTPCollector.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/GEMTPCollector.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/ME0TPCollector.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/GE0TPCollector.h"

#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/CSCTPSelector.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/RPCTPSelector.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/GEMTPSelector.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/ME0TPSelector.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/GE0TPSelector.h"

#include "L1Trigger/L1TMuonEndCapPhase2/interface/TrackFinder.h"

using namespace emtf::phase2;

TrackFinder::TrackFinder(const edm::ParameterSet& i_config, edm::ConsumesCollector&& i_consumes_collector)
    : context_(i_config, i_consumes_collector), tp_collectors_(), sector_processors_() {
  // ===========================================================================
  // Emulation Setup
  // ===========================================================================

  // Register Trigger Primitives
  if (this->context_.config_.csc_en_) {
    tp_collectors_.push_back(std::make_unique<CSCTPCollector>(context_, i_consumes_collector));
    tp_selectors_[L1TMuon::kCSC] = std::make_unique<CSCTPSelector>(context_);
  }

  if (this->context_.config_.rpc_en_) {
    tp_collectors_.push_back(std::make_unique<RPCTPCollector>(context_, i_consumes_collector));
    tp_selectors_[L1TMuon::kRPC] = std::make_unique<RPCTPSelector>(context_);
  }

  if (this->context_.config_.gem_en_) {
    tp_collectors_.push_back(std::make_unique<GEMTPCollector>(context_, i_consumes_collector));
    tp_selectors_[L1TMuon::kGEM] = std::make_unique<GEMTPSelector>(context_);
  }

  if (this->context_.config_.me0_en_) {
    tp_collectors_.push_back(std::make_unique<ME0TPCollector>(context_, i_consumes_collector));
    tp_selectors_[L1TMuon::kME0] = std::make_unique<ME0TPSelector>(context_);
  }

  if (this->context_.config_.ge0_en_) {
    tp_collectors_.push_back(std::make_unique<GE0TPCollector>(context_, i_consumes_collector));
    tp_selectors_[L1TMuon::kME0] = std::make_unique<GE0TPSelector>(context_);
  }

  // Register Sector Processor
  for (unsigned int endcap = kMinEndcap; endcap <= kMaxEndcap; ++endcap) {
    for (unsigned int sector = kMinTrigSector; sector <= kMaxTrigSector; ++sector) {
      sector_processors_[endcap][sector] = std::make_unique<SectorProcessor>(context_, endcap, sector);
    }
  }
}

TrackFinder::~TrackFinder() {
  // Do Nothing
}

void TrackFinder::process(
    // Input
    const edm::Event& i_event,
    const edm::EventSetup& i_event_setup,
    // Output
    EMTFHitCollection& out_hits,
    EMTFTrackCollection& out_tracks,
    EMTFInputCollection& out_inputs) {

  // ===========================================================================
  // Clear output collections
  // ===========================================================================
  out_hits.clear();
  out_tracks.clear();
  out_inputs.clear();

  // ===========================================================================
  // Load the event configuration
  // ===========================================================================
  context_.update(i_event, i_event_setup);

  // ===========================================================================
  // Collect trigger primitives
  // ===========================================================================
  
  // Get all of the trigger primitives in the endcap
  TPCollection tpc;
  for (auto& tp_collector : tp_collectors_) {
    tp_collector->collect(i_event, tpc);
  }

  // Select all of the TPs for each sector - this duplicates neighbor TPs
  SectorTPCMap sector_tpc_map;
  for (const auto& entry : tpc) {
    const auto selector = tp_selectors_.find(entry.tp_.subsystem());
    if (selector != tp_selectors_.end()) {
      selector->second->select(entry, sector_tpc_map);
    }
  }

  // Group selected TPs by [endcap][sector][BX][input-link]
  // This immitates how data goes into SectorProcessor in Firmware
  std::map<int, std::map<int, BXILinkTPCMap>> sector_bx_ilink_tpc_maps;
  for (auto& [endcap, sectors] : sector_tpc_map) {
    for (auto& [sector, entries] : sectors) {
      for (auto& entry : entries) {
        sector_bx_ilink_tpc_maps[endcap][sector][entry.info_.bx][entry.info_.ilink].push_back(entry);
      }
    }
  }

  // Debug Info
  if (context_.config_.verbosity_ > 4) {
    printDebuggingTPs(tpc, sector_bx_ilink_tpc_maps);
  }

  // ===========================================================================
  // Run sector processors
  // ===========================================================================

  // Process each sectors complete BX/input-link collection.
  for (const auto& [endcap, sectors] : sector_bx_ilink_tpc_maps) {
    for (const auto& [sector, bx_ilink_tpc_map] : sectors) {
      if (bx_ilink_tpc_map.empty()) {
        continue;
      }

      auto& sector_processor = sector_processors_.at(endcap).at(sector);
      sector_processor->configureEvent(i_event);
      sector_processor->process(bx_ilink_tpc_map, out_hits, out_tracks, out_inputs);
    }
  }
}

void TrackFinder::printDebuggingTPs(
    const TPCollection& tpc,
    const std::map<int, std::map<int, BXILinkTPCMap>>& sector_bx_ilink_tpc_maps) const {

  for (const auto& entry : tpc) {
    entry.tp_.print(std::cout);
  }
  edm::LogInfo("L1TEMTFpp") << "Num of TriggerPrimitive: " << tpc.size();

  // Dump all receiving sectors, including those without trigger primitives.
  for (int endcap = kMinEndcap; endcap <= kMaxEndcap; ++endcap) {
    const auto endcap_it = sector_bx_ilink_tpc_maps.find(endcap);
    for (int sector = kMinTrigSector; sector <= kMaxTrigSector; ++sector) {
      std::cout << "========== Endcap " << endcap << " Sector " << sector << " ==========" << std::endl;
      if (endcap_it == sector_bx_ilink_tpc_maps.end()) {
        continue;
      }

      const auto sector_it = endcap_it->second.find(sector);
      if (sector_it == endcap_it->second.end()) {
        continue;
      }

      for (const auto& [bx, ilink_tpc_map] : sector_it->second) {
        for (const auto& [ilink, entries] : ilink_tpc_map) {
          std::cout << "=== BX " << bx << " Input link " << ilink << " ===" << std::endl;
          for (const auto& entry : entries) {
            entry.tp_.print(std::cout);
          }
        }
      }
    }
  }
}

void TrackFinder::onJobBegin() {
  // Do Nothing
}

void TrackFinder::onJobEnd() {
  // Do Nothing
}
