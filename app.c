#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTClient.h"
#include <unistd.h>
#include <wiringPi.h>
#include <lcd.h>
#include <time.h>
#include <stdbool.h>
#include <locale.h>
/*#include <mongoc/mongoc.h>
#include <bson/bson.h>
#include <json-c/json.h>*/

#define ADDRESS     "ssl://b0b1b4c6d9b148d7bc0f4c535f24c67a.s1.eu.hivemq.cloud:8883"
#define ID          "RASPBERRY"

#define QOS         2
#define TIMEOUT     10000L
#define KEEP_ALIVE  60
/*#define CLIENT_DB	""
#define DATABASE_DB					""
#define COLLECTION_LOG_ALARMS_DB	""
#define COLLECTION_LOG_TOPICS_DB	""*/


volatile MQTTClient_deliveryToken deliveredtoken;


// TOPICS TO PUBLISH
#define TOPIC_ILUMINACAO_JARDIM "JARDIM/ILUMINACAO/VALOR"
#define TOPIC_ILUMINACAO_JARDIM_MAX "JARDIM/ILUMINACAO/HORAMAX"
#define TOPIC_ILUMINACAO_JARDIM_MIN "JARDIM/ILUMINACAO/HORAMIN"
#define TOPIC_ILUMINACAO_INTERNO "INTERNO/ILUMINACAO/VALOR"
#define TOPIC_ILUMINACAO_GARAGEM "GARAGEM/ILUMINACAO/VALOR"
#define TOPIC_ILUMINACAO_GARAGEM_MAX "GARAGEM/ILUMINACAO/HORAMAX"
#define TOPIC_ILUMINACAO_GARAGEM_MIN "GARAGEM/ILUMINACAO/HORAMIN"
#define TOPIC_ARCONDICIONADO "AC/VALOR"
#define TOPIC_ARCONDICIONADO_TEMPERATURA "AC/TEMPERATURA"
#define TOPIC_ARCONDICIONADO_MAX "AC/TEMPERATURAMAX "
#define TOPIC_ARCONDICIONADO_MIN "AC/TEMPERATURAMIN"
#define TOPIC_ARCONDICIONADO_AUSENCIA_PESSOAS "AC/TEMPOAUSENCIAPESSOAS"
#define TOPIC_ALARME "ALARME/VALOR"
#define TOPIC_AUTOMATIC_MODE_VALOR "AUTOMATICMODE/VALOR"
#define TOPIC_PING_RESPONSE "PINGRESPONSE"


//TOPICS TO SUBSCRIBE
#define TOPIC_ARCONDICIONADO_TEMPERATURA_MAX "AC/SETTEMPERATURAMAX"
#define TOPIC_ARCONDICIONADO_TEMPERATURA_MIN "AC/SETTEMPERATURAMIN"
#define TOPIC_ARCONDICIONADO_LIGADO "AC/SETLIGADO"
#define TOPIC_ARCONDICIONADO_DESLIGADO "AC/SETDESLIGADO"
#define TOPIC_ARCONDICIONADO_TEMPO_AUSENCIA_PESSOAS "AC/SETTEMPOAMBIENTEVAZIO"
#define TOPIC_ILUMINACAO_JARDIM_TOGGLE "JARDIM/ILUMINACAO/TOGGLE"
#define TOPIC_ILUMINACAO_JARDIM_HORARIO_MAXIMO "JARDIM/ILUMINACAO/SETHORARIOMAXIMO"
#define TOPIC_ILUMINACAO_JARDIM_HORARIO_MINIMO "JARDIM/ILUMINACAO/SETHORARIOMINIMO"
#define TOPIC_ILUMINACAO_JARDIM_LIGADO "JARDIM/ILUMINACAO/SETLIGADO"
#define TOPIC_ILUMINACAO_JARDIM_DESLIGADO "JARDIM/ILUMINACAO/SETDESLIGADO"
#define TOPIC_ILUMINACAO_GARAGEM_TOGGLE "GARAGEM/ILUMINACAO/TOGGLE"
#define TOPIC_ILUMINACAO_GARAGEM_LIGADO "GARAGEM/ILUMINACAO/LIGADO"
#define TOPIC_ILUMINACAO_GARAGEM_DESLIGADO "GARAGEM/ILUMINACAO/DESLIGADO"
#define TOPIC_ILUMINACAO_GARAGEM_HORARIO_MAXIMO "GARAGEM/ILUMINACAO/SETHORARIOMAXIMO"
#define TOPIC_ILUMINACAO_GARAGEM_HORARIO_MINIMO "GARAGEM/ILUMINACAO/SETHORARIOMINIMO"
#define TOPIC_ILUMINACAO_INTERNO_TOGGLE "INTERNO/ILUMINACAO/TOGGLE"
#define TOPIC_ILUMINACAO_INTERNO_LIGADO "INTERNO/ILUMINACAO/SETLIGADO"
#define TOPIC_ILUMINACAO_INTERNO_DESLIGADO "INTERNO/ILUMINACAO/SETDESLIGADO"
#define TOPIC_ARCONDICIONADO_TOGGLE "AC/TOGGLE"
#define TOPIC_ALARME_TOGGLE "ALARME/TOGGLE"
#define TOPIC_ALARME_LIGADO "ALARME/SETLIGADO"
#define TOPIC_ALARME_DESLIGADO "ALARME/SETDESLIGADO"
#define TOPIC_AC_RESET "AC/RESET"
#define TOPIC_AUTOMATIC_MODE_TOGGLE "AUTOMATICMODE/TOGGLE"
#define TOPIC_AUTOMATIC_MODE_LIGADO "AUTOMATICMODE/SETLIGADO"
#define TOPIC_AUTOMATIC_MODE_DESLIGADO "AUTOMATICMODE/SETDESLIGADO"
#define TOPIC_PING_REQUEST "PINGREQUEST"

//Entradas Switchs e buttons
#define SWITCH_PRESENCA_SALA 4 
#define SWITCH_PRESENCA_GARAGEM 17
#define SWITCH_PRESENCA_INTERNO 27
#define SWITCH_ALARME 22
#define BUTTON_PORTA 5
#define BUTTON_JANELA 19

//Conf LCD
#define LCD_RS  25               //Register select pin
#define LCD_E   1                //Enable Pin
#define LCD_D4  12               //Data pin 4
#define LCD_D5  16               //Data pin 5
#define LCD_D6  20               //Data pin 6
#define LCD_D7  21               //Data pin 7

#define MUDANCA_ESTADO_MODO_AUTOMATICO 1
#define MUDANCA_ILUMINACAO_JARDIM 2
#define MUDANCA_ILUMINACAO_GARAGEM 3
#define MUDANCA_ILUMINACAO_INTERNO 4
#define MUDANCA_ALARME 5
#define MUDANCA_ARCONDICIONADO 6

/////////////////////////////////////////////////////
// CLIENT E TOKEN
/////////////////////////////////////////////////////
MQTTClient client;
int MENSAGEM_RECEBIDA = -1;
int lcd;

// ESTADOS
typedef struct {
	int estado_atual;
	int temp_atual;
	int temp_min;
	int temp_max;
	int alterar_operacao_default;
	int tempo_ausencia_pessoas;
	time_t desligar_em_s;
	int verificarTemperatura;
} AC;

typedef struct {
	int estado_atual;
	int alarmeLigado;
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
	int	garagem;
	int sala;
	int interno;
	int alarme;
	int porta;
	int janela;
} EstadosInputs;

typedef struct {
	AC ac;
	Alarme alarme;
	Jardim jardim;
	Garagem garagem;
	LuzInterna luzInterna;
	EstadosInputs estados_inputs;
	bool automacaoTOGGLE;
} Components;


// VARIAVEIS GLOBAIS
// mongoc_client_t *client_mongo;
// mongoc_collection_t *collection_alarm, *collection_topics;
// bson_error_t error;
// bson_oid_t oid;
// bson_t *doc, *query;
// mongoc_cursor_t *cursor;
// struct json_object *parsed_json;
// struct json_object *data_json;
// size_t len;

struct tm *p;
time_t seconds;
time_t desligar_em = 0;
char *str;
int TEMPERATURA_EXTERNA, ALTERACAO_LOGS = 0, horarioInicio, dataInicio;
int maxMinAtivo = 0, defaultRecebido = 0, estadoAnteriorAC = -1;
Components comp;


void onConnectionLost(void *context, char *cause){
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

void publishMessage(MQTTClient client, char *topic, char *message){
	printf("\nPublishing message\n");
  MQTTClient_message pubmsg = MQTTClient_message_initializer;
  pubmsg.payload = message;
  pubmsg.payloadlen = strlen(pubmsg.payload);
  pubmsg.qos = QOS;
  pubmsg.retained = 1;
	MQTTClient_deliveryToken token;
  MQTTClient_publishMessage(client, topic, &pubmsg, &token);
	printf("Waiting for publication -- %s: %s\n", topic, message);
  // int resp = MQTTClient_publishMessage(client, topic, &pubmsg, token);
}

void subscribeTo(MQTTClient client){
	MQTTClient_subscribe(client, TOPIC_ARCONDICIONADO_TEMPERATURA_MAX, QOS);
	MQTTClient_subscribe(client, TOPIC_ARCONDICIONADO_TEMPERATURA_MIN, QOS);
	MQTTClient_subscribe(client, TOPIC_ARCONDICIONADO_DESLIGADO, QOS);
	MQTTClient_subscribe(client, TOPIC_ARCONDICIONADO_LIGADO, QOS);
	MQTTClient_subscribe(client, TOPIC_ARCONDICIONADO_TEMPO_AUSENCIA_PESSOAS, QOS);
	MQTTClient_subscribe(client, TOPIC_ILUMINACAO_JARDIM_TOGGLE, QOS);
	MQTTClient_subscribe(client, TOPIC_ILUMINACAO_JARDIM_LIGADO, QOS);
	MQTTClient_subscribe(client, TOPIC_ILUMINACAO_JARDIM_DESLIGADO, QOS);
	MQTTClient_subscribe(client, TOPIC_ILUMINACAO_GARAGEM_TOGGLE, QOS);
	MQTTClient_subscribe(client, TOPIC_ILUMINACAO_GARAGEM_LIGADO, QOS);
	MQTTClient_subscribe(client, TOPIC_ILUMINACAO_GARAGEM_DESLIGADO, QOS);
	MQTTClient_subscribe(client, TOPIC_ILUMINACAO_INTERNO_TOGGLE, QOS);
	MQTTClient_subscribe(client, TOPIC_ILUMINACAO_INTERNO_LIGADO, QOS);
	MQTTClient_subscribe(client, TOPIC_ILUMINACAO_INTERNO_DESLIGADO, QOS);
	MQTTClient_subscribe(client, TOPIC_ARCONDICIONADO_TOGGLE, QOS);
	MQTTClient_subscribe(client, TOPIC_ALARME_TOGGLE, QOS);
	MQTTClient_subscribe(client, TOPIC_ALARME_LIGADO, QOS);
	MQTTClient_subscribe(client, TOPIC_ALARME_DESLIGADO, QOS);
	MQTTClient_subscribe(client, TOPIC_AC_RESET, QOS);
	MQTTClient_subscribe(client, TOPIC_AUTOMATIC_MODE_TOGGLE, QOS);
	MQTTClient_subscribe(client, TOPIC_AUTOMATIC_MODE_LIGADO, QOS);
	MQTTClient_subscribe(client, TOPIC_AUTOMATIC_MODE_DESLIGADO, QOS);
	MQTTClient_subscribe(client, TOPIC_PING_REQUEST, QOS);
}


// ATUALIZA DB COM OS VALORES DOS LOGS ATUAIS 
// type 0 = INT, type 1 =BOOL
/*void atualizarMongo(char key[], int type, int value){
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

	doc = bson_new ();
    cursor = mongoc_collection_find_with_opts (collection_topics, doc, NULL, NULL);
	printf("RECUPERANDO DO BANCO DE DADOS\n");
	while (mongoc_cursor_next (cursor, &select)) {
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
		json_object_object_get_ex(parsed_json, "alarme_ligado", &data_json);
		comp.alarme.alarmeLigado = json_object_get_int(data_json);
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

void atualizarLogsLocal(){
	char caractere[2] = ":";
	char caractere_data[2] = "/";
	char hora[10];
	char time2[10];
	char time3[10];
	char data[10];
	char data2[10];
	char data3[10];
	FILE *file;
	time(&seconds);
	p = localtime(&seconds);
	
	snprintf (data, 10, "%d%s", p->tm_mday, caractere_data);
	snprintf (data2, 10, "%d%s", p->tm_mon, caractere_data);
	snprintf (data3, 10, "%d", p->tm_year + 1900);
	strcat(data, data2);
	strcat(data, data3);

    snprintf (hora, 10, "%d%s", p->tm_hour, caractere);
	snprintf (time2, 10, "%d%s", p->tm_min, caractere);
	snprintf (time3, 10, "%d", p->tm_sec);
	strcat(hora, time2);
	strcat(hora, time3);

	if (horarioInicio == p->tm_hour && dataInicio != p->tm_mday){
        file = fopen("logs.csv", "w");
        fprintf(file, "%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s\n", "Data", "Hora", "Ar Condicionado Toggle", "Ar Condicionado Valor Atual", "Ar Condicionado Temperatura Max", "Ar Condicionado Temperatura Min", "Ar Condicionado Tempo Ausencia Pessoas", "Ar Condicionado Reset", "Jardim Toggle", "Jardim Horario Max", "Jardim Horario Min", "Garagem Toggle", "Garagem Horario Max", "Garagem Horario Min", "Interno Toggle", "Alarme Toggle", "Alarme Ligado", "Modo Automatico");
		dataInicio = p->tm_mday;
	} else {
        file = fopen("logs.csv", "a");
    }
	fprintf(file, "%s;%s;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d\n", data, hora, comp.ac.estado_atual, comp.ac.temp_atual, comp.ac.temp_max, comp.ac.temp_min, comp.ac.tempo_ausencia_pessoas, comp.ac.alterar_operacao_default, comp.jardim.estado_atual, comp.jardim.hora_maxima, comp.jardim.hora_minima, comp.garagem.estado_atual, comp.garagem.hora_maxima, comp.garagem.hora_minima, comp.luzInterna.estado_atual, comp.alarme.estado_atual, comp.alarme.alarmeLigado, comp.automacaoTOGGLE);
	fclose(file);
}

void finishConnection(MQTTClient client){
	/*bson_destroy(query);
    bson_destroy(doc);
	mongoc_collection_destroy(collection_alarm);
	mongoc_collection_destroy(collection_topics);
    mongoc_client_destroy(client_mongo);
    mongoc_cleanup();*/

	MQTTClient_disconnect(client, 10000);
	MQTTClient_destroy(&client);
}


// SISTEMA DE ALARME
void alarme(bool temPessoas, bool portasAbertas, bool janelasAbertas, bool alarmeAtivo){
	char valueAux[10];
	int ativo = 0;

	if((temPessoas || portasAbertas || janelasAbertas) && (alarmeAtivo == true)){
		ativo = 1;
		lcdClear(lcd);
    	lcdPuts(lcd, "Alarme:");
    	lcdPosition(lcd, 4, 1);
    	lcdPuts(lcd, "Ativo");
	}

	// ESTADO DIFERENTE DO ESTADO ATUAL, REQUER MUDANÇA
	if(comp.alarme.estado_atual != ativo){
		comp.alarme.estado_atual = ativo;

		printf("Anterior: %d\n", comp.alarme.estado_atual);
		printf("Atual: %d\n", ativo);
		//Aqui ele vai dizer que alterou
		if(ativo == 1){
			printf("Sai­da do alarme por PRESENCA DE INTRUSOS, PORTAS E/OU JANELAS ABERTAS.\n");
		}
		
		//inserirNovoEstadoAlarme(temPessoas, portasAbertas, janelasAbertas);
		//atualizarMongo("alarme_toggle", 1, comp.alarme.estado_atual);
		ALTERACAO_LOGS = 1;

		snprintf (valueAux, 10, "%d", comp.alarme.estado_atual);
		printf("%s\n\n", valueAux);
		publishMessage(client, TOPIC_ALARME, valueAux);
		
	}
}

// ILUMINACAO DOS AMBIENTES INTERNOS
void iluminacaoAmbientesInternos(bool temPessoas){
	char valueAux[10];
	int ativo = 0;

	if(temPessoas){
		ativo = 1;
	}

	// ESTADO DIFERENTE DO ESTADO ATUAL, REQUER MUDANÇA
	if(comp.luzInterna.estado_atual != ativo){
		comp.luzInterna.estado_atual = ativo;

		//Aqui ele vai dizer que alterou
		if(ativo == 1){
			lcdClear(lcd);
    		lcdPuts(lcd, "Luz interna:");
    		lcdPosition(lcd, 4, 1);
    		lcdPuts(lcd, "Ativo");
			printf("Iluminacao do ambiente interno ligada.\n");
		}else {
			lcdClear(lcd);
    		lcdPuts(lcd, "Luz interna:");
    		lcdPosition(lcd, 4, 1);
    		lcdPuts(lcd, "Desligada");
			printf("Iluminaco do ambiente interno desligada.\n");
		}
			
		//atualizarMongo("interno_toggle", 1, comp.luzInterna.estado_atual);
		ALTERACAO_LOGS = 1;

		snprintf (valueAux, 10, "%d", comp.luzInterna.estado_atual);
		printf("%s\n\n", valueAux);
		publishMessage(client, TOPIC_ILUMINACAO_INTERNO, valueAux);
		
	}
}

// ILUMINACAO DA GARAGEM
void iluminacaoGaragem(int horaAtual, bool temPessoas){
	char valueAux[10];
	int ativo = 0;

	if(temPessoas && (horaAtual >= comp.garagem.hora_minima || horaAtual <= comp.garagem.hora_maxima)){
		ativo = 1;
	}

	// ESTADO DIFERENTE DO ESTADO ATUAL, REQUER MUDANÇA
	if(comp.garagem.estado_atual != ativo){
		comp.garagem.estado_atual = ativo;

		if(ativo == 1){
			lcdClear(lcd);
    		lcdPuts(lcd, "Garagem:");
    		lcdPosition(lcd, 4, 1);
    		lcdPuts(lcd, "luz ativa");
			printf("Iluminacao do ambiente ligada.\n");
		}else {
			lcdClear(lcd);
    		lcdPuts(lcd, "Garagem:");
    		lcdPosition(lcd, 4, 1);
    		lcdPuts(lcd, "luz desativada");
			printf("Iluminaco do ambiente desligada.\n");
		}

		//atualizarMongo("garagem_toggle", 1, comp.garagem.estado_atual);
		ALTERACAO_LOGS = 1;
		snprintf (valueAux, 10, "%d", comp.garagem.estado_atual);
		printf("%s\n\n", valueAux);
		publishMessage(client, TOPIC_ILUMINACAO_GARAGEM, valueAux);		

	}
}

// ILUMINACAO DO JARDIM
void iluminacaoJardim(int horaAtual){
	char valueAux[10];
	int ativo = 0;

	if(horaAtual >= comp.jardim.hora_minima|| horaAtual <= comp.jardim.hora_maxima){ //Ex: maxima = 23 minima = 18
		ativo = 1;
	}

	if(comp.jardim.estado_atual != ativo){
		comp.jardim.estado_atual = ativo;

		if(ativo == 1){
			lcdClear(lcd);
    		lcdPuts(lcd, "Luz jardim:");
    		lcdPosition(lcd, 4, 1);
    		lcdPuts(lcd, "Ativa");
			printf("Iluminacao do Jardim ligada.\n");
		}else {
			lcdClear(lcd);
    		lcdPuts(lcd, "Luz jardim:");
    		lcdPosition(lcd, 4, 1);
    		lcdPuts(lcd, "Desligado");
			printf("Iluminacao do Jardim desligada.\n");
		}

		//atualizarMongo("jardim_toggle", 1, comp.jardim.estado_atual);
		ALTERACAO_LOGS = 1;
		snprintf (valueAux, 10, "%d", comp.jardim.estado_atual);
		printf("%s\n\n", valueAux);
		publishMessage(client, TOPIC_ILUMINACAO_JARDIM, valueAux);
	}

}

// VERIFICAÇÃO DE PRESENÇA DE PESSOAS E TEMPERATURA DO AR
// RETURN 1 = temperatura fora da faixa de operação default ou ajustada.
// RETURN 2 = temperatura dentro da faixa de operação porém ausência de pessoas no ambiente.
// RETURN 0 = ar condicionado desligado ou ligado com a temperatura dentro da faixa de operação na presença de pessoas no ambiente.
int arCondicionado(bool temPessoas){
	TEMPERATURA_EXTERNA = (rand() > RAND_MAX / 2);
	char valueAux[10];
	estadoAnteriorAC = comp.ac.estado_atual;
	if(!comp.ac.estado_atual && temPessoas){
		comp.ac.estado_atual = true;
		//atualizarMongo("ac_toggle", 1, 1);
		ALTERACAO_LOGS = 1;
		snprintf (valueAux, 10, "%d", comp.ac.estado_atual);
		publishMessage(client, TOPIC_ARCONDICIONADO, valueAux);
	}
	if(comp.ac.estado_atual && comp.ac.alterar_operacao_default){
		if( comp.ac.temp_atual < comp.ac.temp_min || comp.ac.temp_atual > comp.ac.temp_max ){
			//atualizarMongo("ac_toggle", 1, 0);
			ALTERACAO_LOGS = 1;
			return 1;

		}
	} else {
		if(comp.ac.estado_atual && comp.ac.temp_atual >= 17){
			//atualizarMongo("ac_toggle", 1, 0);
			ALTERACAO_LOGS = 1;
			return 1;
		}
	}

	if(!temPessoas && comp.ac.estado_atual == 1){
		return 2;
	}
	if(estadoAnteriorAC != comp.ac.estado_atual && temPessoas){
		//atualizarMongo("ac_toggle", 1, comp.ac.estado_atual);
		ALTERACAO_LOGS = 1;
		snprintf (valueAux, 10, "%d", comp.ac.estado_atual);
		publishMessage(client, TOPIC_ARCONDICIONADO, valueAux);
	}
	return 0;
}

// VERICAÇÃO FINAL APÓS O TEMPO DO AR CONDICIONADO DESLIGADO POR ESTAR COM A TEMPERATURA FORA DA FAIXA DE OPERAÇÃO OU AUSÊNCIA DE PESSOAS.
// returnAC 1 = temperatura fora da faixa de operação default ou ajustada.
// returnAC 2 = temperatura dentro da faixa de operação porém ausência de pessoas no ambiente.
// returnAC 0 = ar condicionado desligado ou ligado com a temperatura dentro da faixa de operação na presença de pessoas no ambiente.
void verificarArCondicionado(int returnAC){
	int atual = comp.ac.estado_atual;
	int prox;
	char valueAux[10];

	if(returnAC == 2){
		printf("Ar condicionado desligado devido a ausência de pessoas.\n");
		comp.ac.estado_atual = 0;
		prox = 0;
	} else if(returnAC == 1 && comp.ac.estado_atual && comp.ac.alterar_operacao_default && maxMinAtivo == 3){
		
		if( comp.ac.temp_atual >= comp.ac.temp_min || comp.ac.temp_atual <= comp.ac.temp_max ) {
			comp.ac.estado_atual = 1;
			prox = 1;
			lcdClear(lcd);
			lcdPuts(lcd, "AR:");
			lcdPosition(lcd, 4, 1);
			lcdPuts(lcd, "Ligado");
			printf("Ar condicionado ligado.\n");
		} else {
			lcdClear(lcd);
    		lcdPuts(lcd, "AR:");
    		lcdPosition(lcd, 4, 1);
			lcdPuts(lcd, "Desligado");
			printf("Ar condicionado desligado.\n");
			comp.ac.estado_atual = 0;
			prox = 0;
		}
	} else {
		if(comp.ac.estado_atual && comp.ac.temp_atual < 17) {
			comp.ac.estado_atual = 1;
			prox = 1;
			lcdClear(lcd);
			lcdPuts(lcd, "AR:");
			lcdPosition(lcd, 4, 1);
			lcdPuts(lcd, "Ligado");
			printf("Ar condicionado ligado.\n");
		} else {
			lcdClear(lcd);
    		lcdPuts(lcd, "AR:");
    		lcdPosition(lcd, 4, 1);
			lcdPuts(lcd, "Desligado");
			printf("Ar condicionado desligado.\n");
			comp.ac.estado_atual = 0;
			prox = 0;
		}
	}

	if(atual != prox) {
		snprintf (valueAux, 10, "%d", comp.ac.estado_atual);
		publishMessage(client, TOPIC_ARCONDICIONADO, valueAux);
		//atualizarMongo("ac_toggle", 1, comp.ac.estado_atual);
		ALTERACAO_LOGS = 1;
	}
}


void onMessageDelivered(void *context, MQTTClient_deliveryToken dt){
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}


//////////////////////////////////////////////////////
//// TRATAMENTO DE MENSAGENS
//////////////////////////////////////////////////////
void tratar(char topic[], int message) {
	// SETANDO O MODO AUTOMÁTICO, TOGGLE, LIGADO OU DESLIGADO 
	if(strcmp(topic, TOPIC_AUTOMATIC_MODE_TOGGLE) == 0){
		int prox = !comp.automacaoTOGGLE;
		comp.automacaoTOGGLE = prox;
		MENSAGEM_RECEBIDA = MUDANCA_ESTADO_MODO_AUTOMATICO;
	}
	if(strcmp(topic, TOPIC_AUTOMATIC_MODE_LIGADO) == 0){
		comp.automacaoTOGGLE = true;
		MENSAGEM_RECEBIDA = MUDANCA_ESTADO_MODO_AUTOMATICO;
	}
	if(strcmp(topic, TOPIC_AUTOMATIC_MODE_DESLIGADO) == 0){
		comp.automacaoTOGGLE = false;
		MENSAGEM_RECEBIDA = MUDANCA_ESTADO_MODO_AUTOMATICO;
	}

	if(!comp.automacaoTOGGLE){
		//FUNÇÕES DE TOGGLE
		// ILUMINAÇÃO DO JARDIM TOGGLE, LIGADO E DESLIGADO
		
		
		if(strcmp(topic, TOPIC_ILUMINACAO_JARDIM_TOGGLE) == 0){
			bool prox = !comp.jardim.estado_atual;
			comp.jardim.estado_atual = prox;
			MENSAGEM_RECEBIDA = MUDANCA_ILUMINACAO_JARDIM;
		}
		else if(strcmp(topic, TOPIC_ILUMINACAO_JARDIM_LIGADO) == 0){
			comp.jardim.estado_atual = true;
			MENSAGEM_RECEBIDA = MUDANCA_ILUMINACAO_JARDIM;
		}
		else if(strcmp(topic, TOPIC_ILUMINACAO_JARDIM_DESLIGADO) == 0){
			comp.jardim.estado_atual = false;
			MENSAGEM_RECEBIDA = MUDANCA_ILUMINACAO_JARDIM;
		}
		
		// ILUMINAÇÃO DA GARAGEM TOGGLE, LIGADO E DESLIGADO
		else if(strcmp(topic, TOPIC_ILUMINACAO_GARAGEM_TOGGLE) == 0){
			bool prox = !comp.garagem.estado_atual;
			comp.garagem.estado_atual = prox;
			MENSAGEM_RECEBIDA = MUDANCA_ILUMINACAO_GARAGEM;
		}
		else if(strcmp(topic, TOPIC_ILUMINACAO_GARAGEM_LIGADO) == 0){
			comp.garagem.estado_atual = true;
			MENSAGEM_RECEBIDA = MUDANCA_ILUMINACAO_GARAGEM;
		}
		else if(strcmp(topic, TOPIC_ILUMINACAO_GARAGEM_DESLIGADO) == 0){
			comp.garagem.estado_atual = false;
			MENSAGEM_RECEBIDA = MUDANCA_ILUMINACAO_GARAGEM;
		}
		
		// ILUMINAÇÃO INTERNA, TOGLLE, LIGADO, DESLIGADO
		else if(strcmp(topic, TOPIC_ILUMINACAO_INTERNO_TOGGLE) == 0){
			bool prox = !comp.luzInterna.estado_atual;
			comp.luzInterna.estado_atual = prox;
			MENSAGEM_RECEBIDA = MUDANCA_ILUMINACAO_INTERNO;
		}
		else if(strcmp(topic, TOPIC_ILUMINACAO_INTERNO_LIGADO) == 0){
			comp.luzInterna.estado_atual = true;
			MENSAGEM_RECEBIDA = MUDANCA_ILUMINACAO_INTERNO;
		}
		else if(strcmp(topic, TOPIC_ILUMINACAO_INTERNO_DESLIGADO) == 0){
			comp.luzInterna.estado_atual = false;
			MENSAGEM_RECEBIDA = MUDANCA_ILUMINACAO_INTERNO;
		}		

		// ALARME, TOGGLE, LIGADO E DESLIGADO
		else if(strcmp(topic, TOPIC_ALARME_TOGGLE) == 0){
			bool prox = !comp.alarme.alarmeLigado;
			comp.alarme.alarmeLigado = prox;
			MENSAGEM_RECEBIDA = MUDANCA_ALARME;
		}
		else if(strcmp(topic, TOPIC_ALARME_LIGADO) == 0){
			comp.alarme.alarmeLigado = true;
			MENSAGEM_RECEBIDA = MUDANCA_ALARME;
		}
		else if(strcmp(topic, TOPIC_ALARME_DESLIGADO) == 0){
			comp.alarme.alarmeLigado = false;
			MENSAGEM_RECEBIDA = MUDANCA_ALARME;
		}
		
		// AR CONDICIONADO TOGGLE, LIGA E DESLIGA
		else if(strcmp(topic, TOPIC_ARCONDICIONADO_TOGGLE) == 0){
			bool prox = !comp.ac.estado_atual;
			comp.ac.estado_atual = prox;
			MENSAGEM_RECEBIDA = MUDANCA_ARCONDICIONADO;
		}
		else if(strcmp(topic, TOPIC_ARCONDICIONADO_LIGADO) == 0){
			comp.ac.estado_atual = true;
			MENSAGEM_RECEBIDA = MUDANCA_ARCONDICIONADO;
		}
		else if(strcmp(topic, TOPIC_ARCONDICIONADO_DESLIGADO) == 0){
			comp.ac.estado_atual = false;
			MENSAGEM_RECEBIDA = MUDANCA_ARCONDICIONADO;
		}

	}else{
		//FUNÇÕES SEM TOGGLE
		if(strcmp(topic, TOPIC_ARCONDICIONADO_TEMPERATURA_MAX) == 0){
			comp.ac.temp_max = message;
			if(maxMinAtivo < 3){
				maxMinAtivo++;
			}
			MENSAGEM_RECEBIDA = 7;
		}  
		
		else if(strcmp(topic, TOPIC_ARCONDICIONADO_TEMPERATURA_MIN) == 0){
			comp.ac.temp_min = message;
			if(maxMinAtivo < 3){
				maxMinAtivo++;
			}
			MENSAGEM_RECEBIDA = 8;
		}  
		
		else if(strcmp(topic, TOPIC_AC_RESET) == 0){
			comp.ac.alterar_operacao_default = message;
			if(maxMinAtivo < 3 && message == 1){
				maxMinAtivo++;
			} else{
				maxMinAtivo = 0;
			}
			MENSAGEM_RECEBIDA = 9;
		}  
		
		else if(strcmp(topic, TOPIC_ARCONDICIONADO_TEMPO_AUSENCIA_PESSOAS) == 0){
			comp.ac.tempo_ausencia_pessoas = message;
			MENSAGEM_RECEBIDA = 10;
		}

		else if(strcmp(topic, TOPIC_ILUMINACAO_JARDIM_HORARIO_MAXIMO) == 0){
			comp.jardim.hora_maxima = message;
			MENSAGEM_RECEBIDA = 11;
		}  
		
		else if(strcmp(topic, TOPIC_ILUMINACAO_JARDIM_HORARIO_MINIMO) == 0){
			comp.jardim.hora_minima = message;
			MENSAGEM_RECEBIDA = 12;
		}  
		
		else if(strcmp(topic, TOPIC_ILUMINACAO_GARAGEM_HORARIO_MAXIMO) == 0){
			comp.garagem.hora_maxima = message;
			MENSAGEM_RECEBIDA = 13;
		}  
		
		else if(strcmp(topic, TOPIC_ILUMINACAO_GARAGEM_HORARIO_MINIMO) == 0){
			comp.garagem.hora_minima = message;
			MENSAGEM_RECEBIDA = 14;
		}
	
	}
	if(strcmp(topic, TOPIC_PING_REQUEST) == 0){
		MENSAGEM_RECEBIDA = 15;
	}
}

int onMessageArrived(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
	int value = atoi(message->payload);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);

    tratar(topicName, value);
    return 1;
}

	//////////////////////////////////////////////////////
	///		TRATA DAS MUDANÇAS DE ESTADOS VINDAS DO BROKER
	//////////////////////////////////////////////////////
void backlog(){

	if(MENSAGEM_RECEBIDA > 0){
		printf("TEM MENSAGEM: %d\n", MENSAGEM_RECEBIDA);
		char valueAux[10];

		if(MENSAGEM_RECEBIDA<= 6){
			if(MENSAGEM_RECEBIDA == 1){
				snprintf(valueAux, 10, "%d", comp.automacaoTOGGLE);
				publishMessage(client, TOPIC_AUTOMATIC_MODE_VALOR, valueAux);
				//atualizarMongo("automatic_mode", 0, comp.automacaoTOGGLE);
				ALTERACAO_LOGS = 1;
			}else if(MENSAGEM_RECEBIDA == 2){
				snprintf (valueAux, 10, "%d", comp.jardim.estado_atual);
				publishMessage(client, TOPIC_ILUMINACAO_JARDIM, valueAux);
				if(comp.jardim.estado_atual == 1){
					lcdClear(lcd);
					lcdPuts(lcd, "Jardim:");
					lcdPosition(lcd, 4, 1);
					lcdPuts(lcd, "ligado");
					printf("Jardim ligado.\n");
				}else{
					lcdClear(lcd);
					lcdPuts(lcd, "Jardim:");
					lcdPosition(lcd, 4, 1);
					lcdPuts(lcd, "desligado");
					printf("Jardim desligado.\n");
				}
				//atualizarMongo("jardim_toggle", 1, comp.jardim.estado_atual);
				ALTERACAO_LOGS = 1;
				printf("VALOR JARDIM %d", comp.jardim.estado_atual);
			}else if(MENSAGEM_RECEBIDA == 3){
				snprintf (valueAux, 10, "%d", comp.garagem.estado_atual);
				publishMessage(client, TOPIC_ILUMINACAO_GARAGEM, valueAux);
				if(comp.garagem.estado_atual == 1){
					lcdClear(lcd);
					lcdPuts(lcd, "Garagem:");
					lcdPosition(lcd, 4, 1);
					lcdPuts(lcd, "ligado");
					printf("Luz garagem ligado.\n");
				}else{
					lcdClear(lcd);
					lcdPuts(lcd, "Garagem:");
					lcdPosition(lcd, 4, 1);
					lcdPuts(lcd, "desligado");
					printf("Luz garagem: desligado.\n");
				
				}
				//atualizarMongo("garagem_toggle", 1, comp.garagem.estado_atual);
				ALTERACAO_LOGS = 1;
			}else if(MENSAGEM_RECEBIDA == 4){
				snprintf (valueAux, 10, "%d", comp.luzInterna.estado_atual);
				publishMessage(client, TOPIC_ILUMINACAO_INTERNO, valueAux);
				if(comp.luzInterna.estado_atual == 1){
					lcdClear(lcd);
					lcdPuts(lcd, "Luz interna:");
					lcdPosition(lcd, 4, 1);
					lcdPuts(lcd, "ligado");
					printf("Luz interna ligado.\n");
				}else{
					lcdClear(lcd);
					lcdPuts(lcd, "Luz interna:");
					lcdPosition(lcd, 4, 1);
					lcdPuts(lcd, "desligado");
					printf("Luz interna desligado.\n");
				
				}
				//atualizarMongo("interno_toggle", 1, comp.luzInterna.estado_atual);
				ALTERACAO_LOGS = 1;
			}else if(MENSAGEM_RECEBIDA == 5){
				snprintf (valueAux, 10, "%d", comp.alarme.alarmeLigado);
				publishMessage(client, TOPIC_ALARME, valueAux);
				if(comp.alarme.alarmeLigado == 1){
					lcdClear(lcd);
					lcdPuts(lcd, "Alarme:");
					lcdPosition(lcd, 4, 1);
					lcdPuts(lcd, "ligado");
					printf("Alarme ligado.\n");
				}else{
					lcdClear(lcd);
					lcdPuts(lcd, "Alarme:");
					lcdPosition(lcd, 4, 1);
					lcdPuts(lcd, "desligado");
					printf("Alarme desligado.\n");
				
				}
				//inserirNovoEstadoAlarme(0, 0, 0);
				//atualizarMongo("alarme_ligado", 1, comp.alarme.alarmeLigado);
				ALTERACAO_LOGS = 1;
			}else if(MENSAGEM_RECEBIDA == 6){
				snprintf (valueAux, 10, "%d", comp.ac.estado_atual);
				publishMessage(client, TOPIC_ARCONDICIONADO, valueAux);
				if(comp.ac.estado_atual == 1){
					lcdClear(lcd);
					lcdPuts(lcd, "Ar condicionado:");
					lcdPosition(lcd, 4, 1);
					lcdPuts(lcd, "ligado");
					printf("Ar condicionado ligado.\n");
				}else{
					lcdClear(lcd);
					lcdPuts(lcd, "Ar condicionado:");
					lcdPosition(lcd, 4, 1);
					lcdPuts(lcd, "desligado");
					printf("Ar condicionado desligado.\n");
				
				}
				//atualizarMongo("ac_toggle", 1, comp.ac.estado_atual);
				ALTERACAO_LOGS = 1;				
			}
		}else {
			if(MENSAGEM_RECEBIDA== 7){
			snprintf (valueAux, 10, "%d", comp.ac.temp_max);
			publishMessage(client, TOPIC_ARCONDICIONADO_MAX, valueAux);		
			lcdClear(lcd);
			lcdPuts(lcd, "Ar:");
			lcdPosition(lcd, 4, 1);
			lcdPuts(lcd, "Temp max aumentou");
			printf("Ar condicionado temp max aumentou.\n");	
			//atualizarMongo("ac_temp_max", 0, comp.ac.temp_max);
			ALTERACAO_LOGS = 1;
			}else if(MENSAGEM_RECEBIDA== 8){
				snprintf (valueAux, 10, "%d", comp.ac.temp_min);
				publishMessage(client, TOPIC_ARCONDICIONADO_MIN, valueAux);
				lcdClear(lcd);
				lcdPuts(lcd, "Ar:");
				lcdPosition(lcd, 4, 1);
				lcdPuts(lcd, "Temp min diminuiu");
				printf("Ar condicionado temp min diminuiu.\n");	
				//atualizarMongo("ac_temp_min", 0, comp.ac.temp_min);
				ALTERACAO_LOGS = 1;
			}else if(MENSAGEM_RECEBIDA== 9){
				snprintf (valueAux, 10, "%d", comp.ac.alterar_operacao_default);
				publishMessage(client, TOPIC_AC_RESET, valueAux);
				//atualizarMongo("ac_reset", 1, comp.ac.alterar_operacao_default);
				ALTERACAO_LOGS = 1;
			}else if(MENSAGEM_RECEBIDA== 10){
				snprintf (valueAux, 10, "%d", comp.ac.tempo_ausencia_pessoas);
				publishMessage(client, TOPIC_ARCONDICIONADO_AUSENCIA_PESSOAS, valueAux);
				//atualizarMongo("ac_tempo_ausencia_pessoas", 0, comp.ac.tempo_ausencia_pessoas);
				ALTERACAO_LOGS = 1;
			}else if(MENSAGEM_RECEBIDA== 11){
				snprintf (valueAux, 10, "%d", comp.jardim.hora_maxima);
				publishMessage(client, TOPIC_ILUMINACAO_JARDIM_MAX, valueAux);		
				lcdClear(lcd);
				lcdPuts(lcd, "Jardim:");
				lcdPosition(lcd, 4, 1);
				lcdPuts(lcd, "horario min");
				printf("Jardim horario min diminuiu.\n");		
				//atualizarMongo("jardim_hora_max", 0, comp.jardim.hora_maxima);						
				ALTERACAO_LOGS = 1;
			}else if(MENSAGEM_RECEBIDA== 12){
				snprintf (valueAux, 10, "%d", comp.jardim.hora_minima);
				publishMessage(client, TOPIC_ILUMINACAO_JARDIM_MIN, valueAux);
				lcdClear(lcd);
				lcdPuts(lcd, "Jardim:");
				lcdPosition(lcd, 4, 1);
				lcdPuts(lcd, "horario max");
				printf("Jardim horario max aumentou.\n");			
				//atualizarMongo("jardim_hora_min", 0, comp.jardim.hora_minima);			
				ALTERACAO_LOGS = 1;
			}else if(MENSAGEM_RECEBIDA== 13){
				snprintf (valueAux, 10, "%d", comp.garagem.hora_maxima);
				publishMessage(client, TOPIC_ILUMINACAO_GARAGEM_MAX, valueAux);
				lcdClear(lcd);
				lcdPuts(lcd, "Garagem:");
				lcdPosition(lcd, 4, 1);
				lcdPuts(lcd, "horario max");
				printf("Garagem horario max aumentou.\n");	
				//atualizarMongo("garagem_hora_max", 0, comp.garagem.hora_maxima);
				ALTERACAO_LOGS = 1;
			}else if(MENSAGEM_RECEBIDA== 14){
				snprintf (valueAux, 10, "%d", comp.garagem.hora_minima);
				publishMessage(client, TOPIC_ILUMINACAO_GARAGEM_MIN, valueAux);
				lcdClear(lcd);
				lcdPuts(lcd, "Garagem:");
				lcdPosition(lcd, 4, 1);
				lcdPuts(lcd, "horario min");
				printf("Garagem horario min diminuiu.\n");	
				//atualizarMongo("garagem_hora_min", 0, comp.garagem.hora_minima);
				ALTERACAO_LOGS = 1;
			}
		}
		if(MENSAGEM_RECEBIDA== 15){
			publishMessage(client, TOPIC_PING_RESPONSE, "true");
		}
		MENSAGEM_RECEBIDA= -1;
	} 
}

/////////////////////////////////////////////////////////////////////////////////////
///		PARA PEGAR DIFERENTE TIPOS DE INPUT
///		DEIXE SOMENTE UMA DAS LINHAS SEM COMENTÁRIO PARA OBTER O INPUT DESEJADO
/////////////////////////////////////////////////////////////////////////////////////
int getInput(int value){
	// return (rand() > RAND_MAX / 2); 			//INPUT RANDOMICO
	return digitalRead(value);				//INPUT PINOS DA RASP
	// return 0;								//INPUTS SEMPRE 0
	//  return 1;								//INPUTS SEMPRE 1
}

int main() {
	///////////////////////////////////////////////
	///		CONFIGURAÇÃO DO MQTT
	///////////////////////////////////////////////
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;

	int rc = MQTTClient_create(&client, ADDRESS, ID, MQTTCLIENT_PERSISTENCE_NONE, NULL);

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
  conn_opts.username = "mqttuefs20212";
  conn_opts.password = ".h.#q4WEcGk(NAvF";

	rc = MQTTClient_connect(client, &conn_opts);
	if (rc != MQTTCLIENT_SUCCESS) {
		printf("Failed to connect, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}

	subscribeTo(client);

	///////////////////////////////////////////////
	///		CONFIGURAÇÃO DO MONGODB
	///////////////////////////////////////////////

	/*mongoc_init();
	client_mongo = mongoc_client_new (CLIENT_DB);
	collection_topics = mongoc_client_get_collection (client_mongo, DATABASE_DB, COLLECTION_LOG_TOPICS_DB);
	collection_alarm = mongoc_client_get_collection (client_mongo, DATABASE_DB, COLLECTION_LOG_ALARMS_DB);
	recuperaMongo();*/

	time(&seconds);
	p = localtime(&seconds);

	///////////////////////////////////////////////
	///		CONFIGURAÇÃO DO PIN NA RASP
	///////////////////////////////////////////////
	wiringPiSetupGpio () ;
	lcd = lcdInit(2, 16, 4, LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7,0,0,0,0);
	pinMode(SWITCH_PRESENCA_SALA, INPUT);
	pinMode(SWITCH_PRESENCA_GARAGEM, INPUT);
	pinMode(SWITCH_PRESENCA_INTERNO, INPUT);
	pinMode(SWITCH_ALARME, INPUT);
	pinMode(BUTTON_PORTA, INPUT);
	pinMode(BUTTON_JANELA, INPUT);
	printf("Pinos de botão foram configurados. \n");

	int horario_ATUAL = p->tm_hour, returnAC;
	bool estadoCronometroAC = false;
	bool temPessoas_ALARME = false, temPessoas_GARAGEM = false, temPessoas_AC = false, temPessoas_INTERNO = false, portasAbertas_ALARME = false, janelasAbertas_ALARME = false;
	char valueAux[10];
	horarioInicio = p->tm_hour;
	dataInicio = p->tm_mday;
	comp.ac.estado_atual = 0;
	comp.ac.temp_atual = 17;
	comp.ac.temp_max = 24;
	comp.ac.temp_min = 18;
	comp.ac.tempo_ausencia_pessoas = 2;
	comp.ac.alterar_operacao_default = 0;
	comp.jardim.estado_atual = 0;
	comp.jardim.hora_maxima = 23;
	comp.jardim.hora_minima = 18;
	comp.garagem.estado_atual = 0;
	comp.garagem.hora_maxima = 6;
	comp.garagem.hora_minima = 18;
	comp.luzInterna.estado_atual = 0;
	comp.alarme.estado_atual = 0;
	comp.automacaoTOGGLE = 1;

	while(true){

		if(comp.automacaoTOGGLE){
			if(horario_ATUAL < p->tm_hour) {
				horario_ATUAL = p->tm_hour;
			}
		////////////////////////////////////////////////////////////////////////////
			comp.estados_inputs.garagem = getInput(SWITCH_PRESENCA_GARAGEM);
			comp.estados_inputs.sala = getInput(SWITCH_PRESENCA_SALA);
			comp.estados_inputs.interno = getInput(SWITCH_PRESENCA_INTERNO);
			comp.estados_inputs.alarme = getInput(SWITCH_ALARME);
			comp.estados_inputs.porta = getInput(!BUTTON_JANELA);
			comp.estados_inputs.janela = getInput(!BUTTON_PORTA);
		//////////////////////////////////////////////////////////////////////////
			comp.alarme.alarmeLigado = comp.estados_inputs.alarme;
			//atualizarMongo("alarme_ligado", 1, comp.alarme.alarmeLigado);
 		
			if(comp.estados_inputs.sala == 1 || comp.estados_inputs.garagem == 1|| comp.estados_inputs.interno == 1){
				temPessoas_ALARME = true;
			}else{
				temPessoas_ALARME = false;
			}

			if (comp.estados_inputs.porta == 1){
				portasAbertas_ALARME = true;
			}else{
				portasAbertas_ALARME = false;
			}

			if (comp.estados_inputs.janela == 1){
				janelasAbertas_ALARME = true;
			}else{
				janelasAbertas_ALARME = false;
			}	
				
			if(comp.estados_inputs.interno == 1){
				temPessoas_INTERNO = true;
			}else{
				temPessoas_INTERNO = false;
			}	
				
			if(comp.estados_inputs.garagem == 1){
				temPessoas_GARAGEM = true;
			}else{
				temPessoas_GARAGEM = false;
			}	
				
			if(comp.estados_inputs.garagem == 1){
				temPessoas_AC = true;
			}else{
				temPessoas_AC = false;
			}	
			


			alarme(temPessoas_ALARME, portasAbertas_ALARME, janelasAbertas_ALARME, comp.estados_inputs.alarme);
			iluminacaoAmbientesInternos(temPessoas_INTERNO);
			iluminacaoGaragem(horario_ATUAL, temPessoas_GARAGEM);
			iluminacaoJardim(horario_ATUAL);

			// ar condicionado
			// returnAC 1 = temperatura fora da faixa de operação default ou ajustada.
			// returnAC 2 = temperatura dentro da faixa de operação porém ausência de pessoas no ambiente.
			// returnAC 0 = ar condicionado desligado ou ligado com a temperatura dentro da faixa de operação na presença de pessoas no ambiente.
			if(estadoCronometroAC){
				time_t now = time(0);
				if(now > desligar_em && desligar_em > 0){
					desligar_em = 0;
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
					publishMessage(client, TOPIC_ARCONDICIONADO, valueAux);
					estadoCronometroAC = true;
					time_t agora = time(0);
					desligar_em = agora + 5*60;
					printf("Ar condicionado ficará desligado por 5 minutos por estar fora da faixa de operacao.");
				} else if(returnAC == 2){
					estadoCronometroAC = true;
					time_t agora = time(0);
					desligar_em = agora + comp.ac.tempo_ausencia_pessoas*60;
					printf("Ar condicionado ficará ligado por mais %d minutos e sera desligado pela ausencia de pessoas no ambiente.", comp.ac.tempo_ausencia_pessoas);
				}
			}
			if(ALTERACAO_LOGS){
				atualizarLogsLocal();
				ALTERACAO_LOGS = 0;
			}
		}
		backlog();
	}
	finishConnection(client);
	return rc;
}
