#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTClient.h"
#include <unistd.h>


// #define ADDRESS     "tcp://localhost:1883"
//...................................................................
//... ADRESS
//...................................................................
#define ADDRESS     
#define ID          "RASPBERRY"

#define QOS         2
#define TIMEOUT     10000L
#define KEEP_ALIVE  60

volatile MQTTClient_deliveryToken deliveredtoken;

void onMessageDelivered(void *context, MQTTClient_deliveryToken dt){
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

void tratar_mensagem(char topic[], char message[]){
  printf("%s\n", topic);
  printf("%s\n", message);
  switch(topic){
    case "":
      break;
    default;
      break;
  }
}

int onMessageArrived(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    // int i;
    // char* payloadptr;
    // printf("Message arrived\n");
    // printf("     topic: %s\n", topicName);
    // printf("   message: %s\n", message->payload);
    // payloadptr = message->payload;
    // for(i=0; i<message->payloadlen; i++){
    //   putchar(*payloadptr++);
    // }
    // putchar('\n');
    tratar_mensagem(topicName, message->payload);

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
  MQTTClient_subscribe(client, "TOPICO", QOS);
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
		printf("Faild creating client, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}

  rc = MQTTClient_setCallbacks(client, NULL, onConnectionLost, onMessageArrived, onMessageDelivered);
  if (rc != MQTTCLIENT_SUCCESS) {
    printf("Faild setting callbacks, return code %d\n", rc);
    exit(EXIT_FAILURE);
  }

  MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;
  ssl_opts.verify = 1;
  ssl_opts.CApath = NULL;
  ssl_opts.keyStore = NULL;
  ssl_opts.trustStore = NULL;
  ssl_opts.privateKey = NULL;
  ssl_opts.privateKeyPassword = NULL;
  ssl_opts.enabledCipherSuites = NULL;

  conn_opts.ssl = &ssl_opts;
  conn_opts.keepAliveInterval = KEEP_ALIVE;
  conn_opts.cleansession = 1;
  //...................................................................
  //... USERNAME E PASSWORD
  //...................................................................
  conn_opts.username =  
  conn_opts.password = 

  rc = MQTTClient_connect(client, &conn_opts);
  if (rc != MQTTCLIENT_SUCCESS) {
    printf("Failed to connect, return code %d\n", rc);
    exit(EXIT_FAILURE);
  }

  subscribeTo(client);

  // char *topic = NULL;
  // int topic_len;
  // MQTTClient_message *receive_msg = NULL;
  // char *ptr = NULL;

  do{
      scanf("%s", buffer);

      if(strcmp("q", buffer) != 0){
        publishMessage(client, &token, "JARDIM/LUZ", buffer);
      }

  } while(strcmp("q", buffer) != 0); 

  finishConnection(client);
  return rc;
}
