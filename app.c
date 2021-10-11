#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTClient.h"
#include <unistd.h>
#include <wiringPi.h>


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

//Entradas Switchs e buttons
#define SWITCH_PRESENCA_SALA 4 
#define SWITCH_PRESENCA_GARAGEM 17
#define SWITCH_PRESENCA_INTERNO 27
#define SWITCH_ALARME 22
#define BUTTON_PORTA 5
#define BUTTON_JANELA 19

// TOPICS TO PUBLISH
#define TOPIC_ILUMINACAO_JARDIM "JARDIM/ILUMINACAO/VALOR"
#define TOPIC_ILUMINACAO_INTERNO "INTERNO/ILUMINACAO/VALOR"
#define TOPIC_ILUMINACAO_GARAGEM "GARAGEM/ILUMINACAO/VALOR"
#define TOPIC_ARCONDICIONADO_TEMPERATURA "AC/TEMPERATURA/VALOR"
#define TOPIC_ALARME "ALARME/VALOR"

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
	AC ac;
	Alarme alarme;
	Jardim jardim;
	Garagem garagem;
	LuzInterna luzInterna;
	bool automacaoTOGGLE;
} Components;


// VARIÁVEIS GLOBAIS
int TEMPERATURA_EXTERNA;
bool MENSAGEM_RECEBIDA = false;
Components comp;


//verifica alterações na entrada
bool alteracao(int presenca_sala,int presenca_garagem,int presenca_interno,int switch_alarme,int janela,int porta){
	
	int estado_presenca_garagem, estado_presenca_interna, estado_presenca_sala, estado_alarme, estado_porta, estado_janela;
	
	if(presenca_sala != estado_presenca_sala){
		estado_presenca_sala = presenca_sala;
		return true;
	}
	if(presenca_garagem != estado_presenca_garagem){
		estado_presenca_garagem = presenca_garagem;
		return true;
	}
	if(presenca_interno != estado_presenca_interna){
		estado_presenca_interna = presenca_interno;
		return true;
	}
	if(switch_alarme != estado_alarme){
		estado_alarme = switch_alarme;
		return true;
	}
	if(janela != estado_janela){
		estado_janela = janela;
		return true;
	}
	if(porta != estado_porta){
		estado_porta = porta;
		return true;
	}


	return false;
}


if(alteracao(SWITCH_PRESENCA_SALA,SWITCH_PRESENCA_GARAGEM,SWITCH_PRESENCA_INTERNO,SWITCH_ALARME,BUTTON_JANELA,BUTTON_PORTA) == true || MENSAGEM_RECEBIDA)
#define SWITCH_PRESENCA_SALA 4 
#define SWITCH_PRESENCA_GARAGEM 17
#define SWITCH_PRESENCA_INTERNO 27
#define SWITCH_ALARME 22
#define BUTTON_PORTA 5
#define BUTTON_JANELA 19

// SISTEMA DE ALARME
void alarme(bool temPessoas, bool portaJarnelaAbertas){
	if(comp.automacaoTOGGLE){
		if(temPessoas || portaJarnelaAbertas){
			printf("Saída do alarme por PRESENÇA DE INTRUSOS ou PORTAS E/OU JANELAS ABERTAS.\n",setlocale(LC_ALL,""));
			comp.alarme.estado_atual = 1;
		} else {
			comp.alarme.estado_atual = 0;
		}
	}
}

// ILUMINAÇÃO DOS AMBIENTES INTERNOS
void iluminacaoAmbientesInternos(bool temPessoas){
	if(comp.automacaoTOGGLE){
		if(temPessoas){
			printf("Iluminação do ambiente ligada.\n",setlocale(LC_ALL,""));
			comp.luzInterna.estado_atual = 1;
		} else {
			comp.luzInterna.estado_atual = 0;
			printf("Iluminação do ambiente desligada.\n",setlocale(LC_ALL,""));
		}
	}
}

// ILUMINAÇÃO DA GARAGEM
void iluminacaoGaragem(int horaAtual, bool temPessoas){
	if(comp.automacaoTOGGLE){
		if(temPessoas){
			if(horaAtual >= comp.garagem.hora_minima|| horaAtual <= comp.garagem.hora_maxima){ //Ex: maxima = 06 minima = 18
			printf("Iluminação da garagem ligada.\n",setlocale(LC_ALL,""));
			comp.garagem.estado_atual = 1;
			} else {
				comp.garagem.estado_atual = 0;
				printf("Iluminação da garagem desligada.\n",setlocale(LC_ALL,""));
			}
		} else {
			comp.garagem.estado_atual = 0;
			printf("Iluminação da garagem desligada.\n",setlocale(LC_ALL,""));
		}
	}
}

// ILUMINAÇÃO DO JARDIM
void iluminacaoJardim(int horaAtual){
	if(comp.automacaoTOGGLE){
		if(horaAtual >= comp.jardim.hora_minima|| horaAtual <= comp.jardim.hora_maxima){ //Ex: maxima = 23 minima = 18
			comp.jardim.estado_atual = 1;
			printf("Iluminação do jardim ligada.\n",setlocale(LC_ALL,""));
		} else {
			comp.jardim.estado_atual = 0;
		}
	}
}

// SISTEMA DO AR CONDICIONADO
void arCondicionado(bool temPessoas){
	if(comp.automacaoTOGGLE){
		if(comp.ac.alterar_operacao_default){
			if( comp.ac.temp_atual < comp.ac.temp_min || comp.ac.temp_atual > comp.ac.temp_max ){
				comp.ac.temp_atual = arForaFaixaOperacao(comp.ac.temp_atual);
				if( comp.ac.temp_atual >= comp.ac.temp_min || comp.ac.temp_atual <= comp.ac.temp_max ) {
					comp.ac.estado_atual = 1;
					printf("Ar condicionado ligado.\n",setlocale(LC_ALL,""));
				} else {
					comp.ac.estado_atual = 0;
				}
			}
		} else {
			if(comp.ac.temp_atual >= 17){
				comp.ac.temp_atual = arForaFaixaOperacao(comp.ac.temp_atual);
				if(comp.ac.temp_atual < 17) {
					comp.ac.estado_atual = 1;
					printf("Ar condicionado ligado.\n",setlocale(LC_ALL,""));
				} else {
					comp.ac.estado_atual = 0;
				}
				
			}
		}
		
		if(!temPessoas && comp.ac.estado_atual == 1){
			ausenciaPessoas(comp.ac.tempo_ausencia_pessoas);
			comp.ac.estado_atual = 0;
		}
	}
}

// Desliga o ar condicionado por 5 minutos e liga novamente.
int arForaFaixaOperacao(int temperaturaAr){
	int tempoLigarAr = 5;
	int TEMPERATURA_EXTERNA = (double) rand() / (double) RAND_MAX;
	if(TEMPERATURA_EXTERNA > 0.5){
		TEMPERATURA_EXTERNA = 1;
	} else {
		TEMPERATURA_EXTERNA = 0;
	}
	// counter downtime for run a rocket while the delay with more 0
    do {
        // erase the previous line and display remain of the delay
        printf("O ar condicionado será ligado em: %d\n", tempoLigarAr,setlocale(LC_ALL,""));

        // a timeout for display
        setTimeout(1000);
        

        // decrease the delay to 1
        tempoLigarAr--;
        
        // Temperatura do ar aumenta caso a temperatura externa esteja aumentando (igual a 1), caso contrário a temperatura do ar diminui.
        if(TEMPERATURA_EXTERNA == 1) {
        	temperaturaAr++;
		} else {
			temperaturaAr--;
		}

    } while (tempoLigarAr >= 0);
	return temperaturaAr;
}

// Desliga o ar condicionado depois de um determinado periodo de tempo com a sala vazia.
void ausenciaPessoas(int tempoDesligarAr){
	// counter downtime for run a rocket while the delay with more 0
    do {
        // erase the previous line and display remain of the delay
        printf("O ar condicionado será desligado por ausência de pessoas em: %d\n", tempoDesligarAr,setlocale(LC_ALL,""));

        // a timeout for display
        setTimeout(1000);

        // decrease the delay to 1
        tempoDesligarAr--;

    } while (tempoDesligarAr >= 0);
}

/*
    Implementation simple timeout

    Input: count milliseconds as number

    Usage:
        setTimeout(1000) - timeout on 1 second
        setTimeout(10100) - timeout on 10 seconds and 100 milliseconds
 */
void setTimeout(int milliseconds)
{
    // If milliseconds is less or equal to 0
    // will be simple return from function without throw error
    if (milliseconds <= 0) {
        fprintf(stderr, "Count milliseconds for timeout is less or equal to 0\n");
        return;
    }

    // a current time of milliseconds
    int milliseconds_since = clock() * 1000 / CLOCKS_PER_SEC;

    // needed count milliseconds of return from this timeout
    int end = milliseconds_since + milliseconds;

    // wait while until needed time comes
    do {
        milliseconds_since = clock() * 1000 / CLOCKS_PER_SEC;
    } while (milliseconds_since <= end);
}

// FUNÇÕES DE ENTRADAS
int input_horaAtual(int gpio){
	if(gpio){
		return gpio;
	} return 10;
}

int input_temperaturaAr(int gpio){
	if(gpio){
		return gpio;
	} return 16;
}

int input_temperaturaExterna(int gpio){
	if(gpio){
		return gpio;
	} return 0;
}

int input_arOperacaoMax(int gpio){
	if(gpio){
		return gpio;
	} return 20;
}

int input_arOperacaoMin(int gpio){
	if(gpio){
		return gpio;
	} return 5;
}

int input_temPessoas(int gpio){
	if(gpio){
		return gpio;
	} return 0;
}

int input_portaJarnelaAbertas(int gpio){
	if(gpio || ){
		return 1;
	} return 0;
}

int input_alterarOperacaoAr(int gpio){
	if(gpio){
		return gpio;
	} return 0;
}



void onMessageDelivered(void *context, MQTTClient_deliveryToken dt){
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

// TÓPICOS QUE VÃO CHEGAR DA WEB
void tratar_mensagem_recebida(char topic[], int message){
	if(strcmp(topic, TOPIC_ARCONDICIONADO_TEMPERATURA_MAX) == 0){
		comp.ac.temp_max = message;
	}  else if(strcmp(topic, TOPIC_ARCONDICIONADO_TEMPERATURA_MIN) == 0){
		comp.ac.temp_min = message;
	}  else if(strcmp(topic, TOPIC_AC_RESET) == 0){
		comp.ac.alterar_operacao_default = message;
	}  else if(strcmp(topic, TOPIC_ARCONDICIONADO_TEMPO_AUSENCIA_PESSOAS) == 0){
		comp.ac.tempo_ausencia_pessoas = message;
	}  else if(strcmp(topic, TOPIC_ILUMINACAO_JARDIM_TOGGLE) == 0){
		comp.jardim.estado_atual = message;
	}  else if(strcmp(topic, TOPIC_ILUMINACAO_GARAGEM_TOGGLE) == 0){
		comp.ac.estado_atual = message;
	}  else if(strcmp(topic, TOPIC_ILUMINACAO_INTERNO_TOGGLE) == 0){
		comp.luzInterna.estado_atual = message;
	}  else if(strcmp(topic, TOPIC_ALARME_TOGGLE) == 0){
		comp.garagem.estado_atual = message;
	}  else if(strcmp(topic, TOPIC_ARCONDICIONADO_TOGGLE) == 0){
		comp.ac.estado_atual = message;
	}  else if(strcmp(topic, TOPIC_ILUMINACAO_JARDIM_HORARIO_MAXIMO) == 0){
		comp.ac.estado_atual = message;
	}  else if(strcmp(topic, TOPIC_ILUMINACAO_JARDIM_HORARIO_MINIMO) == 0){
		comp.ac.estado_atual = message;
	}  else if(strcmp(topic, TOPIC_ILUMINACAO_GARAGEM_HORARIO_MAXIMO) == 0){
		comp.ac.estado_atual = message;
	}  else if(strcmp(topic, TOPIC_ILUMINACAO_GARAGEM_HORARIO_MINIMO) == 0){
		comp.ac.estado_atual = message;
	}
}

int onMessageArrived(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    tratar_mensagem_recebida(topicName, message->payload - '0');
    MENSAGEM_RECEBIDA = true;

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
  MQTTClient_subscribe(client, TOPIC_ARCONDICIONADO_TEMPERATURA_MAX, QOS);
  MQTTClient_subscribe(client, TOPIC_ARCONDICIONADO_TEMPERATURA_MIN, QOS);
  MQTTClient_subscribe(client, TOPIC_ARCONDICIONADO_TEMPO_AUSENCIA_PESSOAS, QOS);
  MQTTClient_subscribe(client, TOPIC_ILUMINACAO_JARDIM_TOGGLE, QOS);
  MQTTClient_subscribe(client, TOPIC_ILUMINACAO_GARAGEM_TOGGLE, QOS);
  MQTTClient_subscribe(client, TOPIC_ILUMINACAO_INTERNO_TOGGLE, QOS);
  MQTTClient_subscribe(client, TOPIC_ARCONDICIONADO_TOGGLE, QOS);
  MQTTClient_subscribe(client, TOPIC_ALARME_TOGGLE, QOS);
  MQTTClient_subscribe(client, TOPIC_AC_RESET, QOS);
}

void finishConnection(MQTTClient client){
  MQTTClient_disconnect(client, 10000);
  MQTTClient_destroy(&client);
}

int main() {
  MQTTClient client;
  MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
  MQTTClient_deliveryToken token;

//CONF PINOS
	wiringPiSetupGpio () ;

	pinMode (SWITCH_PRESENCA_SALA, INPUT) ;
	pinMode (SWITCH_PRESENCA_GARAGEM, INPUT) ;
	pinMode (SWITCH_PRESENCA_INTERNO, INPUT) ;
	pinMode (SWITCH_ALARME, INPUT) ;
	pinMode (BUTTON_PORTA, INPUT) ;
	pinMode (BUTTON_JANELA, INPUT) ;

    printf ("pinos de botão foram configurados. \n") ;
//fim conf pinos	

  struct tm *p;
	time_t seconds;

	time(&seconds);
  p = localtime(&seconds);
	int horario_ATUAL = p->tm_hour, rc;
  char buffer[512]

  /*/ Inicio de entradas para teste
	bool temPessoas_ALARME = true, temPessoas_GARAGEM = true, temPessoas_AC = false, temPessoas_INTERNO =true, portaJarnelaAbertas_ALARME = false, horarioAtual_ATUALIZOU = false;
	comp.automacaoTOGGLE = 1;
	// Final de entradas para teste
*/

	bool horarioAtual_ATUALIZOU = false,
	comp.automacaoTOGGLE = 1;
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

  /*do{
      scanf("%s", buffer);

      if(strcmp("q", buffer) != 0){
        publishMessage(client, &token, "JARDIM/LUZ", buffer);
      }

  } while(strcmp("q", buffer) != 0); */

  while(true){
		if(horario_ATUAL < p->tm_hour) {
			horario_ATUAL = p->tm_hour;
			horarioAtual_ATUALIZOU = true;
		}
		if(alteracao( digitalRead(SWITCH_PRESENCA_SALA), digitalRead(SWITCH_PRESENCA_GARAGEM),digitalRead(SWITCH_PRESENCA_INTERNO),
								digitalRead(SWITCH_ALARME),digitalRead(BUTTON_JANELA),digitalRead(BUTTON_PORTA)) == true || MENSAGEM_RECEBIDA){
		//if((horarioAtual_ATUALIZOU) || (temPessoas_ALARME) || (temPessoas_GARAGEM) || (temPessoas_AC) || (portaJarnelaAbertas_ALARME) || (MENSAGEM_RECEBIDA)){

			alarme(temPessoas_ALARME, portaJarnelaAbertas_ALARME);
      		publishMessage(client, &token, "ALARME/VALOR", comp.alarme.estado_atual);

			iluminacaoAmbientesInternos(temPessoas_INTERNO);
      		publishMessage(client, &token, "INTERNO/ILUMINACAO/VALOR", comp.luzInterna.estado_atual);

			iluminacaoGaragem(horario_ATUAL, temPessoas_GARAGEM);
      		publishMessage(client, &token, "GARAGEM/ILUMINACAO/VALOR", comp.garagem.estado_atual);

			iluminacaoJardim(horario_ATUAL);
      		publishMessage(client, &token, "JARDIM/ILUMINACAO/VALOR", comp.jardim.estado_atual);

			arCondicionado(temPessoas_AC);
      		publishMessage(client, &token, "AC/TEMPERATURA/VALOR", comp.ac.estado_atual);

			horarioAtual_ATUALIZOU = false;
      		MENSAGEM_RECEBIDA = false;
		}
	}

  finishConnection(client);
  return rc;
}
