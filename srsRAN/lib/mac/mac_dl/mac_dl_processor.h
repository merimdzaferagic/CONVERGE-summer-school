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

#pragma once

#include "../mac_config_interfaces.h"
#include "mac_cell_processor.h"
#include "mac_dl_metric_handler.h"
#include "mac_dl_ue_repository.h"
#include "mac_scheduler_cell_info_handler.h"
#include "srsran/mac/mac.h"
#include "srsran/mac/mac_cell_result.h"
#include "srsran/mac/mac_config.h"

namespace srsran {

class mac_metrics_notifier;

struct mac_dl_config {
  srs_du::du_high_ue_executor_mapper&   ue_exec_mapper;
  srs_du::du_high_cell_executor_mapper& cell_exec_mapper;
  task_executor&                        ctrl_exec;
  mac_result_notifier&                  phy_notifier;
  mac_pcap&                             pcap;
  timer_manager&                        timers;
  std::chrono::milliseconds             metrics_report_period;
  mac_metrics_notifier&                 metrics_notifier;
};

class mac_dl_processor final : public mac_dl_configurator
{
public:
  explicit mac_dl_processor(const mac_dl_config&             mac_cfg,
                            mac_scheduler_cell_info_handler& sched_,
                            du_rnti_table&                   rnti_table_);

  bool has_cell(du_cell_index_t cell_index) const;

  /// Adds new cell configuration to MAC DL.
  mac_cell_controller& add_cell(const mac_cell_creation_request& cell_cfg) override;

  /// Removes cell configuration from MAC DL.
  void remove_cell(du_cell_index_t cell_index) override;

  mac_cell_controller& get_cell_controller(du_cell_index_t cell_index) override { return *cells[cell_index]; }

  mac_cell_time_mapper& get_time_mapper(du_cell_index_t cell_index) override
  {
    return cells[cell_index]->get_time_mapper();
  }

  /// Creates new UE DL context, updates logical channel MUX, adds UE in scheduler.
  async_task<bool> add_ue(const mac_ue_create_request& request) override;

  /// Deletes UE context in MAC MUX.
  async_task<void> remove_ue(const mac_ue_delete_request& request) override;

  /// Add/Modify UE bearers in the MUX.
  async_task<bool> addmod_bearers(du_ue_index_t                                  ue_index,
                                  du_cell_index_t                                pcell_index,
                                  const std::vector<mac_logical_channel_config>& logical_channels) override;

  /// Add/Modify UE bearers in the DEMUX.
  async_task<bool>
  remove_bearers(du_ue_index_t ue_index, du_cell_index_t pcell_index, span<const lcid_t> lcids_to_rem) override;

  mac_cell_slot_handler& get_slot_handler(du_cell_index_t cell_index) { return *cells[cell_index]; }

private:
  mac_dl_config  cfg;
  du_rnti_table& rnti_table;

  std::array<std::unique_ptr<mac_cell_processor>, MAX_NOF_DU_CELLS> cells;

  /// \brief Reference to MAC scheduler interface used by the MAC DL processor.
  mac_scheduler_cell_info_handler& sched;

  /// Handler of MAC DL metrics.
  mac_dl_metric_handler metrics;
};

} // namespace srsran
