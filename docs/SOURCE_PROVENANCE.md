# 来源、净化与公开范围

## 来源边界

- 原始来源：桌面中的 `smart_makeup_cabinet` 原工程；在本次公开整理中保持只读，不初始化 Git、不修改、不提交、不推送。
- 公开仓：从原工程隔离整理并已公开的 `esp32-smart-makeup-cabinet` 目录。
- 原工程不是可用 Git 历史来源；当前文档不把桌面目录、旧文件时间或原始成品称为已发布版本、真机证据或公开构建来源。

## 净化原则

公开仓只保留理解、构建和审查教学源码所需的文件。以下材料不得纳入公开仓：

- Wi-Fi SSID、密码、Token、私网 IP/MAC、位置、个人数据、真实日志、截图 EXIF/GPS；
- `firmware/src/config.local.h`、`wifi_credentials.h`、`credentials.h`、`secrets.h`、`.env*`、`android/local.properties`；
- `.pio/`、`.dart_tool/`、`build/`、`.gradle/`、`.kotlin/`、IDE/编辑器状态、Flutter generated/ephemeral 文件；
- APK/AAB、固件二进制、ELF、Map、archive、私钥、签名文件；
- 未经脱敏/权属确认的实物照片、视频、屏幕截图、EDA、PCB、Gerber、制造文件、客户材料。

公开固件把本地凭据与 experimental opt-in 放入 Git 忽略的 `config.local.h`；仓内仅提供空值示例 `wifi_credentials.example.h`。这不代表原工程从未含任何本机材料，也不代表公开范围之外材料已被审计为安全。

## 来源清点与复算边界

[`source-allowlist.txt`](source-allowlist.txt) 是对原工程可安全复查的相对文件清单；`scripts/source_manifest.py` 只读取该 allowlist，**不读取或哈希可能包含凭据/本机状态**的 `src/config.h`、`src/config.local.h`、`src/wifi_credentials.h` 和 `smart_cabinet_app/android/local.properties`。公开仓新增的空值示例 `firmware/src/wifi_credentials.example.h` 是发布净化资产，不在原工程 allowlist 中。

首次来源核对已只读计算原工程 allowlist 的 SHA-256：`84377e94f2bcfa14f368ccc4354fa2c8f01ff17bb0651342f79d46aac7262b4e`（63 个允许文件）。该摘要只用于证明当时审阅的非敏感源文件集合；它不覆盖被排除文件，也不等同于公开仓、构建、许可证或真机证据。

清单与 manifest 的用途只是审计**选择了哪些安全文件**，不是公开仓构建依赖、许可证证明、真实运行证据或对原目录的全量扫描。执行脚本前应人工确认原始路径、访问授权与 allowlist；不要为了生成 hash 读取被排除的配置。

## 当前未公开或未验证项

截至本文件日期，未提供公开实物照片、演示视频、原理图、PCB、Gerber、制造文件、当前 commit 真机复测、真实网络抓包、真实传感器数据或可公开的成品交付证据。它们缺失时必须继续如实标记为“未提供 / 未复测”，不能用 SVG、CI、模拟数据或旧工程文件替代。
