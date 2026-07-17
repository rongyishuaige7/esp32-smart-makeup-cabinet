import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../providers/cabinet_provider.dart';
import '../services/cabinet_service.dart';

class ControlScreen extends StatelessWidget {
  const ControlScreen({super.key});

  Future<bool> _confirmRisk(BuildContext context) async {
    return await showDialog<bool>(
          context: context,
          builder: (dialogContext) => AlertDialog(
            title: const Text('确认实验性执行器控制'),
            content: const Text(
              '这会通过无认证、明文 HTTP 向本地固件发送请求。HTTP 成功或固件“accepted”只表示请求被接受，不代表灯带、风扇、加湿器、舵机插销或任何物理部件已经动作、到位或安全。\n\n'
              '不得用于门禁、防盗、财产保护、医疗、安全控制、无人值守或生产环境。请只在亲自监督的低压实验台上继续。',
            ),
            actions: [
              TextButton(
                onPressed: () => Navigator.pop(dialogContext, false),
                child: const Text('取消'),
              ),
              FilledButton(
                onPressed: () => Navigator.pop(dialogContext, true),
                child: const Text('我理解并继续'),
              ),
            ],
          ),
        ) ??
        false;
  }

  Future<void> _send(
    BuildContext context,
    CabinetProvider provider,
    Future<void> Function() request,
  ) async {
    try {
      await request();
      await provider.testConnection();
      if (context.mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('请求已被固件接受；已重新读取报告，仍不代表物理执行。')),
        );
      }
    } on CabinetException catch (error) {
      if (context.mounted) {
        ScaffoldMessenger.of(
          context,
        ).showSnackBar(SnackBar(content: Text(error.message)));
      }
    } catch (error) {
      if (context.mounted) {
        ScaffoldMessenger.of(
          context,
        ).showSnackBar(SnackBar(content: Text('请求失败：$error')));
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Consumer<CabinetProvider>(
      builder: (context, provider, _) {
        final controlAllowed = provider.canSendExperimentalCommands;
        final firmwareInert =
            provider.status?.localControlAndActuatorsEnabled != true;
        return ListView(
          padding: const EdgeInsets.all(16),
          children: [
            _SafetyCard(
              controlAllowed: controlAllowed,
              firmwareInert: firmwareInert,
              acknowledged: provider.experimentalControlAcknowledged,
              canAcknowledge:
                  provider.isSessionReachable &&
                  !firmwareInert &&
                  !provider.experimentalControlAcknowledged,
              onAcknowledge: () async {
                if (!provider.isSessionReachable) {
                  return;
                }
                final confirmed = await _confirmRisk(context);
                if (confirmed) {
                  provider.setExperimentalControlAcknowledged(true);
                }
              },
            ),
            const SizedBox(height: 12),
            _ActionCard(
              title: '低压执行器请求',
              subtitle: '只在本会话读取成功、固件非默认惰性且已确认风险时显示可用。',
              enabled: controlAllowed,
              children: [
                _ActionRow(
                  label: '灯带',
                  onAction: () => _send(
                    context,
                    provider,
                    () => provider.service.setLED(
                      power: true,
                      brightness: 96,
                      color: Colors.white,
                    ),
                  ),
                ),
                _ActionRow(
                  label: '风扇',
                  onAction: () => _send(
                    context,
                    provider,
                    () => provider.service.setFan(true),
                  ),
                ),
                _ActionRow(
                  label: '加湿器',
                  onAction: () => _send(
                    context,
                    provider,
                    () => provider.service.setHumidifier(true),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 12),
            _ActionCard(
              title: '舵机插销（高风险）',
              subtitle: '不是门禁或安全锁。无位置反馈；按钮不会显示“已打开/已锁定”等物理状态。',
              enabled: controlAllowed,
              children: [
                _ActionRow(
                  label: '发送打开插销请求',
                  destructive: true,
                  onAction: () async {
                    final ok = await _confirmRisk(context);
                    if (ok && context.mounted) {
                      await _send(
                        context,
                        provider,
                        () => provider.service.setLatch(open: true),
                      );
                    }
                  },
                ),
                _ActionRow(
                  label: '发送关闭插销请求',
                  onAction: () async {
                    final ok = await _confirmRisk(context);
                    if (ok && context.mounted) {
                      await _send(
                        context,
                        provider,
                        () => provider.service.setLatch(open: false),
                      );
                    }
                  },
                ),
              ],
            ),
          ],
        );
      },
    );
  }
}

class _SafetyCard extends StatelessWidget {
  const _SafetyCard({
    required this.controlAllowed,
    required this.firmwareInert,
    required this.acknowledged,
    required this.canAcknowledge,
    required this.onAcknowledge,
  });
  final bool controlAllowed;
  final bool firmwareInert;
  final bool acknowledged;
  final bool canAcknowledge;
  final VoidCallback onAcknowledge;

  @override
  Widget build(BuildContext context) {
    final cs = Theme.of(context).colorScheme;
    return Card(
      color: cs.errorContainer,
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('实验控制安全边界', style: Theme.of(context).textTheme.titleMedium),
            const SizedBox(height: 8),
            const Text('默认公开固件是惰性的。只有你本地显式配置实验模式后，固件才可能接受控制请求；本 App 不会绕过该限制。'),
            const SizedBox(height: 8),
            Text(
              firmwareInert
                  ? '最近一次固件报告：默认惰性/控制禁用。'
                  : '最近一次固件报告：实验控制可能已启用，仍需人工监督。',
              style: Theme.of(context).textTheme.bodySmall,
            ),
            const SizedBox(height: 12),
            OutlinedButton(
              onPressed: canAcknowledge ? onAcknowledge : null,
              child: Text(acknowledged ? '本会话风险已确认' : '确认实验性控制风险'),
            ),
            if (!controlAllowed) ...[
              const SizedBox(height: 8),
              const Text('控制保持禁用，直到：保存地址 → 主动读取成功 → 本地固件报告非惰性 → 本会话确认风险。'),
            ],
          ],
        ),
      ),
    );
  }
}

class _ActionCard extends StatelessWidget {
  const _ActionCard({
    required this.title,
    required this.subtitle,
    required this.enabled,
    required this.children,
  });
  final String title;
  final String subtitle;
  final bool enabled;
  final List<Widget> children;

  @override
  Widget build(BuildContext context) => Card(
    child: Padding(
      padding: const EdgeInsets.all(16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(title, style: Theme.of(context).textTheme.titleMedium),
          const SizedBox(height: 4),
          Text(subtitle, style: Theme.of(context).textTheme.bodySmall),
          const SizedBox(height: 10),
          ...children.map(
            (child) => Opacity(opacity: enabled ? 1 : .55, child: child),
          ),
        ],
      ),
    ),
  );
}

class _ActionRow extends StatelessWidget {
  const _ActionRow({
    required this.label,
    required this.onAction,
    this.destructive = false,
  });
  final String label;
  final Future<void> Function() onAction;
  final bool destructive;

  @override
  Widget build(BuildContext context) {
    final enabled = context.select<CabinetProvider, bool>(
      (p) => p.canSendExperimentalCommands,
    );
    return Padding(
      padding: const EdgeInsets.only(bottom: 8),
      child: SizedBox(
        width: double.infinity,
        child: OutlinedButton(
          onPressed: enabled ? onAction : null,
          style: destructive
              ? OutlinedButton.styleFrom(
                  foregroundColor: Theme.of(context).colorScheme.error,
                )
              : null,
          child: Text(label),
        ),
      ),
    );
  }
}
