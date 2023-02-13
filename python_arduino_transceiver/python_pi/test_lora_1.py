""" This program asks a client for data and waits for the response, then sends an ACK. """

# Copyright 2018 Rui Silva.
#
# This file is part of rpsreal/pySX127x, fork of mayeranalytics/pySX127x.
#
# pySX127x is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
# License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# pySX127x is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
# warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more
# details.
#
# You can be released from the requirements of the license by obtaining a commercial license. Such a license is
# mandatory as soon as you develop commercial activities involving pySX127x without disclosing the source code of your
# own applications, or shipping pySX127x with a closed source product.
#
# You should have received a copy of the GNU General Public License along with pySX127.  If not, see
# <http://www.gnu.org/licenses/>.

import time
from time import sleep
import struct

from SX127x.LoRa import *
#from SX127x.LoRaArgumentParser import LoRaArgumentParser
from SX127x.board_config import BOARD

BOARD.setup()
BOARD.reset()
#parser = LoRaArgumentParser("Lora tester")


class mylora(LoRa):
    def __init__(self, verbose=False):
        super(mylora, self).__init__(verbose)
        self.set_mode(MODE.SLEEP)
        self.set_dio_mapping([0] * 6)
        self.var=0

    def on_rx_done(self):
        # BOARD.led_on()
        print("\nRxDone")
        self.clear_irq_flags(RxDone=1)
        payload = self.read_payload(nocheck=True)
        
        print(payload)
        print("Sent to: 0x" + str(payload[1]))
        print("Received from: 0x" + str(payload[2]))
    
        
        lat = []
        for i in range(0,4):
            lat.append(payload[i+3])
        lat_val = struct.unpack('f', bytearray(lat))[0]
        lat_val_format = float("{:.7f}".format(lat_val))
        print("Latitude : " + str(lat_val_format))


        lon = []
        for i in range(0,4):
            lon.append(payload[i+7])
        lon_val = struct.unpack('f', bytearray(lon))[0]
        lon_val_format = float("{:.7f}".format(lon_val))
        print("Longitude : " + str(lon_val_format))


        temperature = []
        for i in range(0,4):
            temperature.append(payload[i+11])
        temperature_val = struct.unpack('f', bytearray(temperature))[0]
        temperature_val_format = float("{:.3f}".format(temperature_val))
        print("Temperature : " + str(temperature_val_format))



        messageLen = payload[15]
        message = []
        for i in range(0,messageLen):
            message.append(payload[i+16])
        print(bytes(message).decode("utf-8",'ignore'))
    
    
        # References 
        # https://stackoverflow.com/questions/34009653/convert-bytes-to-int
        # https://docs.python.org/3/library/struct.html#format-characters
     
       

        self.set_mode(MODE.SLEEP)
        self.reset_ptr_rx()
        # BOARD.led_off()
        self.set_mode(MODE.RXCONT)
        self.var=1

    def on_tx_done(self):
        print("\nTxDone")
        print(self.get_irq_flags())

    def on_cad_done(self):
        print("\non_CadDone")
        print(self.get_irq_flags())

    def on_rx_timeout(self):
        print("\non_RxTimeout")
        print(self.get_irq_flags())

    def on_valid_header(self):
        print("\non_ValidHeader")
        print(self.get_irq_flags())

    def on_payload_crc_error(self):
        print("\non_PayloadCrcError")
        print(self.get_irq_flags())

    def on_fhss_change_channel(self):
        print("\non_FhssChangeChannel")
        print(self.get_irq_flags())

    def getPayload(self, message, destinationAddress=0XBB, servo_angle=0, direction_a=0, direction_a_speed=0):
        _message = list(message.encode('ascii'))
        localAddress = 0xFF
        data = []
        data.append(255) #This is a spacer important or else arduino wont accept packet.
        # data.append(255)
        # data.append(0)
        # data.append(0)
        data.append(destinationAddress)
        data.append(localAddress)
        data.append(servo_angle)
        data.append(direction_a)
        data.append(direction_a_speed)
        data.append(len(_message))
        for i in range(len(_message)):
            data.append(_message[i])
        return data
    

    def start(self):
        while True:
            print("\n\n")
            print("Sending Data")
            payload = self.getPayload(message='cool message', destinationAddress=0XBB, servo_angle=45, direction_a=1, direction_a_speed=100)
            print(payload)
            self.write_payload(payload)
            # self.write_payload([255, 255, 0, 0, 73, 78, 70]) # Send INF
            self.set_mode(MODE.TX)
            self.reset_ptr_rx()
            self.set_mode(MODE.RXCONT)
            sleep(.4)
            





lora = mylora(verbose=False)
lora.set_freq(434.0)
print(lora.get_version())      # this prints the sx127x chip version
print(lora.get_freq())
# Slow+long range  Bw = 125 kHz, Cr = 4/8, Sf = 4096chips/symbol, CRC on. 13 dBm
# Medium Range  Defaults after init are 434.0MHz, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on 13 dBm
# lora.set_pa_config(pa_select=1)
lora.set_pa_config(pa_select=1, max_power=21, output_power=15)
lora.set_bw(BW.BW125)
lora.set_coding_rate(CODING_RATE.CR4_8)
# lora.set_spreading_factor(12)
lora.set_rx_crc(True)
#lora.set_lna_gain(GAIN.G1)
#lora.set_implicit_header_mode(False)
# lora.set_low_data_rate_optim(True)
assert(lora.get_agc_auto_on() == 1)





try:
    print("START")
    lora.start()
except KeyboardInterrupt:
    sys.stdout.flush()
    print("Exit")
    sys.stderr.write("KeyboardInterrupt\n")
finally:
    sys.stdout.flush()
    print("Exit")
    lora.set_mode(MODE.SLEEP)
    BOARD.teardown()