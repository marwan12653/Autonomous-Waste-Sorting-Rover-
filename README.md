🚛 Project Green-Bot:Autonomous Waste-collecting Rover
An AGV designed for real-time waste sorting. It features a distributed control system where an STM32 handles low-level autonomous navigation and an ESP32 hosts a high-level IoT interface for robotic manipulation.

--------------------------------------------------------------------------------------------------------------------------------------------------------------------

🛠 Project Overview
This rover is built to solve the "last-mile" waste collection problem. It combines autonomous mobility with human-in-the-loop precision.

Autonomous Navigation: The STM32 processes ultrasonic sensor data to navigate obstacles.

IoT Arm Control: The ESP32 hosts a web app allowing users to control a 4-DOF robotic arm via HTTP protocol.

---------------------------------------------------------------------------------------------------------------------------------------------------------------------

🏗 System Architecture
The system is split into two primary control layers:

1. The Navigation Master (STM32)
Task: Obstacle avoidance, PWM motor drive, and sensor fusion.

Logic: Implements a reactive navigation algorithm. If the Hultrasonic detects an object within 20cm, the STM32 executes an emergency halt and recalculates the path.

Peripherals used: TIM1 (PWM), TIM2 (Input Capture), UART (communication between MCs), GPIO.

2. The Communication Slave (ESP32)
Task: Web server hosting and Robotic Arm kinematics.

Interface: An Asynchronous Web Server serving a custom HTML/CSS/JS dashboard.

Arm Logic: Controls 4x High-torque servos for the sorting mechanism.
----------------------------------------------------------------------------------------------------------------------------------------------------------------------
🔧 Hardware used
MCUs: STM32F401RCTx (Black Pill), ESP32.

Actuators/Sensors: L298N Motor Driver, 4x Gear Motors, 4x Servo Motors.

Sensors: HC-SR04 Ultrasonic Sensor.

Power: 11.1V Li-Po Battery (3S) with Step-down converters for 5V/3.3V rails.
