/**
 * @file view_interface.h
 * @brief Définition de l'interface graphique abstraite (V-Table).
 *
 * Ce fichier définit le "contrat" que toutes les Vues (SDL, Ncurses, OpenGL...)
 * doivent respecter. Il utilise des pointeurs de fonctions pour simuler
 * le polymorphisme et le pattern "Strategy" en C pur.
 *
 * Grâce à cette structure, le moteur du jeu (main.c) ne sait pas quelle librairie
 * est utilisée : il appelle simplement `view.render()` et la magie opère.
 */

#ifndef VIEW_INTERFACE_H
#define VIEW_INTERFACE_H

#include "model.h"
#include "controller.h"

// ============================================================================
//                          CONTRAT D'INTERFACE
// ============================================================================

/**
 * @brief Structure regroupant les méthodes virtuelles (API) d'une Vue.
 *
 * Pour créer une nouvelle vue (ex: `view_vulkan.c`), il suffit de créer
 * une instance de cette structure et de remplir ces 4 pointeurs avec
 * les fonctions correspondantes.
 */
typedef struct
{
    /**
     * @brief Initialise le sous-système graphique et audio.
     * Crée la fenêtre, charge les textures/sons, initialise les polices.
     *
     * @return true si tout s'est bien passé, false si une erreur critique (ex: pas de carte graphique) survient.
     */
    bool (*init)(void);

    /**
     * @brief Nettoyeur. Ferme la fenêtre et libère la mémoire.
     * Doit détruire les textures, fermer les flux audio et quitter la librairie proprement.
     */
    void (*close)(void);

    /**
     * @brief La boucle de rendu. Dessine une frame complète.
     * Cette fonction est appelée en boucle (ex: 60 fois par seconde).
     * Elle doit effacer l'écran, dessiner tous les éléments du modèle, et afficher le résultat (flip).
     *
     * @param model Pointeur en lecture seule (const) vers l'état du jeu à dessiner.
     */
    void (*render)(const GameModel *model);

    /**
     * @brief Capteur d'événements (Input).
     * Transforme les appuis touches/boutons bruts (SDL_Event, getch) en commandes de jeu logiques.
     *
     * @param model Pointeur vers le modèle (nécessaire pour le contexte, ex: saisie de texte vs jeu).
     * @return La commande abstraite (GameCommand) correspondant à l'action du joueur.
     */
    GameCommand (*get_input)(GameModel *model);

} ViewInterface;

#endif // VIEW_INTERFACE_H