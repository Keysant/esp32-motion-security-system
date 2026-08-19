# ESP32 Motion Security System

A beginner project combining a laptop webcam with an ESP32 microcontroller to build a motion-triggered security light.

## About This Project

This was my first project working with hardware and software together. My main goal was to learn how embedded systems work and how a laptop can talk to a microcontroller like the ESP32.

Since I am just starting out and don't know how to code yet, I used AI to generate the C++ firmware and the Python code. My focus was on designing how the system should work, assembling the physical hardware circuit, and troubleshooting the hardware and settings until everything worked in real life.

### What I Worked On:
* **System Design:** Figured out how the project should work by letting the laptop do the heavy lifting (webcam motion detection) and telling the ESP32 when to turn on the lights.
* **Hardware Assembly:** Wired up an ESP32 on a breadboard with LEDs, resistors, a push button, and a photoresistor (light sensor).
* **Hardware Debugging:** Fixed physical issues along the way, like fixing button wiring, getting LED polarity right, and resolving USB COM port conflicts on Windows.
* **Sensor Tuning:** Tested the light sensor in different room lighting conditions to find the right threshold values so the LEDs only turn on when it is dark.
* **Project Iteration:** Changed the design while testing, such as changing the button from a basic power switch to a button that switches between different LED light modes.

---

## Features

* **Webcam Motion Detection:** The Python script uses OpenCV on the laptop to watch the webcam and detect when something moves in the frame.
* **Night-Only Trigger:** Uses a photoresistor (LDR) light sensor so the lights only turn on if the room is actually dark.
* **Multi-Mode Button:** Pressing the physical push button changes how the LEDs behave, such as switching between solid light and an alternating flashing mode.
* **30-Second Auto Shutoff:** If no motion is detected for 10 seconds, the lights turn off automatically to save power.
* **Serial Communication:** The laptop sends simple signals ('1' for motion, '0' for no motion) to the ESP32 over a USB cable at 115200 baud.
* **Live Video Feed:** Displays a live camera window on the laptop screen with a box drawn around whatever is moving.

---

## Hardware & Components Used

* ESP32 Microcontroller
* Breadboard & Jumper Wires
* LEDs & Resistors
* Photoresistor (LDR Light Sensor)
* Push Button
* Laptop with Webcam

---

## Software & Tools Used

* **Arduino IDE:** To upload the code to the ESP32.
* **VS Code & Python:** To run the motion detection script.
* **OpenCV & PySerial:** Python libraries for the camera feed and USB connection.
* **AI Assistance:** Used to write the C++ and Python code.

---

## How to Run It

1. Wire up the ESP32 circuit on a breadboard with the LEDs, button, and photoresistor.
2. Open the C++ code in Arduino IDE, select your ESP32 board and COM port, and upload it.
3. Close the Arduino Serial Monitor so the port is free.
4. Open VS Code, navigate to the project folder, and run the Python script:
   ```powershell
   python main.py

https://github.com/user-attachments/assets/110d104c-c62e-48d0-adcc-b656d6599c46


