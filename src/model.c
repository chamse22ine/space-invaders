/**
 * @file model.c
 * @brief Implémentation de la logique du jeu (Physique, IA, Collisions).
 */

#include "model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Gestion des dossiers (Spécifique Linux pour les sauvegardes)
#include <sys/stat.h>
#include <dirent.h>

// ============================================================================
//                          1. FONCTIONS INTERNES (HELPERS)
// ============================================================================

/**
 * @brief Teste la collision entre deux entités via AABB (Axis-Aligned Bounding Box).
 * @return true si les rectangles se chevauchent.
 */
static bool check_collision(const Entity *a, const Entity *b)
{
    if (!a->active || !b->active)
        return false; // Pas de collision si l'un est inactif/mort

    // Vérifie le chevauchement sur les axes X et Y
    return (a->x < b->x + b->width &&
            a->x + a->width > b->x &&
            a->y < b->y + b->height &&
            a->y + a->height > b->y);
}

/**
 * @brief Recherche un slot libre dans le tableau (Pool) et active une balle.
 */
static void spawn_bullet(GameModel *model, float x, float y, float dy, EntityType type)
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (!model->bullets[i].active) // Slot trouvé
        {
            model->bullets[i].active = true;
            model->bullets[i].x = x;
            model->bullets[i].y = y;
            model->bullets[i].dx = 0;
            model->bullets[i].dy = dy; // Vitesse verticale (+ monte, - descend)
            model->bullets[i].width = BULLET_WIDTH;
            model->bullets[i].height = BULLET_HEIGHT;
            model->bullets[i].type = type;

            // Reset animation
            model->bullets[i].anim_timer = 0;
            model->bullets[i].anim_frame = 0;
            return; // Balle créée, on arrête la recherche
        }
    }
}
/**
 * @brief Vérifie l'existence physique d'un fichier dans le répertoire de sauvegarde.
 * * Tente d'ouvrir le fichier en lecture binaire pour confirmer sa présence.
 * * @param filename Le nom du fichier à tester (ex: "Partie1.dat").
 * @return true si le fichier existe et est accessible, false sinon.
 */
static bool save_file_exists(const char *filename)
{
    char path[128];
    // Construction du chemin relatif : sauvegardes/ + filename
    snprintf(path, sizeof(path), "sauvegardes/%s", filename);

    FILE *f = fopen(path, "rb");
    if (f)
    {
        fclose(f);
        return true;
    }
    return false;
}

/**
 * @brief Génère un nom de fichier unique par incrémentation automatique.
 * * Cette fonction évite l'écrasement involontaire. Si "Partie" est demandé mais existe déjà,
 * elle génère "Partie(1).dat", puis "Partie(2).dat", etc., jusqu'à trouver un slot libre.
 * * @param base_name La racine du nom (ex: "Partie").
 * @param out_buffer Buffer de sortie qui recevra le nom final unique.
 */
static void generate_unique_filename(const char *base_name, char *out_buffer)
{
    int i = 1;
    char candidate[128];

    // Recherche itérative d'un suffixe (N) disponible
    while (true)
    {
        snprintf(candidate, sizeof(candidate), "%s(%d).dat", base_name, i);

        if (!save_file_exists(candidate))
        {
            strcpy(out_buffer, candidate);
            return; // Nom unique trouvé
        }
        i++;
    }
}
// ============================================================================
//                          2. LOGIQUE ENNEMIS & UFO
// ============================================================================
/**
 * @brief Initialise la grille d'ennemis (Wave) pour un début de niveau.
 *
 * Crée une formation classique de 5 rangées par 11 colonnes.
 * Réinitialise également la physique de groupe (vitesse, direction) et l'état de l'OVNI.
 *
 * @param model Le modèle contenant le tableau d'ennemis.
 */
static void init_enemies(GameModel *model)
{
    int idx = 0;

    // Formation : 5 rangées de 11 colonnes
    for (int row = 0; row < 5; row++)
    {
        for (int col = 0; col < 11; col++)
        {
            if (idx >= MAX_ENEMIES)
                break; // Sécurité débordement

            Entity *e = &model->enemies[idx];

            // 1. Met à zéro tous les champs (exploding, timers, etc.)
            // Indispensable pour éviter des bugs visuels au redémarrage.
            memset(e, 0, sizeof(Entity));

            // 2. CONFIGURATION PHYSIQUE
            e->active = true;
            e->width = ENEMY_WIDTH;
            e->height = ENEMY_HEIGHT;

            // Calcul de position : Marge + Index * (Taille + Espace)
            e->x = 5 + col * (ENEMY_WIDTH + 2);
            e->y = 7 + row * (ENEMY_HEIGHT + 2);

            // 3. TYPES SELON L'ALTITUDE
            if (row == 0)
                e->type = ENTITY_ENEMY_TYPE_3; // Haut (Calamar)
            else if (row < 3)
                e->type = ENTITY_ENEMY_TYPE_2; // Milieu (Crabe)
            else
                e->type = ENTITY_ENEMY_TYPE_1; // Bas (Pieuvre)

            idx++;
        }
    }

    // Réinitialisation de la logique de groupe (Vague)
    model->enemy_speed_mult = 1.0f;
    model->direction_enemies = 1; // Commence vers la Droite
    model->drop_direction = 1;
    model->drop_step_count = 0;

    // L'OVNI est désactivé au début du niveau
    model->ufo.active = false;
    model->ufo.hasSpawnedThisLevel = false;
    model->ufo.y = 4; // Altitude de croisière fixe
}

/**
 * @brief Active l'apparition de l'OVNI bonus (Mystery Ship).
 *
 * Réinitialise l'entité UFO pour qu'elle soit "neuve" (non explosée)
 * et détermine aléatoirement son côté d'entrée (Gauche ou Droite).
 *
 * @param model Le modèle de jeu.
 */
static void spawn_ufo(GameModel *model)
{
    // 1. NETTOYAGE : Réinitialise les flags d'explosion.
    // Indispensable si l'OVNI précédent a été détruit (évite d'afficher une explosion dès le spawn).
    model->ufo.exploding = false;
    model->ufo.explode_timer = 0.0f;

    // 2. ACTIVATION
    model->ufo.active = true;
    model->ufo.hasSpawnedThisLevel = true; // Bloque le spawn multiple par niveau
    model->ufo.type = ENTITY_UFO;
    model->ufo.width = UFO_WIDTH;
    model->ufo.height = UFO_HEIGHT;
    model->ufo.y = 4; // Altitude fixe (Très haut dans le ciel)

    // 3. DIRECTION ALÉATOIRE
    if (rand() % 2 == 0)
    {
        // Apparition à GAUCHE -> Va à DROITE
        model->ufo.x = -UFO_WIDTH;
        model->ufo.dx = UFO_SPEED;
    }
    else
    {
        // Apparition à DROITE -> Va à GAUCHE
        model->ufo.x = GAME_WIDTH;
        model->ufo.dx = -UFO_SPEED;
    }
}
// ============================================================================
//                          3. INITIALISATION & RESET
// ============================================================================

/**
 * @brief Initialise les boucliers de protection du joueur.
 *
 * Place les 4 boucliers de manière équidistante sur l'écran,
 * juste au-dessus du vaisseau du joueur. Chaque bouclier a une santé
 * initiale définie par SHIELD_MAX_HEALTH.
 *
 * @param model Le modèle de jeu contenant le tableau des boucliers.
 */
static void init_shields(GameModel *model)
{
    float shield_w = 8.0f;
    float shield_h = 6.0f;
    // Espacement dynamique pour centrer
    float spacing = GAME_WIDTH / (MAX_SHIELDS + 1.0f);

    for (int i = 0; i < MAX_SHIELDS; i++)
    {
        model->shields[i].active = true;
        model->shields[i].health = SHIELD_MAX_HEALTH; // 10
        model->shields[i].width = shield_w;
        model->shields[i].height = shield_h;
        model->shields[i].x = (spacing * (i + 1)) - (shield_w / 2.0f);
        model->shields[i].y = GAME_HEIGHT - PLAYER_HEIGHT - 9;
    }
}

/**
 * @brief Initialise et alloue un nouveau modèle de jeu.
 *
 * Crée la structure GameModel avec tous ses composants initialisés :
 * - État initial sur le menu principal
 * - Joueur positionné au centre de l'écran
 * - Grille d'ennemis créée
 * - Boucliers mis en place
 * - Volume audio à 30%
 *
 * @return Pointeur vers le GameModel alloué, ou NULL en cas d'échec d'allocation.
 * @note Le dossier "sauvegardes" est créé s'il n'existe pas.
 */
GameModel *model_init(void)
{
    // 1. Allocation + Mise à zéro automatique (calloc)
    // Plus besoin d'initialiser is_muted, dx, dy, timers... tout est à 0 !
    GameModel *model = calloc(1, sizeof(GameModel));
    if (!model)
        return NULL;

    // 2. Création du dossier sauvegarde (Linux)
    mkdir("sauvegardes", 0777);

    // 3. Valeurs par défaut (Seulement ce qui n'est pas 0)
    model->state = STATE_MENU;
    model->lives = 3;
    model->normal_max_lives = MAX_LIVES_NORMAL;
    model->level = 1;
    model->volume = 30; // 30% volume

    // 4. Initialisation Joueur
    model->player.active = true;
    model->player.type = ENTITY_PLAYER;
    model->player.width = PLAYER_WIDTH;
    model->player.height = PLAYER_HEIGHT;
    model->player.x = (GAME_WIDTH - PLAYER_WIDTH) / 2.0f;
    model->player.y = GAME_HEIGHT - PLAYER_HEIGHT - 1;

    // 5. Initialisation du Monde
    init_enemies(model);
    init_shields(model); // On utilise la fonction helper

    return model;
}

/**
 * @brief Réinitialise complètement une partie de jeu.
 *
 * Remet à zéro tous les éléments du jeu pour commencer une nouvelle partie :
 * - Score remis à 0
 * - Vies remises à 3
 * - Niveau remis à 1
 * - Joueur repositionné au centre
 * - Ennemis et boucliers recréés
 * - Toutes les balles supprimées
 *
 * @param model Le modèle de jeu à réinitialiser.
 */
static void reset_game(GameModel *model)
{
    // 1. Reset Stats
    model->score = 0;
    model->lives = 3;
    model->level = 1;
    model->hit_timer = 0;

    // 2. Reset Difficulté
    model->enemy_speed_mult = 1.0f;
    model->direction_enemies = 1;
    model->drop_direction = 1;
    model->drop_step_count = 0;

    // 3. Nettoyage des balles (memset = instantané)
    // Remplace ta boucle for qui parcourait tout le tableau
    memset(model->bullets, 0, sizeof(model->bullets));

    // 4. Reset Entités
    model->ufo.active = false;
    model->ufo.hasSpawnedThisLevel = false;

    // Replacer le joueur au centre
    model->player.x = (GAME_WIDTH - PLAYER_WIDTH) / 2.0f;
    model->player.dx = 0;

    // 5. Recréation du niveau
    init_enemies(model);
    init_shields(model); // Plus de duplication de code ici !

    // 6. Lancement
    model->state = STATE_PLAYING;
}

/**
 * @brief Libère la mémoire allouée pour le modèle de jeu.
 *
 * @param model Le modèle de jeu à libérer (peut être NULL).
 */
void model_free(GameModel *model)
{
    if (model)
        free(model);
}

// ============================================================================
//                          5. GESTION DES ENTRÉES (CONTROLLER -> MODEL)
// ============================================================================

/**
 * @brief Gère les entrées utilisateur en fonction de l'état actuel du jeu.
 *
 * Dispatche les commandes vers la logique appropriée selon l'état du jeu :
 * - STATE_MENU : Navigation et sélection dans le menu principal
 * - STATE_LOAD_MENU : Sélection d'une sauvegarde à charger
 * - STATE_TUTORIAL : Affichage du tutoriel
 * - STATE_PLAYING : Contrôle du vaisseau et tir
 * - STATE_PAUSED : Menu pause avec options
 * - STATE_GAME_OVER : Écran de fin de partie
 * - STATE_SAVE_* : Gestion des sauvegardes
 * - STATE_CONFIRM_QUIT : Confirmation avant de quitter
 *
 * @param model Le modèle de jeu à mettre à jour.
 * @param cmd La commande reçue du contrôleur (GameCommand).
 */
void model_handle_input(GameModel *model, GameCommand cmd)
{
    // ---------------------------------------------------------
    // 1. MENU PRINCIPAL
    // ---------------------------------------------------------
    if (model->state == STATE_MENU)
    {
        if (cmd == CMD_EXIT)
            exit(0);

        // Navigation (0 à 4)
        if (cmd == CMD_UP)
            model->menu_selection = (model->menu_selection - 1 < 0) ? 4 : model->menu_selection - 1;
        if (cmd == CMD_DOWN)
            model->menu_selection = (model->menu_selection + 1 > 4) ? 0 : model->menu_selection + 1;

        // Gestion Volume (Option 3)
        if (model->menu_selection == 3)
        {
            if (cmd == CMD_LEFT || cmd == CMD_MOVE_LEFT)
            {
                model->volume = (model->volume - 10 < 0) ? 0 : model->volume - 10;
                model->is_muted = false;
            }
            if (cmd == CMD_RIGHT || cmd == CMD_MOVE_RIGHT)
            {
                model->volume = (model->volume + 10 > 100) ? 100 : model->volume + 10;
                model->is_muted = false;
            }
            if (cmd == CMD_SHOOT || cmd == CMD_RETURN)
                model->is_muted = !model->is_muted;
        }
        // Validation
        else if (cmd == CMD_RETURN || cmd == CMD_SHOOT)
        {
            model->sounds.play_select_sound = true;

            if (model->menu_selection == 0)
                reset_game(model);
            else if (model->menu_selection == 1)
                model->state = STATE_TUTORIAL;
            else if (model->menu_selection == 2)
            {
                model_scan_saves(model);
                if (model->save_file_count > 0)
                {
                    model->state = STATE_LOAD_MENU;
                    model->menu_selection = 0;
                }
                else
                {
                    model->state = STATE_LOAD_MENU;
                }
            }
            else if (model->menu_selection == 4)
                exit(0);
        }
        return;
    }

    // ---------------------------------------------------------
    // 2. MENU CHARGEMENT
    // ---------------------------------------------------------
    if (model->state == STATE_LOAD_MENU)
    {
        if (cmd == CMD_EXIT || cmd == CMD_PAUSE)
        {
            model->state = STATE_MENU;
            model->menu_selection = 2;
            return;
        }

        if (cmd == CMD_UP)
            model->menu_selection = (model->menu_selection - 1 < 0) ? model->save_file_count - 1 : model->menu_selection - 1;
        if (cmd == CMD_DOWN)
            model->menu_selection = (model->menu_selection + 1 >= model->save_file_count) ? 0 : model->menu_selection + 1;

        if (cmd == CMD_RETURN || cmd == CMD_SHOOT)
        {
            if (model->save_file_count > 0)
            {
                model->sounds.play_select_sound = true;
                model_load_named(model, model->save_files[model->menu_selection]);
            }
        }
        return;
    }

    // ---------------------------------------------------------
    // 3. TUTORIEL
    // ---------------------------------------------------------
    if (model->state == STATE_TUTORIAL)
    {
        if (cmd == CMD_EXIT || cmd == CMD_PAUSE || cmd == CMD_RETURN || cmd == CMD_SHOOT)
        {
            model->sounds.play_select_sound = true;
            model->state = STATE_MENU;
            model->menu_selection = 1;
        }
        return;
    }

    // ---------------------------------------------------------
    // 4. JEU (PLAYING)
    // ---------------------------------------------------------
    if (model->state == STATE_PLAYING)
    {
        if (cmd == CMD_PAUSE)
        {
            model->state = STATE_PAUSED;
            model->menu_selection = 0;
            return;
        }

        if (cmd == CMD_MOVE_LEFT || cmd == CMD_LEFT)
            model->player.dx = -PLAYER_SPEED;
        else if (cmd == CMD_MOVE_RIGHT || cmd == CMD_RIGHT)
            model->player.dx = PLAYER_SPEED;
        else if (cmd == CMD_NONE)
            model->player.dx = 0;

        if (cmd == CMD_SHOOT && model->player.shoot_timer <= 0.0f)
        {
            // Tir centré par rapport au joueur
            spawn_bullet(model,
                         model->player.x + 1.5f,
                         model->player.y - 1,
                         -BULLET_SPEED,
                         ENTITY_BULLET_PLAYER);
            model->player.shoot_timer = 0.5f;
            model->sounds.play_shoot = true;
        }
        return;
    }

    // ---------------------------------------------------------
    // 5. PAUSE
    // ---------------------------------------------------------
    if (model->state == STATE_PAUSED)
    {
        if (cmd == CMD_PAUSE)
        {
            model->state = STATE_PLAYING;
            return;
        }

        if (cmd == CMD_UP)
            model->menu_selection = (model->menu_selection - 1 < 0) ? 3 : model->menu_selection - 1;
        if (cmd == CMD_DOWN)
            model->menu_selection = (model->menu_selection + 1 > 3) ? 0 : model->menu_selection + 1;

        // Volume (Index 1)
        if (model->menu_selection == 1)
        {
            if (cmd == CMD_LEFT)
            {
                model->volume = (model->volume < 10) ? 0 : model->volume - 10;
                model->is_muted = false;
            }
            if (cmd == CMD_RIGHT)
            {
                model->volume = (model->volume > 90) ? 100 : model->volume + 10;
                model->is_muted = false;
            }
            if (cmd == CMD_SHOOT || cmd == CMD_RETURN)
                model->is_muted = !model->is_muted;
        }
        else if (cmd == CMD_RETURN || cmd == CMD_SHOOT)
        {
            model->sounds.play_select_sound = true;
            if (model->menu_selection == 0)
                model->state = STATE_PLAYING;
            else if (model->menu_selection == 2)
            {
                model_scan_saves(model);
                model->state = STATE_SAVE_SELECT;
                model->menu_selection = 0;
            }
            else if (model->menu_selection == 3)
            {
                model->previous_state = STATE_PAUSED;
                model->state = STATE_CONFIRM_QUIT;
                model->menu_selection = 1;
            }
        }
        return;
    }

    // ---------------------------------------------------------
    // 6. GAME OVER
    // ---------------------------------------------------------
    if (model->state == STATE_GAME_OVER)
    {
        // Navigation (0, 1, 2)
        if (cmd == CMD_UP)
            model->menu_selection = (model->menu_selection - 1 < 0) ? 2 : model->menu_selection - 1;

        if (cmd == CMD_DOWN)
            model->menu_selection = (model->menu_selection + 1 > 2) ? 0 : model->menu_selection + 1;

        // Validation
        if (cmd == CMD_RETURN || cmd == CMD_SHOOT)
        {
            model->sounds.play_select_sound = true;

            if (model->menu_selection == 0)
            {
                model_scan_saves(model);
                model->state = STATE_SAVE_SELECT;
                model->menu_selection = 0;
            }
            else if (model->menu_selection == 1)
            {
                reset_game(model);
            }
            else if (model->menu_selection == 2)
            {
                exit(0);
            }
        }
        return;
    }

    // ---------------------------------------------------------
    // 7. SYSTÈME DE SAUVEGARDE (Select / Input / Confirm)
    // ---------------------------------------------------------

    // Étape A : Choix du Slot
    if (model->state == STATE_SAVE_SELECT)
    {
        if (cmd == CMD_PAUSE || cmd == CMD_EXIT)
        {
            model->state = STATE_PAUSED;
            return;
        }

        int max = model->save_file_count;
        if (cmd == CMD_UP)
            model->menu_selection = (model->menu_selection - 1 < 0) ? max : model->menu_selection - 1;
        if (cmd == CMD_DOWN)
            model->menu_selection = (model->menu_selection + 1 > max) ? 0 : model->menu_selection + 1;

        if (cmd == CMD_RETURN || cmd == CMD_SHOOT)
        {
            model->sounds.play_select_sound = true;
            if (model->menu_selection == 0)
            {
                model->state = STATE_SAVE_INPUT;
                model->input_buffer[0] = '\0';
            }
            else
            {
                char *f = model->save_files[model->menu_selection - 1];
                int len = strlen(f) - 4;
                if (len > 0)
                {
                    strncpy(model->input_buffer, f, len);
                    model->input_buffer[len] = '\0';
                }

                model->state = STATE_OVERWRITE_CONFIRM;
                model->menu_selection = 0;
            }
        }
        return;
    }

    // Étape B : Saisie du nom
    if (model->state == STATE_SAVE_INPUT)
    {
        if (cmd == CMD_PAUSE)
        {
            model->state = STATE_PAUSED;
            return;
        }

        if (cmd == CMD_RETURN || cmd == CMD_SHOOT)
        {
            if (strlen(model->input_buffer) > 0)
            {
                model->sounds.play_select_sound = true;
                char filename[64];
                snprintf(filename, 64, "%s.dat", model->input_buffer);

                if (save_file_exists(filename))
                {
                    model->state = STATE_OVERWRITE_CONFIRM;
                    model->menu_selection = 1;
                }
                else
                {
                    model_save_named(model, filename);
                    model->state = STATE_SAVE_SUCCESS;
                    model->save_success_timer = 2.0f;
                }
            }
        }
        if (cmd == CMD_BACKSPACE)
        {
            int l = strlen(model->input_buffer);
            if (l > 0)
                model->input_buffer[l - 1] = '\0';
        }
        return;
    }

    // ---------------------------------------------------------
    // 9. CONFIRMATION ÉCRASEMENT
    // ---------------------------------------------------------
    if (model->state == STATE_OVERWRITE_CONFIRM)
    {
        // Navigation Gauche/Droite entre les 2 options
        if (cmd == CMD_LEFT || cmd == CMD_RIGHT || cmd == CMD_UP || cmd == CMD_DOWN)
        {
            model->menu_selection = !model->menu_selection; // Bascule 0 <-> 1
        }

        else if (cmd == CMD_RETURN || cmd == CMD_SHOOT)
        {
            model->sounds.play_select_sound = true;

            if (model->menu_selection == 0)
            {
                char filename[64];
                snprintf(filename, 64, "%s.dat", model->input_buffer);

                model_save_named(model, filename);
                model->state = STATE_SAVE_SUCCESS;
                model->save_success_timer = 2.0f;
            }
            else
            {
                char new_name[128];
                generate_unique_filename(model->input_buffer, new_name);
                model_save_named(model, new_name);

                model->state = STATE_SAVE_SUCCESS;
                model->save_success_timer = 2.0f;
            }
        }
        else if (cmd == CMD_PAUSE)
        {
            model->state = STATE_SAVE_INPUT;
        }
        return;
    }

    // ---------------------------------------------------------
    // 8. CONFIRMATION QUITTER
    // ---------------------------------------------------------
    if (model->state == STATE_CONFIRM_QUIT)
    {
        if (cmd == CMD_UP)
            model->menu_selection = (model->menu_selection - 1 < 0) ? 2 : model->menu_selection - 1;
        if (cmd == CMD_DOWN)
            model->menu_selection = (model->menu_selection + 1 > 2) ? 0 : model->menu_selection + 1;

        if (cmd == CMD_RETURN || cmd == CMD_SHOOT)
        {
            model->sounds.play_select_sound = true;
            if (model->menu_selection == 0)
                exit(0);
            else if (model->menu_selection == 1)
            {
                model->state = (model->previous_state == STATE_GAME_OVER) ? STATE_GAME_OVER : STATE_PAUSED;
                model->menu_selection = 3;
            }
            else if (model->menu_selection == 2)
            {
                model_scan_saves(model);
                model->state = STATE_SAVE_SELECT;
                model->menu_selection = 0;
            }
        }
        return;
    }
}

// ============================================================================
//                          6. MISE À JOUR DU MONDE (GAME LOOP)
// ============================================================================

/**
 * @brief Met à jour l'état du jeu pour une frame.
 *
 * Fonction principale de la boucle de jeu qui gère :
 * - Les timers (tir, invincibilité, animations)
 * - Le déplacement du joueur avec contraintes de bords
 * - Le comportement de l'OVNI (spawn, mouvement, explosion)
 * - Le déplacement collectif des ennemis (pattern classique)
 * - La montée de difficulté (accélération progressive)
 * - Les tirs ennemis aléatoires
 * - La physique des balles et détection de collisions
 * - Les transitions de niveau quand tous les ennemis sont éliminés
 * - Les conditions de Game Over (vies <= 0)
 *
 * @param model Le modèle de jeu à mettre à jour.
 * @param dt Delta time en secondes depuis la dernière frame.
 */
void model_update(GameModel *model, double dt)
{
    // A. ÉTATS SPÉCIAUX
    if (model->state == STATE_SAVE_SUCCESS)
    {
        model->save_success_timer -= dt;
        if (model->save_success_timer <= 0)
            exit(0);
        return;
    }
    if (model->state == STATE_GAME_OVER)
    {
        model->game_over_timer += dt;
        return;
    }
    if (model->state != STATE_PLAYING)
        return;

    // B. TIMERS
    if (model->player.shoot_timer > 0)
        model->player.shoot_timer -= dt;
    if (model->hit_timer > 0)
        model->hit_timer -= dt;

    float beat = 0.5f - (model->level * 0.05f);
    if (beat < 0.05f)
        beat = 0.05f;
    model->animation_timer += dt;
    if (model->animation_timer >= beat)
    {
        model->animation_frame = !model->animation_frame;
        model->animation_timer = 0;
        model->sounds.play_beat = true;
        model->sounds.beat_index = (model->sounds.beat_index + 1) % 4;
    }

    // C. JOUEUR
    if (model->player.active)
    {
        model->player.x += model->player.dx * dt;
        if (model->player.x < 0)
            model->player.x = 0;
        if (model->player.x > GAME_WIDTH - PLAYER_WIDTH)
            model->player.x = GAME_WIDTH - PLAYER_WIDTH;
    }

    // D. UFO (OVNI)
    if (model->ufo.active)
    {
        model->sounds.ufo_loopING = !model->ufo.exploding;

        if (model->ufo.exploding)
        {
            model->ufo.explode_timer -= dt;
            if (model->ufo.explode_timer <= 0)
                model->ufo.active = false;
        }
        else
        {
            model->ufo.x += model->ufo.dx * dt;
            if ((model->ufo.dx > 0 && model->ufo.x > GAME_WIDTH) ||
                (model->ufo.dx < 0 && model->ufo.x < -UFO_WIDTH))
            {
                model->ufo.active = false;
            }
        }
    }
    else
    {
        model->sounds.ufo_loopING = false;
        // Vérifie s'il reste des ennemis vivants
        bool enemies_alive = false;
        for (int i = 0; i < MAX_ENEMIES; i++)
            if (model->enemies[i].active)
            {
                enemies_alive = true;
                break;
            }

        // Spawn aléatoire
        if (!model->ufo.hasSpawnedThisLevel && enemies_alive && (rand() % 500 == 0))
            spawn_ufo(model);
    }

    // E. ENNEMIS
    int active_enemies = 0;
    bool touch_edge = false;

    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (!model->enemies[i].active)
            continue;
        if (model->enemies[i].exploding)
        {
            model->enemies[i].explode_timer -= dt;
            if (model->enemies[i].explode_timer <= 0)
                model->enemies[i].active = false;
            continue;
        }
        active_enemies++;
        if ((model->enemies[i].x <= 0 && model->direction_enemies == -1) ||
            (model->enemies[i].x >= GAME_WIDTH - ENEMY_WIDTH && model->direction_enemies == 1))
            touch_edge = true;
    }

    if (active_enemies == 0 && !model->ufo.active)
    {
        model->level++;
        model->sounds.play_level_up = true;
        init_enemies(model);
        return;
    }

    if (touch_edge)
    {
        model->direction_enemies *= -1;
        float dy = (model->drop_direction == 1) ? ENEMY_DROP_HEIGHT : -ENEMY_DROP_HEIGHT;
        if (model->drop_direction == 1)
        {
            if (++model->drop_step_count >= 3)
                model->drop_direction = -1;
        }
        else
        {
            if (--model->drop_step_count <= 0)
                model->drop_direction = 1;
        }

        for (int i = 0; i < MAX_ENEMIES; i++)
            if (model->enemies[i].active && !model->enemies[i].exploding)
            {
                model->enemies[i].y += dy;
                model->enemies[i].x += model->direction_enemies * 2.0f;
            }
    }
    else
    {
        float spd = ENEMY_SPEED_BASE * model->enemy_speed_mult * model->direction_enemies;
        for (int i = 0; i < MAX_ENEMIES; i++)
            if (model->enemies[i].active && !model->enemies[i].exploding)
                model->enemies[i].x += spd * dt;
    }

    // Tirs Ennemis
    if ((rand() % 100) < (model->level * 2))
    {
        for (int k = 0; k < 10; k++)
        {
            int idx = rand() % MAX_ENEMIES;
            if (model->enemies[idx].active && !model->enemies[idx].exploding)
            {
                spawn_bullet(model, model->enemies[idx].x + ENEMY_WIDTH / 2.0f, model->enemies[idx].y + ENEMY_HEIGHT, BULLET_SPEED * 0.6f, ENTITY_BULLET_ENEMY);
                break;
            }
        }
    }

    // F. BALLES & COLLISIONS
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        Entity *b = &model->bullets[i];
        if (!b->active)
            continue;

        b->y += b->dy * dt;
        b->anim_timer += dt;
        if (b->anim_timer > 0.1f)
        {
            b->anim_timer = 0;
            b->anim_frame = (b->anim_frame + 1) % 4;
        }
        if (b->y < -10 || b->y > GAME_HEIGHT)
        {
            b->active = false;
            continue;
        }

        // --- Boucliers ---
        bool hit_shield = false;
        for (int s = 0; s < MAX_SHIELDS; s++)
        {
            if (model->shields[s].active)
            {
                if (b->x < model->shields[s].x + model->shields[s].width &&
                    b->x + b->width > model->shields[s].x &&
                    b->y < model->shields[s].y + model->shields[s].height &&
                    b->y + b->height > model->shields[s].y)
                {
                    b->active = false;
                    hit_shield = true;
                    model->shields[s].health--;
                    if (model->shields[s].health <= 0)
                        model->shields[s].active = false;
                    break;
                }
            }
        }
        if (hit_shield)
            continue;

        if (b->type == ENTITY_BULLET_PLAYER)
        {
            if (model->ufo.active && !model->ufo.exploding)
            {
                if (b->x < model->ufo.x + model->ufo.width &&
                    b->x + b->width > model->ufo.x &&
                    b->y < model->ufo.y + model->ufo.height &&
                    b->y + b->height > model->ufo.y)
                {
                    b->active = false;
                    model->ufo.exploding = true;
                    model->ufo.explode_timer = 0.5f;
                    model->score += 100;
                    model->lives++;
                    model->sounds.play_invader_killed = true;
                    continue;
                }
            }

            for (int e = 0; e < MAX_ENEMIES; e++)
            {
                if (model->enemies[e].active && !model->enemies[e].exploding && check_collision(b, &model->enemies[e]))
                {
                    b->active = false;
                    model->enemies[e].exploding = true;
                    model->enemies[e].explode_timer = 0.2f;
                    int pts = (model->enemies[e].type == ENTITY_ENEMY_TYPE_1) ? 10 : (model->enemies[e].type == ENTITY_ENEMY_TYPE_2 ? 20 : 30);
                    model->score += pts;
                    model->sounds.play_invader_killed = true;
                    break;
                }
            }
        }
        else
        {
            if (model->player.active && model->hit_timer <= 0 && check_collision(b, &model->player))
            {
                b->active = false;
                model->lives--;
                model->hit_timer = 2.0f;
                model->sounds.play_player_explosion = true;

                if (model->lives <= 0)
                {
                    model->state = STATE_GAME_OVER;
                    model->sounds.play_game_over = true;

                    model->menu_selection = 0;
                    model->game_over_timer = 0;
                }
            }
        }
    }
}
// ============================================================================
//                          7. GESTION DES FICHIERS (SAUVEGARDE / CHARGEMENT)
// ============================================================================

/**
 * @brief Scanne le dossier de sauvegardes et remplit la liste des fichiers disponibles.
 *
 * Parcourt le répertoire "sauvegardes/" et collecte tous les fichiers
 * avec l'extension ".dat". Les noms sont stockés dans model->save_files[]
 * et le compteur model->save_file_count est mis à jour.
 *
 * @param model Le modèle de jeu dont la liste de sauvegardes sera mise à jour.
 * @note Limite de MAX_SAVE_FILES fichiers maximum.
 */
void model_scan_saves(GameModel *model)
{
    model->save_file_count = 0;

    // Ouverture du dossier (Standard POSIX/Linux)
    DIR *d = opendir("sauvegardes");
    if (!d)
        return; // Le dossier n'existe peut-être pas encore

    struct dirent *dir;
    while ((dir = readdir(d)) != NULL)
    {
        // On ignore les dossiers cachés (. et ..)
        if (dir->d_name[0] == '.')
            continue;

        // On cherche uniquement les fichiers finissant par .dat
        if (strstr(dir->d_name, ".dat"))
        {
            if (model->save_file_count < MAX_SAVE_FILES)
            {
                // On copie le nom (ex: "partie1.dat")
                snprintf(model->save_files[model->save_file_count], 256, "%s", dir->d_name);
                model->save_file_count++;
            }
        }
    }
    closedir(d);
}

/**
 * @brief Sauvegarde l'état actuel du jeu dans un fichier binaire.
 *
 * Écrit la structure GameModel complète dans un fichier du dossier
 * "sauvegardes/". La sauvegarde est effectuée en mode binaire (dump mémoire).
 *
 * @param model Le modèle de jeu à sauvegarder.
 * @param filename Le nom du fichier de sauvegarde (ex: "partie1.dat").
 * @note Le dossier "sauvegardes" est créé automatiquement s'il n'existe pas.
 */
void model_save_named(const GameModel *model, const char *filename)
{
    // On s'assure que le dossier existe (Linux : 0777 = permissions complètes)
    mkdir("sauvegardes", 0777);

    char path[128];
    snprintf(path, sizeof(path), "sauvegardes/%s", filename);

    FILE *f = fopen(path, "wb");
    if (f)
    {
        fwrite(model, sizeof(GameModel), 1, f);
        fclose(f);
        printf("[SYSTEM] Sauvegarde reussie : %s\n", path);
    }
    else
    {
        fprintf(stderr, "[ERREUR] Impossible d'ecrire dans %s\n", path);
    }
}

/**
 * @brief Charge une sauvegarde depuis un fichier binaire.
 *
 * Lit le fichier spécifié et restaure l'état complet du jeu.
 * Après le chargement, l'état est automatiquement mis sur STATE_PLAYING
 * et les timers visuels sont réinitialisés.
 *
 * @param model Le modèle de jeu à restaurer.
 * @param filename Le nom du fichier de sauvegarde (ex: "partie1.dat").
 * @return true si le chargement a réussi, false sinon (fichier inexistant ou corrompu).
 */
bool model_load_named(GameModel *model, const char *filename)
{
    char path[128];
    snprintf(path, sizeof(path), "sauvegardes/%s", filename);

    FILE *f = fopen(path, "rb");
    if (!f)
        return false;

    GameModel temp;
    size_t res = fread(&temp, sizeof(GameModel), 1, f);
    fclose(f);

    if (res == 1)
    {
        *model = temp;
        model->state = STATE_PLAYING;
        model->hit_timer = 0;
        memset(&model->sounds, 0, sizeof(SoundState));
        printf("[SYSTEM] Chargement reussi : %s\n", path);
        return true;
    }

    printf("[ERREUR] Fichier de sauvegarde corrompu.\n");
    return false;
}