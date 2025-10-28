
import serial
import matplotlib
matplotlib.use('TkAgg')  # Use interactive backend for PyCharm
import matplotlib.pyplot as plt

# --- Serial connection ---
ser = serial.Serial('COM5', 9600)  # Change COM port to match your Arduino

# --- Live plot setup ---
plt.ion()
fig, ax = plt.subplots()
x_vals, y_vals = [], []

try:
    while True:
        line = ser.readline().decode().strip()
        if line.isdigit():  # Make sure we got a number
            pot_value = int(line)
            print("Potentiometer Value:", pot_value)
            x_vals.append(len(x_vals))
            y_vals.append(pot_value)

            ax.clear()
            ax.plot(x_vals, y_vals, color='green', linewidth=2)
            ax.set_xlabel("Sample Number")
            ax.set_ylabel("Potentiometer Value (0–1023)")
            ax.set_title("Real-Time Potentiometer, Servo, and LED Control")
            ax.grid(True)
            plt.pause(0.1)
except KeyboardInterrupt:
    print("\nStopped by user.")
finally:
    ser.close()
    plt.ioff()
    plt.show()
    print("Serial connection closed.")