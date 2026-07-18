# 项目状态与证据边界

**项目：** 基于ESP32的智能化妆品收纳与环境管理系统
**公开状态：** 已公开；首发源码提交的 GitHub Actions 构建已通过，当前真机复测仍未执行。
**更新时间：** 2026-07-18（文档生成日期，不代表硬件测试日期）

| 范围 | 已确认事实 | 不可据此推出 |
| :-- | :-- | :-- |
| 公开默认 | 源码默认无 Wi-Fi 凭据，Wi-Fi/HTTP 不启动，执行器相关 GPIO 不初始化输出而保持输入高阻 | 设备离线、安全、不可被修改、实体负载已关闭 |
| 固件 | `platformio.ini` 固定 ESP32 / Arduino 依赖版本 | 真板可烧录、每个外设可用、电气正确 |
| Flutter | 客户端以显式测试读取 sensors + status；无默认地址/后台轮询；Android 普通 release manifest 不声明 `INTERNET` 或 cleartext 网络能力；debug/profile 仅为开发期本地 HTTP 演示保留网络与 cleartext 权限 | 手机已验证、持续连接、真实设备在线 |
| HTTP | opt-in 时存在本地明文 JSON 接口 | 认证、TLS、授权、远程可控、控制送达 |
| 执行器 | API 将结果收窄为 `accepted` 与 `*Commanded*` | 风扇/灯/加湿器/舵机/插销发生实际动作 |
| RFID/记录 | 公开 HTTP 不返回 UID；记录仅本地实验数据，刷卡不会自动命令插销 | 身份认证、门禁、商品真实性或有效期结论 |
| 真机复测 | **未执行**当前公开 commit 的日期化复测 | 任何“当前硬件已验证”“已联调”结论 |
| 媒体/EDA | **未提供**实物照片、视频、原理图、PCB、Gerber、制造文件 | 外观、焊接、布局、实际接线或生产能力 |

## 可验证的构建证据

首发源码提交 [`4746c090eb2fc358aa61e02df2c7f55effba40f3`](https://github.com/rongyishuaige7/esp32-smart-makeup-cabinet/commit/4746c090eb2fc358aa61e02df2c7f55effba40f3) 的 [GitHub Actions Validate 成功记录](https://github.com/rongyishuaige7/esp32-smart-makeup-cabinet/actions/runs/29608194135) 已完成公开范围扫描、仓库资产、源码契约、ESP32 默认/双 opt-in/仅执行器宏编译覆盖，以及 Flutter format/test/analyze/debug APK 构建。所有编译覆盖均不提供 Wi-Fi 凭据、不烧录硬件，也不执行实体动作。结果仅适用于该 exact commit，详见 [VERIFICATION](VERIFICATION.md)。

即使全部通过，结果也只代表静态公开检查和构建；不代表烧录、传感器、Wi-Fi、HTTP、Android/iOS、执行器、RFID、机械机构或真实环境结果。

## 2026-07-18 素材补充

已补充项目照片、界面截图和相关资料；文件处理说明见 [MEDIA_EVIDENCE](MEDIA_EVIDENCE.md)。

本次仅补充项目素材，当前未进行真机复测。
