#include "multimon.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "MQTTAsync.h"

#define DEFAULT_CFG_FILE "/etc/multimon.cfg"
#define DEFAULT_ADDRESS     "tcp://localhost:1883"
#define DEFAULT_CLIENTID    "ExampleClientPub"
#define DEFAULT_TOPIC       "MQTT Examples"
#define DEFAULT_PAYLOAD     "Hello World!"
#define DEFAULT_QOS         1
#define TIMEOUT     10000L

struct cfg {
    char cfg_file[1024];
    char address[255];
    char clientid[64];
    char topic[64];
    char username[64];
    char password[64];
    char qos[1];
    char topicmode[1];
} mqtt_cfg = {
    DEFAULT_CFG_FILE,
    DEFAULT_ADDRESS,
    DEFAULT_CLIENTID,
    DEFAULT_TOPIC,
    "",
    "",
    "1",
    "1"
};

#define NUM_CFG_OPTS 7

const char *cfg_options[] = {
	"address", "clientid", "topic", "username", "password", 
	"qos", "topicmode"
};

char *cfg_vars[] = {
    mqtt_cfg.address, mqtt_cfg.clientid, mqtt_cfg.topic, mqtt_cfg.username, mqtt_cfg.password, mqtt_cfg.qos, mqtt_cfg.topicmode
};

unsigned long ToUInt(char* str)
{
    unsigned long mult = 1;
    unsigned long re = 0;
    int len = strlen(str);
    for(int i = len -1 ; i >= 0 ; i--)
    {
        re = re + ((int)str[i] -48)*mult;
        mult = mult*10;
    }
    return re;
}

MQTTAsync *client;
volatile MQTTAsync_token deliveredtoken;

int finished = 0;

void connlost(void *context, char *cause)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	int rc;

	verbprintf (1, "\nMQTT: Connection lost\n");
	verbprintf (1, "     cause: %s\n", cause);

	verbprintf (1, "MQTT: Reconnecting\n");
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		verbprintf (1, "MQTT: Failed to start connect, return code %d\n", rc);
 		finished = 1;
	}
}


void onDisconnect(void* context, MQTTAsync_successData* response)
{
	MQTTAsync client = (MQTTAsync)context;
	verbprintf (0, "MQTT: Successful disconnection\n");
	finished = 1;
}


void onSend(void* context, MQTTAsync_successData* response)
{
	MQTTAsync client = (MQTTAsync)context;
	verbprintf (1, "MQTT: Message with token value %d delivery confirmed\n", response->token);
}


void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
	MQTTAsync client = (MQTTAsync)context;
	verbprintf (1, "MQTT: Connect failed, rc %d\n", response ? response->code : 0);
	finished = 1;
}


void onConnect(void* context, MQTTAsync_successData* response)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
	int rc;

	verbprintf (1, "MQTT: Successful connection\n");
	
	opts.onSuccess = onSend;
	opts.context = client;

	pubmsg.payload = "Multimon-ng MQTT Client Started";
	pubmsg.payloadlen = (int)strlen(pubmsg.payload);
	pubmsg.qos = 0;
	pubmsg.retained = 0;
	deliveredtoken = 0;

	if ((rc = MQTTAsync_sendMessage(client, mqtt_cfg.topic, &pubmsg, &opts)) != MQTTASYNC_SUCCESS)
	{
		verbprintf (1, "MQTT: Failed to start sendMessage, return code %d\n", rc);
		mqtt_shutdown();
	}
}

void mqtt_publish_msg(int address, char *message)
{
    int rc;
    MQTTAsync_message msg = MQTTAsync_message_initializer;
    msg.payload = message;
    msg.payloadlen = (int)strlen(message);
    msg.qos = 0;
    msg.retained = 0;

    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    opts.onSuccess = onSend;
    opts.context = client;

    verbprintf (2, "MQTT: Received payload to send\n");
    verbprintf (2, "MQTT: Address - %u\n", address);
    verbprintf (2, "MQTT: Message - %s\n", message);
    char mqtttopic[64];
	sprintf(mqtttopic, "%s", mqtt_cfg.topic);

    if (ToUInt(mqtt_cfg.topicmode) == 2) 
    {
        verbprintf (2, "MQTT: Altering Topic to include address: %u\n", address);
        sprintf(mqtttopic, "%s/%u", mqtt_cfg.topic, address);
    }
	verbprintf (2, "MQTT: Sending to topic: %s\n", mqtttopic);
    if ((rc = MQTTAsync_sendMessage(client, mqtttopic, &msg, &opts)) != MQTTASYNC_SUCCESS)
    {
        verbprintf (1, "MQTT: Failed to start sendMessage, return code %d\n", rc);
        mqtt_shutdown();
    }
    verbprintf (2, "MQTT: ----\n");
}

static int cfg_load (void)
{
	FILE *in_file;
	char cfg_buff[64];
	int i;
	

	//strcpy (cfg_buff, getenv("HOME"));
	//strcat (cfg_buff, mqtt_cfg.cfg_file);
	//strcpy (mqtt_cfg.cfg_file, cfg_buff);
	
	verbprintf (2, "MQTT: Attempting to load config file %s\n", mqtt_cfg.cfg_file);

	in_file = fopen (mqtt_cfg.cfg_file, "r");

	if (in_file == NULL)
		return 1;

	//Copy a line from cfg file into cfg_buff
	while (fgets (cfg_buff, 64, in_file) != NULL)
	{
		//Cycle through the different options
		for (i = 0; i < NUM_CFG_OPTS; i++)
		{
			//Look for a match and make sure there's a = after it
			if (strncmp (cfg_buff, cfg_options[i], strlen(cfg_options[i])) == 0 &&
					strncmp (cfg_buff + strlen (cfg_options[i]), "=", 1) == 0)
			{
				//Copy to variable
				strcpy (cfg_vars[i], cfg_buff + strlen (cfg_options[i]) + 1);
				//Strip newline
				strtok (cfg_vars[i], "\n");
				//Check if empty var
				if (strcmp (cfg_vars[i], "\n") == 0)
				{
					verbprintf (1, "MQTT: Empty cfg option %s\n", cfg_options[i]);
					fclose (in_file);
					return 1;
				}
			}
		}
	}
	fclose (in_file);
	return 0;

}

int mqtt_init (void)
{
  //MQTTAsync client;
  MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
  int rc;
  
  if (cfg_load ())
    verbprintf (2, "MQTT: No configuration file found, using defaults\n");
  verbprintf (2, "MQTT: Configuration options:\n");
  verbprintf (2, "MQTT: address = %s\n", mqtt_cfg.address);
  verbprintf (2, "MQTT: clientid = %s\n", mqtt_cfg.clientid);
  verbprintf (2, "MQTT: topic = %s\n", mqtt_cfg.topic);
  verbprintf (2, "MQTT: username = %s\n", mqtt_cfg.username);
  verbprintf (2, "MQTT: qos = %s\n", mqtt_cfg.qos);
  verbprintf (2, "MQTT: topicmode = %s\n", mqtt_cfg.topicmode);
  
  MQTTAsync_create(&client, mqtt_cfg.address, mqtt_cfg.clientid, MQTTCLIENT_PERSISTENCE_NONE, NULL);
  conn_opts.keepAliveInterval = 20;
  conn_opts.cleansession = 1;
  conn_opts.onSuccess = onConnect;
  conn_opts.onFailure = onConnectFailure;
  conn_opts.context = client;
  conn_opts.username = mqtt_cfg.username;
  conn_opts.password = mqtt_cfg.password;
  if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
  {
    verbprintf (1, "MQTT: Failed to connect, return code %d\n", rc);
    return 1;
  }
  
  return 0;
}

void mqtt_shutdown (void)
{
    MQTTAsync_disconnect(client, 10000);
    MQTTAsync_destroy(client);
}
