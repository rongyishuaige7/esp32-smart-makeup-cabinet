# 第三方组件、来源与许可证提示

本文件仅列出当前公开源码中可辨认的直接构建依赖与本机缓存可复核的许可线索；它不是完整 SBOM、法律意见或二进制再分发许可清单。使用者在构建、修改、分发或发布二进制前必须自行复核当前上游版本、传递依赖、NOTICE 和许可证条件。

本仓不分发 `.pio`、pub 缓存、APK/AAB、固件二进制或 SDK；CI 也不上传这些构建产物。PlatformIO、Arduino framework、Flutter SDK 和库依赖均由构建环境从上游获取。

## ESP32 / PlatformIO 直接库

| 组件 | `platformio.ini` 固定版本 | 当前许可线索 | 上游 |
| :-- | :-- | :-- | :-- |
| PlatformIO Core | 6.1.19（本项目构建门禁） | Apache-2.0 | https://github.com/platformio/platformio-core |
| Espressif32 Platform | 6.13.0 | Apache-2.0 | https://github.com/platformio/platform-espressif32 |
| Arduino-ESP32 framework | 由 PlatformIO 解析，未随仓分发 | LGPL-2.1-or-later（以上游当前 LICENSE 为准） | https://github.com/espressif/arduino-esp32 |
| Adafruit GFX Library | 1.12.6 | BSD | https://github.com/adafruit/Adafruit-GFX-Library |
| Adafruit SSD1306 | 2.5.17 | BSD-3-Clause | https://github.com/adafruit/Adafruit_SSD1306 |
| DHT sensor library | 1.4.7 | MIT | https://github.com/adafruit/DHT-sensor-library |
| Adafruit Unified Sensor（传递依赖） | 1.1.15（本机构建缓存记录） | Apache-2.0 | https://github.com/adafruit/Adafruit_Sensor |
| Adafruit BusIO（传递依赖） | 1.17.4（本机构建缓存记录） | MIT | https://github.com/adafruit/Adafruit_BusIO |
| MFRC522 | 1.4.12 | Unlicense | https://github.com/miguelbalboa/rfid |
| FastLED | 3.10.3 | MIT | https://github.com/FastLED/FastLED |
| ArduinoJson | 6.21.6 | MIT | https://github.com/bblanchon/ArduinoJson |
| ESP32Servo | 3.2.1 | LGPL-2.1-or-later；本机构建缓存的 README 内含该声明，但缓存缺少独立 LICENSE 文件，发布二进制前仍应复核上游 tag | https://github.com/madhephaestus/ESP32Servo |

`espressif32@6.13.0`、Arduino framework、ESP32 core、WebServer、WiFi、Preferences 和其他传递包的精确许可链由 PlatformIO 上游管理；发布衍生物前应重新核验。

## Flutter 直接包

| 组件 | `pubspec.lock` 当前解析版本 | 当前许可线索 | 上游 |
| :-- | :-- | :-- | :-- |
| cupertino_icons | 1.0.8 | MIT | https://pub.dev/packages/cupertino_icons |
| flex_color_picker | 3.8.0 | BSD-3-Clause | https://pub.dev/packages/flex_color_picker |
| flutter_lints | 6.0.0 | BSD-3-Clause | https://pub.dev/packages/flutter_lints |
| http | 1.6.0 | BSD-3-Clause | https://pub.dev/packages/http |
| provider | 6.1.5+1 | MIT | https://pub.dev/packages/provider |
| shared_preferences | 2.5.4 | BSD-3-Clause | https://pub.dev/packages/shared_preferences |

Flutter、Dart、Android Gradle Plugin、Gradle、Kotlin、iOS/macOS/Linux/Windows runner 及其传递依赖受各自上游条款约束。构建能通过不等于可任意再分发。

## 本项目许可

Rongyi 自有的候选源码、文档、BOM 与接线边界图按 [MIT License](LICENSE) 发布。第三方名称、商标和许可证仍归各自权利人所有；MIT 不覆盖第三方组件或其二进制产物。
