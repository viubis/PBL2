#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTClient.h"
#include <unistd.h>
#include <wiringPi.h>
#include <time.h>
#include <stdbool.h>
#include <locale.h>
/*#include <mongoc/mongoc.h>
#include <bson/bson.h>
#include <json-c/json.h>*/

// #define ADDRESS     "tcp://localhost:1883"
//...................................................................
//... ADRESS
//...................................................................


volatile MQTTClient_deliveryToken deliveredtoken;


// TOPICS TO PUBLISH
#define TOPIC_ILUMINACAO_JARDIM "JARDIM/ILUMINACAO/VALOR"
#define TOPIC_ILUMINACAO_JARDIM_MAX "JARDIM/ILUMINACAO/MAX"
#define TOPIC_ILUMINACAO_JARDIM_MIN "JARDIM/ILUMINACAO/MIN"

#define TOPIC_ILUMINACAO_INTERNO "INTERNO/ILUMINACAO/VALOR"

#define TOPIC_ILUMINACAO_GARAGEM "GARAGEM/ILUMINACAO/VALOR"
#define TOPIC_ILUMINACAO_GARAGEM_MAX "GARAGEM/ILUMINACAO/MAX"
#define TOPIC_ILUMINACAO_GARAGEM_MIN "GARAGEM/ILUMINACAO/MIN"

#define TOPIC_ARCONDICIONADO "AC/VALOR"
#define TOPIC_ARCONDICIONADO_TEMPERATURA "AC/TEMPERATURA"
#define TOPIC_ARCONDICIONADO_MAX "AC/MAX"
#define TOPIC_ARCONDICIONADO_MIN "AC/MIN"
#define TOPIC_ARCONDICIONADO_AUSENCIA_PESSOAS "AC/AUSENCIA_PESSOAS"

#define TOPIC_ALARME "ALARME/VALOR"

#define TOPIC_AUTOMATIC_MODE_VALOR "AUTOMATICMODE/VALOR"


//TOPICS TO SUBSCRIBE
#define TOPIC_ARCONDICIONADO_TEMPERATURA_MAX "AC/TEMPERATURA/MAX"
#define TOPIC_ARCONDICIONADO_TEMPERATURA_MIN "AC/TEMPERATURA/MIN"
#define TOPIC_ARCONDICIONADO_TEMPO_AUSENCIA_PESSOAS "AC/TEMPERATURA/TEMPOAMBIENTEVAZIO"
#define TOPIC_ILUMINACAO_JARDIM_TOGGLE "JARDIM/ILUMINACAO/TOGGLE"
#define TOPIC_ILUMINACAO_JARDIM_HORARIO_MAXIMO "JARDIM/ILUMINACAO/HORARIOMAXIMO"
#define TOPIC_ILUMINACAO_JARDIM_HORARIO_MINIMO "JARDIM/ILUMINACAO/HORARIOMINIMO"
#define TOPIC_ILUMINACAO_GARAGEM_TOGGLE "GARAGEM/ILUMINACAO/TOGGLE"
#define TOPIC_ILUMINACAO_GARAGEM_HORARIO_MAXIMO "GARAGEM/ILUMINACAO/HORARIOMAXIMO"
#define TOPIC_ILUMINACAO_GARAGEM_HORARIO_MINIMO "GARAGEM/ILUMINACAO/HORARIOMINIMO"
#define TOPIC_ILUMINACAO_INTERNO_TOGGLE "INTERNO/ILUMINACAO/TOGGLE"
#define TOPIC_ARCONDICIONADO_TOGGLE "AC/TOGGLE"
#define TOPIC_ALARME_TOGGLE "ALARME/TOGGLE"
#define TOPIC_AC_RESET "AC/RESET"
#define TOPIC_AUTOMATIC_MODE_TOGGLE "AUTOMATICMODE/TOGGLE"

//Entradas Switchs e buttons
#define SWITCH_PRESENCA_SALA 4 
#define SWITCH_PRESENCA_GARAGEM 17
#define SWITCH_PRESENCA_INTERNO 27
#define SWITCH_ALARME 22
#define BUTTON_PORTA 5
#define BUTTON_JANELA 19

/////////////////////////////////////////////////////
// CLIENT E TOKEN
/////////////////////////////////////////////////////
MQTTClient client;
MQTTClient_deliveryToken token;


// ESTADOS
typedef struct {
	int estado_atual;
	int temp_atual;
	int temp_min;
	int temp_max;
	int alterar_operacao_default;
	int tempo_ausencia_pessoas;
} AC;

typedef struct {
	int estado_atual;
} Alarme;

typedef struct {
	int estado_atual;
	int hora_minima;
	int hora_maxima;
} Jardim;

typedef struct {
	int estado_atual;
	int hora_minima;
	int hora_maxima;
} Garagem;

typedef struct {
	int estado_atual;
} LuzInterna;

typedef struct {
	int estado_presenca_garagem_atual;
	int estado_presenca_interna_atual;
	int estado_presenca_sala_atual;
	int estado_alarme_atual;
	int estado_porta_atual;
	int estado_janela_atual;
} EstadoAtualAmbientes;

typedef struct {
	int estado_presenca_garagem_antigo;
	int estado_presenca_interna_antigo;
	int estado_presenca_sala_antigo;
	int estado_alarme_antigo;
	int estado_porta_antigo;
	int estado_janela_antigo;
} EstadoAntigosAmbientes;
	
typedef struct {
	AC ac;
	Alarme alarme;
	Jardim jardim;
	Garagem garagem;
	LuzInterna luzInterna;
	EstadoAtualAmbientes estadoAtualAmbientes;
	EstadoAntigosAmbientes estadoAntigosAmbientes;
	bool automacaoTOGGLE;
} Components;


// VARIÃVEIS GLOBAIS
/*mongoc_client_t *client_mongo;
mongoc_collection_t *collection_alarm, *collection_topics;
//mongoc_collection_t *collection;
bson_error_t error;
bson_oid_t oid;
bson_t *doc, *query;
mongoc_cursor_t *cursor;
struct json_object *parsed_json;
struct json_object *data_json;
size_t len;*/

struct tm *p;
time_t seconds;
time_t desligar_em = 0;
char *str;
int TEMPERATURA_EXTERNA;
bool MENSAGEM_RECEBIDA = false;
int maxMinAtivo = 0, defaultRecebido = 0;
Components comp;


void onConnectionLost(void *context, char *cause){
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

void publishMessage(MQTTClient client, MQTTClient_deliveryToken * token, 
                  char *topic, char *message){
	printf("PREPARANDO PARA PUBLICAR A MENSAGEM\n");
  MQTTClient_message pubmsg = MQTTClient_message_initializer;
  pubmsg.payload = message;
  pubmsg.payloadlen = strlen(message);
  pubmsg.qos = QOS;
  pubmsg.retained = 0;
  deliveredtoken = 0;

  int resp = MQTTClient_publishMessage(client, topic, &pubmsg, token);
	printf("\nRESPOSTA %d\n", resp);
  printf("Waiting for publication -- %s: %s\n", topic, message);
}

void subscribeTo(MQTTClient client){
  MQTTClient_subscribe(client, TOPIC_ARCONDICIONADO_TEMPERATURA_MAX, QOS);
  MQTTClient_subscribe(client, TOPIC_ARCONDICIONADO_TEMPERATURA_MIN, QOS);
  MQTTClient_subscribe(client, TOPIC_ARCONDICIONADO_TEMPO_AUSENCIA_PESSOAS, QOS);
  MQTTClient_subscribe(client, TOPIC_ILUMINACAO_JARDIM_TOGGLE, QOS);
  MQTTClient_subscribe(client, TOPIC_ILUMINACAO_GARAGEM_TOGGLE, QOS);
  MQTTClient_subscribe(client, TOPIC_ILUMINACAO_INTERNO_TOGGLE, QOS);
  MQTTClient_subscribe(client, TOPIC_ARCONDICIONADO_TOGGLE, QOS);
  MQTTClient_subscribe(client, TOPIC_ALARME_TOGGLE, QOS);
  MQTTClient_subscribe(client, TOPIC_AC_RESET, QOS);
  MQTTClient_subscribe(client, TOPIC_AUTOMATIC_MODE_TOGGLE, QOS);
}


// ATUALIZA DB COM OS VALORES DOS LOGS ATUAIS
// type 0 = INT, type 1 =BOOL
/*void atualizarMongo(char key[], int type, int value){
	//collection = mongoc_client_get_collection (client_mongo, DATABASE_DB, COLLECTION_LOG_TOPICS_DB);
	query=bson_new();
	BSON_APPEND_UTF8(query,"id","6164a7695a312709b0574e52");
	if(type == 0){
		doc = BCON_NEW ("$set", "{", "id",BCON_UTF8("6164a7695a312709b0574e52"), key,BCON_INT32(value), "}");
	} else if(type == 1){
		doc = BCON_NEW ("$set", "{", "id",BCON_UTF8("6164a7695a312709b0574e52"), key,BCON_BOOL(value), "}");
	}
	int b = mongoc_collection_update_one(collection_topics,query,doc,NULL,NULL,&error);
	if(!b){
		printf("%s\n",error.message);
	}
}

// RECUPERA OS LOGS DO DB
void recuperaMongo(){
	const bson_t *select;
	//collection = mongoc_client_get_collection (client_mongo, DATABASE_DB, COLLECTION_LOG_TOPICS_DB);

	doc = bson_new ();
    cursor = mongoc_collection_find_with_opts (collection_topics, doc, NULL, NULL);
	printf("RECUPERANDO DO BANCO DE DADOS\n");
	while (mongoc_cursor_next (cursor, &select)) {
		//str = bson_as_canonical_extended_json (select, NULL);
		str = bson_as_relaxed_extended_json (select, &len);
		parsed_json = json_tokener_parse(str);
		json_object_object_get_ex(parsed_json, "ac_toggle", &data_json);
		comp.ac.estado_atual = json_object_get_int(data_json);
		json_object_object_get_ex(parsed_json, "ac_valor_atual", &data_json);
		comp.ac.temp_atual = json_object_get_int(data_json);
		json_object_object_get_ex(parsed_json, "ac_temp_max", &data_json);
		comp.ac.temp_max = json_object_get_int(data_json);
		json_object_object_get_ex(parsed_json, "ac_temp_min", &data_json);
		comp.ac.temp_min = json_object_get_int(data_json);
		json_object_object_get_ex(parsed_json, "ac_tempo_ausencia_pessoas", &data_json);
		comp.ac.tempo_ausencia_pessoas = json_object_get_int(data_json);
		json_object_object_get_ex(parsed_json, "ac_reset", &data_json);
		comp.ac.alterar_operacao_default = json_object_get_int(data_json);
		json_object_object_get_ex(parsed_json, "jardim_toggle", &data_json);
		comp.jardim.estado_atual = json_object_get_int(data_json);
		json_object_object_get_ex(parsed_json, "jardim_hora_max", &data_json);
		comp.jardim.hora_maxima = json_object_get_int(data_json);
		json_object_object_get_ex(parsed_json, "jardim_hora_min", &data_json);
		comp.jardim.hora_minima = json_object_get_int(data_json);
		json_object_object_get_ex(parsed_json, "garagem_toggle", &data_json);
		comp.garagem.estado_atual = json_object_get_int(data_json);
		json_object_object_get_ex(parsed_json, "garagem_hora_max", &data_json);
		comp.garagem.hora_maxima = json_object_get_int(data_json);
		json_object_object_get_ex(parsed_json, "garagem_hora_min", &data_json);
		comp.garagem.hora_minima = json_object_get_int(data_json);
		json_object_object_get_ex(parsed_json, "interno_toggle", &data_json);
		comp.luzInterna.estado_atual = json_object_get_int(data_json);
		json_object_object_get_ex(parsed_json, "alarme_toggle", &data_json);
		comp.alarme.estado_atual = json_object_get_int(data_json);
		json_object_object_get_ex(parsed_json, "automatic_mode", &data_json);
		comp.automacaoTOGGLE = json_object_get_int(data_json);
		bson_free (str);

	}
}

// RECUPERA UM NOVO ESTADO DO ALARME NO DB
void inserirNovoEstadoAlarme(bool peopleAlarm, bool doorsAlert, bool windowsAlert){
	char caractere[2] = ":";
	char caractere_data[2] = "/";
	char time1[10];
	char time2[10];
	char time3[10];
	time(&seconds);
	p = localtime(&seconds);
	//collection = mongoc_client_get_collection (client_mongo, DATABASE_DB, COLLECTION_LOG_ALARMS_DB);
	
	doc = bson_new ();
    bson_oid_init (&oid, NULL);
    BSON_APPEND_OID (doc, "_id", &oid);
	
	snprintf (time1, 10, "%d%s", p->tm_mday, caractere_data);
	snprintf (time2, 10, "%d%s", p->tm_mon, caractere_data);
	snprintf (time3, 10, "%d", p->tm_year + 1900);
	strcat(time1, time2);
	strcat(time1, time3);
	
    BSON_APPEND_UTF8 (doc, "date", time1);

	snprintf (time1, 10, "%d%s", p->tm_hour, caractere);
	snprintf (time2, 10, "%d%s", p->tm_min, caractere);
	snprintf (time3, 10, "%d", p->tm_sec);
	strcat(time1, time2);
	strcat(time1, time3);

    BSON_APPEND_UTF8 (doc, "hour", time1);
    BSON_APPEND_BOOL (doc, "peopleAlarm", peopleAlarm);
    BSON_APPEND_BOOL (doc, "doorsAlert", doorsAlert);
    BSON_APPEND_BOOL (doc, "windowsAlert", windowsAlert);

	if (!mongoc_collection_insert_one (collection_alarm, doc, NULL, NULL, &error)) {
        fprintf (stderr, "%s\n", error.message);
    }
}*/

void finishConnection(MQTTClient client){
	/*bson_destroy(query);
    bson_destroy(doc);
	mongoc_collection_destroy(collection_alarm);
	mongoc_collection_destroy(collection_topics);
	//mongoc_collection_destroy(collection);
    mongoc_client_destroy(client_mongo);
    mongoc_cleanup();*/
		
  	MQTTClient_disconnect(client, 10000);
  	MQTTClient_destroy(&client);
}


// SISTEMA DE ALARME
void alarme(bool temPessoas, bool portasAbertas, bool jarnelasAbertas){
	if(temPessoas || portasAbertas || jarnelasAbertas){
		printf("SaÃ­da do alarme por PRESENÃ‡A DE INTRUSOS, PORTAS E/OU JANELAS ABERTAS.\n");
		comp.alarme.estado_atual = 1;
	} else {
		comp.alarme.estado_atual = 0;
	}
	//inserirNovoEstadoAlarme(temPessoas, portasAbertas, jarnelasAbertas);
	//atualizarMongo("alarme_toggle", 1, comp.alarme.estado_atual);
}

// ILUMINAÃ‡ÃƒO DOS AMBIENTES INTERNOS
void iluminacaoAmbientesInternos(bool temPessoas){
	if(temPessoas){
		printf("IluminaÃ§Ã£o do ambiente ligada.\n");
		comp.luzInterna.estado_atual = 1;
	} else {
		comp.luzInterna.estado_atual = 0;
		printf("IluminaÃ§Ã£o do ambiente desligada.\n");
	}
	//atualizarMongo("interno_toggle", 1, comp.luzInterna.estado_atual);
}

// ILUMINAÃ‡ÃƒO DA GARAGEM
void iluminacaoGaragem(int horaAtual, bool temPessoas){
	if(temPessoas){
		if(horaAtual >= comp.garagem.hora_minima|| horaAtual <= comp.garagem.hora_maxima){ //Ex: maxima = 06 minima = 18
			printf("IluminaÃ§Ã£o da garagem ligada.\n");
			comp.garagem.estado_atual = 1;
		} else {
			comp.garagem.estado_atual = 0;
			printf("IluminaÃ§Ã£o da garagem desligada.\n");
		}
	} else {
		comp.garagem.estado_atual = 0;
		printf("IluminaÃ§Ã£o da garagem desligada.\n");
	}
	//atualizarMongo("garagem_toggle", 1, comp.garagem.estado_atual);
}

// ILUMINAÃ‡ÃƒO DO JARDIM
void iluminacaoJardim(int horaAtual){
	if(horaAtual >= comp.jardim.hora_minima|| horaAtual <= comp.jardim.hora_maxima){ //Ex: maxima = 23 minima = 18
		comp.jardim.estado_atual = 1;
		printf("IluminaÃ§Ã£o do jardim ligada.\n");
	} else {
		comp.jardim.estado_atual = 0;
	}
	//atualizarMongo("jardim_toggle", 1, comp.jardim.estado_atual);
}

// SISTEMA DO AR CONDICIONADO
/*void arCondicionado(bool temPessoas){
	if(!comp.ac.estado_atual && temPessoas){
		comp.ac.estado_atual = true;
	}
	if(comp.ac.alterar_operacao_default){
		if( comp.ac.temp_atual < comp.ac.temp_min || comp.ac.temp_atual > comp.ac.temp_max ){
			comp.ac.temp_atual = arForaFaixaOperacao(comp.ac.temp_atual);
			if( comp.ac.temp_atual >= comp.ac.temp_min || comp.ac.temp_atual <= comp.ac.temp_max ) {
				comp.ac.estado_atual = 1;
				printf("Ar condicionado ligado.\n");
			} else {
				comp.ac.estado_atual = 0;
			}
		}
	} else {
		if(comp.ac.temp_atual >= 17){
			comp.ac.temp_atual = arForaFaixaOperacao(comp.ac.temp_atual);
			if(comp.ac.temp_atual < 17) {
				comp.ac.estado_atual = 1;
				printf("Ar condicionado ligado.\n");
			} else {
				comp.ac.estado_atual = 0;
			}
		}
	}

	if(!temPessoas && comp.ac.estado_atual == 1){
		ausenciaPessoas(comp.ac.tempo_ausencia_pessoas);
		comp.ac.estado_atual = 0;
	}
	atualizarMongo("ac_toggle", 1, comp.ac.estado_atual);
}*/

int arCondicionado(bool temPessoas){
	TEMPERATURA_EXTERNA = (rand() > RAND_MAX / 2);
	if(!comp.ac.estado_atual && temPessoas){
		comp.ac.estado_atual = true;
	}
	if(comp.ac.estado_atual && comp.ac.alterar_operacao_default){
		if( comp.ac.temp_atual < comp.ac.temp_min || comp.ac.temp_atual > comp.ac.temp_max ){
			//atualizarMongo("ac_toggle", 1, 0);
			return 1;
			/*if( comp.ac.temp_atual >= comp.ac.temp_min || comp.ac.temp_atual <= comp.ac.temp_max ) {
				comp.ac.estado_atual = 1;
				printf("Ar condicionado ligado.\n");
			} else {
				comp.ac.estado_atual = 0;
			}*/
		}
	} else {
		if(comp.ac.estado_atual && comp.ac.temp_atual >= 17){
			//atualizarMongo("ac_toggle", 1, 0);
			return 1;
			/*if(comp.ac.temp_atual < 17) {
				comp.ac.estado_atual = 1;
				printf("Ar condicionado ligado.\n");
			} else {
				comp.ac.estado_atual = 0;
			}*/
		}
	}

	if(!temPessoas && comp.ac.estado_atual == 1){
		return 2;
		//comp.ac.estado_atual = 0;
	}
	//atualizarMongo("ac_toggle", 1, comp.ac.estado_atual);
	return 0;
}

void verificarArCondicionado(int returnAC){
	if(returnAC == 2){
		printf("Ar condicionado desligado devido a ausência de pessoas.\n");
		comp.ac.estado_atual = 0;
	} else if(returnAC == 1 && comp.ac.estado_atual && comp.ac.alterar_operacao_default && maxMinAtivo == 3){
		
		if( comp.ac.temp_atual >= comp.ac.temp_min || comp.ac.temp_atual <= comp.ac.temp_max ) {
			comp.ac.estado_atual = 1;
			printf("Ar condicionado ligado.\n");
		} else {
			printf("Ar condicionado ligado.\n");
			comp.ac.estado_atual = 0;
		}
	} else {
		if(comp.ac.estado_atual && comp.ac.temp_atual >= 17) {
			comp.ac.estado_atual = 1;
			printf("Ar condicionado ligado.\n");
		} else {
			printf("Ar condicionado ligado.\n");
			comp.ac.estado_atual = 0;
		}
	}
	//atualizarMongo("ac_toggle", 1, comp.ac.estado_atual);
}



void onMessageDelivered(void *context, MQTTClient_deliveryToken dt){
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}


//////////////////////////////////////////////////////
//// TRATAMENTO DE MENSAGENS
//////////////////////////////////////////////////////
void tratar(char topic[], int message) {
	printf("****************************************\n");
	printf("%s\n", topic);
	printf("****************************************\n");
	char valueAux[10];

	if(strcmp(topic, TOPIC_AUTOMATIC_MODE_TOGGLE) == 0){
		int prox = !comp.automacaoTOGGLE;
		comp.automacaoTOGGLE = prox;
		printf("VALUE NOW: %d\n", prox);
		// atualizarMongo("automatic_mode", 0, prox);
		snprintf(valueAux, 10, "%d", prox);
		publishMessage(client, &token, TOPIC_AUTOMATIC_MODE_VALOR, valueAux);
	}


	if(!comp.automacaoTOGGLE){
		//FUNÇÕES DE TOGGLE
		if(strcmp(topic, TOPIC_ILUMINACAO_JARDIM_TOGGLE) == 0){
			bool prox = !comp.jardim.estado_atual;
			comp.jardim.estado_atual = prox;
			// atualizarMongo("jardim_toggle", 1, message);
			snprintf (valueAux, 10, "%d", message);
			publishMessage(client, &token, TOPIC_ILUMINACAO_JARDIM, valueAux);
		}
		
		else if(strcmp(topic, TOPIC_ILUMINACAO_GARAGEM_TOGGLE) == 0){
			bool prox = !comp.garagem.estado_atual;
			comp.garagem.estado_atual = prox;
			// atualizarMongo("garagem_toggle", 1, message);
			snprintf (valueAux, 10, "%d", message);
			publishMessage(client, &token, TOPIC_ILUMINACAO_GARAGEM, valueAux);
		}
		
		else if(strcmp(topic, TOPIC_ILUMINACAO_INTERNO_TOGGLE) == 0){
			bool prox = !comp.luzInterna.estado_atual;
			comp.luzInterna.estado_atual = prox;
			// atualizarMongo("interno_toggle", 1, message);
			snprintf (valueAux, 10, "%d", message);
			publishMessage(client, &token, TOPIC_ILUMINACAO_INTERNO, valueAux);
		}
		
		else if(strcmp(topic, TOPIC_ALARME_TOGGLE) == 0){
			bool prox = !comp.alarme.estado_atual;
			comp.alarme.estado_atual = prox;
			//inserirNovoEstadoAlarme(0, 0, 0);
			// atualizarMongo("alarme_toggle", 1, message);
			snprintf (valueAux, 10, "%d", message);
			publishMessage(client, &token, TOPIC_ALARME, valueAux);
		}
		
		else if(strcmp(topic, TOPIC_ARCONDICIONADO_TOGGLE) == 0){
			bool prox = !comp.ac.estado_atual;
			comp.ac.estado_atual = prox;
			// atualizarMongo("ac_toggle", 1, message);
			snprintf (valueAux, 10, "%d", message);
			publishMessage(client, &token, TOPIC_ARCONDICIONADO, valueAux);
		}

	}else{
		//FUNÇÕES SEM TOGGLE
		if(strcmp(topic, TOPIC_ARCONDICIONADO_TEMPERATURA_MAX) == 0){
			comp.ac.temp_max = message;
			if(maxMinAtivo < 3){
				maxMinAtivo++;
			}
			// atualizarMongo("ac_temp_max", 0, message);
			snprintf (valueAux, 10, "%d", message);
			publishMessage(client, &token, TOPIC_ARCONDICIONADO_MAX, valueAux);
		}  
		
		else if(strcmp(topic, TOPIC_ARCONDICIONADO_TEMPERATURA_MIN) == 0){
			comp.ac.temp_min = message;
			if(maxMinAtivo < 3){
				maxMinAtivo++;
			}
			// atualizarMongo("ac_temp_min", 0, message);
			snprintf (valueAux, 10, "%d", message);
			publishMessage(client, &token, TOPIC_ARCONDICIONADO_MIN, valueAux);
		}  
		
		else if(strcmp(topic, TOPIC_AC_RESET) == 0){
			comp.ac.alterar_operacao_default = message;
			if(maxMinAtivo < 3 && message == 1){
				maxMinAtivo++;
			} else{
				maxMinAtivo = 0;
			}
			// atualizarMongo("ac_reset", 1, message);
			snprintf (valueAux, 10, "%d", message);
			publishMessage(client, &token, TOPIC_AC_RESET, valueAux);
		}  
		
		else if(strcmp(topic, TOPIC_ARCONDICIONADO_TEMPO_AUSENCIA_PESSOAS) == 0){
			comp.ac.tempo_ausencia_pessoas = message;
			// atualizarMongo("ac_tempo_ausencia_pessoas", 0, message);
			snprintf (valueAux, 10, "%d", message);
			publishMessage(client, &token, TOPIC_ARCONDICIONADO_AUSENCIA_PESSOAS, valueAux);
		}

		else if(strcmp(topic, TOPIC_ILUMINACAO_JARDIM_HORARIO_MAXIMO) == 0){
			comp.jardim.hora_maxima = message;
			// atualizarMongo("jardim_hora_max", 0, message);
			snprintf (valueAux, 10, "%d", message);
			publishMessage(client, &token, TOPIC_ILUMINACAO_JARDIM_MIN, valueAux);
		}  
		
		else if(strcmp(topic, TOPIC_ILUMINACAO_JARDIM_HORARIO_MINIMO) == 0){
			comp.jardim.hora_minima = message;
			// atualizarMongo("jardim_hora_min", 0, message);
			snprintf (valueAux, 10, "%d", message);
			publishMessage(client, &token, TOPIC_ILUMINACAO_JARDIM_MAX, valueAux);
		}  
		
		else if(strcmp(topic, TOPIC_ILUMINACAO_GARAGEM_HORARIO_MAXIMO) == 0){
			comp.garagem.hora_maxima = message;
			// atualizarMongo("garagem_hora_max", 0, message);
			snprintf (valueAux, 10, "%d", message);
			publishMessage(client, &token, TOPIC_ILUMINACAO_GARAGEM_MAX, valueAux);
		}  
		
		else if(strcmp(topic, TOPIC_ILUMINACAO_GARAGEM_HORARIO_MINIMO) == 0){
			comp.garagem.hora_minima = message;
			// atualizarMongo("garagem_hora_min", 0, message);
			snprintf (valueAux, 10, "%d", message);
			publishMessage(client, &token, TOPIC_ILUMINACAO_GARAGEM_MIN, valueAux);
		}
	
	}
}

int onMessageArrived(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
	int value = atoi(message->payload);
    //tratar_mensagem_recebida(topicName, value);
    tratar(topicName, value);
    MENSAGEM_RECEBIDA = true;

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}


int main() {
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	/*mongoc_init();
	client_mongo = mongoc_client_new (CLIENT_DB);
	collection_topics = mongoc_client_get_collection (client_mongo, DATABASE_DB, COLLECTION_LOG_TOPICS_DB);
	collection_alarm = mongoc_client_get_collection (client_mongo, DATABASE_DB, COLLECTION_LOG_ALARMS_DB);
	recuperaMongo();*/
	comp.ac.estado_atual = true;
	comp.ac.temp_atual = 16;
	comp.ac.temp_max = 18;
	comp.ac.temp_min = 21;
	comp.ac.tempo_ausencia_pessoas = 10;
	comp.ac.alterar_operacao_default = 0;
	comp.jardim.estado_atual = false;
	comp.jardim.hora_maxima = 23;
	comp.jardim.hora_minima = 18;
	comp.garagem.estado_atual = false;
	comp.garagem.hora_maxima = 06;
	comp.garagem.hora_minima = 18;
	comp.luzInterna.estado_atual = false;
	comp.alarme.estado_atual = false;
	comp.automacaoTOGGLE = true;
	time(&seconds);
	p = localtime(&seconds);
	wiringPiSetupGpio () ;

	pinMode (SWITCH_PRESENCA_SALA, INPUT) ;
	pinMode (SWITCH_PRESENCA_GARAGEM, INPUT) ;
	pinMode (SWITCH_PRESENCA_INTERNO, INPUT) ;
	pinMode (SWITCH_ALARME, INPUT) ;
	pinMode (BUTTON_PORTA, INPUT) ;
	pinMode (BUTTON_JANELA, INPUT) ;
	printf ("Pinos de botão foram configurados. \n") ;

	int horario_ATUAL = p->tm_hour, rc, returnAC;
	char valueAux[10];
	bool alteracao = false, estadoCronometroAC = false;
	bool temPessoas_ALARME = false, temPessoas_GARAGEM = false, temPessoas_AC = false, temPessoas_INTERNO = false, portasAbertas_ALARME = false, janelasAbertas_ALARME = false, horarioAtual_ATUALIZOU = false;
	comp.automacaoTOGGLE = 1;

	rc = MQTTClient_create(&client, ADDRESS, ID, MQTTCLIENT_PERSISTENCE_NONE, NULL);

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
	conn_opts.username = "";
	conn_opts.password = "";

	rc = MQTTClient_connect(client, &conn_opts);
	if (rc != MQTTCLIENT_SUCCESS) {
		printf("Failed to connect, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}

	subscribeTo(client);

	while(true){

		if(comp.automacaoTOGGLE){
			if(horario_ATUAL < p->tm_hour) {
				horario_ATUAL = p->tm_hour;
				horarioAtual_ATUALIZOU = true;
			}

			comp.estadoAtualAmbientes.estado_presenca_sala_atual = digitalRead(SWITCH_PRESENCA_SALA);
			comp.estadoAtualAmbientes.estado_presenca_garagem_atual = digitalRead(SWITCH_PRESENCA_GARAGEM);
			comp.estadoAtualAmbientes.estado_presenca_interna_atual= digitalRead(SWITCH_PRESENCA_INTERNO);
			comp.estadoAtualAmbientes.estado_alarme_atual= digitalRead(SWITCH_ALARME);
			comp.estadoAtualAmbientes.estado_janela_atual = digitalRead(BUTTON_JANELA);
			comp.estadoAtualAmbientes.estado_porta_atual= digitalRead(BUTTON_PORTA);
			/*comp.estadoAtualAmbientes.estado_presenca_sala_atual = (rand() > RAND_MAX / 2);
			comp.estadoAtualAmbientes.estado_presenca_garagem_atual = (rand() > RAND_MAX / 2);
			comp.estadoAtualAmbientes.estado_presenca_interna_atual= (rand() > RAND_MAX / 2);
			comp.estadoAtualAmbientes.estado_alarme_atual= (rand() > RAND_MAX / 2);
			comp.estadoAtualAmbientes.estado_janela_atual = (rand() > RAND_MAX / 2);
			comp.estadoAtualAmbientes.estado_porta_atual= (rand() > RAND_MAX / 2);
			/*comp.estadoAtualAmbientes.estado_presenca_sala_atual = 0;
			comp.estadoAtualAmbientes.estado_presenca_garagem_atual = 0;
			comp.estadoAtualAmbientes.estado_presenca_interna_atual= 0;
			comp.estadoAtualAmbientes.estado_alarme_atual= 0;
			comp.estadoAtualAmbientes.estado_janela_atual = 0;
			comp.estadoAtualAmbientes.estado_porta_atual= 0;*/

			/*if(	(comp.estadoAtualAmbientes.estado_porta_atual == 0 || comp.estadoAtualAmbientes.estado_janela_atual == 0)&& comp.estadoAtualAmbientes.estado_alarme_atual == 1 ){
				portaJarnelaAbertas_ALARME = true;
			}else{
				portaJarnelaAbertas_ALARME = false;
			}*/
			if(comp.estadoAtualAmbientes.estado_porta_atual == 1){
				portasAbertas_ALARME = true;
			}else{
				portasAbertas_ALARME = false;
			}

			if(comp.estadoAtualAmbientes.estado_janela_atual == 1){
				janelasAbertas_ALARME = true;
			}else{
				janelasAbertas_ALARME = false;
			}

			if( (comp.estadoAtualAmbientes.estado_presenca_sala_atual == 1 || comp.estadoAtualAmbientes.estado_presenca_garagem_atual == 1|| comp.estadoAtualAmbientes.estado_presenca_interna_atual == 1) 
					&& comp.estadoAtualAmbientes.estado_alarme_atual == 1){
						temPessoas_ALARME = true;	
			}else{
				temPessoas_ALARME = false;
			}

			if( comp.estadoAtualAmbientes.estado_presenca_interna_atual == 1){
				temPessoas_INTERNO = true;
			}else{
				temPessoas_INTERNO = false;
			}

			if( comp.estadoAtualAmbientes.estado_presenca_garagem_atual == 1){
				temPessoas_GARAGEM = true;
			}else{
				temPessoas_GARAGEM = false;
			}

			if(comp.estadoAtualAmbientes.estado_presenca_sala_atual == 1){
				temPessoas_AC = true;
			}else{
				temPessoas_AC = false;
			}

			if(comp.estadoAtualAmbientes.estado_porta_atual != comp.estadoAntigosAmbientes.estado_porta_antigo ){
				alteracao = true;
			}else if(comp.estadoAtualAmbientes.estado_janela_atual != comp.estadoAntigosAmbientes.estado_janela_antigo){
				alteracao = true;
			}else if(comp.estadoAtualAmbientes.estado_alarme_atual != comp.estadoAntigosAmbientes.estado_alarme_antigo){
				alteracao = true;
			}else if(comp.estadoAtualAmbientes.estado_presenca_sala_atual != comp.estadoAntigosAmbientes.estado_presenca_sala_antigo){
				alteracao = true;
			}else if(comp.estadoAtualAmbientes.estado_presenca_garagem_atual != comp.estadoAntigosAmbientes.estado_presenca_garagem_antigo){
				alteracao = true;
			}else if(comp.estadoAtualAmbientes.estado_presenca_interna_atual != comp.estadoAntigosAmbientes.estado_presenca_interna_antigo){
				alteracao = true;
			}else{
				alteracao = false;
			}

			if( alteracao || MENSAGEM_RECEBIDA || horarioAtual_ATUALIZOU){

				alarme(temPessoas_ALARME, portasAbertas_ALARME, janelasAbertas_ALARME);
				comp.estadoAntigosAmbientes.estado_alarme_antigo = comp.estadoAtualAmbientes.estado_alarme_atual;
				comp.estadoAntigosAmbientes.estado_porta_antigo = comp.estadoAtualAmbientes.estado_porta_atual;
				comp.estadoAntigosAmbientes.estado_janela_antigo = comp.estadoAtualAmbientes.estado_janela_atual;
				snprintf (valueAux, 10, "%d", comp.alarme.estado_atual);
				printf("%s\n\n", valueAux);
				publishMessage(client, &token, "ALARME/VALOR", valueAux);

				iluminacaoAmbientesInternos(temPessoas_INTERNO);
				comp.estadoAntigosAmbientes.estado_presenca_interna_antigo = comp.estadoAtualAmbientes.estado_presenca_interna_atual;
				snprintf (valueAux, 10, "%d", comp.luzInterna.estado_atual);
				printf("%s\n\n", valueAux);
				publishMessage(client, &token, "INTERNO/ILUMINACAO/VALOR", valueAux);

				iluminacaoGaragem(horario_ATUAL, temPessoas_GARAGEM);
				comp.estadoAntigosAmbientes.estado_presenca_garagem_antigo = comp.estadoAtualAmbientes.estado_presenca_garagem_atual;
				snprintf (valueAux, 10, "%d", comp.garagem.estado_atual);
				printf("%s\n\n", valueAux);
				publishMessage(client, &token, "GARAGEM/ILUMINACAO/VALOR", valueAux);

				iluminacaoJardim(horario_ATUAL);
				comp.estadoAntigosAmbientes.estado_presenca_sala_antigo = comp.estadoAtualAmbientes.estado_presenca_sala_atual;
				snprintf (valueAux, 10, "%d", comp.jardim.estado_atual);
				printf("%s\n\n", valueAux);
				publishMessage(client, &token, "JARDIM/ILUMINACAO/VALOR", valueAux);

				if(estadoCronometroAC && !temPessoas_AC && desligar_em > 0){
					desligar_em = 0;
					estadoCronometroAC = false;
					printf("********************************* TEM PESSOAS E O CRONOMETRO ESTA ATIVO *********************************");
				} else if(estadoCronometroAC){
					time_t now = time(0);
					if(now > desligar_em && desligar_em > 0){
						desligar_em = 0;
						printf("********************************* VAI VERIFICAR O AR CONDICIONADO *********************************");
						if(returnAC == 1){
							// Temperatura do ar aumenta caso a temperatura externa esteja aumentando (igual a 1), caso contrÃ¡rio a temperatura do ar diminui.
							if(TEMPERATURA_EXTERNA == 1) {
								comp.ac.temp_atual+=5;
							} else {
								comp.ac.temp_atual-=5;
							}
						}
						verificarArCondicionado(returnAC);
						estadoCronometroAC = false;
					}
				} else if(!estadoCronometroAC){
					returnAC = arCondicionado(temPessoas_AC);
					if(returnAC == 1){
						snprintf (valueAux, 10, "%d", 0);
						publishMessage(client, &token, "AC/TEMPERATURA/VALOR", valueAux);
						estadoCronometroAC = true;
						time_t agora = time(0);
						desligar_em = agora + 5;
						printf("********************************* PASSOU IF 1 *********************************");
					} else if(returnAC == 2){
						estadoCronometroAC = true;
						time_t agora = time(0);
						desligar_em = agora + 10;
						printf("********************************* PASSOU IF 2 *********************************");
					} else if(returnAC == 0){
						printf("********************************* PASSOU IF 3 *********************************");
						snprintf (valueAux, 10, "%d", comp.ac.estado_atual);
						publishMessage(client, &token, "AC/TEMPERATURA/VALOR", valueAux);
					}
				}
				horarioAtual_ATUALIZOU = false;
				MENSAGEM_RECEBIDA = false;
				alteracao = false;
			}
		}
	}

  	finishConnection(client);
  	return rc;
}


// #include <stdio.h>
// #include <time.h>

// int main(){
//   int controle = 0;
//   int sem_pessoas = 1;
//   time_t desligar_em = 0;


//   while(controle == 0){
//     // ESSA PARTE AQUI EU ACHO QUE VAI SER QUANDO ELE PEGAR DO SENSOR QUE NÃO TEM PESSOA 
//     // HORA DE SETAR O TEMPO PRA DESLIGAR (AUSENCIA DE PESSOAS NO AR CONDICIONADO)

//     if(sem_pessoas && desligar_em == 0){
//       printf("Entrou na função para determinar o tempo que ela vai desligar");
//       time_t agora = time(0);
//       desligar_em = agora + 5 * 60; // PEGA O VALOR ATUAL E INCREMENTA EM 5 MINUTOS
//     }
//     // ESSE IF CONTROLA SE APARECER ALGUMA PESSOA DEPOIS QUE COMEÇOU A CONTAGEM PARA DESLIGAR O AR CONDICIONADO 
//     if(!sem_pessoas && desligar_em > 0){
//       desligar_em = 0;
//     }

//     //HORA PARA CHECAR SE É PRA DESLIGAR
//     // ESSA AQUI EU NÃO SEI QUE HORAS É NÃO 
//     // NEM SEI ONDE COLOCAR TAMBÉM, TEM QUE SER EM QUALQUER LUGAR NO LOOP

//     time_t now = time(0);
//     if(now > desligar_em && desligar_em > 0){
//       controle = 1;
//       desligar_em = 0;
//     }

//     printf("Desligar em: %d ; Agora: %d ; controle: %d\n", desligar_em, now, controle);
//   }

//   return 0;

// }