/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2016 Paul Asmuth, FnordCorp B.V. <paul@asmuth.com>
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnordmetric/ingest.h>
#include <fnordmetric/util/time.h>
#include <fnordmetric/util/logging.h>
#include <fnordmetric/listen_udp.h>
#include <fnordmetric/listen_http.h>

namespace fnordmetric {

IngestionTaskConfig::IngestionTaskConfig() :
    metric_id_rewrite_enabled(false) {}

IngestionService::IngestionService(
    AggregationService* aggregation_service) :
    aggregation_service_(aggregation_service) {}

ReturnCode IngestionService::applyConfig(const ConfigList* config) {
  auto statsd_server = std::unique_ptr<StatsdServer>(new StatsdServer(aggregation_service_));
  statsd_server->listen("localhost", 8125);
  addTask(std::move(statsd_server));
  return ReturnCode::success();
}

void IngestionService::addTask(std::unique_ptr<IngestionTask> task) {
  std::unique_lock<std::mutex> lk(mutex_);
  auto task_ptr = task.get();
  tasks_.emplace_back(
      std::thread(std::bind(&IngestionTask::start, task_ptr)),
      std::move(task));
}

void IngestionService::shutdown() {
  std::unique_lock<std::mutex> lk(mutex_);
  for (auto& t : tasks_) {
    t.second->shutdown();
    t.first.join();
  }

  tasks_.clear();
}

} // namespace fnordmetric
