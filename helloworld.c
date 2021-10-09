#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <locale.h>
#include <string.h>

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
#define TOPIC_ILUMINACAO_GARAGEM_TOGGLE "GARAGEM/ILUMINACAO/TOGGLE"
#define TOPIC_ILUMINACAO_INTERNO_TOGGLE "INTERNO/ILUMINACAO/TOGGLE"
#define TOPIC_ARCONDICIONADO_TOGGLE "AC/TOGGLE"
#define TOPIC_ALARME_TOGGLE "ALARME/TOGGLE"
#define TOPIC_AC_RESET "AC/RESET"

// VARIÁVEIS GLOBAIS
int TEMPERATURA_EXTERNA;

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

// SISTEMA DE ALARME
void alarme(bool temPessoas, bool portaJarnelaAbertas, Components comp){
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
void iluminacaoAmbientesInternos(bool temPessoas, Components comp){
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
void iluminacaoGaragem(int horaAtual, bool temPessoas, Components comp){
	if(comp.automacaoTOGGLE){
		if(temPessoas){
			if(horaAtual >= comp.garagem.hora_minima|| horaAtual <= comp.garagem.hora_maxima){ Ex: maxima = 06 minima = 18
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
void iluminacaoJardim(int horaAtual, Components comp){
	if(comp.automacaoTOGGLE){
		if(horaAtual >= comp.jardim.hora_minima|| horaAtual <= comp.jardim.hora_maxima){ Ex: maxima = 23 minima = 18
			comp.jardim.estado_atual = 1;
			printf("Iluminação do jardim ligada.\n",setlocale(LC_ALL,""));
		} else {
			comp.jardim.estado_atual = 0;
		}
	}
}

// SISTEMA DO AR CONDICIONADO
void arCondicionado(bool temPessoas, Components comp){
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
	if(gpio){
		return gpio;
	} return 0;
}

int input_alterarOperacaoAr(int gpio){
	if(gpio){
		return gpio;
	} return 0;
}


// TÓPICOS QUE VÃO CHEGAR DA WEB
void tratar_mensagem_recebida(Components comp, char topic[], char message){
	if(strcmp(topic, TOPIC_ARCONDICIONADO_TEMPERATURA_MAX) == 0){
		comp.ac.temp_max = 20;
	}  else if(strcmp(TOPIC_ARCONDICIONADO_TEMPERATURA_MIN, topic) == 0){
		comp.ac.temp_min = 5;
	}  else if(strcmp(TOPIC_AC_RESET, topic) == 0){
		comp.ac.alterar_operacao_default = 0;
	}  else if(strcmp(TOPIC_ARCONDICIONADO_TEMPO_AUSENCIA_PESSOAS, topic) == 0){
		comp.ac.tempo_ausencia_pessoas = 20;
	}  else if(strcmp(TOPIC_ILUMINACAO_JARDIM_TOGGLE, topic) == 0){
		comp.jardim.estado_atual = 0;
	}  else if(strcmp(TOPIC_ILUMINACAO_GARAGEM_TOGGLE, topic) == 0){
		comp.ac.estado_atual = 1;
	}  else if(strcmp(TOPIC_ILUMINACAO_INTERNO_TOGGLE, topic) == 0){
		comp.luzInterna.estado_atual = 1;
	}  else if(strcmp(TOPIC_ALARME_TOGGLE, topic) == 0){
		comp.garagem.estado_atual = 0;
	}  else if(strcmp(TOPIC_ARCONDICIONADO_TOGGLE, topic) == 0){
		comp.ac.estado_atual = 1;
	}
}

int main(){
	struct tm *p;
	time_t seconds;

	time(&seconds);
    p = localtime(&seconds);
	int horario_ATUAL = p->tm_hour;
	Components comp;

	// Entradas para teste
	bool temPessoas_ALARME = true, temPessoas_GARAGEM = true, temPessoas_AC = false, temPessoas_INTERNO =true, portaJarnelaAbertas_ALARME = false, horarioAtual_ATUALIZOU = false;
	char mensagemTOPICO[] = "JARDIM/ILUMINACAO/TOGGLE";
	char mm[] = "JARDIM/ILUMINACAO/TOGGLE";
	comp.automacaoTOGGLE = 1;
	
	while(true){
		if(horario_ATUAL < p->tm_hour) {
			horario_ATUAL = p->tm_hour;
			horarioAtual_ATUALIZOU = true;
		}
		if((horarioAtual_ATUALIZOU) || (temPessoas_ALARME) || (temPessoas_GARAGEM) || (temPessoas_AC) || (portaJarnelaAbertas_ALARME) || (mensagemTOPICO)){

			if(mensagemTOPICO) {
				tratar_mensagem_recebida(comp, mensagemTOPICO, mm);
			}

			alarme(temPessoas_ALARME, portaJarnelaAbertas_ALARME, comp);
			iluminacaoAmbientesInternos(temPessoas_INTERNO, comp);
			iluminacaoGaragem(horario_ATUAL, temPessoas_GARAGEM, comp);
			iluminacaoJardim(horario_ATUAL, comp);
			arCondicionado(temPessoas_AC, comp);
			horarioAtual_ATUALIZOU = false;
		}
	}
    return 0;
}
