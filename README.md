Smart Traffic Light Control System (IoT + GSM + AI-ready)

This project is an IoT-based Smart Traffic Light System designed to intelligently control traffic flow using ultrasonic distance sensors, IR car detection, and GSM connectivity (SIM800L/LilyGO T-Call).
It aims to reduce congestion, prioritize emergency vehicles, and provide real-time traffic updates through ThingSpeak IoT cloud integration.

🚦 Key Features

Dynamic Traffic Control:
Automatically adjusts traffic light timing based on vehicle presence and distance detection.

Emergency & VIP Mode:
Remotely activate emergency or VIP clearance mode via SMS or IoT commands.

IoT Cloud Dashboard (ThingSpeak):
Real-time monitoring of lane distances, car detection, and light states.
Includes 3 visual graphs for:

Lane Light Status (Red/Yellow/Green)

Ultrasonic Distance (cm)

Car Presence (IR sensors)

GSM Connectivity:
Uses SIM800L or LilyGO T-Call for wireless communication and remote control.

Low Power Operation:
Compatible with 3.7 V Li-ion battery or USB/power bank supply.

🧠 Hardware Used

ESP32 (LilyGO T-Call / SIM800L)

Ultrasonic Sensors (HC-SR04) – for lane distance measurement

IR Sensors – for vehicle presence detection

LEDs (R/Y/G) – to simulate traffic lights

Li-ion Battery (3.7 V) – for mobile/independent power

Optional: Capacitors (100 µF–470 µF) for GSM power stability

🌐 Cloud Integration

ThingSpeak
