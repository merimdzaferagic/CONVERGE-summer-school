/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * This file is part of srsRAN.
 *
 * srsRAN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsRAN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#include "scheduler_result_logger.h"
#include "srsran/ran/csi_report/csi_report_formatters.h"
#include "srsran/scheduler/result/sched_result.h"
#include "srsran/srslog/srslog.h"
#include "srsran/support/format/fmt_to_c_str.h"

using namespace srsran;

scheduler_result_logger::scheduler_result_logger(bool log_broadcast_, pci_t pci_) :
  logger(srslog::fetch_basic_logger("SCHED")), log_broadcast(log_broadcast_), enabled(logger.info.enabled()), pci(pci_)
{
}

void scheduler_result_logger::log_debug(const sched_result& result, std::chrono::microseconds decision_latency)
{
  if (log_broadcast) {
    for (const ssb_information& ssb_info : result.dl.bc.ssb_info) {
      fmt::format_to(std::back_inserter(fmtbuf),
                     "\n- SSB: ssbIdx={}, crbs={}, symb={}",
                     ssb_info.ssb_index,
                     ssb_info.crbs,
                     ssb_info.symbols);
    }
  }
  for (const pdcch_dl_information& pdcch : result.dl.dl_pdcchs) {
    if (not log_broadcast and pdcch.ctx.rnti == rnti_t::SI_RNTI) {
      continue;
    }
    fmt::format_to(std::back_inserter(fmtbuf),
                   "\n- DL PDCCH: rnti={} type={} cs_id={} ss_id={} format={} cce={} al={}",
                   pdcch.ctx.rnti,
                   dci_dl_rnti_config_rnti_type(pdcch.dci.type),
                   fmt::underlying(pdcch.ctx.coreset_cfg->id),
                   fmt::underlying(pdcch.ctx.context.ss_id),
                   dci_dl_rnti_config_format(pdcch.dci.type),
                   pdcch.ctx.cces.ncce,
                   to_nof_cces(pdcch.ctx.cces.aggr_lvl));
    switch (pdcch.dci.type) {
      case dci_dl_rnti_config_type::c_rnti_f1_0: {
        const auto& dci = pdcch.dci.c_rnti_f1_0;
        fmt::format_to(std::back_inserter(fmtbuf),
                       " dci: h_id={} ndi={} rv={} mcs={} res_ind={} tpc={}",
                       dci.harq_process_number,
                       dci.new_data_indicator,
                       dci.redundancy_version,
                       dci.modulation_coding_scheme,
                       dci.pucch_resource_indicator,
                       dci.tpc_command);
      } break;
      case dci_dl_rnti_config_type::tc_rnti_f1_0: {
        const auto& dci = pdcch.dci.tc_rnti_f1_0;
        fmt::format_to(std::back_inserter(fmtbuf),
                       " dci: h_id={} ndi={} rv={} mcs={} res_ind={}",
                       dci.harq_process_number,
                       dci.new_data_indicator,
                       dci.redundancy_version,
                       dci.modulation_coding_scheme,
                       dci.pucch_resource_indicator);
      } break;
      case dci_dl_rnti_config_type::c_rnti_f1_1: {
        const auto& dci = pdcch.dci.c_rnti_f1_1;
        fmt::format_to(std::back_inserter(fmtbuf),
                       " dci: h_id={} ndi={} rv={} mcs={} res_ind={} tpc={}",
                       dci.harq_process_number,
                       dci.tb1_new_data_indicator,
                       dci.tb1_redundancy_version,
                       dci.tb1_modulation_coding_scheme,
                       dci.pucch_resource_indicator,
                       dci.tpc_command);
        if (dci.downlink_assignment_index.has_value()) {
          fmt::format_to(std::back_inserter(fmtbuf), " dai={}", *dci.downlink_assignment_index);
        }
      } break;
      default:
        break;
    }
  }
  for (const pdcch_ul_information& pdcch : result.dl.ul_pdcchs) {
    fmt::format_to(std::back_inserter(fmtbuf),
                   "\n- UL PDCCH: rnti={} type={} cs_id={} ss_id={} format={} cce={} al={}",
                   pdcch.ctx.rnti,
                   dci_ul_rnti_config_rnti_type(pdcch.dci.type),
                   fmt::underlying(pdcch.ctx.coreset_cfg->id),
                   fmt::underlying(pdcch.ctx.context.ss_id),
                   dci_ul_rnti_config_format(pdcch.dci.type),
                   pdcch.ctx.cces.ncce,
                   to_nof_cces(pdcch.ctx.cces.aggr_lvl));
    switch (pdcch.dci.type) {
      case dci_ul_rnti_config_type::c_rnti_f0_0: {
        const auto& dci = pdcch.dci.c_rnti_f0_0;
        fmt::format_to(std::back_inserter(fmtbuf),
                       " h_id={} ndi={} rv={} mcs={}",
                       dci.harq_process_number,
                       dci.new_data_indicator ? 1 : 0,
                       dci.redundancy_version,
                       dci.modulation_coding_scheme);
      } break;
      case dci_ul_rnti_config_type::tc_rnti_f0_0: {
        const auto& dci = pdcch.dci.tc_rnti_f0_0;
        fmt::format_to(std::back_inserter(fmtbuf),
                       "h_id=0 ndi=1 rv={} mcs={} tpc={}",
                       dci.redundancy_version,
                       dci.modulation_coding_scheme,
                       dci.tpc_command);
      } break;
      case dci_ul_rnti_config_type::c_rnti_f0_1: {
        const auto& dci = pdcch.dci.c_rnti_f0_1;
        fmt::format_to(std::back_inserter(fmtbuf),
                       " h_id={} ndi={} rv={} mcs={} dai={} tpc={} mimo={} ant={}",
                       dci.harq_process_number,
                       dci.new_data_indicator ? 1 : 0,
                       dci.redundancy_version,
                       dci.modulation_coding_scheme,
                       dci.first_dl_assignment_index,
                       dci.tpc_command,
                       dci.precoding_info_nof_layers,
                       dci.antenna_ports);
      } break;
      default:
        break;
    }
  }

  if (log_broadcast) {
    for (const csi_rs_info& csi_rs : result.dl.csi_rs) {
      fmt::format_to(std::back_inserter(fmtbuf),
                     "\n- CSI-RS: type={} crbs={} row={} freq={} symb0={} cdm_type={} freq_density={}",
                     csi_rs.type == csi_rs_type::CSI_RS_NZP ? "nzp" : "zp",
                     csi_rs.crbs,
                     csi_rs.row,
                     csi_rs.freq_domain,
                     csi_rs.symbol0,
                     to_string(csi_rs.cdm_type),
                     to_string(csi_rs.freq_density));
      if (csi_rs.type == csi_rs_type::CSI_RS_NZP) {
        fmt::format_to(std::back_inserter(fmtbuf), " scramb_id={}", csi_rs.scrambling_id);
      }
    }

    for (const sib_information& sib : result.dl.bc.sibs) {
      fmt::format_to(std::back_inserter(fmtbuf),
                     "\n- SI{} PDSCH: rb={} symb={} tbs={} mcs={} rv={}",
                     sib.si_indicator == sib_information::sib1 ? "B1" : "",
                     sib.pdsch_cfg.rbs,
                     sib.pdsch_cfg.symbols,
                     sib.pdsch_cfg.codewords[0].tb_size_bytes,
                     sib.pdsch_cfg.codewords[0].mcs_index,
                     sib.pdsch_cfg.codewords[0].rv_index);
    }
  }

  for (const rar_information& rar : result.dl.rar_grants) {
    fmt::format_to(std::back_inserter(fmtbuf),
                   "\n- RAR PDSCH: ra-rnti={} rb={} symb={} tbs={} mcs={} rv={} grants ({}): ",
                   rar.pdsch_cfg.rnti,
                   rar.pdsch_cfg.rbs,
                   rar.pdsch_cfg.symbols,
                   rar.pdsch_cfg.codewords[0].tb_size_bytes,
                   rar.pdsch_cfg.codewords[0].mcs_index,
                   rar.pdsch_cfg.codewords[0].rv_index,
                   rar.grants.size());
    for (const rar_ul_grant& grant : rar.grants) {
      fmt::format_to(std::back_inserter(fmtbuf),
                     "{}tc-rnti={}: rapid={} ta={} time_res={}",
                     (&grant == &rar.grants.front()) ? "" : ", ",
                     grant.temp_crnti,
                     grant.rapid,
                     grant.ta,
                     grant.time_resource_assignment);
    }
  }
  for (const dl_msg_alloc& ue_dl_grant : result.dl.ue_grants) {
    fmt::format_to(std::back_inserter(fmtbuf),
                   "\n- UE PDSCH: ue={} c-rnti={} h_id={} rb={} symb={} tbs={} mcs={} rv={} nrtx={} k1={}",
                   fmt::underlying(ue_dl_grant.context.ue_index),
                   ue_dl_grant.pdsch_cfg.rnti,
                   fmt::underlying(ue_dl_grant.pdsch_cfg.harq_id),
                   ue_dl_grant.pdsch_cfg.rbs,
                   ue_dl_grant.pdsch_cfg.symbols,
                   ue_dl_grant.pdsch_cfg.codewords[0].tb_size_bytes,
                   ue_dl_grant.pdsch_cfg.codewords[0].mcs_index,
                   ue_dl_grant.pdsch_cfg.codewords[0].rv_index,
                   ue_dl_grant.context.nof_retxs,
                   ue_dl_grant.context.k1);
    if (ue_dl_grant.pdsch_cfg.precoding.has_value() and not ue_dl_grant.pdsch_cfg.precoding.value().prg_infos.empty()) {
      const auto& prg_type = ue_dl_grant.pdsch_cfg.precoding->prg_infos[0].type;
      fmt::format_to(
          std::back_inserter(fmtbuf), " ri={} {}", ue_dl_grant.pdsch_cfg.nof_layers, csi_report_pmi{prg_type});
    }
    if (ue_dl_grant.pdsch_cfg.codewords[0].new_data) {
      fmt::format_to(std::back_inserter(fmtbuf), " dl_bo={}", ue_dl_grant.context.buffer_occupancy);
    }
    if (ue_dl_grant.context.olla_offset.has_value()) {
      fmt::format_to(std::back_inserter(fmtbuf), " olla={:.3}", ue_dl_grant.context.olla_offset.value());
    }
    for (const dl_msg_lc_info& lc : ue_dl_grant.tb_list[0].lc_chs_to_sched) {
      fmt::format_to(std::back_inserter(fmtbuf),
                     "{}lcid={}: size={}",
                     (&lc == &ue_dl_grant.tb_list[0].lc_chs_to_sched.front()) ? " grants: " : ", ",
                     lc.lcid,
                     lc.sched_bytes);
    }
  }
  for (const dl_paging_allocation& pg : result.dl.paging_grants) {
    fmt::format_to(std::back_inserter(fmtbuf),
                   "\n- PCCH: rb={} symb={} tbs={} mcs={} rv={}",
                   pg.pdsch_cfg.rbs,
                   pg.pdsch_cfg.symbols,
                   pg.pdsch_cfg.codewords[0].tb_size_bytes,
                   pg.pdsch_cfg.codewords[0].mcs_index,
                   pg.pdsch_cfg.codewords[0].rv_index);

    for (const paging_ue_info& ue : pg.paging_ue_list) {
      fmt::format_to(std::back_inserter(fmtbuf),
                     "{}{}-pg-id={:#x}",
                     (&ue == &pg.paging_ue_list.front()) ? " ues: " : ", ",
                     ue.paging_type_indicator == paging_ue_info::paging_identity_type::cn_ue_paging_identity ? "cn"
                                                                                                             : "ran",
                     ue.paging_identity);
    }
  }

  for (const ul_sched_info& ul_info : result.ul.puschs) {
    fmt::format_to(std::back_inserter(fmtbuf),
                   "\n- UE PUSCH: ue={} {}c-rnti={} h_id={} rb={} symb={} tbs={} rv={} nrtx={} nof_layers={}",
                   fmt::underlying(ul_info.context.ue_index),
                   ul_info.context.ue_index == INVALID_DU_UE_INDEX ? "t" : "",
                   ul_info.pusch_cfg.rnti,
                   fmt::underlying(ul_info.pusch_cfg.harq_id),
                   ul_info.pusch_cfg.rbs,
                   ul_info.pusch_cfg.symbols,
                   ul_info.pusch_cfg.tb_size_bytes,
                   ul_info.pusch_cfg.rv_index,
                   ul_info.context.nof_retxs,
                   ul_info.pusch_cfg.nof_layers);
    if (ul_info.context.olla_offset.has_value()) {
      fmt::format_to(std::back_inserter(fmtbuf), " olla={:.3}", ul_info.context.olla_offset.value());
    }
    if (ul_info.context.ue_index == INVALID_DU_UE_INDEX and ul_info.context.nof_retxs == 0 and
        ul_info.context.msg3_delay.has_value()) {
      fmt::format_to(std::back_inserter(fmtbuf), " msg3_delay={}", ul_info.context.msg3_delay.value());
    } else {
      fmt::format_to(std::back_inserter(fmtbuf), " k2={}", ul_info.context.k2);
    }

    if (ul_info.uci.has_value()) {
      fmt::format_to(
          std::back_inserter(fmtbuf),
          " uci: harq_bits={} csi-1_bits={} csi-2_present={}",
          ul_info.uci.value().harq.has_value() ? ul_info.uci.value().harq.value().harq_ack_nof_bits : 0U,
          ul_info.uci.value().csi.has_value() ? ul_info.uci.value().csi.value().csi_part1_nof_bits : 0U,
          ul_info.uci.value().csi.has_value() && ul_info.uci.value().csi.value().beta_offset_csi_2.has_value() ? "Yes"
                                                                                                               : "No");
    }
  }

  for (const pucch_info& pucch : result.ul.pucchs) {
    switch (pucch.format()) {
      case pucch_format::FORMAT_0: {
        format_to(
            std::back_inserter(fmtbuf), "\n- PUCCH: c-rnti={} format=0 prb={}", pucch.crnti, pucch.resources.prbs);
        if (not pucch.resources.second_hop_prbs.empty()) {
          format_to(std::back_inserter(fmtbuf), " second_prbs={}", pucch.resources.second_hop_prbs);
        }
        format_to(std::back_inserter(fmtbuf),
                  " symb={} uci: harq_bits={} sr={}",
                  pucch.resources.symbols,
                  pucch.uci_bits.harq_ack_nof_bits,
                  fmt::underlying(pucch.uci_bits.sr_bits));
      } break;
      case pucch_format::FORMAT_1: {
        const auto& format_1 = std::get<pucch_format_1>(pucch.format_params);
        format_to(
            std::back_inserter(fmtbuf), "\n- PUCCH: c-rnti={} format=1 prb={}", pucch.crnti, pucch.resources.prbs);
        if (not pucch.resources.second_hop_prbs.empty()) {
          format_to(std::back_inserter(fmtbuf), " second_prbs={}", pucch.resources.second_hop_prbs);
        }
        format_to(std::back_inserter(fmtbuf),
                  " symb={} cs={} occ={} uci: harq_bits={} sr={}",
                  pucch.resources.symbols,
                  format_1.initial_cyclic_shift,
                  format_1.time_domain_occ,
                  pucch.uci_bits.harq_ack_nof_bits,
                  fmt::underlying(pucch.uci_bits.sr_bits));

      } break;
      case pucch_format::FORMAT_2: {
        format_to(
            std::back_inserter(fmtbuf), "\n- PUCCH: c-rnti={} format=2 prb={}", pucch.crnti, pucch.resources.prbs);
        if (not pucch.resources.second_hop_prbs.empty()) {
          format_to(std::back_inserter(fmtbuf), " second_prbs={}", pucch.resources.second_hop_prbs);
        }
        format_to(std::back_inserter(fmtbuf),
                  " symb={} uci: harq_bits={} sr={} csi-1_bits={}",
                  pucch.resources.symbols,
                  pucch.uci_bits.harq_ack_nof_bits,
                  fmt::underlying(pucch.uci_bits.sr_bits),
                  pucch.uci_bits.csi_part1_nof_bits);

      } break;
      case pucch_format::FORMAT_3: {
        format_to(
            std::back_inserter(fmtbuf), "\n- PUCCH: c-rnti={} format=3 prb={}", pucch.crnti, pucch.resources.prbs);
        if (not pucch.resources.second_hop_prbs.empty()) {
          format_to(std::back_inserter(fmtbuf), " second_prbs={}", pucch.resources.second_hop_prbs);
        }
        format_to(std::back_inserter(fmtbuf),
                  " symb={} uci: harq_bits={} sr={} csi-1_bits={}",
                  pucch.resources.symbols,
                  pucch.uci_bits.harq_ack_nof_bits,
                  fmt::underlying(pucch.uci_bits.sr_bits),
                  pucch.uci_bits.csi_part1_nof_bits);
      } break;
      case pucch_format::FORMAT_4: {
        const auto& format_4 = std::get<pucch_format_4>(pucch.format_params);
        format_to(
            std::back_inserter(fmtbuf), "\n- PUCCH: c-rnti={} format=4 prb={}", pucch.crnti, pucch.resources.prbs);
        if (not pucch.resources.second_hop_prbs.empty()) {
          format_to(std::back_inserter(fmtbuf), " second_prbs={}", pucch.resources.second_hop_prbs);
        }
        format_to(std::back_inserter(fmtbuf),
                  " symb={} occ={}/{} uci: harq_bits={} sr={} csi-1_bits={}",
                  pucch.resources.symbols,
                  format_4.orthog_seq_idx,
                  fmt::underlying(format_4.n_sf_pucch_f4),
                  pucch.uci_bits.harq_ack_nof_bits,
                  fmt::underlying(pucch.uci_bits.sr_bits),
                  pucch.uci_bits.csi_part1_nof_bits);
      } break;
      default:
        srsran_assertion_failure("Invalid PUCCH format");
    }
  }

  for (const auto& srs : result.ul.srss) {
    fmt::format_to(std::back_inserter(fmtbuf),
                   "\n- SRS: c-rnti={} symb={} tx-comb=(n{} o={} cs={}) c_srs={} f_sh={} seq_id={}",
                   srs.crnti,
                   srs.symbols,
                   static_cast<unsigned>(srs.tx_comb),
                   srs.comb_offset,
                   srs.cyclic_shift,
                   srs.config_index,
                   srs.freq_shift,
                   srs.sequence_id);
  }

  if (log_broadcast) {
    for (const prach_occasion_info& prach : result.ul.prachs) {
      fmt::format_to(std::back_inserter(fmtbuf),
                     "\n- PRACH: pci={} format={} nof_occasions={} nof_preambles={}",
                     prach.pci,
                     to_string(prach.format),
                     prach.nof_prach_occasions,
                     prach.nof_preamble_indexes);
    }
  }

  if (fmtbuf.size() > 0) {
    const unsigned nof_pdschs = result.dl.paging_grants.size() + result.dl.rar_grants.size() +
                                result.dl.ue_grants.size() + result.dl.bc.sibs.size();
    const unsigned nof_puschs       = result.ul.puschs.size();
    const unsigned nof_failed_pdcch = result.failed_attempts.pdcch;
    const unsigned nof_failed_uci   = result.failed_attempts.uci;
    logger.debug("Slot decisions pci={} t={}us ({} PDSCH{}, {} PUSCH{}, {} attempted PDCCH{}, {} attempted UCI{}):{}",
                 pci,
                 decision_latency.count(),
                 nof_pdschs,
                 nof_pdschs == 1 ? "" : "s",
                 nof_puschs,
                 nof_puschs == 1 ? "" : "s",
                 nof_failed_pdcch,
                 nof_failed_pdcch == 1 ? "" : "s",
                 nof_failed_uci,
                 nof_failed_uci == 1 ? "" : "s",
                 to_c_str(fmtbuf));
    fmtbuf.clear();
  }
}

void scheduler_result_logger::log_info(const sched_result& result, std::chrono::microseconds decision_latency)
{
  if (log_broadcast) {
    for (const sib_information& sib_info : result.dl.bc.sibs) {
      fmt::format_to(std::back_inserter(fmtbuf),
                     "{}SI{}: rb={} tbs={}",
                     fmtbuf.size() == 0 ? "" : ", ",
                     sib_info.si_indicator == sib_information::sib1 ? "B1" : "",
                     sib_info.pdsch_cfg.rbs,
                     sib_info.pdsch_cfg.codewords[0].tb_size_bytes);
    }
  }
  for (const rar_information& rar_info : result.dl.rar_grants) {
    fmt::format_to(std::back_inserter(fmtbuf),
                   "{}RAR: ra-rnti={} rb={} tbs={}",
                   fmtbuf.size() == 0 ? "" : ", ",
                   rar_info.pdsch_cfg.rnti,
                   rar_info.pdsch_cfg.rbs,
                   rar_info.pdsch_cfg.codewords[0].tb_size_bytes);
  }
  for (const dl_msg_alloc& ue_msg : result.dl.ue_grants) {
    fmt::format_to(std::back_inserter(fmtbuf),
                   "{}DL: ue={} c-rnti={} h_id={} ss_id={} rb={} k1={} newtx={} rv={} tbs={}",
                   fmtbuf.size() == 0 ? "" : ", ",
                   fmt::underlying(ue_msg.context.ue_index),
                   ue_msg.pdsch_cfg.rnti,
                   fmt::underlying(ue_msg.pdsch_cfg.harq_id),
                   fmt::underlying(ue_msg.context.ss_id),
                   ue_msg.pdsch_cfg.rbs,
                   ue_msg.context.k1,
                   ue_msg.pdsch_cfg.codewords[0].new_data,
                   ue_msg.pdsch_cfg.codewords[0].rv_index,
                   ue_msg.pdsch_cfg.codewords[0].tb_size_bytes);
    if (ue_msg.pdsch_cfg.codewords[0].new_data) {
      fmt::format_to(
          std::back_inserter(fmtbuf), " ri={} dl_bo={}", ue_msg.pdsch_cfg.nof_layers, ue_msg.context.buffer_occupancy);
    }
  }
  for (const ul_sched_info& ue_msg : result.ul.puschs) {
    fmt::format_to(std::back_inserter(fmtbuf),
                   "{}UL: ue={} c-rnti={} h_id={} ss_id={} rb={} newtx={} rv={} tbs={} ",
                   fmtbuf.size() == 0 ? "" : ", ",
                   fmt::underlying(ue_msg.context.ue_index),
                   ue_msg.pusch_cfg.rnti,
                   fmt::underlying(ue_msg.pusch_cfg.harq_id),
                   fmt::underlying(ue_msg.context.ss_id),
                   ue_msg.pusch_cfg.rbs,
                   ue_msg.pusch_cfg.new_data,
                   ue_msg.pusch_cfg.rv_index,
                   ue_msg.pusch_cfg.tb_size_bytes);
    if (ue_msg.context.ue_index == INVALID_DU_UE_INDEX and ue_msg.context.nof_retxs == 0 and
        ue_msg.context.msg3_delay.has_value()) {
      fmt::format_to(std::back_inserter(fmtbuf), "msg3_delay={}", ue_msg.context.msg3_delay.value());
    } else {
      fmt::format_to(std::back_inserter(fmtbuf), "k2={}", ue_msg.context.k2);
    }
  }
  for (const dl_paging_allocation& pg_info : result.dl.paging_grants) {
    fmt::format_to(std::back_inserter(fmtbuf),
                   "{}PG: rb={} tbs={}",
                   fmtbuf.size() == 0 ? "" : ", ",
                   pg_info.pdsch_cfg.rbs,
                   pg_info.pdsch_cfg.codewords[0].tb_size_bytes);
    for (const paging_ue_info& ue : pg_info.paging_ue_list) {
      fmt::format_to(std::back_inserter(fmtbuf),
                     "{}{}-pg-id={:#x}",
                     (&ue == &pg_info.paging_ue_list.front()) ? " ues: " : ", ",
                     ue.paging_type_indicator == paging_ue_info::paging_identity_type::cn_ue_paging_identity ? "cn"
                                                                                                             : "ran",
                     ue.paging_identity);
    }
  }

  if (fmtbuf.size() > 0) {
    const unsigned nof_pdschs = result.dl.paging_grants.size() + result.dl.rar_grants.size() +
                                result.dl.ue_grants.size() + result.dl.bc.sibs.size();
    const unsigned nof_puschs = result.ul.puschs.size();
    logger.info("Slot decisions pci={} t={}us ({} PDSCH{}, {} PUSCH{}): {}",
                pci,
                decision_latency.count(),
                nof_pdschs,
                nof_pdschs == 1 ? "" : "s",
                nof_puschs,
                nof_puschs == 1 ? "" : "s",
                to_c_str(fmtbuf));
    fmtbuf.clear();
  }
}

void scheduler_result_logger::on_scheduler_result(const sched_result&       result,
                                                  std::chrono::microseconds decision_latency)
{
  if (not enabled) {
    return;
  }
  if (logger.debug.enabled()) {
    log_debug(result, decision_latency);
  } else {
    log_info(result, decision_latency);
  }
}
