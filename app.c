#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTClient.h"

#define ADDRESS     "tcp://localhost:1883"
#define PUBID       "ExampleClientPub"
#define SUBID       "ExampleClientSub"
#define ID          "RASPBERRY"

// #define TOPIC       "LIGHT"
// #define PAYLOAD     "Hello World!"
#define QOS         2
#define TIMEOUT     10000L
#define KEEP_ALIVE  60

volatile MQTTClient_deliveryToken deliveredtoken;

void onMessageDelivered(void *context, MQTTClient_deliveryToken dt){
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int onMessageArrived(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    int i;
    char* payloadptr;
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");
    payloadptr = message->payload;
    for(i=0; i<message->payloadlen; i++)
    {
        putchar(*payloadptr++);
    }
    putchar('\n');
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void onConnectionLost(void *context, char *cause){
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

void publishMessage(MQTTClient client, MQTTClient_deliveryToken * token, 
                  char *topic, char *message){
  MQTTClient_message pubmsg = MQTTClient_message_initializer;
  pubmsg.payload = message;
  pubmsg.payloadlen = strlen(message);
  pubmsg.qos = QOS;
  pubmsg.retained = 0;
  deliveredtoken = 0;
  MQTTClient_publishMessage(client, topic, &pubmsg, token);
  printf("Waiting for publication -- %s: %s\n", topic, message);
}

void subscribeTo(MQTTClient client){
  MQTTClient_subscribe(client, "JARDIM/LUZ", QOS);
}

void finishConnection(MQTTClient client){
  MQTTClient_disconnect(client, 10000);
  MQTTClient_destroy(&client);
}

int main() {
  MQTTClient client;
  MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
  MQTTClient_deliveryToken token;

  int rc;
  char buffer[512];

  rc = MQTTClient_create(&client, ADDRESS, ID,
      MQTTCLIENT_PERSISTENCE_NONE, NULL);

	if(rc != MQTTCLIENT_SUCCESS){
		printf("Faild creating client: %d\n", rc);
		exit(EXIT_FAILURE);
	}

  conn_opts.keepAliveInterval = KEEP_ALIVE;
  conn_opts.cleansession = 1;
  // conn_opts.

  MQTTClient_setCallbacks(client, NULL, onConnectionLost, onMessageArrived, onMessageDelivered);

  rc = MQTTClient_connect(client, &conn_opts);
  if (rc != MQTTCLIENT_SUCCESS) {
    printf("Failed to connect, return code %d\n", rc);
    exit(EXIT_FAILURE);
  }

  subscribeTo(client);

  do{
      // ch = getchar();
      scanf("%s", buffer);

      if(strcmp("q", buffer) != 0){
        publishMessage(client, &token, "JARDIM/LUZ", buffer);
      }

  } while(strcmp("q", buffer) != 0); 

  finishConnection(client);
  return rc;
}
