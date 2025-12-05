# 计划

## 步骤

1) 理解数据与接口

- 确认 Redis key 示例 `selfcheck:PILE#...:CCU#X` 采用 hash，字段名形如 `idx;描述`，值为 `1|2`（1 正常 true，2 失败 false）。
- 需要新增/使用 Redis hgetall 读取 0~31 全字段。

2) 扩展 RedisClient 提供 hgetall 读取

- 在 `include/client/redis_client.h` 与 `src/client/redis_client.cpp` 增加 `HGetAll` 接口（或返回 `std::unordered_map<std::string,std::string>`），封装 `redis_->hgetall`，含连接检查与异常日志。

3) 实现 CCUAttributes 解析

- 在 `src/check_manager.cpp::getResultFromRedis`：
- 基于 device type 仅处理 `kPILE`。
- 调用 `HGetAll` 取 hash，遍历字段。
- 解析字段名左侧编号（分号前部分），转为 int 0-31；值转 int。
- 规则：value==1 表示正常，将对应布尔置 true；value==2 置 false；未知值记录告警。
- 按映射填充 `CCUAttributes`：
- ac_contactor_1: 0 拒动, 1 粘连；ac_contactor_2: 2 拒动, 3 粘连。
- parallel_contactor: 4 正极拒动, 5 正极粘连, 6 负极拒动, 7 负极粘连。
- fan_1~fan_4: (8,9), (10,11), (12,13), (14,15) 对应停转/转动。
- gun_a: 16 正极拒动, 17 正极粘连, 18 负极拒动, 19 负极粘连, 20 解锁, 21 上锁, 22 12V, 23 24V。
- gun_b: 24 正极拒动, 25 正极粘连, 26 负极拒动, 27 负极粘连, 28 解锁, 29 上锁, 30 12V, 31 24V。
- 对缺失或解析失败的条目记录 LOG(WARNING)。
- 返回填充好的 `CCUAttributes`。

4) 结果校验与防御

- 对 redis 客户端未连接、key 不存在、hash 为空等返回 Status 错误并日志。
- 保持 2-space 缩进与 Google C++ 风格，避免编译/测试操作。