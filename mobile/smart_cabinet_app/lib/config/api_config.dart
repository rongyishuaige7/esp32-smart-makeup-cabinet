/// The app starts without a device address. Set a trusted local ESP32 HTTP URL
/// in Settings after verifying the device and local network.
class ApiConfig {
  ApiConfig._();

  static const String prefsKeyBaseUrl = 'esp32_base_url';
  static const String defaultBaseUrl = '';
}
