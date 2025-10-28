import serial
import time

# Replace 'COM3' with the correct port for your Arduino (e.g., '/dev/ttyACM0' on Linux/Mac)
arduino_port = 'COM5'
baud_rate = 9600

try:
    # Initialize the serial connection
    ser = serial.Serial(arduino_port, baud_rate)
    time.sleep(2)  # Give the connection time to establish
    print(f"Connected to Arduino on {arduino_port}")

    while True:
        # Read a line from the serial port (this will be the potValue)
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8').strip()

            try:
                # Convert the received string to an integer
                pot_value = int(line)
                print(f"Potentiometer Value: {pot_value}")

                # Python equivalent of the Arduino's IF logic:
                if pot_value > 500:
                    print("Status: Above Threshold (500)")
                else:
                    print("Status: Below Threshold (500)")

            except ValueError:
                # Handle non-integer data
                pass

        time.sleep(0.1)  # Python delay (equivalent to Arduino's delay(100))

except serial.SerialException as e:
    print(f"Error opening serial port: {e}")
except KeyboardInterrupt:
    print("\nProgram terminated.")
finally:
    if 'ser' in locals() and ser.is_open:
        ser.close()
        print("Serial connection closed.")

