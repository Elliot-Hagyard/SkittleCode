import queue
import time
import csv
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import serial
import threading

# arduino = serial.Serial(port='/dev/cu.usbmodem1101',
#                        baudrate = 115200, timeout = .1)
header = ['R', 'G', 'B', 'ID']
COLORS = ["RED", "YELLOW", "GREEN", "WOOD", "ORANGE", "PURPLE"]
MARKERS = ["v", ">", "<", "s", "p", "X"]
WOOD = [0.743, 0.557, 0.371]


def plot_data(filename):
    df = pd.read_csv(filename)
    fig = plt.figure()
    ax = fig.add_subplot(111, projection='3d')
    for idx, color in enumerate(COLORS):
        color_df = df[df['ID'] == color]
        print(color_df)
        x = np.array(color_df['R'])
        y = np.array(color_df['G'])
        z = np.array(color_df['B'])

        color_avg = (color_df['R'].mean(),
                     color_df['G'].mean(), color_df['B'].mean())
        color_array = np.array([i for i in zip(x, y, z)])
        print(color_array)
        print(
            color +
            f" avg: {color_avg}"
        )
        if color_df['R'].size != 0:
            ax.scatter(x, y, z, marker=MARKERS[idx], s=40,
                       c=color_array)
    ax.axis(([1, 0, 0, 1]))
    ax.set_zlim(0, 1)

    ax.set_xlabel("R")
    ax.set_ylabel("G")
    ax.set_zlabel("B")
    plt.legend(COLORS)
    plt.show()


def write_read():
    time.sleep(0.05)
    data = arduino.readline()

    return data


def read_kbd_input(inputQueue):
    print('Ready for keyboard input:')
    while (True):
        input_str = input().strip()

        inputQueue.put(input_str)


def dot_product(a, b):
    dot_sum = 0
    for idx in range(len(a)):
        dot_sum += a[idx]*b[idx]
    return dot_sum


def main():
    # Multi-threaded part just QOL, got it from https://stackoverflow.com/questions/5404068/how-to-read-keyboard-input/53344690#53344690
    EXIT_COMMAND = "exit"
    color = input("Color?")
    f = open(f"color_{color}.csv", 'w', encoding='UTF8')
    f2 = open(f"color_{color}_log", "w")
    inputQueue = queue.Queue()
    writer = csv.writer(f)
    setup = True
    writer.writerow(header)
    inputThread = threading.Thread(
        target=read_kbd_input, args=(inputQueue,), daemon=True)
    inputThread.start()
    main_color_count = 0
    while (True):
        if (inputQueue.qsize() > 0):
            input_str = inputQueue.get()
            print("input_str = {}".format(input_str))

            if (input_str):
                print("Exiting serial terminal.")
                break

            # Insert your code here to do whatever you want with the input_str.

        # The rest of your program goes here.

        data = str(write_read()).strip("b'\\r\\n").split(",")
        f2.write(str(data)+"\n")
        if len(data) == 4 and setup:

            if data[3].lower() == color:
                main_color_count += 1
                print(main_color_count)
            # if dot_product([float(i) for i in data[:-1]], WOOD) > .99:
            #     if data[-1] == "WOOD":
            #         continue
            print(data)
            writer.writerow(data)
        elif setup:
            print(data)
        # #if data and data[0] == "Setup Complete!":
        #     print("Setup Complete!")
        #     setup = True
    print("End.")
    f.close()
    f2.close()
    return f"color_{color}.csv"


if (__name__ == '__main__'):
    # filename = main()
    filename = input()
    plot_data(filename)
