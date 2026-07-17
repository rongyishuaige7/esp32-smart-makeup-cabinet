import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../models/cosmetic.dart';
import '../providers/cabinet_provider.dart';

/// The public firmware deliberately does not expose raw RFID UID scan,
/// registration, or deletion flows over HTTP. This screen is read-only so it
/// cannot imply that RFID is a secure credential, that an item was scanned, or
/// that a UID can be automatically inserted into a record.
class CosmeticsScreen extends StatelessWidget {
  const CosmeticsScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Consumer<CabinetProvider>(
      builder: (context, provider, _) => RefreshIndicator(
        onRefresh: () async {
          if (!provider.isSessionReachable) {
            await provider.testConnection();
          } else {
            await provider.refreshCosmetics();
          }
        },
        child: CustomScrollView(
          physics: const AlwaysScrollableScrollPhysics(),
          slivers: [
            SliverToBoxAdapter(
              child: Padding(
                padding: const EdgeInsets.all(16),
                child: Card(
                  child: Padding(
                    padding: const EdgeInsets.all(16),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Text(
                          '本地物品记录（只读）',
                          style: Theme.of(context).textTheme.titleMedium,
                        ),
                        const SizedBox(height: 8),
                        const Text(
                          '公开 HTTP 不提供 RFID UID 读取、自动回填、注册、删除或鉴权。UID 只是可读标识符，不构成安全认证、防复制能力、门禁权限或物品存在证明。',
                        ),
                        const SizedBox(height: 12),
                        OutlinedButton.icon(
                          onPressed: provider.isSessionReachable
                              ? provider.refreshCosmetics
                              : null,
                          icon: const Icon(Icons.refresh),
                          label: const Text('读取本地记录'),
                        ),
                        if (!provider.isSessionReachable) ...[
                          const SizedBox(height: 8),
                          const Text('先在“概览”或“设置”中主动读取当前固件报告，才可读取本地记录。'),
                        ],
                      ],
                    ),
                  ),
                ),
              ),
            ),
            if (provider.cosmetics.isEmpty)
              const SliverFillRemaining(
                hasScrollBody: false,
                child: Center(child: Text('尚无本会话读取到的本地记录。')),
              )
            else
              SliverPadding(
                padding: const EdgeInsets.fromLTRB(12, 0, 12, 16),
                sliver: SliverList.builder(
                  itemCount: provider.cosmetics.length,
                  itemBuilder: (context, index) =>
                      _CosmeticCard(cosmetic: provider.cosmetics[index]),
                ),
              ),
          ],
        ),
      ),
    );
  }
}

class _CosmeticCard extends StatelessWidget {
  const _CosmeticCard({required this.cosmetic});
  final Cosmetic cosmetic;

  String get _expiryLabel => switch (cosmetic.expiry) {
    'not_set' => '固件报告：未设置日期',
    'valid' => '固件报告：日期尚未到期',
    'expired' => '固件报告：日期已过',
    'time_unavailable' => '固件报告：时间不可用，无法判断日期',
    _ => '固件报告：记录日期无效',
  };

  @override
  Widget build(BuildContext context) => Card(
    child: ListTile(
      leading: const Icon(Icons.inventory_2_outlined),
      title: Text(cosmetic.name),
      subtitle: Text(
        [
          if (cosmetic.brand.isNotEmpty) cosmetic.brand,
          if (cosmetic.expireDate.isNotEmpty) '记录日期：${cosmetic.expireDate}',
          _expiryLabel,
        ].join('\n'),
      ),
    ),
  );
}
