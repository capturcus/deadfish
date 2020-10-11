#!/usr/bin/python3
import pyautogui
import subprocess
import time
import os
import logging
from pytest import mark

def infinite_retry_click(file):
    print("trying to click", file)
    try:
        pyautogui.click(file)
    except Exception as e:
        print(e)
        infinite_retry_click(file)

logger = logging.getLogger()

def test_levelpacker():
    # this will raise an error on a non-zero return code
    subprocess.check_output(["./levelpacker.py"], cwd="../levelpacker")

def test_simple_connect():
    pyautogui.moveTo(100, 100)
    deadfish_path = os.path.abspath("..")
    server_build_path = deadfish_path + "/server/build"
    my_env = os.environ.copy()
    my_env["LD_LIBRARY_PATH"] = server_build_path
    server = subprocess.Popen(["./deadfishserver", "-p", "63987", "-l", "../../levels/big.bin"], cwd="../server/build", env=my_env)
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

def test_two_clients_kill():
    pyautogui.moveTo(100, 100)
    deadfish_path = os.path.abspath("..")
    server_build_path = deadfish_path + "/server/build"
    my_env = os.environ.copy()
    my_env["LD_LIBRARY_PATH"] = server_build_path
    server = subprocess.Popen(["./deadfishserver", "-p", "63987", "-l", "../../levels/test.bin", "-n", "2", "-t"], cwd="../server/build", env=my_env)
    client0 = subprocess.Popen("./deadfishclient", cwd="../client/dfclient-native")
    infinite_retry_click("connect.png")
    pyautogui.moveTo(100, 100)
    client1 = subprocess.Popen("./deadfishclient", cwd="../client/dfclient-native")
    infinite_retry_click("connect.png")
    mouseX, mouseY = pyautogui.position()
    time.sleep(1)
    pyautogui.click(mouseX + 415, mouseY - 290)
    time.sleep(6.5)
    pyautogui.click(mouseX + 45, mouseY - 315, button="right")
    time.sleep(1)
    assert(server.poll() == None)
    assert(client0.poll() == None)
    assert(client1.poll() == None)
    server.kill()
    client0.kill()
    client1.kill()
