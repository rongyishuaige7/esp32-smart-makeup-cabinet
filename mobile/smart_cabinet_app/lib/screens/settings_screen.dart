import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../models/app_settings.dart';
import '../providers/cabinet_provider.dart';
import '../services/cabinet_service.dart';

class SettingsScreen extends StatefulWidget {
  const SettingsScreen({super.key});

  @override
  State<SettingsScreen> createState() => _SettingsScreenState();
}

class _SettingsScreenState extends State<SettingsScreen> {
  late final TextEditingController _urlController;
  final _tempHighController = TextEditingController(text: '28');
  final _tempLowController = TextEditingController(text: '25');
  final _humidityHighController = TextEditingController(text: '85');
  final _humidityLowController = TextEditingController(text: '75');
  final _lightTimeoutController = TextEditingController(text: '10');

  @override
  void initState() {
    super.initState();
    _urlController = TextEditingController(
      text: context.read<CabinetProvider>().baseUrl,
    );
  }

  @override
  void dispose() {
    _urlController.dispose();
    _tempHighController.dispose();
    _tempLowController.dispose();
    _humidityHighController.dispose();
    _humidityLowController.dispose();
    _lightTimeoutController.dispose();
    super.dispose();
  }

  Future<void> _saveAddress() async {
    final provider = context.read<CabinetProvider>();
    try {
      await provider.saveBaseUrl(_urlController.text);
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('地址已保存；尚未联网。请点击“读取当前固件报告”进行本会话验证。')),
        );
      }
    } on CabinetException catch (error) {
      if (mounted) {
        ScaffoldMessenger.of(
          context,
        ).showSnackBar(SnackBar(content: Text(error.message)));
      }
    }
  }

  Future<void> _loadSettings() async {
    final provider = context.read<CabinetProvider>();
    await provider.refreshSettings();
    final value = provider.settings;
    if (value == null || !mounted) return;
    setState(() {
      _tempHighController.text = value.tempThresholdHigh.toString();
      _tempLowController.text = value.tempThresholdLow.toString();
      _humidityHighController.text = value.humidThresholdHigh.toString();
      _humidityLowController.text = value.humidThresholdLow.toString();
      _lightTimeoutController.text = value.lightTimeout.toString();
    });
  }

  Future<void> _sendSettings() async {
    final provider = context.read<CabinetProvider>();
    try {
      final value = AppSettings(
        tempThresholdHigh: int.parse(_tempHighController.text.trim()),
        tempThresholdLow: int.parse(_tempLowController.text.trim()),
        humidThresholdHigh: int.parse(_humidityHighController.text.trim()),
        humidThresholdLow: int.parse(_humidityLowController.text.trim()),
        lightTimeout: int.parse(_lightTimeoutController.text.trim()),
      );
      await provider.applySettings(value);
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('阈值请求已被固件接受；不代表自动控制或物理效果已经验证。')),
        );
      }
    } on CabinetException catch (error) {
      if (mounted) {
        ScaffoldMessenger.of(
          context,
        ).showSnackBar(SnackBar(content: Text(error.message)));
      }
    } on FormatException {
      if (mounted) {
        ScaffoldMessenger.of(
          context,
        ).showSnackBar(const SnackBar(content: Text('阈值必须是整数。')));
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Consumer<CabinetProvider>(
      builder: (context, provider, _) => ListView(
        padding: const EdgeInsets.all(16),
        children: [
          Text('连接', style: Theme.of(context).textTheme.titleLarge),
          const SizedBox(height: 8),
          const Text(
            '默认地址为空，保存地址不会发起网络请求。仅接受可信局域网中的 HTTP 主机地址；不接受公网、路径、账号、查询参数或 HTTPS。',
          ),
          const SizedBox(height: 12),
          TextField(
            controller: _urlController,
            keyboardType: TextInputType.url,
            decoration: const InputDecoration(
              labelText: 'ESP32 局域网地址',
              hintText: 'http://192.168.1.50',
              helperText: 'HTTP 明文、无 TLS、无认证、无设备身份验证',
              border: OutlineInputBorder(),
            ),
          ),
          const SizedBox(height: 8),
          FilledButton.icon(
            onPressed: _saveAddress,
            icon: const Icon(Icons.save_outlined),
            label: const Text('仅保存地址（不联网）'),
          ),
          const SizedBox(height: 8),
          OutlinedButton.icon(
            onPressed: provider.isRefreshing ? null : provider.testConnection,
            icon: const Icon(Icons.network_ping),
            label: const Text('读取当前固件报告'),
          ),
          const Divider(height: 36),
          Text('实验性阈值请求', style: Theme.of(context).textTheme.titleLarge),
          const SizedBox(height: 6),
          const Text(
            '阈值只是源码参数，不是环境校准、安全阈值或自动控制承诺。写入仅在本会话连接成功、固件允许实验控制且风险确认后可用。',
          ),
          const SizedBox(height: 10),
          OutlinedButton.icon(
            onPressed: provider.isSessionReachable ? _loadSettings : null,
            icon: const Icon(Icons.download_outlined),
            label: const Text('读取固件阈值报告'),
          ),
          const SizedBox(height: 12),
          _NumberField(controller: _tempHighController, label: '高温阈值 °C'),
          _NumberField(controller: _tempLowController, label: '低温阈值 °C'),
          _NumberField(controller: _humidityHighController, label: '湿度高阈值 %'),
          _NumberField(controller: _humidityLowController, label: '湿度低阈值 %'),
          _NumberField(
            controller: _lightTimeoutController,
            label: '无人后关灯延时（秒）',
          ),
          const SizedBox(height: 8),
          FilledButton.icon(
            onPressed: provider.canSendExperimentalCommands
                ? _sendSettings
                : null,
            icon: const Icon(Icons.upload_outlined),
            label: const Text('发送实验性阈值请求'),
          ),
        ],
      ),
    );
  }
}

class _NumberField extends StatelessWidget {
  const _NumberField({required this.controller, required this.label});
  final TextEditingController controller;
  final String label;

  @override
  Widget build(BuildContext context) => Padding(
    padding: const EdgeInsets.only(bottom: 10),
    child: TextField(
      controller: controller,
      keyboardType: TextInputType.number,
      decoration: InputDecoration(
        labelText: label,
        border: const OutlineInputBorder(),
      ),
    ),
  );
}
