import sys, serial, optparse, signal

ser = 0

def main():
    global ser

    # create option parser
    p = optparse.OptionParser(description='Serial reader: Movuino')

    # set defaults for options
    p.set_defaults(port="/dev/ttyACM1", baud=115200, packet=False, debug=False, cmd="?")

    # create serial port options argument group
    group = optparse.OptionGroup(p, "Connection Options")
    group.add_option('--port', '-p', type="string", help="Serial port device name (default /dev/ttyACM0)", metavar="PORT")
    group.add_option('--baud', '-b', type="int", help="Serial port baud rate (default 115200)", metavar="BAUD")
    group.add_option('--cmd', '-c', type="string", help="Command to send after connection")
    p.add_option_group(group)

    # actually parse all of the arguments
    options, arguments = p.parse_args()

    # create serial port object
    try:
        ser = serial.Serial(port=options.port, baudrate=options.baud, timeout=1, writeTimeout=1)
    except serial.SerialException as e:
        print "\n================================================================"
        print "Port error (name='%s', baud='%ld'): %s" % (options.port, options.baud, e)
        print "================================================================"
        exit(2)

    # flush buffers
    ser.flushInput()
    ser.flushOutput()

    # send serial command
    ser.write(options.cmd + "\n")

    # read the values
    while(True):
        s = ser.read(1)
        if s:
            sys.stdout.write(s)

# gracefully exit without a big exception message if possible
def ctrl_c_handler(signal, frame):
    global ser
    print 'Disconnecting...'
    ser.close()
    exit(0)

signal.signal(signal.SIGINT, ctrl_c_handler)

if __name__ == '__main__':
    main()
