import threading
import collections
import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation

SERIAL_PORT = "/dev/ttyACM0"
BAUD_RATE = 115200
BUFFER_SIZE = 500

data = collections.deque([0.0] * BUFFER_SIZE, maxlen=BUFFER_SIZE)
target = collections.deque([0.0] * BUFFER_SIZE, maxlen=BUFFER_SIZE)
lock = threading.Lock()

def read_serial():
    with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
        while True:
            line = ser.readline().strip()
            if not line:
                print("nothing")
                continue
            try:
                dc = line.decode("ascii", errors="ignore").split(",")
                value = float(dc[0])
                v_target = float(dc[1])
            except ValueError:
                continue
            with lock:
                data.append(value)
                target.append(v_target)
threading.Thread(target=read_serial, daemon=True).start()

fig, ax = plt.subplots()
x = list(range(BUFFER_SIZE))
line, = ax.plot(x, list(data))
line2, = ax.plot(x, list(target))
ax.set_xlim(0, BUFFER_SIZE - 1)

def update(frame):
    with lock:
        y = list(data)
        yt = list(target)
    if len(y) < BUFFER_SIZE:
        y = [y[-1] if y else 0.0] * (BUFFER_SIZE - len(y)) + y
    if len(yt) < BUFFER_SIZE:
        yt = [yt[-1] if y else 0.0] * (BUFFER_SIZE - len(yt)) + yt
    line.set_ydata(y)
    line2.set_ydata(yt)
    ax.set_ylim(0, 1000)
    return line,line2

ani = animation.FuncAnimation(fig, update, interval=50, blit=True)
plt.show()
