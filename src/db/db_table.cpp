#include "db/db_table.h"

#include "client/mysql_client.h"
#include <glog/logging.h>
#include <string>

namespace db {

absl::Status CreateSelfCheckRecordTable() {
  auto *client = MySqlClient::GetInstance();
  if (client == nullptr) {
    return absl::InternalError("MySQL client not initialized");
  }

  const std::string sql = R"(
    CREATE TABLE IF NOT EXISTS `self_check_record` (
        `ID`            BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '主键ID',
        `Type`          VARCHAR(32) NOT NULL COMMENT '设备类型：PILE = 充电桩，SWAP = 换电站',
        `EquipNo`       VARCHAR(64) NOT NULL COMMENT '设备编号（全局唯一编号）',
        `CheckCategory` VARCHAR(32) NOT NULL COMMENT '自检类别：FULL | QUICK | STARTUP | REMOTE',
        `Status`        VARCHAR(16) NOT NULL COMMENT '结果状态：OK / WARN / ERROR',
        `Summary`       VARCHAR(512) NOT NULL DEFAULT '' COMMENT '摘要：如 某模块异常 / 全部正常',
        `DetailsJSON`   JSON NOT NULL COMMENT '详细自检结果（JSON 格式，包含所有子模块结果）',
        `TriggerSource` VARCHAR(16) NOT NULL COMMENT '触发来源：LOCAL / WEB / CLOUD / AUTO',
        `TriggeredBy`   VARCHAR(64) DEFAULT NULL COMMENT '触发人或系统',
        `CreatedAt`     DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
        `UpdatedAt`     DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
        PRIMARY KEY (`ID`),
        INDEX `idx_equip` (`EquipNo`),
        INDEX `idx_type` (`Type`),
        INDEX `idx_ctime` (`CreatedAt`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='设备自检记录表';
  )";

  auto result = client->executeUpdate(sql);
  if (!result.ok()) {
    LOG(ERROR) << "Failed to create self_check_record table: "
               << result.status().message();
    return result.status();
  }

  LOG(INFO)
      << "CreateSelfCheckRecordTable: self_check_record table checked/created.";
  return absl::OkStatus();
}

} // namespace db
