"""
============================================================================
Motion-Activated Security System — Laptop Application (Python)
Library: OpenCV (cv2) + pyserial

Pipeline:
  1. Open the default webcam.
  2. Convert each frame to grayscale and apply a Gaussian blur.
  3. Compute frame-differencing motion detection (cv2.absdiff between the
     current and previous processed frame), threshold it, and confirm
     motion via contour area (filters out single-pixel sensor noise).
  4. Send '1' over USB Serial to the ESP32 if motion crosses the area
     threshold this frame, otherwise send '0'.
  5. Display a live video window with an overlay showing motion status
     and the serial connection state.

Install dependencies:
    pip install opencv-python pyserial

Usage:
    python laptop_app.py [COM_PORT]
    e.g.  python laptop_app.py COM5

    If no port is given, it defaults to DEFAULT_COM_PORT below.
    Find your ESP32's port in Device Manager (Windows) -> Ports (COM & LPT),
    or Arduino IDE -> Tools -> Port.

Quit the video window with 'q' or ESC.
============================================================================
"""

import sys
import time

import cv2
import serial
from serial import SerialException

# ---------------------------- Tunables --------------------------------------
DEFAULT_COM_PORT   = "COM3"      # Fallback if no port passed on the command line
BAUD_RATE           = 115200
MOTION_AREA_THRESH  = 1500.0     # Min. contour pixel area to count as "motion"
DIFF_THRESH_VALUE   = 25         # Per-pixel intensity difference threshold
BLUR_KERNEL         = (21, 21)   # Gaussian blur kernel size (must be odd, odd)
BOOT_DELAY_SEC      = 2.0        # Wait for ESP32 auto-reset-on-connect to finish


def open_serial(port_name: str):
    """Attempts to open the serial connection to the ESP32. Returns a
    serial.Serial instance on success, or None on failure (app keeps
    running without hardware output)."""
    try:
        ser = serial.Serial(port=port_name, baudrate=BAUD_RATE, timeout=0.05)
        # Give the ESP32 time to finish its boot/auto-reset sequence, which
        # is commonly triggered by the DTR/RTS lines toggling on connect.
        time.sleep(BOOT_DELAY_SEC)
        print(f"[Serial] Connected to {port_name} at {BAUD_RATE} baud.")
        return ser
    except SerialException as e:
        print(f"[Warning] Could not open {port_name}: {e}")
        print("[Warning] Continuing WITHOUT serial connection — motion will "
              "be detected but not transmitted.\n"
              "Pass the correct port, e.g. python laptop_app.py COM5")
        return None


def main():
    com_port = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_COM_PORT
    esp32 = open_serial(com_port)

    # --- Open the webcam -----------------------------------------------------
    cap = cv2.VideoCapture(0)  # default camera
    if not cap.isOpened():
        print("[Error] Could not open webcam.")
        sys.exit(1)

    prev_blurred = None
    last_sent_signal = "0"

    print("[App] Starting motion detection. Press 'q' or ESC to quit.")

    try:
        while True:
            ok, frame = cap.read()
            if not ok or frame is None:
                print("[Error] Empty frame captured — exiting.")
                break

            # --- Preprocess: grayscale + blur to reduce sensor noise --------
            gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
            blurred = cv2.GaussianBlur(gray, BLUR_KERNEL, 0)

            motion_detected = False

            if prev_blurred is not None:
                # --- Frame differencing motion detection ---------------------
                diff = cv2.absdiff(prev_blurred, blurred)
                _, thresh = cv2.threshold(
                    diff, DIFF_THRESH_VALUE, 255, cv2.THRESH_BINARY
                )
                thresh = cv2.dilate(thresh, None, iterations=2)

                contours, _ = cv2.findContours(
                    thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE
                )

                for c in contours:
                    if cv2.contourArea(c) >= MOTION_AREA_THRESH:
                        motion_detected = True
                        # Draw a box around the moving region for visual feedback.
                        x, y, w, h = cv2.boundingRect(c)
                        cv2.rectangle(frame, (x, y), (x + w, y + h), (0, 0, 255), 2)

            # --- Send the motion signal to the ESP32 (once per frame) -------
            signal = "1" if motion_detected else "0"
            if esp32 is not None:
                try:
                    esp32.write(signal.encode("ascii"))
                except SerialException as e:
                    print(f"[Serial] Write failed: {e}")
            last_sent_signal = signal

            # --- Overlay status text on the video feed -----------------------
            status_text = "MOTION DETECTED" if motion_detected else "No Motion"
            status_color = (0, 0, 255) if motion_detected else (0, 200, 0)
            cv2.putText(frame, status_text, (15, 30),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.9, status_color, 2)

            serial_text = (f"Serial: TX '{last_sent_signal}'"
                           if esp32 is not None else "Serial: DISCONNECTED")
            cv2.putText(frame, serial_text, (15, 60),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 0), 2)

            cv2.imshow("Security Camera Feed", frame)

            # --- Prepare for next iteration ------------------------------------
            prev_blurred = blurred

            key = cv2.waitKey(1) & 0xFF
            if key == ord('q') or key == 27:  # 'q' or ESC
                break

    finally:
        # Tell the ESP32 no motion is happening on the way out.
        if esp32 is not None:
            try:
                esp32.write(b"0")
                esp32.close()
            except SerialException:
                pass

        cap.release()
        cv2.destroyAllWindows()
        print("[App] Shut down cleanly.")


if __name__ == "__main__":
    main()