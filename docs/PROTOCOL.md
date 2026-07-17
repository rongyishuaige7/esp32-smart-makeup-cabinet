# 本地 HTTP 协议与字段边界

> 本文件描述当前公开源码的固定接口，不是已连接设备的实时接口文档。公开默认不开 Wi-Fi/HTTP；任何网络与硬件行为都待当前真机复测。

## 启动前提与网络边界

公开默认：`WIFI_SSID` 为空、`ENABLE_EXPERIMENTAL_ACTUATORS=0`、`ENABLE_EXPERIMENTAL_LOCAL_CONTROL=0`，`initHTTPServer()` 保持 Wi-Fi/HTTP 关闭，执行器相关 GPIO 不初始化输出而保持输入高阻。仅当使用者在被 Git 忽略的 `firmware/src/config.local.h` 填入非空 Wi-Fi 信息，并同时把 `ENABLE_EXPERIMENTAL_ACTUATORS` 与 `ENABLE_EXPERIMENTAL_LOCAL_CONTROL` 设为 `1`，且 ESP32 成功加入网络时，端口 `80` 才可能启动。

HTTP 没有 TLS、认证、会话、授权、设备身份、审计或速率限制。它只限短期、隔离、可信、受监督局域网；不得暴露到公网、端口转发、公共 Wi-Fi、共享热点或不可信网络。

Flutter 客户端默认空地址、不自动联网、不后台轮询。它要求纯 `http://` origin：拒绝路径、查询、fragment、用户信息和公网主机；只接受 RFC1918 IPv4 或 loopback/localhost。保存地址不视为连接成功；必须显式读取 `/api/sensors` 和 `/api/status`，两者均可解析时才显示本会话 HTTP 读取成功。一次成功不代表设备在线、持续连接、数据准确、执行器动作或有人处理。

## 所有路径的共同规则

- JSON 写入请求要求 `Content-Type: application/json`、非空 body、字段白名单和长度/范围检查。
- 任一实验控制前置条件未满足时，写入路径返回 `403`：`{"accepted":false,"error":"experimental_local_control_disabled"}`。
- 未知路径返回 `404`：`{"accepted":false,"error":"not_found"}`。
- `200` 与 `accepted:true` 仅说明当前 handler 返回或接受请求；不表示物理输出、网络送达、数据真实、机械位置或成功结果。
- 本项目不提供 CORS、浏览器跨域、TLS、认证、授权、审计或设备发现协议。

## `GET /api/sensors`

| 字段 | 类型 | 当前源码含义 | 不代表 |
| :-- | :-- | :-- |
| `climate` | `available` / `unavailable` | 本次 DHT11 温湿度读取是否同时取得数值 | 传感器在线、准确、环境适宜、安全 |
| `temp` | number / `null` | `climate=available` 时的温度读数 | 化妆品保存建议、健康或安全结论 |
| `humid` | number / `null` | `climate=available` 时的湿度读数 | 化妆品质量、霉变、健康或安全结论 |
| `uvEstimate` | boolean | 是否提供未校准模块输出估算 | UV 安全、剂量、皮肤/产品健康结论 |
| `uv` | integer / `null` | `uvEstimate=true` 时的固定映射估算 | 真实 UV 指数或防护建议 |
| `pir` | boolean | 一次 GPIO5 数字输入 | 人员在场、身份、授权、安防或告警 |

示例仅为**结构**，不是实测数据：

```json
{"climate":"unavailable","temp":null,"humid":null,"uvEstimate":false,"uv":null,"pir":false}
```

## `GET /api/status`

| 字段 | 类型 | 当前源码含义 | 不代表 |
| :-- | :-- | :-- |
| `publicDefaultInert` | boolean | 当前构建是否未同时启用本地控制与执行器实验 | RFID 维护状态、设备离线、硬件安全、物理输出状态 |
| `localControlAndActuatorsEnabled` | boolean | 本地控制与执行器实验两个开关是否同时启用 | 设备在线、实体输出、网络安全或任何机械结果 |
| `ledCommandedOn` | boolean | 软件灯带命令变量 | 灯带亮起、供电/电平正确 |
| `fanCommandedOn` | boolean | 软件风扇命令变量 | 风扇转动、风量/温控有效 |
| `fanManual` | boolean | 软件记录的手动覆盖标记 | 人已操作、实体输出成功 |
| `humidifierCommandedOn` | boolean | 软件加湿器命令变量 | 加湿器运行、湿度变化或安全 |
| `humidifierManual` | boolean | 软件记录的手动覆盖标记 | 人已操作、实体输出成功 |
| `latchCommandedClosed` | boolean | 软件插销关闭命令变量 | 插销到位、门锁状态、安全或防盗 |
| `physicalPositionVerified` | `false` | 设计没有位置、力矩、电流或驱动确认反馈 | 任何机械实际状态 |

## 实验性写入端点

以下端点仅在 `ENABLE_EXPERIMENTAL_LOCAL_CONTROL=1` 且 `ENABLE_EXPERIMENTAL_ACTUATORS=1` 的本地受监督低压台架中才可能成功；公开默认将拒绝。

| 方法 | 路径 | 允许 body | 成功响应 | 额外限制 |
| :-- | :-- | :-- | :-- | :-- |
| `POST` | `/api/led` | `power` bool、`brightness` 0..255、`color` `[r,g,b]` 0..255 | `{"accepted":true}` | 仅命令语义 |
| `POST` | `/api/fan` | `power` bool，可选 `manual` bool | `{"accepted":true}` | 仅命令语义 |
| `POST` | `/api/humidifier` | `power` bool，可选 `manual` bool | `{"accepted":true}` | 仅命令语义 |
| `POST` | `/api/latch` | `{"action":"open"}` 或 `{"action":"close"}` | `{"accepted":true,"physicalPositionVerified":false}` | 不可作为门锁确认 |
| `POST` | `/api/settings` | 部分阈值/延时字段，须满足范围与顺序 | `{"accepted":true}` | 固定阈值不是控制安全规则 |

## RFID 本地记录端点

| 方法 | 路径 | 当前源码行为 | 数据/安全边界 |
| :-- | :-- | :-- | :-- |
| `GET` | `/api/rfid/list` | 返回本地记录的 `name`、`brand`、`expireDate`、`expiry`，**不含 UID** | 个人资料；日期只是手工记录，不是真实性/安全判断 |
| `POST` | `/api/rfid/register` | 仅在 RFID 维护、执行器和本地控制三个实验开关都精确设为 `1`，且本地 HTTP 已因非空 Wi-Fi 凭据成功入网而启动时，才可能返回 `accepted:true` | UID 可复制，不能当认证凭据；刷卡不会自动命令插销 |
| `POST` | `/api/rfid/delete` | 公开 HTTP 固定 `403` | 无公开删除能力 |
| `POST` | `/api/rfid/scan/start`、`/stop`；`GET /result` | 公开 HTTP 固定 `403` | 绝不通过公开 HTTP 回显原始 UID |

`expiry` 是本地日期字段的程序比较结果（如 `not_set` / `valid` / `expired` / `time_unavailable`），不等于商品官方信息、质量鉴定、皮肤安全或使用建议。

## 根路径

`GET /` 返回简短说明，明确其不证明硬件在线或执行器已动作。根路径也不是健康检查、设备发现、认证或上线状态接口。
