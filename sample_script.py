import TACDev
import time
import os

def reset(dev1):
    if dev1.Open():
        print("Sending power Off command")
        dev1.SendCommand("powerOff", True)
        time.sleep(5)
        print("Sending boot to EDL command")
        dev1.SendCommand("bootToEDL", True)
        time.sleep(5)
        while not dev1.IsCommandQueueClear():
            time.sleep(1)
        dev1.Close()
        time.sleep(10)

pid = os.getpid()
print(f"My script's PID is: {pid}")
n = TACDev.GetDeviceCount()
print(f"Number of devices detected: {n}")
for i in range(n):
    dev1 = TACDev.GetDevice(i)
    reset(dev1)
