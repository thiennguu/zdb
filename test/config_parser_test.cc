/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2016 Paul Asmuth <paul@asmuth.com>
 *
 * FnordTable is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "unittest.h"
#include "fnordmetric/config_parser.h"
#include "fnordmetric/listen_udp.h"
#include "fnordmetric/listen_http.h"
#include "fnordmetric/fetch_http.h"
#include "fnordmetric/util/time.h"

using namespace fnordmetric;

UNIT_TEST(ConfigParserTest);

TEST_CASE(ConfigParserTest, TestTokenize, [] () {
  std::string confstr =
      R"(metric users_online {
        summarize_group sum
      })";

  ConfigParser parser(confstr.data(), confstr.size());
  ConfigParser::TokenType ttype;
  std::string tbuf;

  EXPECT(parser.getToken(&ttype, &tbuf) == true);
  EXPECT(ttype == ConfigParser::T_STRING);
  EXPECT(tbuf == "metric");
  parser.consumeToken();

  EXPECT(parser.getToken(&ttype, &tbuf) == true);
  EXPECT(ttype == ConfigParser::T_STRING);
  EXPECT(tbuf == "users_online");
  parser.consumeToken();

  EXPECT(parser.getToken(&ttype, &tbuf) == true);
  EXPECT(ttype == ConfigParser::T_LCBRACE);
  EXPECT(tbuf == "");
  parser.consumeToken();

  EXPECT(parser.getToken(&ttype, &tbuf) == true);
  EXPECT(ttype == ConfigParser::T_ENDLINE);
  EXPECT(tbuf == "");
  parser.consumeToken();

  EXPECT(parser.getToken(&ttype, &tbuf) == true);
  EXPECT(ttype == ConfigParser::T_STRING);
  EXPECT(tbuf == "summarize_group");
  parser.consumeToken();

  EXPECT(parser.getToken(&ttype, &tbuf) == true);
  EXPECT(ttype == ConfigParser::T_STRING);
  EXPECT(tbuf == "sum");
  parser.consumeToken();

  EXPECT(parser.getToken(&ttype, &tbuf) == true);
  EXPECT(ttype == ConfigParser::T_ENDLINE);
  EXPECT(tbuf == "");
  parser.consumeToken();

  EXPECT(parser.getToken(&ttype, &tbuf) == true);
  EXPECT(ttype == ConfigParser::T_RCBRACE);
  EXPECT(tbuf == "");
  parser.consumeToken();

  EXPECT(parser.getToken(&ttype, &tbuf) == false);
});

TEST_CASE(ConfigParserTest, TestTokenizeWithComments, [] () {
  std::string confstr =
      R"(metric users_online {
        # test
        summarize_group sum
      })";

  ConfigParser parser(confstr.data(), confstr.size());
  ConfigParser::TokenType ttype;
  std::string tbuf;

  EXPECT(parser.getToken(&ttype, &tbuf) == true);
  EXPECT(ttype == ConfigParser::T_STRING);
  EXPECT(tbuf == "metric");
  parser.consumeToken();

  EXPECT(parser.getToken(&ttype, &tbuf) == true);
  EXPECT(ttype == ConfigParser::T_STRING);
  EXPECT(tbuf == "users_online");
  parser.consumeToken();

  EXPECT(parser.getToken(&ttype, &tbuf) == true);
  EXPECT(ttype == ConfigParser::T_LCBRACE);
  EXPECT(tbuf == "");
  parser.consumeToken();

  EXPECT(parser.getToken(&ttype, &tbuf) == true);
  EXPECT(ttype == ConfigParser::T_ENDLINE);
  EXPECT(tbuf == "");
  parser.consumeToken();

  EXPECT(parser.getToken(&ttype, &tbuf) == true);
  EXPECT(ttype == ConfigParser::T_STRING);
  EXPECT(tbuf == "summarize_group");
  parser.consumeToken();

  EXPECT(parser.getToken(&ttype, &tbuf) == true);
  EXPECT(ttype == ConfigParser::T_STRING);
  EXPECT(tbuf == "sum");
  parser.consumeToken();

  EXPECT(parser.getToken(&ttype, &tbuf) == true);
  EXPECT(ttype == ConfigParser::T_ENDLINE);
  EXPECT(tbuf == "");
  parser.consumeToken();

  EXPECT(parser.getToken(&ttype, &tbuf) == true);
  EXPECT(ttype == ConfigParser::T_RCBRACE);
  EXPECT(tbuf == "");
  parser.consumeToken();

  EXPECT(parser.getToken(&ttype, &tbuf) == false);
});

TEST_CASE(ConfigParserTest, TestParseCreateTablesOn, [] () {
  std::string confstr = R"(create_tables on)";

  ConfigList config;
  ConfigParser parser(confstr.data(), confstr.size());
  auto rc = parser.parse(&config);
  EXPECT_RC(rc);
  EXPECT(config.getCreateTables() == true);
});

TEST_CASE(ConfigParserTest, TestParseCreateTablesOff, [] () {
  std::string confstr = R"(create_tables off)";

  ConfigList config;
  ConfigParser parser(confstr.data(), confstr.size());
  auto rc = parser.parse(&config);
  EXPECT_RC(rc);
  EXPECT(config.getCreateTables() == false);
});

TEST_CASE(ConfigParserTest, TestParseBackendURL, [] () {
  std::string confstr = R"(backend 'mysql://localhost:3306/mydb?user=root')";

  ConfigList config;
  ConfigParser parser(confstr.data(), confstr.size());
  auto rc = parser.parse(&config);
  EXPECT_RC(rc);
  EXPECT(config.getBackendURL() == "mysql://localhost:3306/mydb?user=root");
});

TEST_CASE(ConfigParserTest, TestParseTableIntervalStanza, [] () {
  std::string confstr =
      R"(table users_online {
        interval 1m
      })";

  ConfigList config;
  ConfigParser parser(confstr.data(), confstr.size());
  auto rc = parser.parse(&config);
  EXPECT_RC(rc);

  EXPECT(config.getTableConfigs().size() == 1);
  {
    auto mc = config.getTableConfig("users_online");
    EXPECT(mc != nullptr);
    EXPECT(mc->interval == 60000000);
  }
});

TEST_CASE(ConfigParserTest, TestParseTableLabelStanza, [] () {
  std::string confstr =
      R"(table users_online {
        label datacenter
        label host
      })";

  ConfigList config;
  ConfigParser parser(confstr.data(), confstr.size());
  auto rc = parser.parse(&config);
  EXPECT_RC(rc);

  EXPECT(config.getTableConfigs().size() == 1);
  {
    auto mc = config.getTableConfig("users_online");
    EXPECT(mc != nullptr);
    EXPECT(mc->labels.size() == 2);
    EXPECT(mc->labels[0].column_name == "datacenter");
    EXPECT(mc->labels[0].type == DataType::STRING);
    EXPECT(mc->labels[1].column_name == "host");
    EXPECT(mc->labels[1].type == DataType::STRING);
  }
});

TEST_CASE(ConfigParserTest, TestParseTableMeasureStanza, [] () {
  std::string confstr =
      R"(table users_online {
        measure load_avg max(float64)
        measure request_count sum(uint64)
      })";

  ConfigList config;
  ConfigParser parser(confstr.data(), confstr.size());
  auto rc = parser.parse(&config);
  EXPECT_RC(rc);

  EXPECT(config.getTableConfigs().size() == 1);
  {
    auto mc = config.getTableConfig("users_online");
    EXPECT(mc != nullptr);
    EXPECT(mc->measures.size() == 2);
  }
});

TEST_CASE(ConfigParserTest, ParseTableDefinitionWithComments, [] () {
  std::string confstr =
      R"(
        # testing
        table users_online {
          measure load_avg max(float64)
          # test
          measure request_count sum(uint64)
        }
        # test
      )";

  ConfigList config;
  ConfigParser parser(confstr.data(), confstr.size());
  auto rc = parser.parse(&config);
  EXPECT_RC(rc);

  EXPECT(config.getTableConfigs().size() == 1);
  {
    auto mc = config.getTableConfig("users_online");
    EXPECT(mc != nullptr);
    EXPECT(mc->measures.size() == 2);
  }
});

//TEST_CASE(ConfigParserTest, TestParseSensorTableIDRewriteStanza, [] () {
//  std::string confstr =
//      R"(sensor_http sensor1 {
//        metric_id_rewrite ".*" "blah-$1"
//      })";
//
//  ConfigList config;
//  ConfigParser parser(confstr.data(), confstr.size());
//  auto rc = parser.parse(&config);
//  EXPECT_RC(rc);
//
//  EXPECT(config.getSensorConfigs().size() == 1);
//  {
//    auto sc = config.getSensorConfig("sensor1");
//    EXPECT(sc != nullptr);
//    EXPECT(sc->metric_id_rewrite_enabled == true);
//    EXPECT(sc->series_id_rewrite_enabled == false);
//  }
//});

//TEST_CASE(ConfigParserTest, TestParseSensorSeriesIDRewriteStanza, [] () {
//  std::string confstr =
//      R"(sensor_http sensor1 {
//        series_id_rewrite ".*" "blah-$1"
//      })";
//
//  ConfigList config;
//  ConfigParser parser(confstr.data(), confstr.size());
//  auto rc = parser.parse(&config);
//  EXPECT_RC(rc);
//
//  EXPECT(config.getSensorConfigs().size() == 1);
//  {
//    auto sc = config.getSensorConfig("sensor1");
//    EXPECT(sc != nullptr);
//    EXPECT(sc->series_id_rewrite_enabled == true);
//    EXPECT(sc->metric_id_rewrite_enabled == false);
//  }
//});

TEST_CASE(ConfigParserTest, TestParseListenUDP, [] () {
  std::string confstr =
      R"(listen_udp {
        port 8175
        bind 127.0.0.1
      })";

  ConfigList config;
  ConfigParser parser(confstr.data(), confstr.size());
  auto rc = parser.parse(&config);
  EXPECT_RC(rc);

  EXPECT(config.getIngestionTaskConfigs().size() == 1);
  auto c = dynamic_cast<UDPIngestionTaskConfig*>(
      config.getIngestionTaskConfigs()[0].get());

  EXPECT(c != nullptr);
  EXPECT(c->port == 8175);
  EXPECT(c->bind == "127.0.0.1");
  EXPECT(c->format == IngestionSampleFormat::STATSD);
});

TEST_CASE(ConfigParserTest, TestParseListenUDPWithFormat, [] () {
  std::string confstr =
      R"(listen_udp {
        port 8175
        format json
      })";

  ConfigList config;
  ConfigParser parser(confstr.data(), confstr.size());
  auto rc = parser.parse(&config);
  EXPECT_RC(rc);

  EXPECT(config.getIngestionTaskConfigs().size() == 1);
  auto c = dynamic_cast<UDPIngestionTaskConfig*>(
      config.getIngestionTaskConfigs()[0].get());

  EXPECT(c != nullptr);
  EXPECT(c->port == 8175);
  EXPECT(c->format == IngestionSampleFormat::JSON);
});

TEST_CASE(ConfigParserTest, TestParseListenHTTP, [] () {
  std::string confstr =
      R"(listen_http {
        port 8175
        bind 127.0.0.1
      })";

  ConfigList config;
  ConfigParser parser(confstr.data(), confstr.size());
  auto rc = parser.parse(&config);
  EXPECT_RC(rc);

  EXPECT(config.getIngestionTaskConfigs().size() == 1);
  auto c = dynamic_cast<HTTPPushIngestionTaskConfig*>(
      config.getIngestionTaskConfigs()[0].get());

  EXPECT(c != nullptr);
  EXPECT(c->port == 8175);
  EXPECT(c->bind == "127.0.0.1");
});

TEST_CASE(ConfigParserTest, TestParseFetchHTTP, [] () {
  std::string confstr =
      R"(fetch_http {
        url "http://example.com/asd?123"
        interval 30s
        format json
      })";

  ConfigList config;
  ConfigParser parser(confstr.data(), confstr.size());
  auto rc = parser.parse(&config);
  EXPECT_RC(rc);

  EXPECT(config.getIngestionTaskConfigs().size() == 1);
  auto c = dynamic_cast<HTTPPullIngestionTaskConfig*>(
      config.getIngestionTaskConfigs()[0].get());

  EXPECT(c != nullptr);
  EXPECT(c->url == "http://example.com/asd?123");
  EXPECT(c->interval == 30 * kMicrosPerSecond);
  EXPECT(c->format == IngestionSampleFormat::JSON);
});

