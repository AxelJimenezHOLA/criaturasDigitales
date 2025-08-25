#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

/**************/
/* CONSTANTES */
/**************/
#define MAXIMO_CARACTERES 32
#define NIVEL_MINIMO 1
#define NIVEL_MAXIMO 100
#define EXPERIENCIA_POR_NIVEL 1575
#define ARCHIVO_ESPECIES "especiesDisponibles.txt"
#define ARCHIVO_LISTA "pokemonGuardados.bin"

/***********/
/* STRUCTS */
/***********/
typedef struct pokemon {
    int identificadorUnico;
    char especie[MAXIMO_CARACTERES];
    char apodo[MAXIMO_CARACTERES];
    int nivel;
    int experiencia;
} pokemon_t;

typedef struct nodo {
    pokemon_t pokemon;
    struct nodo* siguiente;
} nodo_t;

/*************************/
/* FUNCIONES ADICIONALES */
/*************************/
void mostrarMenu() {
    printf("1. Capturar Pokemon\n");
    printf("2. Mostrar Pokemon Capturados\n");
    printf("3. Liberar Pokemon\n");
    printf("4. Salir\n");
}

int obtenerOpcion() {
    char buffer[MAXIMO_CARACTERES];
    printf("Selecciona una opcion: ");
    fgets(buffer, sizeof(buffer), stdin);
    return atoi(buffer);
}

int generarNumeroAleatorio(int min, int max) {
    return rand() % (max + 1 - min) + min;
}

/***********************/
/*  FUNCIONES DE LISTA */
/***********************/
nodo_t* crearNodo(pokemon_t pokemon) {
    nodo_t* nodo = malloc(sizeof(nodo_t));
    if (!nodo) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    nodo -> pokemon = pokemon;
    nodo -> siguiente = NULL;
    return nodo;
}

void agregar(nodo_t** inicioLista, pokemon_t pokemon) {
    nodo_t* nodo = crearNodo(pokemon);

    if (*inicioLista == NULL) { // Lista vacia
        *inicioLista = nodo;
        return;
    }

    nodo_t* auxiliar = *inicioLista;
    while (auxiliar -> siguiente != NULL) {
        auxiliar = auxiliar -> siguiente;
    }
    auxiliar -> siguiente = nodo;
}

int eliminar(nodo_t** inicioLista, int identificadorUnico) {
    if (*inicioLista == NULL) return 0;

    nodo_t* auxiliar = *inicioLista;
    nodo_t* previo = NULL;

    while (auxiliar != NULL && auxiliar -> pokemon.identificadorUnico != identificadorUnico) {
        previo = auxiliar;
        auxiliar = auxiliar -> siguiente;
    }

    if (auxiliar == NULL) return 0;

    if (previo == NULL) {
        *inicioLista = auxiliar -> siguiente;
    } else {
        previo -> siguiente = auxiliar -> siguiente;
    }

    free(auxiliar);
    return 1;
}

/**************************/
/*  FUNCIONES DE ARCHIVOS */
/**************************/
int abrirArchivo(const char* nombreArchivo, int flags) {
    int fileDescriptor = open(nombreArchivo, flags, 0644);
    if (fileDescriptor == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    return fileDescriptor;
}

char* elegirEspecieAleatoria() {
    ssize_t bytesRead;

    // Abrir archivo
    int fileDescriptor = abrirArchivo(ARCHIVO_ESPECIES, O_RDONLY);

    // Obtener el tamaño del archivo
    off_t fileSize = lseek(fileDescriptor, 0, SEEK_END);
    if (fileSize == -1) {
        perror("lseek");
        close(fileDescriptor);
        exit(EXIT_FAILURE);
    }

    // Posición aleatoria
    off_t position = rand() % fileSize;

    // Buscar el siguiente salto de línea
    char c;
    while (position > 0) {
        if (lseek(fileDescriptor, --position, SEEK_SET) == -1) {
            perror("lseek");
            close(fileDescriptor);
            exit(EXIT_FAILURE);
        }
        if (read(fileDescriptor, &c, 1) == -1) {
            perror("read");
            close(fileDescriptor);
            exit(EXIT_FAILURE);
        }
        if (c == '\n') break; // Inicio de línea encontrada
    }

    // Asignar memoria dinámicamente
    char *buffer = malloc(MAXIMO_CARACTERES);
    if (!buffer) {
        perror("malloc");
        close(fileDescriptor);
        exit(EXIT_FAILURE);
    }

    // Lectura de línea completa
    int index = 0;
    while ((bytesRead = read(fileDescriptor, &c, 1)) == 1 && c != '\n' && index < MAXIMO_CARACTERES - 1) {
        buffer[index++] = c;
    }
    buffer[index] = '\0';
    close(fileDescriptor);
    return buffer;
}

void guardarPokemon(pokemon_t pokemon) {
    int fileDescriptor = abrirArchivo(ARCHIVO_LISTA, O_WRONLY | O_CREAT | O_APPEND);

    ssize_t bytesEscritos = write(fileDescriptor, &pokemon, sizeof(pokemon_t));
    if (bytesEscritos == -1) {
        perror("write");
        close(fileDescriptor);
        exit(EXIT_FAILURE);
    }
    close(fileDescriptor);
}

void guardarLista(nodo_t* inicioLista) {
    int fileDescriptor = abrirArchivo(ARCHIVO_LISTA, O_WRONLY | O_CREAT | O_TRUNC);

    // Recorrer lista
    nodo_t* auxiliar = inicioLista;
    while (auxiliar) {
        ssize_t bytesEscritos = write(fileDescriptor, &(auxiliar -> pokemon), sizeof(pokemon_t));
        if (bytesEscritos == -1) {
            perror("write");
            close(fileDescriptor);
            exit(EXIT_FAILURE);
        }
        auxiliar = auxiliar -> siguiente;
    }
    close(fileDescriptor);
}

void cargarArchivo(nodo_t** inicioLista) {
    int fileDescriptor = abrirArchivo(ARCHIVO_LISTA, O_RDONLY | O_CREAT);

    nodo_t* lista = NULL;
    nodo_t* ultimo = NULL;
    pokemon_t pkm;

    while (read(fileDescriptor, &pkm, sizeof(pokemon_t))) {
        nodo_t* nuevoNodo = crearNodo(pkm);

        if (!lista) {
            lista = nuevoNodo;
        } else {
            ultimo -> siguiente = nuevoNodo;
        }
        ultimo = nuevoNodo;
    }

    *inicioLista = lista;

    close(fileDescriptor);
}

/***********************/
/*  FUNCIONES POKEMON  */
/***********************/

char* escribirApodo() {
    char* apodo = malloc(MAXIMO_CARACTERES);
    printf("Escribe el apodo del Pokemon: ");
    fgets(apodo, MAXIMO_CARACTERES, stdin);
    apodo[strcspn(apodo, "\n")] = '\0';
    return apodo;
}

pokemon_t crearPokemon() {
    pokemon_t nuevoPokemon;
    char* especieAleatoria = elegirEspecieAleatoria();
    nuevoPokemon.identificadorUnico = generarNumeroAleatorio(0, INT_MAX);
    nuevoPokemon.nivel = generarNumeroAleatorio(NIVEL_MINIMO, NIVEL_MAXIMO);
    nuevoPokemon.experiencia = nuevoPokemon.nivel * EXPERIENCIA_POR_NIVEL;
    strcpy(nuevoPokemon.especie, especieAleatoria);
    free(especieAleatoria);
    return nuevoPokemon;
}

pokemon_t capturarPokemon(nodo_t** inicioLista) {
    pokemon_t pokemonCapturado = crearPokemon();
    printf("\nHas capturado un %s de nivel %d (%d puntos de experiencia).\n",
        pokemonCapturado.especie, pokemonCapturado.nivel, pokemonCapturado.experiencia);
    char* apodo = escribirApodo();
    strcpy(pokemonCapturado.apodo, apodo);
    free(apodo);
    agregar(inicioLista, pokemonCapturado);
    printf("\n");
    return pokemonCapturado;
}

void mostrarTodosLosPokemon(nodo_t** inicioLista) {
    if (*inicioLista == NULL) { // Lista vacia
        printf("No has capturado ningun Pokemon aun.\n\n");
        return;
    }

    printf("\n***** POKEMON CAPTURADOS *****\n");
    nodo_t* auxiliar = *inicioLista;
    while (auxiliar != NULL) {
        printf("ID: %d\n", auxiliar -> pokemon.identificadorUnico);
        printf("Especie: %s\n", auxiliar -> pokemon.especie);
        printf("Apodo: %s\n", auxiliar -> pokemon.apodo);
        printf("Nivel: %d\n", auxiliar -> pokemon.nivel);
        printf("Experiencia: %d\n", auxiliar -> pokemon.experiencia);
        printf("\n");
        auxiliar = auxiliar -> siguiente;
    }
}

void liberarPokemon(nodo_t** inicioLista) {
    char buffer[MAXIMO_CARACTERES];
    printf("Escribe el identificador del Pokemon a liberar: ");
    fgets(buffer, sizeof(buffer), stdin);
    int identificadorUnico = atoi(buffer);
    if (eliminar(inicioLista, identificadorUnico) == 1) {
        printf("Pokemon liberado.\n\n");
    } else {
        printf("El Pokemon especificado no existe.\n\n");
    }
}

/********/
/* MAIN */
/********/

int main() {
    srand(time(NULL));
    int opcionElegida = 0;
    bool programaTerminado = false;
    nodo_t* pokemonCapturados = NULL;

    while (!programaTerminado) {
        mostrarMenu();
        opcionElegida = obtenerOpcion();
        switch (opcionElegida) {
            case 1: // Capturar Pokemon
                guardarPokemon(capturarPokemon(&pokemonCapturados));
                break;
            case 2: // Mostrar todos los Pokemon
                cargarArchivo(&pokemonCapturados);
                mostrarTodosLosPokemon(&pokemonCapturados);
                break;
            case 3: // Liberar Pokemon
                cargarArchivo(&pokemonCapturados);
                liberarPokemon(&pokemonCapturados);
                guardarLista(pokemonCapturados);
                break;
            case 4: // Salir del almacenamiento
                programaTerminado = true;
                printf("Has salido del almacenamiento de Pokemon.\n\n");
                break;
            default: // Opcion inexistente
                printf("Error: opcion no disponible.\n\n");
        }
    }
    return 0;
}