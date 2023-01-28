/*
 Copyright 2022 Primihub

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#include "src/primihub/data_store/mysql/mysql_driver.h"
#include "src/primihub/data_store/driver.h"
#include "src/primihub/util/util.h"
#include <glog/logging.h>

#include <arrow/api.h>
#include <arrow/io/api.h>
#include <fstream>
#include <glog/logging.h>
#include <iostream>

namespace primihub {

auto sql_result_deleter = [](MYSQL_RES* result) {
    if (result) {
        mysql_free_result(result);
    }
};

// mysql cursor implementation
MySQLCursor::MySQLCursor(const std::string& sql, std::shared_ptr<MySQLDriver> driver) {
    this->sql_ = sql;
    this->driver_ = driver;
    auto access_info = dynamic_cast<MySQLAccessInfo*>(this->driver_->dataSetAccessInfo().get());
    if (access_info->query_colums_.empty()) {
        for (const auto& col : driver->tableColums()) {
            query_cols.push_back(col);
        }
    } else {
        for (const auto& col : access_info->query_colums_) {
            query_cols.push_back(col);
        }
    }
}

MySQLCursor::~MySQLCursor() {
    this->close();
}

void MySQLCursor::close() {}

std::shared_ptr<arrow::Field>
MySQLCursor::makeArrowField(sql_type_t sql_type, const std::string& col_name) {
    switch (sql_type) {
    case sql_type_t::INT64:
        return arrow::field(col_name, arrow::int64());
    case sql_type_t::INT:
        return arrow::field(col_name, arrow::int32());
    case sql_type_t::FLOAT:
        return arrow::field(col_name, arrow::float64());
    case sql_type_t::DOUBLE:
        return arrow::field(col_name, arrow::float64());
    case sql_type_t::STRING:
    case sql_type_t::BINARY:
        return arrow::field(col_name, arrow::binary());
    case sql_type_t::UNKNOWN:
        LOG(ERROR) << "unkonw sql type: " << static_cast<int>(sql_type);
        return nullptr;
    default:
        LOG(ERROR) << "unimplement sql type: " << static_cast<int>(sql_type);
        return nullptr;
    }
}

MySQLCursor::sql_type_t MySQLCursor::getSQLType(const std::string& col_type) {
    if (col_type == "bigint" ) {
        return sql_type_t::INT64;
    } else if (col_type == "tinyint") {
        return sql_type_t::INT;
    } else if (col_type == "varchar" ||
            col_type == "char" ||
            col_type == "text") {
        return sql_type_t::STRING;
    } else if (col_type == "binary" ||
            col_type == "blob" ){
        return sql_type_t::BINARY;
    } else if (col_type == "datetime" ||
            col_type == "timestamp") {
        return sql_type_t::STRING;
    } else {
        LOG(ERROR) << "unknown sql type:" << col_type;
        return sql_type_t::UNKNOWN;
    }
}

std::shared_ptr<arrow::Schema> MySQLCursor::makeArrowSchema() {
    std::vector<std::shared_ptr<arrow::Field>> result_schema_filed;
    auto access_info = dynamic_cast<MySQLAccessInfo*>(this->driver_->dataSetAccessInfo().get());
    auto& table_schema = this->driver_->tableSchema();
    for (const auto& col_name : this->query_cols) {
        auto it = table_schema.find(col_name);
        if (it == table_schema.end()) {
            LOG(ERROR) << "invalid table colume: " << col_name;
            return nullptr;
        }
        auto sql_type = getSQLType(it->second);
        result_schema_filed.push_back(makeArrowField(sql_type, col_name));
    }
    return std::make_shared<arrow::Schema>(result_schema_filed);
}

std::shared_ptr<arrow::Array>
MySQLCursor::makeArrowArray(sql_type_t sql_type, const std::vector<std::string>& arr) {
    std::shared_ptr<arrow::Array> array;
    switch (sql_type) {
    case sql_type_t::INT64: {
        std::vector<int64_t> tmp_arr;
        tmp_arr.reserve(arr.size());
        for (const auto& item : arr) {
            tmp_arr.push_back(stoi(item));
        }
        arrow::Int64Builder builder;
        builder.AppendValues(tmp_arr);
        builder.Finish(&array);
        break;
    }
    case sql_type_t::INT: {
        std::vector<int32_t> tmp_arr;
        tmp_arr.reserve(arr.size());
        for (const auto& item : arr) {
            tmp_arr.push_back(stoi(item));
        }
        arrow::Int32Builder builder;
        builder.AppendValues(tmp_arr);
        builder.Finish(&array);
        break;
    }
    case sql_type_t::FLOAT: {
        std::vector<float> tmp_arr;
        tmp_arr.reserve(arr.size());
        for (const auto& item : arr) {
            tmp_arr.push_back(std::stof(item));
        }
        arrow::FloatBuilder builder;
        builder.AppendValues(tmp_arr);
        builder.Finish(&array);
        break;
    }
    case sql_type_t::DOUBLE: {
        std::vector<double> tmp_arr;
        tmp_arr.reserve(arr.size());
        for (const auto& item : arr) {
            tmp_arr.push_back(std::stod(item));
        }
        arrow::DoubleBuilder builder;
        builder.AppendValues(tmp_arr);
        builder.Finish(&array);
        break;
    }
    case sql_type_t::STRING: {
        arrow::StringBuilder builder;
        builder.AppendValues(arr);
        builder.Finish(&array);
        break;
    }
    case sql_type_t::UNKNOWN:
        LOG(ERROR) << "unkonw sql type: " << static_cast<int>(sql_type);
        break;
    }
    return array;
}

retcode MySQLCursor::fetchData(std::vector<std::shared_ptr<arrow::Array>>* data_arr) {
    VLOG(5) << "query sql: " << this->sql_;
    std::vector<std::vector<std::string>> result_data;
    result_data.resize(this->query_cols.size());
    // fetch data from db
    auto db_connector = this->driver_->getDBConnector();
    if (sql_.empty()) {
        LOG(ERROR) << "query sql is invalid: ";
        return retcode::FAIL;
    }
    if (0 != mysql_real_query(db_connector, sql_.data(), sql_.length())) {
        LOG(ERROR)  << "query execute failed: " << mysql_error(db_connector);
        return retcode::FAIL;
    }
    // fetch data
    std::unique_ptr<MYSQL_RES, decltype(sql_result_deleter)> result{nullptr, sql_result_deleter};
    result.reset(mysql_store_result(db_connector));
    if (result == nullptr) {
        LOG(ERROR) << "fetch result failed: " << mysql_error(db_connector);
        return retcode::FAIL;
    }
    uint32_t num_fields = mysql_num_fields(result.get());
    VLOG(5) << "numbers of fields: " << num_fields;
    MYSQL_ROW row;
    while (nullptr != (row = mysql_fetch_row(result.get()))) {
        unsigned long* lengths;
        lengths = mysql_fetch_lengths(result.get());
        for (uint32_t i = 0; i < num_fields; i++) {
            result_data[i].push_back(row[i] ? std::string(row[i], lengths[i]) : std::string("NULL"));
        }
    }

    // convert data to arrow format
    auto& table_schema = this->driver_->tableSchema();
    for (size_t i = 0; i < this->query_cols.size(); i++) {
        auto& col_name = this->query_cols[i];
        auto it = table_schema.find(col_name);
        if (it == table_schema.end()) {
            LOG(ERROR) << "no colum type found for: " << col_name;
            return retcode::FAIL;
        }
        auto& sql_type_str = it->second;
        auto sql_type = getSQLType(sql_type_str);
        auto array = makeArrowArray(sql_type, result_data[i]);
        data_arr->push_back(std::move(array));
    }
    return retcode::SUCCESS;
}

// read all data from mysql
std::shared_ptr<primihub::Dataset> MySQLCursor::read() {
    auto schema = makeArrowSchema();
    std::vector<std::shared_ptr<arrow::Array>> array_data;
    auto ret = fetchData(&array_data);
    if (ret != retcode::SUCCESS) {
        return nullptr;
    }
    auto table = arrow::Table::Make(schema, array_data);
    auto dataset = std::make_shared<primihub::Dataset>(table, this->driver_);
    return dataset;
}

std::shared_ptr<primihub::Dataset>
MySQLCursor::read(int64_t offset, int64_t limit) {
    return nullptr;
}

int MySQLCursor::write(std::shared_ptr<primihub::Dataset> dataset) {}

// ======== MySQL Driver implementation ========
MySQLDriver::MySQLDriver(const std::string& nodelet_addr)
        : DataDriver(nodelet_addr) {
    setDriverType();
    initMySqlLib();
}

MySQLDriver::MySQLDriver(
        const std::string& nodelet_addr, std::unique_ptr<DataSetAccessInfo> access_info)
        : DataDriver(nodelet_addr, std::move(access_info)) {
    setDriverType();
    initMySqlLib();
}

MySQLDriver::~MySQLDriver() {
    releaseMySqlLib();
}

void MySQLDriver::setDriverType() {
    driver_type = "MySQL";
}

retcode MySQLDriver::releaseMySqlLib() {
    mysql_library_end();
    return retcode::SUCCESS;
}

retcode MySQLDriver::initMySqlLib() {
    mysql_library_init(0, nullptr, nullptr);
    return retcode::SUCCESS;
}

std::string MySQLDriver::getMySqlError() {
    return std::string(mysql_error(db_connector_.get()));
}

retcode MySQLDriver::connect(MySQLAccessInfo* access_info) {
    const std::string& host = access_info->ip_;
    const std::string& user = access_info->user_name_;
    const std::string& password = access_info->password_;
    const std::string& db_name = access_info->db_name_;
    const uint32_t db_port = access_info->port_;
    if (connected.load()) {
        return retcode::SUCCESS;
    }
    db_connector_.reset(mysql_init(nullptr));
    mysql_options(db_connector_.get(), MYSQL_OPT_CONNECT_TIMEOUT, &connect_timeout_ms);
    if (mysql_real_connect(db_connector_.get(), host.c_str(), user.c_str(), password.c_str(),
            db_name.c_str(), db_port, /*unix_socket*/nullptr, /*client_flag*/0) == nullptr) {
        LOG(ERROR) << "connect failed:" << getMySqlError();
        return retcode::FAIL;
    }
    LOG(INFO) << "connect to mysql db success";
    connected.store(true);
    return retcode::SUCCESS;
}

bool MySQLDriver::isConnected() {
    return true;
}

bool MySQLDriver::reConnect() {
    return true;
}

retcode MySQLDriver::executeQuery(const std::string& sql_query) {
    if (sql_query.empty()) {
        LOG(ERROR) << "query sql is invalid: ";
        return retcode::FAIL;
    }
    if (0 != mysql_real_query(this->db_connector_.get(), sql_query.data(), sql_query.length())) {
        LOG(ERROR)  << "query execute failed: " << getMySqlError();
        return retcode::FAIL;
    }
    return retcode::SUCCESS;
}

std::string MySQLDriver::buildQuerySQL(MySQLAccessInfo* access_info) {
    const auto& table_name = access_info->table_name_;
    auto& query_cols = access_info->query_colums_;
    std::string sql_str = "SELECT ";
    std::vector<std::string>* col_info{nullptr};
    if (query_cols.empty()) {
        col_info = &(this->table_cols_);

    } else {
        col_info = &query_cols;
    }
    for (size_t i = 0; i < col_info->size(); ++i) {
        auto& col_name = (*col_info)[i];
        if (col_name.empty()) {
            continue;
        }
        if (i == col_info->size()-1) {
            sql_str.append(col_name).append(" ");
        } else {
            sql_str.append(col_name).append(",");
        }
    }
    sql_str.append(" FROM ").append(table_name);
    VLOG(5) << "query sql: " << sql_str;
    return sql_str;
}

retcode MySQLDriver::getTableSchema(const std::string& db_name, const std::string& table_name) {
    if (!this->isConnected()) {
        this->reConnect();
    }
    std::string sql_table_schema{
        "SELECT column_name, data_type from information_schema.COLUMNS where TABLE_NAME = '"};
    sql_table_schema.append(table_name);
    sql_table_schema.append("' and TABLE_SCHEMA = '");
    sql_table_schema.append(db_name);
    sql_table_schema.append("';");
    auto ret = executeQuery(sql_table_schema);
    if (ret != retcode::SUCCESS) {
        return retcode::FAIL;
    }
    // get table schema
    // result = mysql_store_result(mysql);

    std::unique_ptr<MYSQL_RES, decltype(sql_result_deleter)> result{nullptr, sql_result_deleter};
    result.reset(mysql_use_result(this->db_connector_.get()));

    if (result == nullptr) {
        LOG(ERROR) << "fetch result failed: " << getMySqlError();
        return retcode::FAIL;
    }

    uint32_t num_fields = mysql_num_fields(result.get());
    VLOG(5) << "numbers of result: " << num_fields;
    if (num_fields != 2) { // we just query 2 fileds
        LOG(ERROR) << "2 num_fields is expected, but get " << num_fields;
        return retcode::FAIL;
    }
    MYSQL_ROW row;
    table_cols_.clear();
    table_schema_.clear();
    while (nullptr != (row = mysql_fetch_row(result.get()))) {
        unsigned long* lengths;
        lengths = mysql_fetch_lengths(result.get());
        std::string column_name = std::string(row[0], lengths[0]);
        std::string data_type = std::string(row[1], lengths[1]);
        VLOG(5) << "column_name: " << column_name << " "
                << "data_type: " << data_type;
        table_cols_.push_back(column_name);
        table_schema_.insert({std::move(column_name), std::move(data_type)});
    }
    return retcode::SUCCESS;
}

std::shared_ptr<Cursor>& MySQLDriver::read() {
    // first connect to db
    auto access_info_ptr = dynamic_cast<MySQLAccessInfo*>(this->access_info_.get());
    if (access_info_ptr == nullptr) {
        LOG(ERROR) << "mysqlite access info is not available";
        return getCursor();
    }
    auto ret = this->connect(access_info_ptr);
    if (ret != retcode::SUCCESS) {
        return getCursor();
    }
    std::string sql_change_db{"USE "};
    sql_change_db.append(access_info_ptr->db_name_);
    this->executeQuery(sql_change_db);
    this->getTableSchema(access_info_ptr->db_name_, access_info_ptr->table_name_);
    std::string query_sql = buildQuerySQL(access_info_ptr);
    this->cursor = std::make_shared<MySQLCursor>(query_sql, shared_from_this());
    return getCursor();
}

std::shared_ptr<Cursor>& MySQLDriver::read(const std::string &conn_str) {
    return this->initCursor(conn_str);
}

std::shared_ptr<Cursor>& MySQLDriver::initCursor(const std::string &conn_str) {
    std::string sql_str;
    this->cursor = std::make_shared<MySQLCursor>(sql_str, shared_from_this());
    return getCursor();
}

// write data to specifiy table
int MySQLDriver::write(std::shared_ptr<arrow::Table> table, const std::string &table_name) {
    return 0;
}

std::string MySQLDriver::getDataURL() const { return conn_info_; };

} // namespace primihub