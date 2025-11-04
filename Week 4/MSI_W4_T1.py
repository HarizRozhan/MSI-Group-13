import serial
import matplotlib
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt

# === SERIAL SETUP ===
ser = serial.Serial('COM4', 9600, timeout=1)

# === PLOT SETUP ===
plt.ion()
fig, ax = plt.subplots()
ax.set_xlim(-20, 20)   # in m/s²
ax.set_ylim(-20, 20)
ax.set_xlabel('Accel X (m/s²)')
ax.set_ylabel('Accel Y (m/s²)')
ax.set_title('MPU6050 Live Acceleration Data (X vs Y)')
scat = ax.scatter([], [], color='blue')

x_vals, z_vals = [], []

print("Reading acceleration data... (press Ctrl+C to stop)")

try:
    while True:
        line = ser.readline().decode(errors='ignore').strip()
        if not line:
            continue

        try:
            ax_raw, ay_raw, az_raw = map(float, line.split(','))
        except ValueError:
            continue

        # Convert to m/s² (assuming ±2g range)
        ax_accel = (ax_raw / 16384.0) * 9.81
        az_accel = (az_raw / 16384.0) * 9.81

        x_vals.append(ax_accel)
        z_vals.append(az_accel)

        if len(x_vals) > 100:
            x_vals.pop(0)
            z_vals.pop(0)

        scat.set_offsets(list(zip(x_vals, z_vals)))
        plt.draw()
        plt.pause(0.001)

except KeyboardInterrupt:
    print("Stopped by user.")
finally:
    ser.close()
    plt.ioff()
    plt.show()