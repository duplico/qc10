#! /bin/env python
import argparse

def main(id, src_path):
    eeprom_path = "eeprom_temp.hex"
    
    cmd_string = "avrdude -p m328p -c usbtiny -u -U lfuse:w:0xDE:m -U hfuse:w:0xBF:m -U flash:w:%s -U eeprom:w:%s" % src_path, eeprom_path
    pass

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Flash a Queercon 10 badge.")
    parser.add_argument('id', metavar='ID', type=int, 
                        help="The ID of the badge to flash.")
    parser.add_argument('src', metavar='PATH', type=str,
                        help="Path to the image of the program to flash.")
    args = parser.parse_args()
    main(args.id, args.src)