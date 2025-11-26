import 'dart:typed_data';
import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter_reactive_ble/flutter_reactive_ble.dart';
import 'package:permission_handler/permission_handler.dart';

void main() {
  runApp(const GloveApp());
}

class GloveApp extends StatefulWidget {
  const GloveApp({super.key});

  @override
  State<GloveApp> createState() => _GloveAppState();
}

class _GloveAppState extends State<GloveApp> {
  final flutterReactiveBle = FlutterReactiveBle();

  StreamSubscription<DiscoveredDevice>? _scanSubscription;
  StreamSubscription<ConnectionStateUpdate>? _connectionSubscription;
  StreamSubscription<List<int>>? _notificationSubscription;

  DiscoveredDevice? _gloveDevice;

  List<double> leftValues = List.filled(6, 0);

  String statusMessage = "🔍 Initializing...";
  bool isConnected = false;

  List<String> foundDevices = [];

  final Uuid serviceUuid =
  Uuid.parse("12345678-1234-5678-1234-56789abcdef0");
  final Uuid characteristicUuid =
  Uuid.parse("12345678-1234-5678-1234-56789abcdef1");

  @override
  void initState() {
    super.initState();
    _initBle();
  }

  Future<void> _initBle() async {
    setState(() {
      statusMessage = "🟡 Requesting permissions...";
      foundDevices.clear();
    });

    final permissions = await [
      Permission.bluetooth,
      Permission.bluetoothScan,
      Permission.bluetoothConnect,
      Permission.locationWhenInUse,
    ].request();

    if (permissions.values.any((status) => !status.isGranted)) {
      setState(() {
        statusMessage = "❌ Required permissions not granted!";
      });
      return;
    }

    setState(() {
      statusMessage = "🔍 Scanning for BLE devices...";
    });

    await _scanSubscription?.cancel();

    _scanSubscription = flutterReactiveBle
        .scanForDevices(withServices: [serviceUuid])
        .listen((device) {
      final deviceName = device.name ?? "Unknown";
      final displayName = "$deviceName (${device.id})";

      if (!foundDevices.contains(displayName)) {
        setState(() {
          foundDevices.add(displayName);
        });
      }

      if (deviceName == "LeftGlove" && _gloveDevice == null) {
        _gloveDevice = device;
        _scanSubscription?.cancel();
        setState(() {
          statusMessage = "📱 Found LeftGlove, connecting...";
        });
        _connectToDevice(device);
      }
    }, onError: (error) {
      setState(() {
        statusMessage = "❌ Scan error: $error";
      });
    });
  }

  void _connectToDevice(DiscoveredDevice device) async {
    await _connectionSubscription?.cancel();

    _connectionSubscription =
        flutterReactiveBle.connectToDevice(id: device.id).listen(
              (connectionState) async {
            switch (connectionState.connectionState) {
              case DeviceConnectionState.connecting:
                setState(() {
                  statusMessage = "🔗 Connecting to ${device.name}...";
                });
                break;
              case DeviceConnectionState.connected:
                setState(() {
                  statusMessage = "✅ Connected to ${device.name}!";
                  isConnected = true;
                });

                // 🔹 MTU-Handshake direkt nach Verbindung
                try {
                  final mtu = await flutterReactiveBle.requestMtu(
                    deviceId: device.id,
                    mtu: 64, // gewünschte MTU
                  );
                  print("MTU successfully set to $mtu bytes");
                } catch (e) {
                  print("Failed to request MTU: $e");
                }

                _subscribeToSensorData(device);
                break;
              case DeviceConnectionState.disconnecting:
                setState(() {
                  statusMessage = "⚡ Disconnecting...";
                });
                break;
              case DeviceConnectionState.disconnected:
                setState(() {
                  statusMessage =
                  "❌ Disconnected.\nAttempting reconnect in 3 seconds...";
                  isConnected = false;
                });
                _notificationSubscription?.cancel();
                Future.delayed(const Duration(seconds: 3), () {
                  if (_gloveDevice != null) {
                    _connectToDevice(_gloveDevice!);
                  } else {
                    _initBle();
                  }
                });
                break;
            }
          },
          onError: (error) {
            setState(() {
              statusMessage = "❌ Connection error: $error";
            });
          },
        );
  }

  void _subscribeToSensorData(DiscoveredDevice device) async {
    await _notificationSubscription?.cancel();

    final characteristic = QualifiedCharacteristic(
      serviceId: serviceUuid,
      characteristicId: characteristicUuid,
      deviceId: device.id,
    );

    _notificationSubscription =
        flutterReactiveBle.subscribeToCharacteristic(characteristic).listen(
              (data) {
            final floatList = _bytesToFloatList(Uint8List.fromList(data));
            if (floatList.length == 6) {
              setState(() {
                leftValues = floatList;
                statusMessage = "📡 Receiving data...";
              });
            } else {
              print("⚠️ Unexpected data length: ${data.length}");
            }
          },
          onError: (error) {
            setState(() {
              statusMessage = "⚠️ Data stream error: $error";
            });
          },
        );
  }

  List<double> _bytesToFloatList(Uint8List bytes) {
    if (bytes.length % 4 != 0) return [];
    final byteData = ByteData.sublistView(bytes);
    final floats = <double>[];
    for (int i = 0; i < bytes.length; i += 4) {
      floats.add(byteData.getFloat32(i, Endian.little));
    }
    return floats;
  }

  @override
  void dispose() {
    _scanSubscription?.cancel();
    _connectionSubscription?.cancel();
    _notificationSubscription?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(
          title: const Text("BLE Device Scanner"),
          backgroundColor: isConnected ? Colors.green : Colors.blueGrey,
        ),
        body: Padding(
          padding: const EdgeInsets.all(20),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(
                statusMessage,
                style:
                const TextStyle(fontSize: 16, fontWeight: FontWeight.bold),
              ),
              const SizedBox(height: 20),
              const Text("🟢 Found BLE devices:"),
              Expanded(
                child: ListView.builder(
                  itemCount: foundDevices.length,
                  itemBuilder: (context, index) {
                    return Text(foundDevices[index]);
                  },
                ),
              ),
              if (isConnected) ...[
                const SizedBox(height: 10),
                const Text(
                  "Left Glove:",
                  style: TextStyle(fontWeight: FontWeight.bold),
                ),
                Text(leftValues.toString()),
              ],
              const SizedBox(height: 20),
              Center(
                child: ElevatedButton.icon(
                  onPressed: () {
                    setState(() {
                      statusMessage = "🔄 Restarting scan...";
                      isConnected = false;
                      foundDevices.clear();
                      _gloveDevice = null;
                    });
                    _scanSubscription?.cancel();
                    _connectionSubscription?.cancel();
                    _notificationSubscription?.cancel();
                    _initBle();
                  },
                  icon: const Icon(Icons.refresh),
                  label: const Text("Restart Scan"),
                ),
              )
            ],
          ),
        ),
      ),
    );
  }
}