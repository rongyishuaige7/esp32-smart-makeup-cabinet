import 'package:flutter_test/flutter_test.dart';
import 'package:http/http.dart' as http;
import 'package:http/testing.dart';
import 'package:provider/provider.dart';

import 'package:smart_cabinet_app/main.dart';
import 'package:smart_cabinet_app/providers/cabinet_provider.dart';
import 'package:smart_cabinet_app/services/cabinet_service.dart';

void main() {
  test('default address is empty and cannot create a request', () {
    var requests = 0;
    final service = CabinetService(
      '',
      client: MockClient((request) async {
        requests += 1;
        return http.Response('{}', 200);
      }),
    );

    expect(service.isConfigured, isFalse);
    expect(() => service.getStatus(), throwsA(isA<CabinetException>()));
    expect(requests, 0);
    service.dispose();
  });

  test('only a private HTTP origin is accepted', () {
    expect(
      CabinetService.normalizeLocalHttpUrl('192.168.1.50'),
      'http://192.168.1.50',
    );
    expect(
      () => CabinetService.normalizeLocalHttpUrl('https://192.168.1.50'),
      throwsA(isA<CabinetException>()),
    );
    expect(
      () => CabinetService.normalizeLocalHttpUrl('http://8.8.8.8'),
      throwsA(isA<CabinetException>()),
    );
    expect(
      () => CabinetService.normalizeLocalHttpUrl(
        'http://192.168.1.50/api/status',
      ),
      throwsA(isA<CabinetException>()),
    );
    expect(
      () => CabinetService.normalizeLocalHttpUrl('http://user@192.168.1.50'),
      throwsA(isA<CabinetException>()),
    );
  });

  test(
    'explicit connection test reaches current public firmware protocol',
    () async {
      final requestedPaths = <String>[];
      final service = CabinetService(
        'http://192.168.1.50',
        client: MockClient((request) async {
          requestedPaths.add(request.url.path);
          if (request.url.path == '/api/sensors') {
            return http.Response(
              '{"climate":"unavailable","temp":null,"humid":null,"uvEstimate":false,"uv":null,"pir":false}',
              200,
            );
          }
          return http.Response(
            '{"publicDefaultInert":true,"localControlAndActuatorsEnabled":false,"ledCommandedOn":false,"fanCommandedOn":false,"fanManual":false,"humidifierCommandedOn":false,"humidifierManual":false,"latchCommandedClosed":true,"physicalPositionVerified":false}',
            200,
          );
        }),
      );
      final provider = CabinetProvider(service);

      expect(provider.connectionStatus, CabinetConnectionStatus.configured);
      expect(requestedPaths, isEmpty);
      await provider.testConnection();

      expect(
        requestedPaths,
        containsAll(<String>['/api/sensors', '/api/status']),
      );
      expect(provider.connectionStatus, CabinetConnectionStatus.reachable);
      expect(provider.status!.publicDefaultInert, isTrue);
      expect(provider.status!.localControlAndActuatorsEnabled, isFalse);
      expect(provider.canSendExperimentalCommands, isFalse);
      provider.dispose();
    },
  );

  test('failed explicit test clears old reports', () async {
    var calls = 0;
    final service = CabinetService(
      'http://10.0.0.2',
      client: MockClient((request) async {
        calls += 1;
        if (calls <= 2) {
          if (request.url.path == '/api/sensors') {
            return http.Response(
              '{"climate":"available","temp":24.0,"humid":61.0,"uvEstimate":true,"uv":2,"pir":false}',
              200,
            );
          }
          return http.Response(
            '{"publicDefaultInert":false,"localControlAndActuatorsEnabled":true,"ledCommandedOn":false,"fanCommandedOn":false,"fanManual":false,"humidifierCommandedOn":false,"humidifierManual":false,"latchCommandedClosed":true,"physicalPositionVerified":false}',
            200,
          );
        }
        return http.Response('{"error":"unavailable"}', 503);
      }),
    );
    final provider = CabinetProvider(service);
    await provider.testConnection();
    expect(provider.sensors, isNotNull);
    expect(provider.status!.localControlAndActuatorsEnabled, isTrue);
    await provider.testConnection();
    expect(provider.connectionStatus, CabinetConnectionStatus.failed);
    expect(provider.sensors, isNull);
    expect(provider.status, isNull);
    expect(provider.experimentalControlAcknowledged, isFalse);
    provider.dispose();
  });

  testWidgets('main shell starts with no automatic connection report', (
    tester,
  ) async {
    final provider = CabinetProvider(CabinetService(''));
    await tester.pumpWidget(
      ChangeNotifierProvider<CabinetProvider>.value(
        value: provider,
        child: const SmartCabinetApp(),
      ),
    );
    expect(find.textContaining('默认不会发起网络请求。'), findsOneWidget);
    expect(find.textContaining('门锁: 已'), findsNothing);
    provider.dispose();
  });
}
