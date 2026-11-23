#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"

#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFConfiguration.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFConstants.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/EMTFContext.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/RPCTPCollector.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/SubsystemTags.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/DAQ/TPrimitives.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/Utils/DebugUtils.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/Utils/RPCUtils.h"

using namespace emtf::phase2;

RPCTPCollector::RPCTPCollector(const EMTFContext& context, edm::ConsumesCollector& i_consumes_collector)
    : context_(context),
      input_token_(i_consumes_collector.consumes<RPCTag::rechit_collection_type>(context.config_.rpc_input_)) {}

void RPCTPCollector::collect(const edm::Event& i_event, TPCollection& tpc) const {
  edm::Handle<RPCTag::rechit_collection_type> rpc_digis;
  i_event.getByToken(input_token_, rpc_digis);

  for (const auto& digi : *rpc_digis) {
    
    // Skip Barrel RPC (region = 0)
    if (digi.rpcId().region() == 0) {
      continue;
    }

    tpc.emplace_back(digi.rpcId(), digi);
    auto& info = tpc.back().info_;
    const auto& det_id = tpc.back().tp_.detId<RPCDetId>();
    const auto& data = tpc.back().tp_.getRPCData();

    const int region = det_id.region();
    info.bx = data.bx + context_.config_.rpc_bx_shift_;
    info.endcap = (region == -1) ? 2 : region;
    info.endcap_pm = (info.endcap == 2) ? -1 : info.endcap;
    info.station = det_id.station();
    info.ring = det_id.ring();
    info.roll = det_id.roll();
    info.layer = det_id.layer();
    info.rpc_type = (info.station >= 3 && info.ring == 1) ? rpc::Type::kiRPC : rpc::Type::kRPC;
    info.flag_substitute = (info.ring == 3);

    const int rpc_sector = det_id.sector();
    const int rpc_subsector = det_id.subsector();
    info.chamber = (info.rpc_type == rpc::Type::kiRPC) ? (rpc_sector - 1) * 3 + rpc_subsector
                                                      : (rpc_sector - 1) * 6 + rpc_subsector;
    info.sector = csc::getTriggerSector(info.station, info.ring, info.chamber);
    info.subsector = csc::getTriggerSubsector(info.station, info.chamber);
    info.csc_id = csc::getId(info.station, info.ring, info.chamber);
    info.csc_facing = csc::getFaceDirection(info.station, info.ring, info.chamber);

#ifdef EMTF_ALLOW_ASSERT
    const int strip = (data.strip_low + data.strip_hi) / 2;
    emtf_assert(kMinEndcap <= info.endcap && info.endcap <= kMaxEndcap);
    emtf_assert(kMinTrigSector <= info.sector && info.sector <= kMaxTrigSector);
    emtf_assert(0 <= info.subsector && info.subsector <= 2);
    emtf_assert(1 <= info.station && info.station <= 4);
    emtf_assert(1 <= info.chamber && info.chamber <= 36);
    emtf_assert(1 <= info.csc_id && info.csc_id <= 9);
    if (info.rpc_type == rpc::Type::kiRPC) {
      emtf_assert(info.ring == 1);
      emtf_assert(1 <= info.roll && info.roll <= 5);
      emtf_assert(1 <= strip && strip <= 96);
    } else {
      emtf_assert(2 <= info.ring && info.ring <= 3);
      emtf_assert(1 <= info.roll && info.roll <= 3);
      emtf_assert(data.isCPPF || (1 <= strip && strip <= 32));
    }
    emtf_assert(data.valid);
#endif
  }
}
