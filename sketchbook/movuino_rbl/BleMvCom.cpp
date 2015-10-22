#include "BleMvCom.h"

#define DEVICE_NAME "Movuino"

// TODO: Clean this code, remove static var and add then in the object

// It seems that 20 is the maximum value for BLE
#define TXRX_BUF_LEN 20

// Local buffers
// TODO: check this value
#define L_RX_BUF_LEN 128
#define L_TX_BUF_LEN 128

#define UPDATE_CONNECTION_PARAMS_DELAY 500 /* 0.5 seconds */
/* Don't use a really low interval as 8 because the movuino can't
 * keep it, and the processor keeps busy with the ble stack */
#define CONN_INTERVAL 30 /* An integer in miliseconds */
static const Gap::ConnectionParams_t conn_params = {
    /* Set min and max to the same value for now, be deterministic for testing */
    .minConnectionInterval = Gap::MSEC_TO_GAP_DURATION_UNITS(CONN_INTERVAL),
    .maxConnectionInterval = Gap::MSEC_TO_GAP_DURATION_UNITS(CONN_INTERVAL),
    .slaveLatency = 20,
    .connectionSupervisionTimeout = 300, /* 3 seconds (in 10 ms units) */
};
static bool waiting_tx = false;

static BLE ble;

static uint8_t rx_buffer[L_RX_BUF_LEN];
static uint16_t rx_index_b = 0;
static uint16_t rx_index_e = 0;

static uint8_t tx_buffer[L_TX_BUF_LEN];
static uint16_t tx_index_e = 0;
static uint16_t tx_index_b = 0;

// The Nordic UART Service
static const uint8_t service1_uuid[]                = {0x71, 0x3D, 0, 0, 0x50, 0x3E, 0x4C, 0x75, 0xBA, 0x94, 0x31, 0x48, 0xF1, 0x8D, 0x94, 0x1E};
static const uint8_t service1_tx_uuid[]             = {0x71, 0x3D, 0, 3, 0x50, 0x3E, 0x4C, 0x75, 0xBA, 0x94, 0x31, 0x48, 0xF1, 0x8D, 0x94, 0x1E};
static const uint8_t service1_rx_uuid[]             = {0x71, 0x3D, 0, 2, 0x50, 0x3E, 0x4C, 0x75, 0xBA, 0x94, 0x31, 0x48, 0xF1, 0x8D, 0x94, 0x1E};
static const uint8_t uart_base_uuid_rev[]           = {0x1E, 0x94, 0x8D, 0xF1, 0x48, 0x31, 0x94, 0xBA, 0x75, 0x4C, 0x3E, 0x50, 0, 0, 0x3D, 0x71};

static uint8_t tx_value[TXRX_BUF_LEN] = {0,};
static uint8_t rx_value[TXRX_BUF_LEN] = {0,};

static GattCharacteristic  characteristic1(service1_tx_uuid, tx_value, 1, TXRX_BUF_LEN,
       GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE_WITHOUT_RESPONSE );

static GattCharacteristic  characteristic2(service1_rx_uuid, rx_value, 1, TXRX_BUF_LEN, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY);

static GattCharacteristic *uartChars[] = {&characteristic1, &characteristic2};

static GattService         uartService(service1_uuid, uartChars, sizeof(uartChars) / sizeof(GattCharacteristic *));

static void disconnectionCallBack(Gap::Handle_t handle, Gap::DisconnectionReason_t reason)
{
    ble.startAdvertising();
}

static void connectionCallBack(const Gap::ConnectionCallbackParams_t *params)
{
    ble_error_t err;
    Gap::Handle_t gap_handle = params->handle;

    // Wait to update the connection, otherwise it is not taken into account
    // TODO: check this value
    delay(UPDATE_CONNECTION_PARAMS_DELAY);
    err = ble.gap().updateConnectionParams(gap_handle, &conn_params);
    if (err != BLE_ERROR_NONE)
    {
        Serial1.print("Ble error updating conn params:");
        Serial1.println((int)err);
    }
}

void writtenHandle(const GattWriteCallbackParams *Handler)
{
    uint16_t bytesRead;

    /* Reset buffer */
    /* TODO: this should never happen, but if it does, it empty the buffer, not a good thing to do */
    if (rx_index_e + TXRX_BUF_LEN > L_RX_BUF_LEN)
    {
        Serial1.println("BLE overflow");
        rx_index_e = 0;
        rx_index_b = 0;
    }

    if (Handler->handle == characteristic1.getValueAttribute().getHandle()) {
        ble.readCharacteristicValue(characteristic1.getValueAttribute().getHandle(), &rx_buffer[rx_index_e], &bytesRead);
        rx_index_e += bytesRead;
    }
}

static void sendData(void)
{
    uint8_t *b, *e;
    uint16_t size;

    /* If we are still waiting previous tx, return*/
    /* TODO: maybe we should add a timeout here */
    if (waiting_tx)
        return;

    b = &tx_buffer[tx_index_b];
    e = &tx_buffer[tx_index_e];

    /* If the buffer is empty, clean the buffer and return*/
    if (b == e)
    {
        tx_index_b = 0;
        tx_index_e = 0;
        return;
    }

    size = e - b < TXRX_BUF_LEN ? e - b : TXRX_BUF_LEN;
    //Serial1.print("BLE Sending ");
    //Serial1.println(size);
    waiting_tx = true;
    ble.updateCharacteristicValue(characteristic2.getValueAttribute().getHandle(),
                                  b, size);
    tx_index_b += size;
}

static void datasentCallBack(unsigned count)
{
    //Serial1.print("BLE data sent ");
    //Serial1.println(count);
    waiting_tx = false;
    sendData();
}

/**
 * BleMvCom
 *
 * @brief initialize a structure to be used as a MvCom with a ble port
 */
BleMvCom::BleMvCom(void) : GenMvCom()
{
    ble.init();
    // TODO: check it is necessary to set preferred connections params
    ble.setPreferredConnectionParams(&conn_params);
    ble.gattServer().onDataSent(datasentCallBack);
    ble.onConnection(connectionCallBack);
    ble.onDisconnection(disconnectionCallBack);
    ble.onDataWritten(writtenHandle);
      
    // setup adv_data and srp_data
    ble.accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED);
    ble.accumulateAdvertisingPayload(GapAdvertisingData::SHORTENED_LOCAL_NAME,
                                     (const uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME) - 1);
    ble.accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_128BIT_SERVICE_IDS,
                                     (const uint8_t *)uart_base_uuid_rev, sizeof(uart_base_uuid_rev));
							  
    // set adv_type
    ble.setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);    
	  // add service
    ble.addService(uartService);
    // set device name
    ble.setDeviceName((const uint8_t *)DEVICE_NAME);
    // set tx power,valid values are -40, -20, -16, -12, -8, -4, 0, 4
    ble.setTxPower(4);
    // set adv_interval, 100ms in multiples of 0.625ms.
    ble.setAdvertisingInterval(160);
    // set adv_timeout, in seconds
    ble.setAdvertisingTimeout(0);
    // start advertising
    ble.startAdvertising();
}

char BleMvCom::read_byte(void)
{
    // Block if there is no byte available
    while(!available_byte());
    return rx_buffer[rx_index_b++];
}

void BleMvCom::write_bytes(char *string)
{
    int size = sprintf((char*)&tx_buffer[tx_index_e], "%s", string);
    tx_index_e += size;
}

void BleMvCom::write_bytes(char c)
{
    tx_buffer[tx_index_e++] = c;
}

void BleMvCom::write_bytes(char *bytes, int size)
{
    for (int i = 0; i < size; i++)
        tx_buffer[tx_index_e++] = bytes[i];
}

void BleMvCom::write_bytes(int n)
{
    int size = sprintf((char*)&tx_buffer[tx_index_e], "%d", n);
    tx_index_e += size;
}

bool BleMvCom::available_byte(void)
{
    if (rx_index_b == rx_index_e)
    {
        // reset the pointers to the begining of the buffer
        rx_index_b = rx_index_e = 0;
        return false;
    }
    return true;
}

void BleMvCom::flush_bytes(void)
{
    sendData();
}