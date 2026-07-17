class Cosmetic {
  const Cosmetic({
    required this.name,
    required this.brand,
    required this.expireDate,
    required this.expiry,
  });

  final String name;
  final String brand;
  final String expireDate;

  /// Firmware-derived inventory label, not an authentication result and not a
  /// guarantee that a physical item is present.
  final String expiry;

  factory Cosmetic.fromJson(Map<String, dynamic> json) {
    String stringField(String key) {
      final value = json[key];
      if (value is! String) {
        throw FormatException('化妆品记录字段无效：$key');
      }
      return value;
    }

    final expiry = stringField('expiry');
    const allowedExpiry = {
      'not_set',
      'valid',
      'expired',
      'time_unavailable',
      'invalid_record',
    };
    if (!allowedExpiry.contains(expiry)) {
      throw const FormatException('化妆品记录中的到期状态无效。');
    }
    return Cosmetic(
      name: stringField('name'),
      brand: stringField('brand'),
      expireDate: stringField('expireDate'),
      expiry: expiry,
    );
  }
}
