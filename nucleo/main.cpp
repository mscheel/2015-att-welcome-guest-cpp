#include "mbed.h"
#include "MbedJSONValue.h"
#include <string>
#include <vector>

Serial pc(SERIAL_TX, SERIAL_RX); // tx, rx
Serial mdot(PA_9, PA_10);
//Motion Sensor
InterruptIn motion(D3);
int motion_detected = 0;

void irq_handler(void)
{
    motion_detected = 1;
}

#define CR              0xD
#define EXPECT          "mDot: "

//m2x.att.com - mscheel@digitalconstruction.com / mscheel
#define M2X_FEED_ID "a2965eb576080bee27f8aec30c4060ac"
#define M2X_API_KEY "83b96039a0c5168b46a9ddb1017efac2"

// define an alias to substitute for device id in M2X streams
// #define ALIAS "5280SENSORS"

static bool send_command(Serial* ser, const string& tx, string& rx);
static int raw_send_command(Serial* ser, const string& tx, string& rx);
static bool rx_done(const string& rx, const string& expect);
MbedJSONValue parse_rx_messages(const string& messages);



/** Send M2X configuration data to Condit Server
 *
 */
void configureMdot()
{
    string cmd;
    string res;

    printf("Configuring mDot...\r\n");

    cmd = "ATSEND feed-id:" M2X_FEED_ID;
    printf("setting feed-id [%s]\r\n", cmd.c_str());
    if (! send_command(&mdot, cmd, res))
        printf("failed to set feed-id\r\n");

    cmd = "ATSEND m2x-key:" M2X_API_KEY;
    printf("setting m2x-key [%s]\r\n", cmd.c_str());
    if (! send_command(&mdot, cmd, res))
        printf("failed to set m2x-key\r\n");

#ifdef ALIAS
    cmd = "ATSEND alias:" ALIAS;
    printf("setting alias [%s]\r\n", cmd.c_str());
    if (! send_command(&mdot, cmd, res))
        printf("failed to set alias\r\n");
#endif

}

/** Read device Id from mDot card
 *
 */
string getDeviceId()
{
    string cmd = "ATID";
    string res;
    string id = "";

    // loop here till we get response
    // if we can't get the id the radio isn't working
    while (id == "") {
        if (! send_command(&mdot, cmd, res)) {
            printf("failed to get device id\r\n");
        } else {
            int id_beg = res.find("Id: ");
            int id_end = res.find("\r", id_beg);

            if (id_beg != string::npos && id_end != string::npos) {
                id_beg += 4;
                id = res.substr(id_beg, id_end-id_beg);
                if (id.size() == 1)
                    id = "0" + id;
            }
        }
    }

    return id;
}

int main()
{
    //motion sensor
    motion.rise(&irq_handler);


    mdot.baud(9600);
    pc.baud(9600);

    string id = getDeviceId();

    printf("Device Id: %s\r\n", id.c_str());

    string cmd;
    string res;

#ifdef ALIAS
    cmd = "ATSEND alias:" ALIAS;
    printf("setting alias [%s]\r\n", cmd.c_str());
    if (! send_command(&mdot, cmd, res))
        printf("failed to set alias\r\n");
#endif

    cmd = "ATSEND feed-id:" M2X_FEED_ID;
    printf("setting feed-id [%s]\r\n", cmd.c_str());
    if (! send_command(&mdot, cmd, res))
        printf("failed to set feed-id\r\n");


    cmd = "ATSEND m2x-key:" M2X_API_KEY;
    printf("setting m2x-key [%s]\r\n", cmd.c_str());
    if (! send_command(&mdot, cmd, res))
        printf("failed to set m2x-key\r\n");


    while (true) {
        if (send_command(&mdot, "ATRECV", res)) {
            MbedJSONValue messages = parse_rx_messages(res);

            if (messages.size() > 0) {
                printf("\r\nparsed %d json messages\r\n\r\n", messages.size());
            } else {
                printf("\r\nNo messages received.\r\n");
            }

            // check messages for config, subscriptions and triggers
            for (int i = 0; i < messages.size(); i++) {
                printf("message %d\r\n", i);
                if (messages[i].hasMember("config")) {
                    // display configuration
                    printf("config %s\r\n", messages[i]["config"].serialize().c_str());

                    bool configured = false;

                    MbedJSONValue &config = messages[i]["config"];

                    if (config.hasMember("feed-id") && config.hasMember("m2x-key")) {
                        // verify configuration
                        string dev_id = config["feed-id"].get<string>();
                        string api_key = config["m2x-key"].get<string>();


                        if (dev_id.compare(M2X_FEED_ID) != 0) {
                            printf("mDot m2x feed-id did not match\r\n");
                            printf("configured: '%s' expected: '%s'\r\n", dev_id.c_str(), M2X_FEED_ID);
                        } else if (api_key.compare(M2X_API_KEY) != 0) {
                            printf("mDot m2x api-key did not match\r\n");
                            printf("configured: '%s' expected: '%s'\r\n", api_key.c_str(), M2X_API_KEY);
                        } else {
                            printf("mDot configured correctly\r\n");
                            configured = true;
                        }
                    }

                    if (!configured) {
                        configureMdot();
                        printf("waiting for server to respond...\r\n");
                        wait(5);
                        continue;
                    }

                } else if (messages[i].hasMember("subs")) {
                    // display subscriptions
                    MbedJSONValue& subs = messages[i]["subs"];

                    if (subs.size() > 0) {
                        printf("%d subscriptions\r\n", subs.size());
                        for (int j = 0; j < subs.size(); j++) {
                            printf("%d - %s\r\n", j, subs[j].get<string>().c_str());
                        }
                    }
                } else if (messages[i].hasMember("s") && messages[i].hasMember("v")) {

                    string stream = messages[i]["s"].get<string>();

                    // display trigger response
                    printf("trigger-stream: '%s'\r\n", stream.c_str());

                    if (messages[i]["v"].getType() == MbedJSONValue::TypeInt)
                        printf("trigger-value: %d\r\n", messages[i]["v"].get<int>());
                    else if (messages[i]["v"].getType() == MbedJSONValue::TypeDouble)
                        printf("trigger-value: %f\r\n", messages[i]["v"].get<double>());
                    else
                        printf("trigger-value: '%s'\r\n\r\n", messages[i]["v"].get<string>().c_str());
                }

                printf("\r\n");
            }

        } else {
            printf("failed to check messages\r\n");
        }


        if(motion_detected) {
            
            motion_detected = 0;
            if (!send_command(&mdot, "atsend hallwaymotion:1", res))
                printf("failed to send temp value\r\n");

            printf("Hello! I've detected motion.\n");
        }


    }
}

static bool send_command(Serial* ser, const string& tx, string& rx)
{
    int ret;

    rx.clear();

    if ((ret = raw_send_command(ser, tx, rx)) < 0) {
        printf("raw_send_command failed %d\r\n", ret);
        return false;
    }
    if (rx.find("OK") == string::npos) {
        printf("no OK in response [%s]\r\n", rx.c_str());
        return false;
    }

    return true;
}

static bool rx_done(const string& rx, const string& expect)
{
    return (rx.find(expect) == string::npos) ? false : true;
}

static int raw_send_command(Serial* ser, const string& tx, string& rx)
{
    if (! ser)
        return -1;

    int to_send = tx.size();
    int sent = 0;
    Timer tmr;
    string junk;

    // send a CR and read any leftover/garbage data
    ser->putc(CR);
    tmr.start();
    while (tmr.read_ms() < 500 && !rx_done(junk, EXPECT))
        if (ser->readable())
            junk += ser->getc();

    tmr.reset();
    tmr.start();
    while (sent < to_send && tmr.read_ms() <= 1000)
        if (ser->writeable())
            ser->putc(tx[sent++]);

    if (sent < to_send)
        return -1;

    // send newline after command
    ser->putc(CR);

    tmr.reset();
    tmr.start();
    while (tmr.read_ms() < 10000 && !rx_done(rx, EXPECT))
        if (ser->readable())
            rx += ser->getc();

    return rx.size();
}

/** Parse ATRECV output for json messages
 */
MbedJSONValue parse_rx_messages(const string& messages)
{
    MbedJSONValue msgs;

    // find start of first json object
    int beg_pos = messages.find("{");
    int end_pos = 0;
    int count = 0;

    while (beg_pos != string::npos) {
        // find end of line
        end_pos = messages.find("\r", beg_pos);

        // pull json string from messages
        string json = messages.substr(beg_pos, end_pos-beg_pos);

        // parse json string
        string ret = parse(msgs[count], json.c_str());

        if (ret != "") {
            printf("invalid json: '%s'\r\n", json.c_str());
        } else {
            count++;
        }

        // find start of next json object
        beg_pos = messages.find("{", end_pos);
    }

    return msgs;
}

