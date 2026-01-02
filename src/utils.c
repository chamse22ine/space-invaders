/**
 * @file utils.c
 * @brief Implémentation des fonctions utilitaires (Version POSIX/Linux).
 *
 * Ce fichier implémente les outils de gestion du temps et d'aléatoire.
 * Il utilise les API standards POSIX (`clock_gettime`, `nanosleep`) pour garantir
 * une précision à la nanoseconde, nécessaire pour une boucle de jeu fluide (60 FPS).
 */

/** @def _POSIX_C_SOURCE
 *  @brief Active les fonctionnalités POSIX 1993 (requis pour clock_gettime).
 */
#define _POSIX_C_SOURCE 199309L

#include "utils.h"
#include <time.h>
#include <stdlib.h>

/**
 * @brief Récupère le temps écoulé depuis le démarrage système.
 *
 * Utilise `CLOCK_MONOTONIC` : C'est une horloge qui ne revient jamais en arrière,
 * même si l'utilisateur change l'heure de son ordinateur. C'est vital pour éviter
 * que la physique du jeu ne plante ou ne saute.
 */
double utils_get_time(void)
{
    struct timespec ts;
    // Récupère le temps actuel (Secondes + Nanosecondes)
    clock_gettime(CLOCK_MONOTONIC, &ts);

    // Conversion en double (ex: 1s et 500ms devient 1.5)
    return ts.tv_sec + ts.tv_nsec / 1000000000.0;
}

/**
 * @brief Endort le processus pour libérer le CPU.
 *
 * Utilise `nanosleep` qui est plus précis que le `sleep()` standard
 * et n'interfère pas avec les signaux système.
 */
void utils_sleep_ms(int ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000;              // Partie entière (Secondes)
    ts.tv_nsec = (ms % 1000) * 1000000; // Reste converti en Nanosecondes

    nanosleep(&ts, NULL);
}

/**
 * @brief Générateur pseudo-aléatoire simple.
 *
 * Utilise l'arithmétique modulaire pour borner le résultat entre min et max.
 * Note : Suppose que srand() a été appelé au début du programme (fait dans model_init).
 */
int utils_random_int(int min, int max)
{
    return min + rand() % (max - min + 1);
}