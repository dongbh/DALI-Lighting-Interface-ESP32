import struct
import sys

sn = 0
HASSEB_DALI_FRAME = 0X07

def construct(command):
    global sn
    # sequence number
    sn = sn + 1
    if sn > 255:
        sn = 1
    frame_length = 16
    expect_reply = 0
    transmitter_settling_time = 0
    send_twice = 0
    byte_a = cmd[0] 
    byte_b = cmd[1]
    data = struct.pack('BBBBBBBBBB', 0xAA, HASSEB_DALI_FRAME, sn,
       frame_length, expect_reply,
       transmitter_settling_time, send_twice,
       byte_a, byte_b,
       0)
    return data

if __name__ == "__main__":
    cmd = [0,0]
    if len(sys.argv) == 1:
        print(f"Usage: python {sys.argv[0]} command [shortaddress]")
        print(f"e.g. : python {sys.argv[0]} 5 0")
        exit(-1)

    if len(sys.argv) > 2:
        cmd[0] = int(sys.argv[2])
    cmd[1] = int(sys.argv[1])

    # print(cmd[0],cmd[1])    
    print(construct(cmd))

    import serial
    ser = serial.Serial('COM16',115200)  # open serial port
    print(ser.name)         # check which port was really used
    ser.write(construct(cmd))     # write a string
    ser.close()     

