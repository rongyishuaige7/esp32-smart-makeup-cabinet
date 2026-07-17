class SensorData {
  const SensorData({
    required this.climateAvailable,
    required this.temperature,
    required this.humidity,
    required this.uvEstimate,
    required this.uvValue,
    required this.pirDetected,
  });

  final bool climateAvailable;
  final double? temperature;
  final double? humidity;
  final bool uvEstimate;
  final int? uvValue;
  final bool pirDetected;

  factory SensorData.fromJson(Map<String, dynamic> json) {
    final climate = json['climate'];
    final uvEstimate = json['uvEstimate'];
    final pir = json['pir'];
    if (climate is! String ||
        (climate != 'available' && climate != 'unavailable') ||
        uvEstimate is! bool ||
        pir is! bool) {
      throw const FormatException('固件传感器报告格式无效。');
    }

    final temperature = json['temp'];
    final humidity = json['humid'];
    final uv = json['uv'];
    if (climate == 'available' && (temperature is! num || humidity is! num)) {
      throw const FormatException('固件声明气候数据可用，但数值格式无效。');
    }
    if (climate == 'unavailable' && (temperature != null || humidity != null)) {
      throw const FormatException('固件声明气候数据不可用，但返回了数值。');
    }
    if (uvEstimate && uv is! num) {
      throw const FormatException('固件声明 UV 估算可用，但数值格式无效。');
    }
    if (!uvEstimate && uv != null) {
      throw const FormatException('固件声明 UV 估算不可用，但返回了数值。');
    }

    return SensorData(
      climateAvailable: climate == 'available',
      temperature: temperature is num ? temperature.toDouble() : null,
      humidity: humidity is num ? humidity.toDouble() : null,
      uvEstimate: uvEstimate,
      uvValue: uv is num ? uv.toInt() : null,
      pirDetected: pir,
    );
  }
}
