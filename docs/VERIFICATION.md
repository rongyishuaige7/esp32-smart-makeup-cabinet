# 验证记录与真机复测清单

## 当前记录

首发源码提交 [`4746c090eb2fc358aa61e02df2c7f55effba40f3`](https://github.com/rongyishuaige7/esp32-smart-makeup-cabinet/commit/4746c090eb2fc358aa61e02df2c7f55effba40f3) 的 [GitHub Actions Validate](https://github.com/rongyishuaige7/esp32-smart-makeup-cabinet/actions/runs/29608194135) 已于 2026-07-18（Asia/Shanghai）完成并成功。下表的“通过”只表示该提交的静态检查与构建；不表示真机、网络、传感器、实体输出或机械位置。

| 检查项 | 状态 | 证据边界 |
| :-- | :-- | :-- |
| 公开范围扫描 | 通过（首发 exact-SHA CI） | 通过只表示扫描规则覆盖到的公开文件未命中 |
| 仓库资产检查 | 通过（首发 exact-SHA CI） | 通过只表示必需文件、结构与静态声明存在 |
| Python 源码契约 | 通过（首发 exact-SHA CI） | 通过只表示受检源码文字/结构符合约束 |
| ESP32 PlatformIO 默认、双 opt-in 与仅执行器宏仅编译覆盖 | 通过（首发 exact-SHA CI） | 通过不等于烧录、外设、供电、Wi-Fi、实体输出或真实运行 |
| Flutter format/test/analyze/debug APK | 通过（首发 exact-SHA CI） | 通过不等于 Android/iOS 真机、网络或端到端验证；debug/profile 为开发期本地 HTTP 演示保留网络与 cleartext 权限，不能作为普通 release 网络策略或真机联调的验证 |
| 当前 commit 真机复测 | **未执行** | 没有日期化、可回读的烧录/接线/测试证据 |

后续重新运行以下命令时，应在本文件追加实际日期、被验证的 exact Git commit、命令摘要、PASS/FAIL/BLOCKED 与失败原因；不得把脚本存在、历史运行或 CI 配置泛化为当前真机证据。

```bash
bash scripts/verify.sh
```

## 当前 commit 真机复测（尚未执行）

在公开文案从“未复测”升级前，至少记录以下**每项**的日期、完整 commit、执行者、板型、模块型号、供电、电平/驱动、接线、输入条件、观察结果、失败/偏差与照片/视频是否已脱敏：

1. ESP32 精确板型、USB/供电、Flash、启动与串口；
2. DHT11 的成功与失败读取；
3. GUVA/兼容模块的供电、输出电平与 ADC 输入确认（不作 UV 安全结论）；
4. PIR 输入和公共地；
5. OLED I²C 地址、电平和显示；
6. RC522 的 3.3 V、启动脚影响、读卡；不得以 UID 作为认证；
7. WS2812B 外部低压供电、数据电平、限流、公共地；
8. 风扇、加湿器驱动的电流、极性、保护、默认 off 与人工低压受监督测试；
9. 舵机独立供电、机械限位、夹伤/卡滞检查；不得将软件命令写为位置反馈；
10. 本地 `config.local.h` 不泄露凭据的前提下，Wi-Fi 加入失败/成功、HTTP disabled/default 与 opt-in 边界；
11. Flutter Android/iOS（分别记录）地址校验、显式测试、读取失败清空、写入风险确认及真机网络；
12. 断电、重启、网络丢失、传感器不可用、负载拔除等失败路径。

在未完成所有相关项前，README、Hardware Lab 和 GitHub 页面必须继续写“当前真机复测：未执行 / 范围未覆盖”。
