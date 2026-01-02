/**
 * @file utils.h
 * @brief Bibliothèque de fonctions utilitaires transversales (Cross-cutting concerns).
 *
 * Ce fichier regroupe des aides bas niveau pour interagir avec le système d'exploitation
 * de manière abstraite. Il gère principalement le temps (pour la boucle de jeu et la physique)
 * et l'aléatoire (pour le gameplay), sans que le reste du programme ait besoin de savoir
 * si on est sous Windows ou Linux.
 */

#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

// ============================================================================
//                          GESTION DU TEMPS (TIMING)
// ============================================================================

/**
 * @brief Récupère le temps actuel avec une haute précision (seconde).
 *
 * Cette fonction est cruciale pour le calcul du "Delta Time" (dt) dans la boucle de jeu.
 * Elle utilise une horloge "monotone" pour garantir que le temps ne recule jamais,
 * même si l'utilisateur change l'heure de son système.
 *
 * @return Le temps écoulé en secondes depuis le lancement (ex: 12.5 pour 12s et 500ms).
 */
double utils_get_time(void);

/**
 * @brief Met le thread principal en pause (Sleep).
 *
 * Sert à "ralentir" la boucle de jeu pour ne pas consommer 100% du CPU inutilement
 * et pour respecter la consigne de FPS (Frames Per Second) définie dans common.h.
 *
 * @param ms Durée de la pause en millisecondes.
 */
void utils_sleep_ms(int ms);

// ============================================================================
//                          MATHÉMATIQUES & ALÉATOIRE
// ============================================================================

/**
 * @brief Génère un nombre entier pseudo-aléatoire borné.
 *
 * Utilitaire simple pour le gameplay (ex: probabilité qu'un ennemi tire,
 * apparition de l'OVNI, direction initiale).
 *
 * @param min Borne inférieure (incluse).
 * @param max Borne supérieure (incluse).
 * @return Un entier 'n' tel que : min <= n <= max.
 */
int utils_random_int(int min, int max);

#endif // UTILS_H