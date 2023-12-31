import machine
import utime
import ustruct
import sys
import ujson
import network
import requests
ssid = 'PUT SSID HERE'
password = 'PASSWORD'
led0 = machine.Pin("LED", machine.Pin.OUT)
led0.off()

###############################################
# Constants

# I2C address
PCBARTISTS_DBM = 0x48

# Registers
I2C_REG_VERSION		= 0x00
I2C_REG_ID3			= 0x01
I2C_REG_ID2			= 0x02
I2C_REG_ID1			= 0x03
I2C_REG_ID0			= 0x04
I2C_REG_SCRATCH		= 0x05
I2C_REG_CONTROL		= 0x06
I2C_REG_TAVG_HIGH	= 0x07
I2C_REG_TAVG_LOW	= 0x08
I2C_REG_RESET		= 0x09
I2C_REG_DECIBEL		= 0x0A
I2C_REG_MIN			= 0x0B
I2C_REG_MAX			= 0x0C
I2C_REG_THR_MIN     = 0x0D
I2C_REG_THR_MAX     = 0x0E
I2C_REG_HISTORY_0	= 0x14
I2C_REG_HISTORY_99	= 0x77

###############################################
# Settings

# Initialize I2C with pins
i2c = machine.I2C(0,
                  scl=machine.Pin(17),
                  sda=machine.Pin(16),
                  freq=100000)
###############################################
# Functions

def reg_write(i2c, addr, reg, data):
    """
    Write bytes to the specified register.
    """
    
    # Construct message
    msg = bytearray()
    msg.append(data)
    
    # Write out message to register
    i2c.writeto_mem(addr, reg, msg)
    
def reg_read(i2c, addr, reg, nbytes=1):
    """
    Read byte(s) from specified register. If nbytes > 1, read from consecutive
    registers.
    """
    
    # Check to make sure caller is asking for 1 or more bytes
    if nbytes < 1:
        return bytearray()
    
    # Request data from specified register(s) over I2C
    data = i2c.readfrom_mem(addr, reg, nbytes)
    
    return data

###############################################
# Main

# Read device ID to make sure that we can communicate with the ADXL343
data = reg_read(i2c, PCBARTISTS_DBM, I2C_REG_VERSION)
print("dbMeter VERSION = 0x{:02x}".format(int.from_bytes(data, "big")))

data = reg_read(i2c, PCBARTISTS_DBM, I2C_REG_ID3, 4)
print("Unique ID: 0x{:02x} ".format(int.from_bytes(data, "big")))

#1000ms
#reg_write(i2c, PCBARTISTS_DBM, I2C_REG_TAVG_HIGH, 0x03) # 1000ms
#reg_write(i2c, PCBARTISTS_DBM, I2C_REG_TAVG_LOW, 0xe8) # 1000ms

thi = reg_read(i2c, PCBARTISTS_DBM, I2C_REG_TAVG_HIGH)
tlo = reg_read(i2c, PCBARTISTS_DBM, I2C_REG_TAVG_LOW)
if tlo == b'\xe8' and thi == b'\x03':
    print("T avg = 1000ms")
else:
    print("Tavg high = 0x{:02x}".format(int.from_bytes(thi, "big")))
    print("Tavg low = 0x{:02x}".format(int.from_bytes(tlo, "big")))

#data = reg_read(i2c, PCBARTISTS_DBM, I2C_REG_CONTROL)
#print("Control = 0B{:08b}".format(int.from_bytes(data, "big")))

reg_write(i2c, PCBARTISTS_DBM, I2C_REG_RESET, 0B00000010) # resets min/max

#open logfile
file=open("log.log","a")
print("STARTUP")
file.write(str(utime.localtime()) + " STARTUP\n")
file.flush()

#now get the wifi going
wlan = network.WLAN(network.STA_IF)
wlan.active(True)
wlan.connect(ssid, password)
# Wait for connection to establish
max_wait = 10
while max_wait > 0:
    if wlan.status() < 0 or wlan.status() >= 3:
            break
    max_wait -= 1
    print('waiting for connection...')
    utime.sleep(1)
# Manage connection errors
if wlan.status() != 3:
    file.write(str(utime.localtime()) + " could not connect to wifi\n")
    file.flush()
    raise RuntimeError('could not connect to wifi')
else:
    print('connected')

led0.on()    
utime.sleep(0.1)
led0.off()

#authenticate to ubidots
headers = {
    "x-ubidots-apikey": "PUT API KEY HERE",
    "Content-Type": "application/json"
}
tokenrawdata = ""
token = ""

try:
    tokenrawdata=requests.post("https://industrial.api.ubidots.com/api/v1.6/auth/token",headers=headers)
    file.write(str(utime.localtime()) + " STARTUP\n")
    file.write(str(utime.localtime()) + " tokenrawdata.text: "+str(tokenrawdata.text)+"\n")
    file.flush()
except:
    print("UNSPECIFIED ERROR GETTING tokenrawdata")
    raise

file.write(str(utime.localtime()) + " STARTUP\n")
file.write(str(utime.localtime()) + " tokenrawdata.text: "+str(tokenrawdata.text)+"\n")
file.flush()

print("tokenrawdata.text: " +str(tokenrawdata.text))
token = tokenrawdata.json()['token'] # WE ARE ASSUMING WE GOT THE TOKEN
tokenrawdata.close()
if token == "":
    print("TOKEN IS EMPTY")
    file.write(str(utime.localtime()) + " TOKEN IS EMPTY")
    file.flush()

while True:
    utime.sleep(59)
    data = reg_read(i2c, PCBARTISTS_DBM, I2C_REG_DECIBEL)
    dbmin = reg_read(i2c, PCBARTISTS_DBM, I2C_REG_MIN)
    dbmax = reg_read(i2c, PCBARTISTS_DBM, I2C_REG_MAX)
    print("Sound Level (dB SPL) = {:03d} / min {:03d} / max {:03d}".format(int.from_bytes(data, "big"),int.from_bytes(dbmin, "big"),int.from_bytes(dbmax, "big")))
    #send data
    headers = {
        "X-Auth-Token": token,
        "Content-Type": "application/json"
    }
    datatosend = {
        "dBa": int.from_bytes(data, "big"),
        "min": int.from_bytes(dbmin, "big"),
        "max": int.from_bytes(dbmax, "big")
    }
    resp = None
    jsonstr = ""
    try:
        resp=requests.post("https://industrial.api.ubidots.com/api/v1.6/devices/noisemeter/", headers=headers, data=ujson.dumps(datatosend))
        jsonstr = str(resp.text).replace("\n0\r","").replace("\n","").replace("\r","").replace("\n","").replace("\r","").replace("55{","{").replace("}0","}")    
    except:
        if resp is None:
            print("request error (no other info)")
            jsonstr = "[EMPTY]"
        else:
            print("non-fatal request error: " + str(resp.reason) + " / " + str(resp.text))
    print("jsonstr: " + jsonstr)
    file.write(str(utime.localtime()) + " jsonstr: "+jsonstr+"\n")
    file.flush()

    if not '"status_code":201' in jsonstr:
        if jsonstr is None:
            print("unexpected status (no other info)")
            file.write(str(utime.localtime()) + " unexpected status (no other info)")
            file.flush()
        else:
            print("unexpected status: " + jsonstr)
            file.write(str(utime.localtime()) + " unexpected status: " + jsonstr)
            file.flush()
            
    reg_write(i2c, PCBARTISTS_DBM, I2C_REG_RESET, 0B00000010) # resets min/max
    led0.on()
    utime.sleep(1)
    led0.off()
sys.exit()
