import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;

import '../models/app_settings.dart';
import '../models/cosmetic.dart';
import '../models/device_status.dart';
import '../models/sensor_data.dart';

class CabinetException implements Exception {
  CabinetException(this.message, [this.statusCode]);
  final String message;
  final int? statusCode;

  @override
  String toString() => message;
}

/// A deliberately narrow client for a personally supervised ESP32 bench test.
///
/// It permits only a plain HTTP origin on a private/loopback LAN. That is a
/// user-error guard, not TLS, authentication, authorization, or device
/// identity. The caller must make the user explicitly test the address before
/// presenting any action UI.
class CabinetService {
  CabinetService(String initialBaseUrl, {http.Client? client})
    : _client = client ?? http.Client() {
    baseUrl = initialBaseUrl;
  }

  final http.Client _client;
  String _baseUrl = '';

  static const _kQueryTimeout = Duration(seconds: 8);
  static const _kControlTimeout = Duration(seconds: 12);

  String get baseUrl => _baseUrl;
  bool get isConfigured => _baseUrl.isNotEmpty;

  set baseUrl(String value) {
    _baseUrl = normalizeLocalHttpUrl(value);
  }

  static String normalizeLocalHttpUrl(String raw) {
    final trimmed = raw.trim();
    if (trimmed.isEmpty) return '';
    final candidate = trimmed.contains('://') ? trimmed : 'http://$trimmed';
    final uri = Uri.tryParse(candidate);
    if (uri == null ||
        uri.scheme != 'http' ||
        uri.host.isEmpty ||
        uri.userInfo.isNotEmpty ||
        uri.hasQuery ||
        uri.hasFragment ||
        (uri.path.isNotEmpty && uri.path != '/') ||
        uri.port == 0 ||
        !isPrivateOrLoopbackHost(uri.host)) {
      throw CabinetException(
        '地址必须是可信局域网中的纯 HTTP 主机地址，例如 http://192.168.1.50；不接受路径、账号、查询参数或公网地址。',
      );
    }
    final port = uri.hasPort ? ':${uri.port}' : '';
    return 'http://${uri.host}$port';
  }

  static bool isPrivateOrLoopbackHost(String host) {
    final lower = host.toLowerCase();
    if (lower == 'localhost' || lower == '::1') return true;
    final parts = lower.split('.');
    if (parts.length != 4) return false;
    final octets = parts.map(int.tryParse).toList();
    if (octets.any((value) => value == null || value < 0 || value > 255)) {
      return false;
    }
    final first = octets[0]!;
    final second = octets[1]!;
    return first == 10 ||
        first == 127 ||
        (first == 192 && second == 168) ||
        (first == 172 && second >= 16 && second <= 31);
  }

  Map<String, String> get _jsonHeaders => const {
    'Content-Type': 'application/json; charset=utf-8',
    'Accept': 'application/json',
  };

  Uri _uri(String path) {
    if (!isConfigured) {
      throw CabinetException('尚未配置设备地址；默认不会发送网络请求。');
    }
    return Uri.parse('$_baseUrl$path');
  }

  Map<String, dynamic> _decodeObject(http.Response response, String action) {
    if (response.statusCode < 200 || response.statusCode >= 300) {
      String detail = '';
      try {
        final parsed = jsonDecode(response.body);
        if (parsed is Map && parsed['error'] != null) {
          detail = '：${parsed['error']}';
        }
      } catch (_) {}
      throw CabinetException(
        '$action失败（HTTP ${response.statusCode}）$detail',
        response.statusCode,
      );
    }
    try {
      final decoded = jsonDecode(response.body);
      if (decoded is! Map<String, dynamic>) {
        throw const FormatException('响应不是 JSON 对象。');
      }
      return decoded;
    } on FormatException catch (error) {
      throw CabinetException('$action返回了无法识别的响应：${error.message}');
    } catch (_) {
      throw CabinetException('$action返回了无法识别的 JSON。');
    }
  }

  Future<SensorData> getSensors() async {
    final response = await _client
        .get(_uri('/api/sensors'))
        .timeout(_kQueryTimeout);
    return SensorData.fromJson(_decodeObject(response, '读取传感器报告'));
  }

  Future<DeviceStatus> getStatus() async {
    final response = await _client
        .get(_uri('/api/status'))
        .timeout(_kQueryTimeout);
    return DeviceStatus.fromJson(_decodeObject(response, '读取固件状态报告'));
  }

  Future<void> _postAccepted(
    String path,
    Map<String, dynamic> body,
    String action,
  ) async {
    final response = await _client
        .post(_uri(path), headers: _jsonHeaders, body: jsonEncode(body))
        .timeout(_kControlTimeout);
    final result = _decodeObject(response, action);
    if (result['accepted'] != true) {
      throw CabinetException('$action未被固件接受。');
    }
  }

  Future<void> setLED({bool? power, int? brightness, Color? color}) async {
    final body = <String, dynamic>{};
    if (power != null) body['power'] = power;
    if (brightness != null) body['brightness'] = brightness.clamp(0, 255);
    if (color != null) {
      body['color'] = [
        (color.r * 255).round().clamp(0, 255),
        (color.g * 255).round().clamp(0, 255),
        (color.b * 255).round().clamp(0, 255),
      ];
    }
    if (body.isEmpty) throw CabinetException('请至少填写一个灯带命令字段。');
    await _postAccepted('/api/led', body, '发送灯带实验请求');
  }

  Future<void> setFan(bool on) =>
      _postAccepted('/api/fan', {'power': on, 'manual': true}, '发送风扇实验请求');

  Future<void> setHumidifier(bool on) => _postAccepted('/api/humidifier', {
    'power': on,
    'manual': true,
  }, '发送加湿器实验请求');

  Future<void> setLatch({required bool open}) => _postAccepted('/api/latch', {
    'action': open ? 'open' : 'close',
  }, '发送舵机插销实验请求');

  Future<List<Cosmetic>> getCosmeticList() async {
    final response = await _client
        .get(_uri('/api/rfid/list'))
        .timeout(_kQueryTimeout);
    if (response.statusCode < 200 || response.statusCode >= 300) {
      _decodeObject(response, '读取本地物品记录');
    }
    try {
      final decoded = jsonDecode(response.body);
      if (decoded is! List) throw const FormatException('响应不是 JSON 数组。');
      return decoded
          .map((item) {
            if (item is! Map<String, dynamic>) {
              throw const FormatException('记录不是 JSON 对象。');
            }
            return Cosmetic.fromJson(item);
          })
          .toList(growable: false);
    } on FormatException catch (error) {
      throw CabinetException('本地物品记录格式无效：${error.message}');
    } catch (_) {
      throw CabinetException('本地物品记录不是有效 JSON。');
    }
  }

  Future<AppSettings> getSettings() async {
    final response = await _client
        .get(_uri('/api/settings'))
        .timeout(_kQueryTimeout);
    return AppSettings.fromJson(_decodeObject(response, '读取阈值报告'));
  }

  Future<void> saveSettings(AppSettings settings) =>
      _postAccepted('/api/settings', settings.toJson(), '发送阈值实验请求');

  void dispose() => _client.close();
}
