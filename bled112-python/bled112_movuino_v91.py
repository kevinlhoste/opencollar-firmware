import sys, bglib, serial, optparse, time, signal

ble = 0
ser = 0

uuid_service = [0x28, 0x00] # 0x2800 (uuid of primary services)
uuid_client_characteristic_configuration = [0x29, 0x02] # 0x2902

#uuid_mv_chr = [0x71, 0x3D, 0, 0x02, 0x50, 0x3E, 0x4C, 0x75, 0xBA, 0x94, 0x31, 0x48, 0xF1, 0x8D, 0x94, 0x1E]
uuid_mv_chr = [0xe7, 0xad, 0xd7, 0x80, 0xb0, 0x42, 0x48, 0x76, 0xaa, 0xe1, 0x11, 0x28, 0x55, 0x35, 0x3c, 0xc1]
attr_mv_handle = 0
attr_mv_handle_ccc = 0
connection_handle = 0
cmd_to_send = ""

STATE_STANDBY = 0
STATE_CONNECTING = 1
STATE_FINDING_ATTRIBUTES = 2
STATE_ENABLING_NOTIFICATIONS = 3
STATE_LISTENING_NOTIFICATIONS = 4
state = STATE_STANDBY

# gap_scan_response handler
def my_ble_evt_gap_scan_response(sender, args):
    global ble, ser, state

    # pull all advertised service info from ad packet
    ad_services = []
    local_name = 'unknown'
    this_field = []
    bytes_left = 0
    for b in args['data']:
        if bytes_left == 0:
            bytes_left = b
            this_field = []
        else:
            this_field.append(b)
            bytes_left = bytes_left - 1
            if bytes_left == 0:
                if this_field[0] == 0x02 or this_field[0] == 0x03: # partial or complete list of 16-bit UUIDs
                    for i in xrange((len(this_field) - 1) / 2):
                        ad_services.append(this_field[-1 - i*2 : -3 - i*2 : -1])
                if this_field[0] == 0x04 or this_field[0] == 0x05: # partial or complete list of 32-bit UUIDs
                    for i in xrange((len(this_field) - 1) / 4):
                        ad_services.append(this_field[-1 - i*4 : -5 - i*4 : -1])
                if this_field[0] == 0x06 or this_field[0] == 0x07: # partial or complete list of 128-bit UUIDs
                    for i in xrange((len(this_field) - 1) / 16):
                        ad_services.append(this_field[-1 - i*16 : -17 - i*16 : -1])
                if this_field[0] == 0x08 or this_field[0] == 0x09: # shortened or complete local name
                    local_name = "".join(chr(b) for b in this_field[1:])

    print('Device found:' + local_name)
    print(args)
    #print [map(hex, l) for l in ad_services]

    # connect to this device

    # send "gap_connect_direct" command
    # arguments:
    #  - MAC address
    #  - use detected address type (will work with either public or private addressing)
    #  - 6 = 6*1.25ms = 7.5ms minimum connection interval
    #  - 6 = 6*1.25ms = 7.5ms maximum connection interval
    #  - 10 = 10*10ms = 100ms supervision timeout
    #  - 0 = no slave latency
    ble.send_command(ser, ble.ble_cmd_gap_connect_direct(args['sender'], args['address_type'], 6, 6, 10, 0))
    ble.check_activity(ser, 1)
    state = STATE_CONNECTING

# connection_status handler
def my_ble_evt_connection_status(sender, args):
    global ble, ser, state

    if (args['flags'] & 0x05) == 0x05:
        # connected, now perform service discovery
        print "Connected to %s" % ':'.join(['%02X' % b for b in args['address'][::-1]])
        connection_handle = args['connection']
        # Look for all the characteristics
        #ble.send_command(ser, ble.ble_cmd_attclient_read_by_group_type(args['connection'], 0x0001, 0xFFFF, list(reversed(uuid_service))))
        ble.send_command(ser, ble.ble_cmd_attclient_find_information(args['connection'], 0x0001, 0xFFFF))
        ble.check_activity(ser, 1)
        state = STATE_FINDING_ATTRIBUTES

def my_ble_evt_attclient_find_information_found(sender, args):
    global ble, ser, attr_mv_handle, attr_mv_handle_ccc

    # check for movuino rx characteristic
    if args['uuid'] == list(reversed(uuid_mv_chr)):
        print "Found attribute " + str(map(hex, args['uuid'])) + " handle=%d" % args['chrhandle']
        attr_mv_handle = args['chrhandle']

    # check for subsequent client characteristic configuration
    elif args['uuid'] == list(reversed(uuid_client_characteristic_configuration)) and attr_mv_handle > 0 and attr_mv_handle_ccc == 0:
        print "Found attribute CCC " + str(map(hex, args['uuid'])) + " handle=%d" % args['chrhandle']
        attr_mv_handle_ccc = args['chrhandle']

# attclient_procedure_completed handler
def my_ble_evt_attclient_procedure_completed(sender, args):
    global state, ble, ser, connection_handle

    # check if we just finished searching for services
    if state == STATE_FINDING_ATTRIBUTES:
        if attr_mv_handle > 0 and attr_mv_handle_ccc > 0:
            print "Found 'Movuino' attributes"
            # found the movuino characteristics, so enable notifications
            # (this is done by writing 0x01 to the client characteristic configuration attribute)
            state = STATE_ENABLING_NOTIFICATIONS

            # found the movuino + client characteristic configuration, so enable indications
            # (this is done by writing 0x0002 to the client characteristic configuration attribute)
            ble.send_command(ser, ble.ble_cmd_attclient_attribute_write(connection_handle, attr_mv_handle_ccc, [0x02, 0x00]))
            ble.check_activity(ser, 1)
        else:
            print "Could not find 'Movuino' attributes"

    # We finished enabling notifications
    elif state == STATE_ENABLING_NOTIFICATIONS:
        print "Notification are enabled"
        state = STATE_LISTENING_NOTIFICATIONS
        send_cmd()

# attclient_attribute_value handler
def my_ble_evt_attclient_attribute_value(sender, args):
    global state, ble, ser, connection_handle, att_handle_measurement
    #print("Rcv:" + "".join(chr(b) for b in args['value']), end="")
    #print("Rcv:" + "".join(chr(b) for b in args['value'])),
    sys.stdout.write("".join(chr(b) for b in args['value']))

def my_ble_evt_connection_disconnected(sender, args):
    print "ble_evt_connection_disconnected"
    print sender
    print args
    exit(0)

def send_cmd():
    global ble, ser, state, connection_handle, attr_mv_handle

    data_array = map(ord, cmd_to_send + "\n")
    print("Tx:" + cmd_to_send + str(map(hex, data_array)))
    ble.send_command(ser, ble.ble_cmd_attclient_attribute_write(connection_handle, attr_mv_handle, data_array))
    ble.check_activity(ser, 1)
    state = -1

# handler to notify of an API parser timeout condition
def my_timeout(sender, args):
    # might want to try the following lines to reset, though it probably
    # wouldn't work at this point if it's already timed out:
    #ble.send_command(ser, ble.ble_cmd_system_reset(0))
    #ble.check_activity(ser, 1)
    print "BGAPI parser timed out. Make sure the BLE device is in a known/idle state."

def dummy_ble_evt_attclient_indicated(sender, args):
    print 'dummy_ble_evt_attclient_indicated'
    print args
def dummy_ble_evt_attclient_procedure_completed(sender, args):
    print 'dummy_ble_evt_attclient_procedure_completed'
    print args
def dummy_ble_evt_attclient_group_found(sender, args):
    print 'dummy_ble_evt_attclient_group_found'
    print args
def dummy_ble_evt_attclient_attribute_found(sender, args):
    print 'dummy_ble_evt_attclient_attribute_found'
    print args
def dummy_ble_evt_attclient_find_information_found(sender, args):
    print 'dummy_ble_evt_attclient_find_information_found'
    print args
def dummy_ble_evt_attclient_attribute_value(sender, args):
    print 'dummy_ble_evt_attclient_attribute_value'
    print args
def dummy_ble_evt_attclient_read_multiple_response(sender, args):
    print 'dummy_ble_evt_attclient_read_multiple_response'
    print args

def main():
    global ble, ser, state, cmd_to_send

    # create option parser
    p = optparse.OptionParser(description='BGLib Demo: Movuino')

    # set defaults for options
    p.set_defaults(port="/dev/ttyACM0", baud=115200, packet=False, debug=False, cmd="?")

    # create serial port options argument group
    group = optparse.OptionGroup(p, "Connection Options")
    group.add_option('--port', '-p', type="string", help="Serial port device name (default /dev/ttyACM0)", metavar="PORT")
    group.add_option('--baud', '-b', type="int", help="Serial port baud rate (default 115200)", metavar="BAUD")
    group.add_option('--cmd', '-c', type="string", help="Command to send after connection")
    group.add_option('--packet', '-k', action="store_true", help="Packet mode (prefix API packets with <length> byte)")
    group.add_option('--debug', '-d', action="store_true", help="Debug mode (show raw RX/TX API packets)")
    p.add_option_group(group)

    # actually parse all of the arguments
    options, arguments = p.parse_args()

    # setup the command to be sent
    cmd_to_send = options.cmd

    # create and setup BGLib object
    ble = bglib.BGLib()
    ble.packet_mode = options.packet
    ble.debug = options.debug

    # add handler for BGAPI timeout condition (hopefully won't happen)
    ble.on_timeout += my_timeout

    # debug handlers for BGAPI events
    # ble.ble_evt_attclient_indicated                 += dummy_ble_evt_attclient_indicated
    # ble.ble_evt_attclient_procedure_completed       += dummy_ble_evt_attclient_procedure_completed
    # ble.ble_evt_attclient_group_found               += dummy_ble_evt_attclient_group_found
    # ble.ble_evt_attclient_attribute_found           += dummy_ble_evt_attclient_attribute_found
    # ble.ble_evt_attclient_find_information_found    += dummy_ble_evt_attclient_find_information_found
    # ble.ble_evt_attclient_attribute_value           += dummy_ble_evt_attclient_attribute_value
    # ble.ble_evt_attclient_read_multiple_response    += dummy_ble_evt_attclient_read_multiple_response

    # add handlers for BGAPI events
    ble.ble_evt_gap_scan_response += my_ble_evt_gap_scan_response
    ble.ble_evt_connection_status += my_ble_evt_connection_status
    ble.ble_evt_attclient_find_information_found += my_ble_evt_attclient_find_information_found
    ble.ble_evt_attclient_procedure_completed += my_ble_evt_attclient_procedure_completed
    ble.ble_evt_attclient_attribute_value += my_ble_evt_attclient_attribute_value
    ble.ble_evt_connection_disconnected += my_ble_evt_connection_disconnected

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

    # disconnect if we are connected already
    ble.send_command(ser, ble.ble_cmd_connection_disconnect(0))
    ble.check_activity(ser, 1)

    # stop advertising if we are advertising already
    ble.send_command(ser, ble.ble_cmd_gap_set_mode(0, 0))
    ble.check_activity(ser, 1)

    # stop scanning if we are scanning already
    ble.send_command(ser, ble.ble_cmd_gap_end_procedure())
    ble.check_activity(ser, 1)

    # set scan parameters
    ble.send_command(ser, ble.ble_cmd_gap_set_scan_parameters(0xC8, 0xC8, 1))
    ble.check_activity(ser, 1)

    # start scanning now
    print "Scanning for BLE peripherals..."
    ble.send_command(ser, ble.ble_cmd_gap_discover(2))
    ble.check_activity(ser, 1)

    while (1):
        # check for all incoming data (no timeout, non-blocking)
        ble.check_activity(ser)

        #if state == STATE_LISTENING_NOTIFICATIONS:
        #    send_cmd()

        # don't burden the CPU
        time.sleep(0.01)

# gracefully exit without a big exception message if possible
def ctrl_c_handler(signal, frame):
    print 'Disconnecting...'
    ble.send_command(ser, ble.ble_cmd_connection_disconnect(0))
    current_time = time.time()
    while (time.time() < current_time+5): # Busy wait a little
        ble.check_activity(ser, 1)
    print 'Timeout: Could not disconnect properly'
    exit(0)

signal.signal(signal.SIGINT, ctrl_c_handler)

if __name__ == '__main__':
    main()
