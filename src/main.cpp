#include <HASensor.h>
#include <HADevice.h>
#include <HAMqtt.h>
#include <ArduinoOTA.h>
#include <HABinarySensor.h>
#include <HASwitch.h>
#include <WiFiClient.h>
#include <SPI.h>
#include "RF24.h"

WiFiClient client;
HADevice device("smoke-gateway-1");
HAMqtt mqtt(client, device);

const char * ssid = "WaitingOnComcast";
const char * pwd = "";
const char * mqtt_host = "tiltpi.equationoftime.tech";

uint8_t address [] = {0x96,0xb6, 0xd9, 0xca, 0x60};
HASensor * probe1_temp = new HASensor("smoke1_probe1_temp");
HASensor * probe1_alarm_high = new HASensor("smoke1_probe1_alarm_high");
HASensor * probe1_alarm_low = new HASensor("smoke1_probe1_alarm_low");

HASensor * probe2_temp = new HASensor("smoke1_probe2_temp");
HASensor * probe2_alarm_high = new HASensor("smoke1_probe2_alarm_high");
HASensor * probe2_alarm_low = new HASensor("smoke1_probe2_alarm_low");

RF24 radio(22, 5);

struct Smoke_t {
    uint16_t probe1Temp;
    uint16_t probe1High;
    uint16_t probe1Low;

    uint16_t probe2Temp;
    uint16_t probe2High;
    uint16_t probe2Low;

    uint8_t alarm1;
    uint8_t probe1;
    uint8_t alarm2;
    uint8_t probe2;
    uint8_t farenheight;
    uint8_t _nop1;
    uint16_t _end;
    uint16_t _nop2;
};

void configure_smoke() {
    Serial.println("Starting nRF24l0a+");
    probe1_temp->setName("Smoke Probe 1");
    probe1_temp->setDeviceClass("temperature");

    probe1_alarm_high->setName("Smoke Probe 1 alarm high");
    probe1_alarm_high->setDeviceClass("temperature");

    probe1_alarm_low->setName("Smoke Probe 1 alarm low");
    probe1_alarm_low->setDeviceClass("temperature");

    probe2_temp->setName("Smoke Probe 2");
    probe2_temp->setDeviceClass("temperature");

    probe2_alarm_high->setName("Smoke Probe 2 alarm high");
    probe2_alarm_high->setDeviceClass("temperature");

    probe2_alarm_low->setName("Smoke Probe 2 alarm low");
    probe2_alarm_low->setDeviceClass("temperature");

    probe1_temp->setUnitOfMeasurement("°F");
    probe1_alarm_high->setUnitOfMeasurement("°F");
    probe1_alarm_low->setUnitOfMeasurement("°F");
    probe2_temp->setUnitOfMeasurement("°F");
    probe2_alarm_high->setUnitOfMeasurement("°F");
    probe2_alarm_low->setUnitOfMeasurement("°F");

    if (!radio.begin())
    {
        Serial.println("radio hardware didn't respond");
        return;
    }

    //https://hackaday.io/project/160386-blanket-the-smoke-signal-gateway
    radio.setPayloadSize(21);

    radio.setCRCLength(RF24_CRC_16);

    radio.disableDynamicPayloads();
    radio.setAutoAck(false);

    radio.setChannel(70);
    int channel = radio.getChannel();
    Serial.println(channel) ;

    radio.setDataRate(RF24_250KBPS);

//  Promiscuous mode
//    radio.disableCRC();
//    uint8_t address[3] = {0xaa, 0x00};
//    radio.setAddressWidth(2);

    radio.setAddressWidth(5);
    radio.openReadingPipe(1, address);

    radio.startListening();

    Serial.println("radio listening");
}

void setup() {
    Serial.begin(115200);
    while (!Serial) { // needed to keep leonardo/micro from starting too fast!
        delay(10);
    }

    WiFi.setAutoReconnect(true);

    //TODO: A webportal to configure wifi stuff would be awesome.
    //TODO: Save logs to internal memory for recovery
    do{
        Serial.println("Attempting Wifi connection");
        WiFi.begin(ssid, pwd);
    } while (WiFi.waitForConnectResult() != WL_CONNECTED);

    Serial.println("wifi connected");

    device.enableSharedAvailability();
    device.setAvailability(true);

    device.setName("Smoke Gateway");
    device.setSoftwareVersion("1.0.0");
    device.enableLastWill();

    configure_smoke();

    if (!mqtt.begin(mqtt_host, 1883))
    {
        Serial.println("Failed to connect to mqtt broker");
    }

    while (!mqtt.isConnected())
    {
        Serial.println("Connecting to mqtt");
        mqtt.loop();
        delay(500);
    }

}

unsigned long lastReadTime = 0;
void loop() {

    if (!client.connected())
    {
        esp_restart();
    }

    if (!mqtt.isConnected())
    {
        esp_restart();
    }

    if (millis() - lastReadTime > 240000)
    {
        probe1_temp->setAvailability(false);
        probe2_temp->setAvailability(false);
    }

    if (millis() - lastReadTime > 5000)
    {
        uint8_t payload[25] ={0};
        uint8_t pipe;
        if (radio.available(&pipe))
        {
            lastReadTime = millis();
            uint8_t bytes = radio.getPayloadSize();  // get the size of the payload
            Serial.printf("bytes: %d\n", bytes);
            radio.read(&payload, bytes);             // fetch payload from FIFO
            Smoke_t * smoke = (Smoke_t*)payload;

            if (!smoke->probe1)
            {
                probe1_temp->setAvailability(true);
                probe1_temp->setValue(smoke->probe1Temp/10.0);

            } else {
                probe1_temp->setAvailability(false);
            }

            probe1_alarm_high->setValue(smoke->probe1High/10.0);
            probe1_alarm_low->setValue(smoke->probe1Low/10.0);

            if (!smoke->probe2)
            {
                probe2_temp->setValue(smoke->probe2Temp/10.0);
            }
            else {
                probe2_temp->setAvailability(false);
            }
            probe2_alarm_high->setValue(smoke->probe2High/10.0);
            probe2_alarm_low->setValue(smoke->probe2Low/10.0);

        }
    }
}

