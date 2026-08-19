# esp32-motion-security-system

## About This Project

This was my first hardware + software integration project, built to learn embedded 
systems and computer vision fundamentals.

**What I did:**
- Designed the full system architecture (how the laptop and ESP32 divide responsibilities 
  and communicate over serial)
- Selected, wired, and assembled all hardware (LEDs, button, photoresistor voltage divider, 
  ESP32)
- Debugged wiring issues (button pin configuration, LED polarity, COM port conflicts)
- Calibrated sensor thresholds (DARK_THRESHOLD, MOTION_AREA_THRESH) against real-world 
  readings
- Iterated on the design (removed a component, changed the button's function from a master 
  switch to a mode-cycling control) based on my own testing

**AI-assisted:**
- Initial code implementation (ESP32 firmware and Python/OpenCV script) was generated 
  with AI assistance, then modified and debugged by me as the hardware and design evolved.

  ## Features
  
- **Real-time motion detection** using OpenCV frame differencing (grayscale + Gaussian blur + `cv2.absdiff`)
- **Serial communication** between laptop and ESP32 over USB (115200 baud)
- **Light-aware triggering** — alarm only activates when the room is dark, using an LDR voltage divider
- **Non-blocking firmware** — all timing (debounce, alarm duration, blink interval) uses `millis()`, no `delay()`
- **Two LED display modes** — solid or alternating blink, cycled via a physical button
- **Live telemetry** — ESP32 prints sensor readings and system state to Serial Monitor once per second
- **Visual feedback** — live webcam window with motion status overlay and bounding boxes around detected movement
