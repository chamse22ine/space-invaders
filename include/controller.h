/**
 * @file controller.h
 * @brief Définition de l'interface de commande (Controller).
 *
 * Ce fichier centralise toutes les actions possibles que le joueur peut effectuer.
 * Il sert d'interface normalisée entre les entrées physiques et la logique du jeu.
 * Cette structure garantit une grande évolutivité : elle permet d'ajouter facilement
 * le support d'une manette (Gamepad) ou de reconfigurer les touches sans jamais
 * avoir à modifier le code du Modèle.
 */

#ifndef CONTROLLER_H
#define CONTROLLER_H

// ============================================================================
//                          SIGNAUX DU CONTRÔLEUR
// ============================================================================

/**
 * @brief Liste des commandes interprétables par le Contrôleur.
 *
 * Le Modèle reçoit uniquement ces valeurs, ignorant si elles proviennent
 * d'un clavier, d'une manette ou d'un autre périphérique.
 */
typedef enum
{
    /** @name Commandes Système */
    ///@{
    CMD_NONE,  ///< Aucune action (état neutre ou touche relâchée).
    CMD_PAUSE, ///< Signal de pause ou reprise (ex: Touche P, Bouton Start).
    CMD_EXIT,  ///< Demande de fermeture immédiate ou retour forcé.
    ///@}

    /** @name Commandes de Gameplay */
    ///@{
    CMD_MOVE_LEFT,  ///< Ordre de déplacement continu vers la gauche.
    CMD_MOVE_RIGHT, ///< Ordre de déplacement continu vers la droite.
    CMD_SHOOT,      ///< Ordre de tir (ex: Barre Espace, Bouton A/X).
    ///@}

    /** @name Navigation dans les Menus */
    ///@{
    CMD_UP,        ///< Monter dans la liste / Option précédente.
    CMD_DOWN,      ///< Descendre dans la liste / Option suivante.
    CMD_LEFT,      ///< Diminuer une valeur / Navigation gauche.
    CMD_RIGHT,     ///< Augmenter une valeur / Navigation droite.
    CMD_RETURN,    ///< Valider une sélection / Entrer dans un menu.
    CMD_BACKSPACE, ///< Effacer un caractère (Correction lors de la sauvegarde).
    ///@}

} GameCommand;

#endif // CONTROLLER_H