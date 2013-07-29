#! /bin/env python
import argparse
import os
from intelhex import IntelHex

def main(id, src_path):
    eeprom_path = "eeprom_temp.hex"
    eeprom_contents = IntelHex("eedump_0.hex")
    eeprom_contents[6] = id
    eeprom_contents[10] = 0
    # Seen all badges & uber badges
    """
    for i in range(100):
        eeprom_contents[10+i] = 1
    """
    # Seen some badges & uber badges
    """
    for i in range(0,100,2):
        eeprom_contents[10+i] = 1
    """
    # Seen all uber badges
    """
    for i in range(10):
        eeprom_contents[10+i] = 1
    """
    # Seen all badges, not all uber
    """
    for i in range(95):
        eeprom_contents[15+i] = 1
    """
    # 
    with open(eeprom_path, 'w') as eeprom_file:
        eeprom_contents.write_hex_file(eeprom_file)
    cmd_string = "avrdude -p m328p -c usbtiny -u -U hfuse:w:0xDE:m -U lfuse:w:0xBF:m -U flash:w:%s -U eeprom:w:%s" % (src_path, eeprom_path)
    os.system(cmd_string)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Flash a Queercon 10 badge.")
    parser.add_argument('src', metavar='PATH', type=str,
                        help="Path to the image of the program to flash.")
    parser.add_argument('id', metavar='ID', type=int, 
                        help="The ID of the badge to flash.")
    args = parser.parse_args()
    main(args.id, args.src)