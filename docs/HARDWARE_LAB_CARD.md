# Hardware Lab 索引卡片

> 供 `rongyishuaige7/hardware-lab` 索引引用；发布后应由索引仓维护者以实际仓库 URL、commit 和 CI 结果回填。

## 基于ESP32的智能化妆品收纳与环境管理系统

- **仓库 slug：** `esp32-smart-makeup-cabinet`
- **一句话：** 基于 ESP32、DHT11、GUVA-S12SD、PIR、RC522、低压执行器实验接口和 Flutter 的局域网教学原型。
- **技术：** ESP32 / Arduino / PlatformIO / Flutter / RFID / DHT11 / SSD1306 / WS2812B / local HTTP
- **公开默认：** 无 Wi-Fi/HTTP；不执行实体输出；需要本地受监督 low-voltage opt-in。
- **当前真机状态：** 未以当前公开 commit 复测。
- **未公开素材：** 实物照片、演示视频、EDA、PCB、Gerber、制造文件。
- **不适用：** 化妆品/皮肤/UV/健康判断、门禁/电子锁/防盗/认证、远程控制、安全或生产系统。
- **验证边界：** CI 或本地构建仅表示源码门禁和构建，不表示真实硬件、网络、执行器或机械位置。

发布前须核对：仓库实际存在、LICENSE/README/SECURITY/THIRD_PARTY 已同步、Actions exact SHA 成功、Description/Topics 已设置、索引标题为中文且不使用“智能门锁”“安全系统”“生产可用”等误导表述。
