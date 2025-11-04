import serial
import time
import numpy as np

# --- Serial Setup ---
ARDUINO_PORT = "COM3"   # Change if needed
BAUD_RATE = 9600
arduino = serial.Serial(ARDUINO_PORT, BAUD_RATE, timeout=1)
time.sleep(2)
print(f" Connected to Arduino on {ARDUINO_PORT}")

# --- Authorized RFID UID ---
AUTHORIZED_ID = "0003855091"

# --- Helper: Read one MPU6050 line ---
def read_mpu_line():
    """Reads one sensor line in the form 'AcX=...|AcY=...|AcZ=...'"""
    try:
        line = arduino.readline().decode(errors='ignore').strip()
        if not line or "AcX" not in line:
            return None
        parts = line.replace(" ", "").split("|")
        ax = int(parts[0].split("=")[1])
        ay = int(parts[1].split("=")[1])
        return ax, ay
    except Exception:
        return None

# --- Baseline Calibration ---
print(" Calibrating baseline (keep sensor still for 3s)...")
baseline_ax, baseline_ay = [], []
start_time = time.time()
while time.time() - start_time < 3:
    data = read_mpu_line()
    if data:
        ax, ay = data
        baseline_ax.append(ax)
        baseline_ay.append(ay)
    time.sleep(0.05)

if baseline_ax and baseline_ay:
    mean_ax = np.mean(baseline_ax)
    mean_ay = np.mean(baseline_ay)
    print(f" Baseline set: AX={mean_ax:.2f}, AY={mean_ay:.2f}")
else:
    mean_ax = mean_ay = 0
    print(" No baseline data, defaulting to 0.")

# --- Main Loop ---
print("\nRFID + Motion System Running...\n")

while True:
    try:
        card = input("Tap card: ").strip()
        print(f"Card Scanned: {card}")

        if card == AUTHORIZED_ID:
            print(" Authorized card detected! Move the sensor now...")

            # Collect MPU data for 2 seconds
            ax_samples, ay_samples = [], []
            start_time = time.time()
            while time.time() - start_time < 2:
                data = read_mpu_line()
                if data:
                    ax, ay = data
                    ax_samples.append(ax)
                    ay_samples.append(ay)
                time.sleep(0.05)

            if ax_samples and ay_samples:
                # --- Smoothed motion detection based on deviation from baseline ---
                ax_deltas = [abs(ax - mean_ax) for ax in ax_samples]
                ay_deltas = [abs(ay - mean_ay) for ay in ay_samples]
                motion_score = np.mean(ax_deltas + ay_deltas)

                print(f" Motion Score: {motion_score:.2f}")

                # --- Motion threshold (tune as needed) ---
                if motion_score > 100:
                    print(" Motion detected! Sending '1' to Arduino.")
                    arduino.reset_input_buffer()
                    arduino.write(b'1')
                    arduino.flush()
                else:
                    print(" No significant motion. Sending '2' to Arduino.")
                    arduino.reset_input_buffer()
                    arduino.write(b'2')
                    arduino.flush()

                time.sleep(0.2)  # brief pause for Arduino to process
            else:
                print(" No MPU data collected, skipping motion check.")
                arduino.write(b'2')
                arduino.flush()

        else:
            print(" Unauthorized card.")
            arduino.write(b'2')
            arduino.flush()

        time.sleep(0.2)  # allow Arduino to process each command

    except KeyboardInterrupt:
        print("\nExiting...")
        arduino.close()
        break