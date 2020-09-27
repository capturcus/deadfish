#!/usr/bin/python3
import pyautogui
import subprocess
import time

def infinite_retry_click(file):
    print("trying to click", file)
    try:
        pyautogui.click(file)
    except Exception as e:
        print(e)
        infinite_retry_click(file)

def main():
    server = subprocess.Popen("./run_server.sh", cwd="../server/build")
    client = subprocess.Popen("./deadfishclient", cwd="../client/dfclient-native")
    infinite_retry_click("connect.png")
    mouseX, mouseY = pyautogui.position()
    time.sleep(1)
    mouseY -= 100
    pyautogui.click(mouseX, mouseY)
    time.sleep(1)
    assert(server.poll() == None)
    assert(client.poll() == None)
    print("TEST PASSED")
    server.kill()
    client.kill()

if __name__ == "__main__":
    main()
