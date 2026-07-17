import 'package:flutter/foundation.dart';
import 'package:shared_preferences/shared_preferences.dart';

import '../config/api_config.dart';
import '../models/app_settings.dart';
import '../models/cosmetic.dart';
import '../models/device_status.dart';
import '../models/sensor_data.dart';
import '../services/cabinet_service.dart';

enum CabinetConnectionStatus {
  unconfigured,
  configured,
  checking,
  reachable,
  failed,
}

/// Session-scoped state. It intentionally has no background polling: every
/// request is caused by an explicit user action and a failed/new address clears
/// old firmware reports rather than making them look current.
class CabinetProvider extends ChangeNotifier {
  CabinetProvider(this._service) {
    _connectionStatus = _service.isConfigured
        ? CabinetConnectionStatus.configured
        : CabinetConnectionStatus.unconfigured;
  }

  final CabinetService _service;

  SensorData? sensors;
  DeviceStatus? status;
  List<Cosmetic> cosmetics = const [];
  AppSettings? settings;
  String? lastError;
  bool isRefreshing = false;
  bool _experimentalControlAcknowledged = false;
  late CabinetConnectionStatus _connectionStatus;

  CabinetService get service => _service;
  String get baseUrl => _service.baseUrl;
  CabinetConnectionStatus get connectionStatus => _connectionStatus;
  bool get isSessionReachable =>
      _connectionStatus == CabinetConnectionStatus.reachable;
  bool get experimentalControlAcknowledged => _experimentalControlAcknowledged;
  bool get canSendExperimentalCommands =>
      isSessionReachable &&
      _experimentalControlAcknowledged &&
      status?.localControlAndActuatorsEnabled == true;

  String get connectionLabel => switch (_connectionStatus) {
    CabinetConnectionStatus.unconfigured => '未配置地址；默认不会联网',
    CabinetConnectionStatus.configured => '地址已保存；本会话尚未验证',
    CabinetConnectionStatus.checking => '正在读取当前固件报告…',
    CabinetConnectionStatus.reachable => '本会话的 HTTP 读取成功',
    CabinetConnectionStatus.failed => '最近一次读取失败；旧报告已清空',
  };

  void _clearRemoteState() {
    sensors = null;
    status = null;
    cosmetics = const [];
    settings = null;
    _experimentalControlAcknowledged = false;
  }

  Future<void> loadSavedBaseUrl() async {
    final prefs = await SharedPreferences.getInstance();
    final saved =
        prefs.getString(ApiConfig.prefsKeyBaseUrl) ?? ApiConfig.defaultBaseUrl;
    try {
      _service.baseUrl = saved;
    } on CabinetException {
      await prefs.remove(ApiConfig.prefsKeyBaseUrl);
      _service.baseUrl = ApiConfig.defaultBaseUrl;
    }
    _connectionStatus = _service.isConfigured
        ? CabinetConnectionStatus.configured
        : CabinetConnectionStatus.unconfigured;
    _clearRemoteState();
    lastError = null;
    notifyListeners();
  }

  /// Persists an address but never requests it. A separate explicit test is
  /// required for this session before any current report is displayed.
  Future<void> saveBaseUrl(String rawUrl) async {
    final normalized = CabinetService.normalizeLocalHttpUrl(rawUrl);
    final changed = normalized != _service.baseUrl;
    _service.baseUrl = normalized;
    final prefs = await SharedPreferences.getInstance();
    if (normalized.isEmpty) {
      await prefs.remove(ApiConfig.prefsKeyBaseUrl);
    } else {
      await prefs.setString(ApiConfig.prefsKeyBaseUrl, normalized);
    }
    if (changed || !isSessionReachable) _clearRemoteState();
    _connectionStatus = normalized.isEmpty
        ? CabinetConnectionStatus.unconfigured
        : CabinetConnectionStatus.configured;
    lastError = null;
    notifyListeners();
  }

  Future<void> testConnection() async {
    if (!_service.isConfigured) {
      lastError = '请先保存可信局域网中的 HTTP 地址。';
      _connectionStatus = CabinetConnectionStatus.unconfigured;
      notifyListeners();
      return;
    }
    _connectionStatus = CabinetConnectionStatus.checking;
    isRefreshing = true;
    lastError = null;
    _clearRemoteState();
    notifyListeners();
    try {
      final results = await Future.wait<Object>([
        _service.getSensors(),
        _service.getStatus(),
      ]);
      sensors = results[0] as SensorData;
      status = results[1] as DeviceStatus;
      _connectionStatus = CabinetConnectionStatus.reachable;
    } catch (error) {
      _clearRemoteState();
      _connectionStatus = CabinetConnectionStatus.failed;
      lastError = error.toString();
    } finally {
      isRefreshing = false;
      notifyListeners();
    }
  }

  Future<void> refreshDashboard() => testConnection();

  void setExperimentalControlAcknowledged(bool accepted) {
    _experimentalControlAcknowledged = accepted && isSessionReachable;
    notifyListeners();
  }

  Future<void> refreshCosmetics() async {
    if (!isSessionReachable) return;
    try {
      cosmetics = await _service.getCosmeticList();
      lastError = null;
    } catch (error) {
      lastError = error.toString();
    }
    notifyListeners();
  }

  Future<void> refreshSettings() async {
    if (!isSessionReachable) return;
    try {
      settings = await _service.getSettings();
      lastError = null;
    } catch (error) {
      lastError = error.toString();
    }
    notifyListeners();
  }

  Future<void> applySettings(AppSettings value) async {
    if (!canSendExperimentalCommands) {
      throw CabinetException('当前会话未满足实验性写入门槛。');
    }
    await _service.saveSettings(value);
    await testConnection();
  }

  @override
  void dispose() {
    _service.dispose();
    super.dispose();
  }
}
