from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


def read(rel: str) -> str:
    return (ROOT / rel).read_text(encoding='utf-8')


class SourceContracts(unittest.TestCase):
    def test_platform_and_gpio_contract(self):
        self.assertIn('platform = espressif32@6.13.0', read('firmware/platformio.ini'))
        config = read('firmware/src/config.h')
        for value in [
            '#define PIN_DHT11 4', '#define PIN_UV_SENSOR 34', '#define PIN_PIR 5',
            '#define PIN_LED_STRIP 27', '#define PIN_FAN_INA 26', '#define PIN_FAN_INB 33',
            '#define PIN_HUMIDIFIER 25', '#define PIN_LOCK_SERVO 14', '#define PIN_OLED_SDA 21',
            '#define PIN_OLED_SCL 22', '#define PIN_RC522_SCK 18', '#define PIN_RC522_MISO 19',
            '#define PIN_RC522_MOSI 23', '#define PIN_RC522_SS 2', '#define PIN_RC522_RST 15',
        ]:
            self.assertIn(value, config)

    def test_public_default_has_no_network_or_physical_opt_in(self):
        config = read('firmware/src/config.h')
        server = read('firmware/src/http_server.cpp')
        main = read('firmware/src/main.cpp')
        for value in [
            '#define WIFI_SSID ""', '#define WIFI_PASSWORD ""',
            '#define ENABLE_EXPERIMENTAL_ACTUATORS 0',
            '#define ENABLE_EXPERIMENTAL_LOCAL_CONTROL 0',
            '#define ENABLE_EXPERIMENTAL_RFID_MAINTENANCE 0',
            '#if __has_include("config.local.h")',
        ]:
            self.assertIn(value, config)
        self.assertIn("if (WIFI_SSID[0] == '\\0' || !controlEnabled())", server)
        self.assertIn('return EXPERIMENTAL_PHYSICAL_OUTPUTS_ENABLED == 1;', server)
        self.assertIn('#if ENABLE_EXPERIMENTAL_ACTUATORS == 1 && ENABLE_EXPERIMENTAL_LOCAL_CONTROL == 1', config)
        self.assertIn('#define EXPERIMENTAL_PHYSICAL_OUTPUTS_ENABLED 1', config)
        self.assertIn('#define EXPERIMENTAL_PHYSICAL_OUTPUTS_ENABLED 0', config)
        self.assertIn('#if ENABLE_EXPERIMENTAL_RFID_MAINTENANCE == 1', config)
        self.assertIn('#define EXPERIMENTAL_RFID_MAINTENANCE_ENABLED 1', config)
        self.assertIn('#define EXPERIMENTAL_RFID_MAINTENANCE_ENABLED 0', config)
        self.assertNotIn('#if ENABLE_EXPERIMENTAL_ACTUATORS && ENABLE_EXPERIMENTAL_LOCAL_CONTROL', config)
        self.assertIn('WiFi.mode(WIFI_OFF)', server)
        self.assertIn('physical outputs and Wi-Fi/HTTP are disabled without both physical-output opt-ins.', main)
        self.assertIn('local experimental physical outputs and optional local HTTP may be enabled only with non-empty local Wi-Fi credentials.', main)
        self.assertNotIn('Serial.println(WIFI_SSID)', server)
        self.assertNotIn('Serial.println(WIFI_PASSWORD)', server)

    def test_inert_actuator_and_latch_semantics_are_not_physical_feedback(self):
        actuators = read('firmware/src/actuators.cpp')
        api = read('firmware/src/http_server.cpp')
        self.assertIn('#if EXPERIMENTAL_PHYSICAL_OUTPUTS_ENABLED', actuators)
        self.assertNotIn('#if ENABLE_EXPERIMENTAL_ACTUATORS', actuators)
        self.assertIn('physical outputs disabled', actuators)
        self.assertIn('both local physical-output opt-ins are exactly enabled', read('firmware/src/actuators.h'))
        self.assertIn('Servo command suppressed without both physical-output opt-ins', actuators)
        # Only inspect initActuators()'s public branch, not opt-in code elsewhere.
        init_actuators = actuators.split('void initActuators() {', 1)[1].split('static void applyLEDOutput()', 1)[0]
        public_init = init_actuators.split('#else', 1)[1].split('#endif', 1)[0]
        for pin in ['PIN_FAN_INA', 'PIN_FAN_INB', 'PIN_HUMIDIFIER', 'PIN_LOCK_SERVO', 'PIN_LED_STRIP']:
            self.assertIn(f'pinMode({pin}, INPUT)', public_init)
        for forbidden in ['digitalWrite(', 'OUTPUT', 'FastLED.addLeds', 'FastLED.show', 'lockServo.attach']:
            self.assertNotIn(forbidden, public_init)
        self.assertIn('not physical feedback', actuators)
        self.assertIn('document["publicDefaultInert"]', api)
        self.assertIn('document["localControlAndActuatorsEnabled"] = controlEnabled()', api)
        self.assertIn('document["latchCommandedClosed"]', api)
        self.assertIn('document["physicalPositionVerified"] = false', api)
        self.assertIn('server.on("/api/latch", HTTP_POST, handleLatch)', api)
        self.assertIn('physicalPositionVerified\\":false', api)
        self.assertNotIn('server.on("/api/lock"', api)

    def test_sensor_unavailability_is_explicit_not_zero(self):
        sensors = read('firmware/src/sensors.cpp')
        api = read('firmware/src/http_server.cpp')
        model = read('mobile/smart_cabinet_app/lib/models/sensor_data.dart')
        self.assertIn('currentData.climateAvailable = !isnan(hum) && !isnan(temp)', sensors)
        self.assertIn('currentData.humidity = NAN', sensors)
        self.assertIn('currentData.temperature = NAN', sensors)
        self.assertIn('document["climate"] = data.climateAvailable ? "available" : "unavailable"', api)
        self.assertIn('document["temp"] = nullptr', api)
        self.assertIn('document["humid"] = nullptr', api)
        self.assertIn('document["uv"] = nullptr', api)
        self.assertIn("climate != 'available' && climate != 'unavailable'", model)
        self.assertIn("climate == 'unavailable' && (temperature != null || humidity != null)", model)
        self.assertIn("!uvEstimate && uv != null", model)

    def test_rfid_uid_is_not_a_public_http_credential_or_response(self):
        rfid = read('firmware/src/rfid.cpp')
        api = read('firmware/src/http_server.cpp')
        docs = read('docs/PROTOCOL.md')
        main = read('firmware/src/main.cpp')
        self.assertIn('UID is cloneable', rfid)
        self.assertNotIn('authenticateAndOpen', rfid)
        self.assertNotIn('authenticateAndOpen', main)
        self.assertNotIn('setLock(', rfid)
        self.assertIn('never commands the latch', main)
        self.assertIn('raw_rfid_scan_not_available_over_public_http', api)
        self.assertIn('raw_rfid_maintenance_not_available_over_public_http', api)
        self.assertIn('item["name"]', api)
        self.assertIn('item["brand"]', api)
        self.assertNotIn('item["uid"]', api)
        self.assertIn('不含 UID', docs)
        self.assertIn('绝不通过公开 HTTP 回显原始 UID', docs)
        self.assertIn('#if EXPERIMENTAL_RFID_MAINTENANCE_ENABLED', rfid)
        self.assertNotIn('#if ENABLE_EXPERIMENTAL_RFID_MAINTENANCE', rfid)
        self.assertIn('#if EXPERIMENTAL_RFID_MAINTENANCE_ENABLED && EXPERIMENTAL_PHYSICAL_OUTPUTS_ENABLED', api)

    def test_flutter_uses_final_api_contract_and_narrow_network_boundary(self):
        service = read('mobile/smart_cabinet_app/lib/services/cabinet_service.dart')
        provider = read('mobile/smart_cabinet_app/lib/providers/cabinet_provider.dart')
        status = read('mobile/smart_cabinet_app/lib/models/device_status.dart')
        self.assertIn('normalizeLocalHttpUrl', service)
        self.assertIn("uri.scheme != 'http'", service)
        self.assertIn('uri.userInfo.isNotEmpty', service)
        self.assertIn('uri.hasQuery', service)
        self.assertIn('uri.hasFragment', service)
        self.assertIn('isPrivateOrLoopbackHost(uri.host)', service)
        self.assertIn("'/api/latch'", service)
        self.assertIn("result['accepted'] != true", service)
        self.assertNotIn("'/api/lock'", service)
        self.assertIn('Future<void> testConnection()', provider)
        self.assertIn('_service.getSensors()', provider)
        self.assertIn('_service.getStatus()', provider)
        self.assertIn('_clearRemoteState()', provider)
        self.assertNotIn('Timer.periodic', provider)
        self.assertNotIn('startPolling', provider)
        fixtures = read('mobile/smart_cabinet_app/test/widget_test.dart')
        for value in [
            'publicDefaultInert', 'localControlAndActuatorsEnabled', 'ledCommandedOn', 'fanCommandedOn', 'humidifierCommandedOn',
            'latchCommandedClosed', 'physicalPositionVerified',
        ]:
            self.assertIn(value, status)
            self.assertIn(value, fixtures)

    def test_public_docs_keep_honest_boundaries(self):
        readme = read('README.md')
        security = read('SECURITY.md')
        protocol = read('docs/PROTOCOL.md')
        status = read('docs/PROJECT_STATUS.md')
        for value in ['当前真机复测 | 未执行', '实物照片、演示视频、原理图、PCB、Gerber', '不是化妆品质量、真假、过期真实性']:
            self.assertIn(value, readme)
        self.assertIn('门禁、电子锁、防盗、身份认证、访问控制', security)
        self.assertIn('禁止公网暴露、端口转发、公共 Wi-Fi、共享热点或不可信网络', security)
        self.assertIn('公开默认不开 Wi-Fi/HTTP', protocol)
        self.assertIn('`accepted:true`', protocol)
        self.assertIn('| 真机复测 | **未执行**当前公开 commit 的日期化复测', status)

    def test_mobile_release_network_and_macos_entitlements_are_minimized(self):
        android_release = read('mobile/smart_cabinet_app/android/app/src/main/AndroidManifest.xml')
        android_debug = read('mobile/smart_cabinet_app/android/app/src/debug/AndroidManifest.xml')
        android_profile = read('mobile/smart_cabinet_app/android/app/src/profile/AndroidManifest.xml')
        macos_debug = read('mobile/smart_cabinet_app/macos/Runner/DebugProfile.entitlements')
        self.assertNotIn('android.permission.INTERNET', android_release)
        self.assertNotIn('usesCleartextTraffic', android_release)
        self.assertIn('android.permission.INTERNET', android_debug)
        self.assertIn('usesCleartextTraffic="true"', android_debug)
        self.assertIn('usesCleartextTraffic="true"', android_profile)
        self.assertNotIn('com.apple.security.network.server', macos_debug)

    def test_gate_scripts_and_ci_cover_all_build_domains(self):
        verify = read('scripts/verify.sh')
        ci = read('.github/workflows/validate.yml')
        for value in [
            'secret_scan.py', 'check_repo.py', 'unittest discover', 'pio run',
            'flutter pub get --enforce-lockfile', 'dart format --output=none --set-exit-if-changed lib test',
            'flutter test', 'flutter analyze', 'flutter build apk --debug',
        ]:
            self.assertIn(value, verify)
            self.assertIn(value, ci)
        self.assertIn('rm -rf', verify)
        self.assertIn('esp32dev-experimental-compile', verify)
        self.assertIn('esp32dev-experimental-compile', ci)
        self.assertIn('esp32dev-actuators-only-compile', verify)
        self.assertIn('esp32dev-actuators-only-compile', ci)
        platformio = read('firmware/platformio.ini')
        self.assertIn('[env:esp32dev-experimental-compile]', platformio)
        self.assertIn('[env:esp32dev-actuators-only-compile]', platformio)
        for flag in [
            '-D ENABLE_EXPERIMENTAL_ACTUATORS=1',
            '-D ENABLE_EXPERIMENTAL_LOCAL_CONTROL=1',
            '-D ENABLE_EXPERIMENTAL_RFID_MAINTENANCE=1',
            '-D ENABLE_EXPERIMENTAL_LOCAL_CONTROL=0',
        ]:
            self.assertIn(flag, platformio)
        self.assertNotIn('actions/upload-artifact', ci)
        self.assertNotIn('firmware.bin', ci)
        self.assertNotIn('app-debug.apk', ci)
        self.assertIn('CI 也不上传这些构建产物', read('THIRD_PARTY_NOTICES.md'))

    def test_clean_checkout_entrypoints_are_not_ignored(self):
        android_ignore = read('mobile/smart_cabinet_app/android/.gitignore')
        ios_ignore = read('mobile/smart_cabinet_app/ios/.gitignore')
        for ignored in ['gradle-wrapper.jar', '/gradlew', '/gradlew.bat', 'GeneratedPluginRegistrant.java']:
            self.assertNotIn(ignored, android_ignore)
        self.assertNotIn('Runner/GeneratedPluginRegistrant.*', ios_ignore)
        check = read('scripts/check_repo.py')
        self.assertIn('required publish file is not tracked', check)
        self.assertIn('gradle-wrapper.jar', check)
        self.assertIn('GeneratedPluginRegistrant.java', check)

    def test_mobile_service_credentials_are_ignored_and_scanned(self):
        ignore = read('.gitignore')
        scan = read('scripts/secret_scan.py')
        check = read('scripts/check_repo.py')
        for value in ['google-services.json', 'GoogleService-Info.plist', '*.xcuserstate']:
            self.assertIn(value, ignore)
        for value in ['google-services.json', 'GoogleService-Info.plist']:
            self.assertIn(value, scan)
            self.assertIn(value, check)

    def test_source_manifest_excludes_local_config(self):
        manifest = read('scripts/source_manifest.py')
        allowlist = read('docs/source-allowlist.txt')
        for value in [
            "'src/config.h'", "'src/config.local.h'",
            "'src/wifi_credentials.h'", "'smart_cabinet_app/android/local.properties'",
        ]:
            self.assertIn(value, manifest)
        self.assertNotIn('src/config.h\n', allowlist)
        self.assertNotIn('src/config.local.h\n', allowlist)
        self.assertNotIn('src/wifi_credentials.h\n', allowlist)


if __name__ == '__main__':
    unittest.main()
