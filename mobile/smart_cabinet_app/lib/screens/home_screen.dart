import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../models/sensor_data.dart';
import '../providers/cabinet_provider.dart';

class HomeScreen extends StatelessWidget {
  const HomeScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Consumer<CabinetProvider>(
      builder: (context, provider, _) {
        final colorScheme = Theme.of(context).colorScheme;
        final sensors = provider.sensors;
        final status = provider.status;
        return RefreshIndicator(
          onRefresh: provider.testConnection,
          child: ListView(
            physics: const AlwaysScrollableScrollPhysics(),
            padding: const EdgeInsets.all(16),
            children: [
              Card(
                color: provider.isSessionReachable
                    ? colorScheme.secondaryContainer
                    : colorScheme.surfaceContainerHighest,
                child: Padding(
                  padding: const EdgeInsets.all(16),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        '本地实验连接',
                        style: Theme.of(context).textTheme.titleMedium,
                      ),
                      const SizedBox(height: 6),
                      Text(provider.connectionLabel),
                      const SizedBox(height: 10),
                      Text(
                        '“读取成功”仅表示这一次 HTTP 请求得到可解析的固件报告；不代表设备在线、传感器准确或执行器实际动作。',
                        style: Theme.of(context).textTheme.bodySmall,
                      ),
                      const SizedBox(height: 12),
                      FilledButton.icon(
                        onPressed: provider.isRefreshing
                            ? null
                            : provider.testConnection,
                        icon: provider.isRefreshing
                            ? const SizedBox(
                                width: 18,
                                height: 18,
                                child: CircularProgressIndicator(
                                  strokeWidth: 2,
                                ),
                              )
                            : const Icon(Icons.refresh),
                        label: const Text('读取当前固件报告'),
                      ),
                    ],
                  ),
                ),
              ),
              if (provider.lastError != null) ...[
                const SizedBox(height: 12),
                Card(
                  color: colorScheme.errorContainer,
                  child: Padding(
                    padding: const EdgeInsets.all(12),
                    child: Text(
                      provider.lastError!,
                      style: TextStyle(color: colorScheme.onErrorContainer),
                    ),
                  ),
                ),
              ],
              const SizedBox(height: 20),
              Text('最近一次固件报告', style: Theme.of(context).textTheme.titleLarge),
              const SizedBox(height: 8),
              if (sensors == null || status == null)
                const _EmptyReport()
              else ...[
                _SensorReport(sensors: sensors),
                const SizedBox(height: 12),
                _CommandReport(provider: provider),
              ],
            ],
          ),
        );
      },
    );
  }
}

class _EmptyReport extends StatelessWidget {
  const _EmptyReport();

  @override
  Widget build(BuildContext context) => Card(
    child: const Padding(
      padding: EdgeInsets.all(16),
      child: Text('尚无当前会话报告。先在“设置”保存地址，再主动读取。默认不会发起网络请求。'),
    ),
  );
}

class _SensorReport extends StatelessWidget {
  const _SensorReport({required this.sensors});
  final SensorData sensors;

  @override
  Widget build(BuildContext context) {
    final climate = sensors.climateAvailable
        ? '${sensors.temperature!.toStringAsFixed(1)} °C / ${sensors.humidity!.toStringAsFixed(1)} %RH'
        : '固件报告不可用';
    final uv = sensors.uvEstimate ? '${sensors.uvValue}' : '固件报告不可用';
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('传感器报告', style: Theme.of(context).textTheme.titleMedium),
            const SizedBox(height: 10),
            _FactRow(label: '温度 / 湿度', value: climate),
            _FactRow(label: 'UV 估算', value: uv),
            _FactRow(
              label: 'PIR 输入',
              value: sensors.pirDetected ? '固件报告为触发' : '固件报告为未触发',
            ),
            const SizedBox(height: 8),
            Text(
              '这些是传感器原始/软件报告，不是经校准的环境结论或安全告警。',
              style: Theme.of(context).textTheme.bodySmall,
            ),
          ],
        ),
      ),
    );
  }
}

class _CommandReport extends StatelessWidget {
  const _CommandReport({required this.provider});
  final CabinetProvider provider;

  @override
  Widget build(BuildContext context) {
    final status = provider.status!;
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('固件命令状态', style: Theme.of(context).textTheme.titleMedium),
            const SizedBox(height: 10),
            _FactRow(
              label: '公开默认模式',
              value: status.localControlAndActuatorsEnabled
                  ? '固件报告允许本地实验控制'
                  : '固件报告为控制禁用',
            ),
            _FactRow(
              label: '灯带命令',
              value: status.ledCommandedOn ? '固件命令为开' : '固件命令为关',
            ),
            _FactRow(
              label: '风扇命令',
              value: status.fanCommandedOn ? '固件命令为开' : '固件命令为关',
            ),
            _FactRow(
              label: '加湿器命令',
              value: status.humidifierCommandedOn ? '固件命令为开' : '固件命令为关',
            ),
            _FactRow(
              label: '舵机插销命令',
              value: status.latchCommandedClosed ? '固件命令为关闭' : '固件命令为打开',
            ),
            const SizedBox(height: 8),
            Text(
              '没有物理位置反馈；以上绝不表示门锁、灯带、风扇、加湿器或其他实体已经动作。',
              style: Theme.of(context).textTheme.bodySmall,
            ),
          ],
        ),
      ),
    );
  }
}

class _FactRow extends StatelessWidget {
  const _FactRow({required this.label, required this.value});
  final String label;
  final String value;

  @override
  Widget build(BuildContext context) => Padding(
    padding: const EdgeInsets.symmetric(vertical: 3),
    child: Row(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        SizedBox(width: 104, child: Text(label)),
        Expanded(child: Text(value)),
      ],
    ),
  );
}
