/**
 * @file common.h
 * @brief Configuration globale et constantes du jeu.
 *
 * Ce fichier définit les dimensions logiques (indépendantes de l'écran),
 * les limites de mémoire (tableaux statiques) et la gestion du temps (FPS).
 * Il est inclus par le Modèle, la Vue et le Contrôleur.
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>

// ============================================================================
//                        DIMENSIONS LOGIQUES
// ============================================================================

/**
 * @name Grille de Jeu
 * Dimensions abstraites du terrain. Le rendu (SDL/Ncurses) se charge
 * d'adapter ces coordonnées à la taille réelle de la fenêtre.
 */
///@{

/** @brief Largeur logique du terrain (0 à 100). */
#define GAME_WIDTH 100

/** @brief Hauteur logique du terrain (0 à 50). */
#define GAME_HEIGHT 50

///@}

// ============================================================================
//                        LIMITES MÉMOIRE (ALLOCATION)
// ============================================================================

/**
 * @name Tableaux Statiques
 * Tailles maximales pour éviter les allocations dynamiques (malloc)
 * durant la boucle de jeu principale.
 */
///@{

/** * @brief Nombre maximum d'ennemis actifs simultanément.
 * (Prévision large pour une grille standard d'envahisseurs).
 */
#define MAX_ENEMIES 60

/** * @brief Nombre maximum de projectiles à l'écran.
 * Cumule les tirs du joueur et ceux des ennemis.
 */
#define MAX_BULLETS 100

///@}

// ============================================================================
//                        TIMING & FRAMERATE
// ============================================================================

/**
 * @name Gestion du Temps
 * Constantes pour stabiliser la vitesse du jeu sur différents CPU.
 */
///@{

/** @brief Cible d'images par seconde (Standard Arcade). */
#define TARGET_FPS 60

/** * @brief Durée minimale d'une frame en millisecondes.
 * (1000ms / 60fps ≈ 16ms). Utilisé pour la pause (sleep) en fin de boucle.
 */
#define FRAME_DELAY (1000 / TARGET_FPS)

///@}

#endif // COMMON_H