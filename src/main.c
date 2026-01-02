/**
 * @file main.c
 * @brief Point d'entrée principal (Main Entry Point) du jeu.
 *
 * Ce fichier orchestre l'ensemble du projet :
 * 1. Il lit les arguments CLI pour choisir le moteur graphique (SDL ou Ncurses).
 * 2. Il initialise le Modèle (Données) et la Vue (Affichage).
 * 3. Il exécute la "Game Loop" (Boucle de jeu) qui gère le temps, les inputs et le rendu.
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include "model.h"
#include "view_ncurses.h"
#include "view_sdl.h"
#include "utils.h"

/**
 * @brief Fonction principale.
 *
 * @param argc Nombre d'arguments.
 * @param argv Tableau des arguments (argv[1] = "sdl" pour le mode graphique).
 * @return 0 si succès, 1 si erreur critique d'initialisation.
 */
int main(int argc, char *argv[])
{
    // ========================================================================
    // 1. SÉLECTION DE L'INTERFACE (PATTERN STRATEGY)
    // ========================================================================
    // On utilise le polymorphisme : 'view' est un pointeur abstrait.
    // Selon l'argument, il pointera vers l'implémentation SDL ou Ncurses.

    const ViewInterface *view = &view_ncurses; // Par défaut : Terminal

    if (argc > 1 && strcmp(argv[1], "sdl") == 0)
    {
        printf("Démarrage en mode SDL (Graphique)...\n");
        view = &view_sdl; // On change la stratégie pour SDL
    }
    else
    {
        printf("Démarrage en mode Ncurses (Texte)...\n");
        printf("Astuce : Lancez './game sdl' pour le mode graphique.\n");
    }

    // ========================================================================
    // 2. INITIALISATIONS
    // ========================================================================

    // Création du Modèle (Données du jeu)
    GameModel *model = model_init();
    if (!model)
    {
        fprintf(stderr, "Erreur Critique: Impossible d'allouer le modèle.\n");
        return 1;
    }

    // Initialisation de la Vue choisie (Fenêtre, Textures...)
    if (!view->init())
    {
        fprintf(stderr, "Erreur Critique: Impossible d'initialiser la vue.\n");
        model_free(model);
        return 1;
    }

    // ========================================================================
    // 3. BOUCLE DE JEU (GAME LOOP) - FIXED TIMESTEP
    // ========================================================================
    // Cette boucle utilise un accumulateur de temps pour garantir que la physique
    // tourne toujours à la même vitesse, quel que soit le framerate de l'écran.

    bool running = true;
    double last_time = utils_get_time();
    double accumulator = 0.0;
    const double dt = 1.0 / TARGET_FPS; // Pas de temps fixe (0.016s pour 60Hz)

    while (running)
    {
        // --- A. Gestion du Temps (Time Management) ---
        double current_time = utils_get_time();
        double frame_time = current_time - last_time; // Temps écoulé pour cette frame
        last_time = current_time;

        // "Spiral of Death" protection : Si l'ordi lag trop (>0.25s),
        // on plafonne le temps pour éviter de calculer trop de mises à jour d'un coup.
        if (frame_time > 0.25)
            frame_time = 0.25;

        accumulator += frame_time;

        // --- B. Gestion des Entrées (Input) ---
        // On demande à la Vue de traduire les touches en Commandes de Jeu
        GameCommand cmd = view->get_input(model);

        if (cmd == CMD_EXIT)
        {
            // Logique spéciale pour quitter proprement
            // 1. Si on est déjà dans le menu de confirmation -> Force Quit
            if (model->state == STATE_CONFIRM_QUIT)
            {
                // running = false; // Alternative propre
                exit(0); // Sortie brutale mais efficace ici
            }

            // 2. Sinon, on passe en état de demande de confirmation
            model->previous_state = model->state;
            model->state = STATE_CONFIRM_QUIT;
            model->menu_selection = 1; // Curseur sur "NON" par sécurité
        }
        else
        {
            // Traitement standard de la commande par le Modèle
            model_handle_input(model, cmd);
        }

        // --- C. Mise à jour Physique (Physics Update) ---
        // On consomme l'accumulateur par tranches fixes de 'dt'.
        // Cela garantit une physique déterministe.
        while (accumulator >= dt)
        {
            model_update(model, dt);
            accumulator -= dt;
        }

        // --- D. Rendu (Render) ---
        // On dessine l'état actuel du modèle
        view->render(model);

        // --- E. Régulation CPU (Sleep) ---
        // Si on a fini le travail plus tôt que prévu (ex: en 5ms au lieu de 16ms),
        // on dort pour ne pas utiliser 100% du CPU inutilement.
        double frame_end = utils_get_time();
        double work_time = frame_end - current_time;
        if (work_time < dt)
        {
            utils_sleep_ms((int)((dt - work_time) * 1000));
        }

        // Vérification de demande de sortie interne (via menu)
        if (model->pending_quit)
            running = false;
    }

    // Petit délai en cas de Game Over pour laisser le joueur réaliser
    if (model->state == STATE_GAME_OVER)
    {
        view->render(model); // Un dernier rendu
        utils_sleep_ms(3000);
    }

    // ========================================================================
    // 4. NETTOYAGE & SORTIE
    // ========================================================================
    view->close();     // Fermeture fenêtre / Restauration terminal
    model_free(model); // Libération mémoire

    printf("Merci d'avoir joué !\n");
    return 0;
}