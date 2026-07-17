class DeviceStatus {
  const DeviceStatus({
    required this.publicDefaultInert,
    required this.localControlAndActuatorsEnabled,
    required this.ledCommandedOn,
    required this.fanCommandedOn,
    required this.fanManual,
    required this.humidifierCommandedOn,
    required this.humidifierManual,
    required this.latchCommandedClosed,
    required this.physicalPositionVerified,
  });

  /// `true` means the firmware intentionally keeps experimental network and
  /// actuator control inactive. It is not a connectivity-health signal.
  final bool publicDefaultInert;

  /// `true` only when both local-control and actuator experiment switches are
  /// enabled in the firmware. It is not a device-health or physical-output
  /// confirmation, and RFID maintenance is reported separately from this flag.
  final bool localControlAndActuatorsEnabled;

  final bool ledCommandedOn;
  final bool fanCommandedOn;
  final bool fanManual;
  final bool humidifierCommandedOn;
  final bool humidifierManual;
  final bool latchCommandedClosed;

  /// The public firmware always reports false: this design has no position
  /// feedback, current sensing, or other physical execution confirmation.
  final bool physicalPositionVerified;

  factory DeviceStatus.fromJson(Map<String, dynamic> json) {
    bool requiredBool(String key) {
      final value = json[key];
      if (value is! bool) {
        throw FormatException('固件报告缺少或包含无效字段：$key');
      }
      return value;
    }

    return DeviceStatus(
      publicDefaultInert: requiredBool('publicDefaultInert'),
      localControlAndActuatorsEnabled: requiredBool(
        'localControlAndActuatorsEnabled',
      ),
      ledCommandedOn: requiredBool('ledCommandedOn'),
      fanCommandedOn: requiredBool('fanCommandedOn'),
      fanManual: requiredBool('fanManual'),
      humidifierCommandedOn: requiredBool('humidifierCommandedOn'),
      humidifierManual: requiredBool('humidifierManual'),
      latchCommandedClosed: requiredBool('latchCommandedClosed'),
      physicalPositionVerified: requiredBool('physicalPositionVerified'),
    );
  }
}
