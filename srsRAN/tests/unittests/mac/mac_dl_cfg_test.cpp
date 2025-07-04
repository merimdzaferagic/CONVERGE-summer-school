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

#include "lib/mac/mac_dl/mac_dl_processor.h"
#include "lib/mac/mac_sched/srsran_scheduler_adapter.h"
#include "lib/mac/rnti_manager.h"
#include "mac_ctrl_test_dummies.h"
#include "mac_test_helpers.h"
#include "tests/test_doubles/mac/dummy_mac_metrics_notifier.h"
#include "tests/test_doubles/mac/dummy_scheduler_ue_metric_notifier.h"
#include "tests/test_doubles/scheduler/scheduler_config_helper.h"
#include "srsran/support/async/eager_async_task.h"
#include "srsran/support/executors/blocking_task_worker.h"
#include "srsran/support/executors/manual_task_worker.h"
#include "srsran/support/executors/task_worker.h"
#include "srsran/support/test_utils.h"
#include <gtest/gtest.h>
#include <thread>

using namespace srsran;

/// Enum used to track the progress of the test task
enum class test_task_event { ue_created, ue_reconfigured, ue_deleted };

/// Test coroutine that calls UE create, reconfiguration, delete in sequence.
struct add_reconf_delete_ue_test_task {
  srslog::basic_logger&                  logger = srslog::fetch_basic_logger("TEST");
  std::thread::id                        tid;
  mac_dl_processor&                      mac_dl;
  unique_function<void(test_task_event)> event_test;
  mac_ue_create_request                  create_msg{};
  mac_ue_reconfiguration_request         reconf_msg{};
  mac_ue_delete_request                  delete_msg{};

  add_reconf_delete_ue_test_task(mac_dl_processor& mac_dl_, unique_function<void(test_task_event)> event_callback_) :
    mac_dl(mac_dl_), event_test(std::move(event_callback_))
  {
    create_msg.ue_index   = to_du_ue_index(0);
    create_msg.crnti      = to_rnti(0x4601);
    create_msg.cell_index = to_du_cell_index(0);
    create_msg.bearers.emplace_back();
    create_msg.bearers[0].lcid = LCID_SRB1;

    reconf_msg.ue_index    = to_du_ue_index(0);
    reconf_msg.crnti       = to_rnti(0x4601);
    reconf_msg.pcell_index = to_du_cell_index(0);
    reconf_msg.bearers_to_addmod.emplace_back();
    reconf_msg.bearers_to_addmod[0].lcid = LCID_SRB2;
    reconf_msg.bearers_to_rem.emplace_back(LCID_SRB1);

    delete_msg.ue_index   = to_du_ue_index(0);
    delete_msg.cell_index = to_du_cell_index(0);
    delete_msg.rnti       = to_rnti(0x4601);
  }

  void operator()(coro_context<eager_async_task<void>>& ctx)
  {
    CORO_BEGIN(ctx);
    // UE creation
    tid = std::this_thread::get_id();
    logger.info("Step 1: Creating UE");
    CORO_AWAIT(mac_dl.add_ue(create_msg));
    logger.info("UE created");
    event_test(test_task_event::ue_created);
    // UE reconfiguration
    logger.info("Step 2: Reconfiguring UE");
    CORO_AWAIT(mac_dl.addmod_bearers(reconf_msg.ue_index, reconf_msg.pcell_index, reconf_msg.bearers_to_addmod));
    CORO_AWAIT(mac_dl.remove_bearers(reconf_msg.ue_index, reconf_msg.pcell_index, reconf_msg.bearers_to_rem));
    logger.info("UE reconfigured");
    event_test(test_task_event::ue_reconfigured);
    // UE deletion
    logger.info("Step 3: Deleting UE");
    CORO_AWAIT(mac_dl.remove_ue(delete_msg));
    logger.info("UE deleted");
    event_test(test_task_event::ue_deleted);
    TESTASSERT(std::this_thread::get_id() == tid);
    CORO_RETURN();
  }
};

/// In this test, we verify the correct executors are called during creation, reconfiguration and deletion of a UE.
TEST(test_mac_dl_cfg, test_dl_ue_procedure_execution_contexts)
{
  test_delimit_logger delimiter{"Test UE procedures execution contexts"};

  auto&                               logger = srslog::fetch_basic_logger("TEST");
  manual_task_worker                  ctrl_worker{128};
  manual_task_worker                  dl_worker{128};
  std::vector<task_executor*>         dl_execs = {&dl_worker};
  dummy_ue_executor_mapper            ul_exec_mapper{ctrl_worker};
  dummy_dl_executor_mapper            dl_exec_mapper{dl_execs[0]};
  dummy_mac_event_indicator           du_mng_notifier;
  dummy_mac_result_notifier           phy_notifier;
  dummy_scheduler_ue_metrics_notifier sched_metrics_notif;
  dummy_mac_metrics_notifier          mac_metrics_notif;
  null_mac_pcap                       pcap;
  timer_manager                       timers;
  mac_dl_config                       mac_dl_cfg{ul_exec_mapper,
                           dl_exec_mapper,
                           ctrl_worker,
                           phy_notifier,
                           pcap,
                           timers,
                           std::chrono::milliseconds{1000},
                           mac_metrics_notif};
  mac_config                          maccfg{du_mng_notifier,
                    ul_exec_mapper,
                    dl_exec_mapper,
                    ctrl_worker,
                    phy_notifier,
                    mac_expert_config{.configs = {{10000, 10000, 10000}}},
                    pcap,
                    timers,
                    mac_metrics_notif,
                    scheduler_expert_config{},
                    sched_metrics_notif};
  rnti_manager                        rnti_mng;

  srsran_scheduler_adapter sched_cfg_adapter{maccfg, rnti_mng};
  mac_dl_processor         mac_dl(mac_dl_cfg, sched_cfg_adapter, rnti_mng);

  auto mac_cell_req      = test_helpers::make_default_mac_cell_config();
  mac_cell_req.sched_req = sched_config_helper::make_default_sched_cell_configuration_request();
  sched_cfg_adapter.add_cell(mac_cell_req);

  // Set this to a valid ARFCN value (band 3, in this case, but it doesn't matter) - Required for SSB.
  mac_dl.add_cell(mac_cell_req);

  // TEST: Thread used for resumption does not change.
  auto test_event = [&ctrl_worker](test_task_event ev) {
    TESTASSERT(ctrl_worker.get_thread_id() == std::this_thread::get_id(), "Procedure must finish in CTRL thread.");
    if (ev == test_task_event::ue_deleted) {
      ctrl_worker.request_stop();
    }
  };
  eager_async_task<void> t = launch_async<add_reconf_delete_ue_test_task>(mac_dl, test_event);

  while (not ctrl_worker.is_stopped()) {
    if (ctrl_worker.has_pending_tasks()) {
      logger.info("Running next task in CTRL worker");
      ctrl_worker.try_run_next();
    }
    if (dl_worker.has_pending_tasks()) {
      logger.info("Running next task in DL worker");
      dl_worker.try_run_next();
    }
  }
  TESTASSERT(not t.empty() and t.ready());
}

TEST(test_mac_dl_cfg, test_dl_ue_procedure_tsan)
{
  test_delimit_logger delimiter{"Test UE procedures TSAN"};

  blocking_task_worker                ctrl_worker{128};
  task_worker                         dl_workers[] = {{"DL#1", 128}, {"DL#2", 128}};
  task_worker_executor                dl_execs[]   = {{dl_workers[0]}, {dl_workers[1]}};
  dummy_ue_executor_mapper            ul_exec_mapper{ctrl_worker};
  dummy_dl_executor_mapper            dl_exec_mapper{&dl_execs[0], &dl_execs[1]};
  dummy_mac_event_indicator           du_mng_notifier;
  dummy_mac_result_notifier           phy_notifier;
  null_mac_pcap                       pcap;
  timer_manager                       timers;
  dummy_scheduler_ue_metrics_notifier sched_metrics_notif;
  dummy_mac_metrics_notifier          mac_metrics_notif;
  mac_dl_config                       mac_dl_cfg{ul_exec_mapper,
                           dl_exec_mapper,
                           ctrl_worker,
                           phy_notifier,
                           pcap,
                           timers,
                           std::chrono::milliseconds{1000},
                           mac_metrics_notif};
  mac_config                          maccfg{du_mng_notifier,
                    ul_exec_mapper,
                    dl_exec_mapper,
                    ctrl_worker,
                    phy_notifier,
                    mac_expert_config{.configs = {{10000, 10000, 10000}}},
                    pcap,
                    timers,
                    mac_metrics_notif,
                    scheduler_expert_config{},
                    sched_metrics_notif};
  rnti_manager                        rnti_mng;

  srsran_scheduler_adapter sched_cfg_adapter{maccfg, rnti_mng};
  mac_dl_processor         mac_dl(mac_dl_cfg, sched_cfg_adapter, rnti_mng);

  // Action: Add Cells.
  mac_cell_creation_request cell_cfg1 = test_helpers::make_default_mac_cell_config();
  cell_cfg1.sched_req                 = sched_config_helper::make_default_sched_cell_configuration_request();
  // Set this to a valid ARFCN value (band 3, in this case, but it doesn't matter) - Required for SSB.
  sched_cfg_adapter.add_cell(cell_cfg1);
  mac_dl.add_cell(cell_cfg1);
  mac_cell_creation_request cell_cfg2  = test_helpers::make_default_mac_cell_config();
  cell_cfg2.cell_index                 = to_du_cell_index(1);
  cell_cfg2.sched_req                  = sched_config_helper::make_default_sched_cell_configuration_request();
  cell_cfg2.sched_req.cell_index       = to_du_cell_index(1);
  cell_cfg2.sched_req.cell_group_index = (du_cell_group_index_t)1;
  sched_cfg_adapter.add_cell(cell_cfg2);
  mac_dl.add_cell(cell_cfg2);

  // TEST: Thread used for resumption does not change
  std::thread::id tid        = std::this_thread::get_id();
  auto            test_event = [&ctrl_worker, &tid](test_task_event ev) {
    TESTASSERT(tid == std::this_thread::get_id()); // resumes back in CTRL thread
    if (ev == test_task_event::ue_deleted) {
      ctrl_worker.request_stop();
    }
  };
  eager_async_task<void> t = launch_async<add_reconf_delete_ue_test_task>(mac_dl, test_event);

  ctrl_worker.run();
  TESTASSERT(not t.empty() and t.ready());
}
