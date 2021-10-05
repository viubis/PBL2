#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./paho.mqtt.c/src/MQTTClient.h"

#define ADDRESS     "tcp://localhost:1883"
#define PUBID       "ExampleClientPub"
#define SUBID       "ExampleClientSub"
#define ID          "RASPBERRY"

#define TOPIC       "LIGHT"
#define PAYLOAD     "Hello World!"
#define QOS         2
#define TIMEOUT     10000L
#define KEEP_ALIVE  60

volatile MQTTClient_deliveryToken deliveredtoken;

void delivered(void *context, MQTTClient_deliveryToken dt){
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
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

void connlost(void *context, char *cause){
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

void sendMessage(){

}

int main() {
  MQTTClient client;
  MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;

  MQTTClient_message pubmsg = MQTTClient_message_initializer;
  MQTTClient_deliveryToken token;
  int rc;
  int ch;

  MQTTClient_create(&client, ADDRESS, ID,
      MQTTCLIENT_PERSISTENCE_NONE, NULL);

  conn_opts.keepAliveInterval = KEEP_ALIVE;
  conn_opts.cleansession = 1;

  MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

  if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
    printf("Failed to connect, return code %d\n", rc);
    exit(EXIT_FAILURE);
  }

  // printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
  //          "Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);
  //   MQTTClient_subscribe(client, TOPIC, QOS);
  //   do
  //   {
  //       ch = getchar();
  //   } while(ch!='Q' && ch != 'q');

    // pubmsg.payload = PAYLOAD;
    // pubmsg.payloadlen = strlen(PAYLOAD);
    // pubmsg.qos = QOS;
    // pubmsg.retained = 0;
    // deliveredtoken = 0;
    // MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
    // printf("Waiting for publication of %s\n"
    //         "on topic %s for client with ClientID: %s\n",
    //         PAYLOAD, TOPIC, CLIENTID);
    // while(deliveredtoken != token);

  MQTTClient_subscribe(client, TOPIC, QOS);

  do{
      ch = getchar();
      if(ch!='Q' && ch != 'q'){
        pubmsg.payload = PAYLOAD;
        pubmsg.payloadlen = strlen(PAYLOAD);
        pubmsg.qos = QOS;
        pubmsg.retained = 0;
        deliveredtoken = 0;
        MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
        printf("Waiting for publication of %s\n"
                "on topic %s for client with ClientID: %s\n",
                PAYLOAD, TOPIC, ID);
      }
  } while(ch!='Q' && ch != 'q' && deliveredtoken != token);

  

  MQTTClient_disconnect(client, 10000);
  MQTTClient_destroy(&client);
  return rc;
}
