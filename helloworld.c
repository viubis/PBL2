#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <locale.h>


// SISTEMA DE ALARME
void alarme(bool temPessoas, bool portaJarnelaAbertas){
	if(temPessoas){
		printf("Saída do alarme por PRESENÇA DE INTRUSOS.\n",setlocale(LC_ALL,""));
	}
	
	if(portaJarnelaAbertas){
		printf("Saída do alarme por PORTAS OU JANELAS ABERTAS.\n",setlocale(LC_ALL,""));
	}
}

// ILUMINAÇÃO DOS AMBIENTES INTERNOS
void iluminacaoAmbientesInternos(bool temPessoas){
	if(temPessoas){
		printf("Iluminação do ambiente ligada.\n",setlocale(LC_ALL,""));
	} else {
		printf("Iluminação do ambiente desligada.\n",setlocale(LC_ALL,""));
	}
}

// ILUMINAÇÃO DA GARAGEM
void iluminacaoGaragem(int horaAtual, bool temPessoas){
	if(temPessoas){
		if(horaAtual >= 18 || horaAtual <= 23 || horaAtual >= 0 || horaAtual <= 6){
		printf("Iluminação da garagem ligada.\n",setlocale(LC_ALL,""));
		} else {
			printf("Iluminação da garagem desligada.\n",setlocale(LC_ALL,""));
		}
	} else {
		printf("Iluminação da garagem desligada.\n",setlocale(LC_ALL,""));
	}
}

// ILUMINAÇÃO DO JARDIM
void iluminacaoJardim(int horaAtual){
	if(horaAtual >= 18 || horaAtual <= 23){
		printf("Iluminação do jardim ligada.\n",setlocale(LC_ALL,""));
	}
}

// SISTEMA DO AR CONDICIONADO
void arCondicionado(int temperaturaAr, int temperaturaExterna, bool temPessoas, bool alterarOperacaoAr, int arOperacaoMax, int arOperacaoMin) {
	if(alterarOperacaoAr){
		if( temperaturaAr < arOperacaoMin || temperaturaAr > arOperacaoMax ){
			arForaFaixaOperacao(temperaturaAr, temperaturaExterna);
			printf("Ar condicionado ligado.\n",setlocale(LC_ALL,""));
		}
	} else {
		if(temperaturaAr >= 17){
			arForaFaixaOperacao(temperaturaAr, temperaturaExterna);
			printf("Ar condicionado ligado.\n",setlocale(LC_ALL,""));
		}
	}
	
	if(!temPessoas){
		ausenciaPessoas();
	}
}

// Desliga o ar condicionado por 5 minutos e liga novamente.
void arForaFaixaOperacao(int temperaturaAr, int temperaturaExterna){
	int tempoLigarAr = 10;
	// counter downtime for run a rocket while the delay with more 0
    do {
        // erase the previous line and display remain of the delay
        printf("O ar condicionado será ligado em: %d\n", tempoLigarAr,setlocale(LC_ALL,""));

        // a timeout for display
        setTimeout(1000);
        

        // decrease the delay to 1
        tempoLigarAr--;
        
        // Temperatura do ar aumenta caso a temperatura externa esteja aumentando (igual a 1), caso contrário a temperatura do ar diminui.
        if(temperaturaExterna == 1) {
        	temperaturaAr++;
		} else {
			temperaturaAr--;
		}

    } while (tempoLigarAr >= 0);
}

// Desliga o ar condicionado depois de um determinado periodo de tempo com a sala vazia.
void ausenciaPessoas(){
	int tempoDesligarAr = 6;
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

int horaAtual_function(int gpio){
	if(gpio){
		return gpio;
	} return 10;
}

int temperaturaAr_function(int gpio){
	if(gpio){
		return gpio;
	} return 16;
}

int temperaturaExterna_function(int gpio){
	if(gpio){
		return gpio;
	} return 0;
}

int arOperacaoMax_function(int gpio){
	if(gpio){
		return gpio;
	} return 20;
}

int arOperacaoMin_function(int gpio){
	if(gpio){
		return gpio;
	} return 5;
}

int temPessoas_function(int gpio){
	if(gpio){
		return gpio;
	} return 0;
}

int portaJarnelaAbertas_function(int gpio){
	if(gpio){
		return gpio;
	} return 0;
}

int alterarOperacaoAr_function(int gpio){
	if(gpio){
		return gpio;
	} return 0;
}

int main()
{
	
	int horaAtual = 10, temperaturaAr = 16, temperaturaExterna = 0, arOperacaoMax = 20, arOperacaoMin = 5;
    //typedef enum {false=0, true=1} bool ;
	bool temPessoas = false, portaJarnelaAbertas = false, alterarOperacaoAr = false;
    printf("%d",arOperacaoMin_function(0));
    alarme(temPessoas, portaJarnelaAbertas);
    iluminacaoAmbientesInternos(temPessoas);
    iluminacaoGaragem(horaAtual, temPessoas);
    iluminacaoJardim(horaAtual);
    arCondicionado(temperaturaAr, temperaturaExterna, temPessoas, alterarOperacaoAr, arOperacaoMax, arOperacaoMin);
    return 0;
}

/*#include <stdio.h>

int main(){
	int inicio,sala,interno,abertura,externo,alarme,alarmeAcionado,arOperacao;
	printf("1- Iniciar sistema\n");
	printf("Qualquer outro botao- Fechar sistema\n");
	scanf("%d",&inicio);
	
	if(inicio == 1){
		printf("Faixa de operacao do ar\n");
		scanf("%d",&arOperacao);
		printf("sistema iniciado\n");
	}

	while(inicio == 1){	
		printf("AMBIENTE INTERNO: presenca de pessoas - 1 para sim - 2 para nao\n");
		scanf("%d",&interno);
		printf("AMBIENTE EXTERNO: presenca de pessoas - 1 para sim - 2 para nao\n");
		scanf("%d",&externo);
		printf("ABERTURA JANELA/PORTAS: - 1 aberto - 2 fechado\n");
		scanf("%d",&abertura);
		printf("AMBIENTE SALA: presenca de pessoas - 1 para sim - 2 para nao\n");
		scanf("%d",&sala);
		printf("ALARME: - 1 para ligado - 2 para desligado\n");
		scanf("%d",&alarme);
		if(interno == 1 && alarmeAcionado != 1){
			printf("AMBIENTE INTERNO: Luz acesa\n");
		}else{
			printf("AMBIENTE INTERNO: Luz apagada\n");
		}
		if(sala == 1 && alarmeAcionado != 1){
			printf("SALA: Luz acesa\n");
			//logica do ar
		}else{
			printf("SALA: Luz apagada\n");
		}	
		if(alarmeAcionado == 1){
			printf("ALARME: ligado n");
		}
	}
	return 0;
}
*/