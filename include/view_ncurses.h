/**
 * @file view_ncurses.h
 * @brief Implémentation de la Vue en mode Texte (Ncurses/ASCII).
 *
 * Ce fichier expose l'implémentation "Low-Tech" du jeu.
 * Il permet d'exécuter le jeu directement dans un terminal, en utilisant
 * des caractères ASCII pour représenter les entités (ex: "A" pour un alien, "^" pour le joueur).
 * C'est une alternative légère à la version SDL.
 */

#ifndef VIEW_NCURSES_H
#define VIEW_NCURSES_H

#include "view_interface.h"

// ============================================================================
//                          INSTANCE GLOBALE
// ============================================================================

/**
 * @brief Instance concrète de l'interface View pour le mode Terminal.
 *
 * Cette structure contient les pointeurs vers les fonctions réelles de Ncurses
 * (ncurses_init, ncurses_render, etc.).
 *
 * Elle est utilisée par le `main.c` lorsque l'argument "text" ou "ncurses"
 * est passé au lancement du programme.
 */
extern const ViewInterface view_ncurses;

#endif // VIEW_NCURSES_H