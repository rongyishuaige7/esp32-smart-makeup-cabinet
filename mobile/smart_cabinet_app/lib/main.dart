import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import 'config/api_config.dart';
import 'providers/cabinet_provider.dart';
import 'screens/control_screen.dart';
import 'screens/cosmetics_screen.dart';
import 'screens/home_screen.dart';
import 'screens/settings_screen.dart';
import 'services/cabinet_service.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  final service = CabinetService(ApiConfig.defaultBaseUrl);
  final provider = CabinetProvider(service);
  await provider.loadSavedBaseUrl();

  runApp(
    ChangeNotifierProvider<CabinetProvider>.value(
      value: provider,
      child: const SmartCabinetApp(),
    ),
  );
}

class SmartCabinetApp extends StatelessWidget {
  const SmartCabinetApp({super.key});

  @override
  Widget build(BuildContext context) {
    final seed = const Color(0xFFC48B9F);
    return MaterialApp(
      title: '智能化妆柜',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(
          seedColor: seed,
          brightness: Brightness.light,
        ),
        useMaterial3: true,
        appBarTheme: const AppBarTheme(centerTitle: true),
      ),
      home: const MainShell(),
    );
  }
}

class MainShell extends StatefulWidget {
  const MainShell({super.key});

  @override
  State<MainShell> createState() => _MainShellState();
}

class _MainShellState extends State<MainShell> {
  int _index = 0;

  static const _titles = ['概览', '控制', '化妆品', '设置'];

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: Text(_titles[_index])),
      body: IndexedStack(
        index: _index,
        children: const [
          HomeScreen(),
          ControlScreen(),
          CosmeticsScreen(),
          SettingsScreen(),
        ],
      ),
      bottomNavigationBar: NavigationBar(
        selectedIndex: _index,
        onDestinationSelected: (i) => setState(() => _index = i),
        destinations: const [
          NavigationDestination(
            icon: Icon(Icons.dashboard_outlined),
            selectedIcon: Icon(Icons.dashboard),
            label: '概览',
          ),
          NavigationDestination(
            icon: Icon(Icons.tune_outlined),
            selectedIcon: Icon(Icons.tune),
            label: '控制',
          ),
          NavigationDestination(
            icon: Icon(Icons.inventory_2_outlined),
            selectedIcon: Icon(Icons.inventory_2),
            label: '化妆品',
          ),
          NavigationDestination(
            icon: Icon(Icons.settings_outlined),
            selectedIcon: Icon(Icons.settings),
            label: '设置',
          ),
        ],
      ),
    );
  }
}
