class AppSettings {
  const AppSettings({
    required this.tempThresholdHigh,
    required this.tempThresholdLow,
    required this.humidThresholdHigh,
    required this.humidThresholdLow,
    required this.lightTimeout,
  });

  final int tempThresholdHigh;
  final int tempThresholdLow;
  final int humidThresholdHigh;
  final int humidThresholdLow;
  final int lightTimeout;

  factory AppSettings.fromJson(Map<String, dynamic> json) {
    int requiredInt(String key) {
      final value = json[key];
      if (value is! num || value.isNaN || value.isInfinite) {
        throw FormatException('固件阈值报告缺少或包含无效字段：$key');
      }
      return value.toInt();
    }

    return AppSettings(
      tempThresholdHigh: requiredInt('tempThresholdHigh'),
      tempThresholdLow: requiredInt('tempThresholdLow'),
      humidThresholdHigh: requiredInt('humidThresholdHigh'),
      humidThresholdLow: requiredInt('humidThresholdLow'),
      lightTimeout: requiredInt('lightTimeout'),
    );
  }

  Map<String, dynamic> toJson() => {
    'tempThresholdHigh': tempThresholdHigh,
    'tempThresholdLow': tempThresholdLow,
    'humidThresholdHigh': humidThresholdHigh,
    'humidThresholdLow': humidThresholdLow,
    'lightTimeout': lightTimeout,
  };
}
