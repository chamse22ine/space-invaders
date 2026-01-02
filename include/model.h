/**
 * @file model.h
 * @brief Définition du Modèle de Données (Data Model).
 *
 * Ce fichier est le cœur du projet. Il définit toutes les structures de données
 * qui représentent l'état complet du jeu à un instant T (Entités, Score, Menus).
 * Il contient aussi les constantes de réglage du gameplay (vitesses, points de vie).
 *
 * Le modèle est totalement indépendant de l'affichage (SDL ou Ncurses).
 */

#ifndef MODEL_H
#define MODEL_H

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// Includes Système pour la gestion des fichiers/dossiers
#include <sys/stat.h>
#include <sys/types.h>
#ifdef _WIN32
#include <direct.h> // Pour _mkdir sous Windows
#else
#include <unistd.h>
#include <dirent.h> // Pour lire les dossiers sous Linux/Mac
#endif

#include "common.h"     // Dimensions globales et FPS
#include "controller.h" // Commandes abstraites (GameCommand)

// ============================================================================
//                        CONSTANTES DE GAMEPLAY (ÉQUILIBRAGE)
// ============================================================================

/** @name Physique & Vitesses
 * Les vitesses sont exprimées en unités logiques par seconde.
 */
///@{
#define PLAYER_SPEED 40.0f     ///< Vitesse de déplacement horizontal du joueur.
#define BULLET_SPEED 60.0f     ///< Vitesse verticale des projectiles (Alliés et Ennemis).
#define ENEMY_SPEED_BASE 10.0f ///< Vitesse initiale du groupe d'ennemis (augmente avec le niveau).
#define ENEMY_DROP_HEIGHT 2.0f ///< Distance de chute des ennemis lorsqu'ils touchent un bord.
///@}

/** @name Dimensions des Hitboxes (Logique)
 * Taille des entités sur la grille logique 100x50.
 */
///@{
#define PLAYER_WIDTH 5  ///< Largeur du vaisseau.
#define PLAYER_HEIGHT 3 ///< Hauteur du vaisseau.
#define ENEMY_WIDTH 4   ///< Largeur d'un alien standard.
#define ENEMY_HEIGHT 3  ///< Hauteur d'un alien standard.
#define BULLET_WIDTH 1  ///< Largeur d'un tir.
#define BULLET_HEIGHT 1 ///< Hauteur d'un tir.
///@}

/** @name Système de Sauvegarde */
///@{
#define MAX_SAVE_FILES 10   ///< Nombre maximum de fichiers affichés dans le menu "Charger".
#define MAX_FILENAME_LEN 32 ///< Taille maximale du nom d'un fichier (ex: "Partie1").
///@}

/** @name Configuration du Jeu */
///@{
#define MAX_SHIELDS 4        ///< Nombre de bunkers/boucliers sur le terrain.
#define SHIELD_MAX_HEALTH 10 ///< Points de vie d'un bunker (10 sprites d'érosion).
#define MAX_LIVES_NORMAL 3   ///< Nombre de vies données au début d'une nouvelle partie.
///@}

/** @name Configuration OVNI (Bonus) */
///@{
#define UFO_POINTS 60   ///< Score de base gagné en détruisant l'OVNI (peut être randomisé).
#define UFO_SPEED 10.0f ///< Vitesse de déplacement latéral de l'OVNI.
#define UFO_WIDTH 4     ///< Largeur logique de l'OVNI.
#define UFO_HEIGHT 2    ///< Hauteur logique de l'OVNI.
///@}

// ============================================================================
//                              ENUMÉRATIONS
// ============================================================================

/**
 * @brief Identifie la nature d'une entité.
 * Indispensable pour gérer les collisions (ex: un tir ennemi ne tue pas un ennemi)
 * et choisir le bon sprite lors de l'affichage.
 */
typedef enum
{
    ENTITY_PLAYER,        ///< Le Vaisseau du joueur.
    ENTITY_BULLET_PLAYER, ///< Projectile tiré par le joueur (monte vers le haut).
    ENTITY_BULLET_ENEMY,  ///< Projectile tiré par un ennemi (descend vers le bas).
    ENTITY_ENEMY_TYPE_1,  ///< Ennemi rangée du bas (Pieuvre) - Rapporte 10 pts.
    ENTITY_ENEMY_TYPE_2,  ///< Ennemi rangée du milieu (Crabe) - Rapporte 20 pts.
    ENTITY_ENEMY_TYPE_3,  ///< Ennemi rangée du haut (Calamar) - Rapporte 40 pts.
    ENTITY_UFO            ///< Soucoupe bonus mystère (apparitions aléatoires).
} EntityType;

/**
 * @brief États de la Machine à États Finis (FSM) du jeu.
 *
 * Cette énumération dicte quel écran afficher (Vue) et comment traiter les inputs (Modèle).
 */
typedef enum
{
    STATE_MENU,              ///< Menu Principal (Jouer, Tuto, Quitter).
    STATE_PLAYING,           ///< Le jeu est en cours (simulation active).
    STATE_PAUSED,            ///< Simulation figée, menu de pause affiché.
    STATE_TUTORIAL,          ///< Écran explicatif des touches et points.
    STATE_GAME_OVER,         ///< Écran de fin de partie (Plus de vies).
    STATE_VICTORY,           ///< Écran de victoire (Tous niveaux terminés - optionnel).
    STATE_CONFIRM_QUIT,      ///< Pop-up de confirmation "Voulez-vous quitter ?".
    STATE_SAVE_INPUT,        ///< Écran de saisie clavier pour nommer la sauvegarde.
    STATE_LOAD_MENU,         ///< Menu listant les fichiers .dat disponibles.
    STATE_OVERWRITE_CONFIRM, ///< Pop-up "Fichier existant : Écraser ou Copier ?".
    STATE_SAVE_SELECT,       ///< Menu intermédiaire "Nouvelle Sauvegarde" vs "Écraser".
    STATE_SAVE_SUCCESS       ///< Message temporaire "Sauvegarde Réussie !".
} GameStateEnum;

// ============================================================================
//                          STRUCTURES DE DONNÉES
// ============================================================================

/**
 * @brief Entité générique (Acteur du jeu).
 * Structure polymorphe simple utilisée pour tout ce qui bouge.
 */
typedef struct
{
    // Physique
    float x;         ///< Position X (Coin haut-gauche, coords logiques).
    float y;         ///< Position Y (Coin haut-gauche, coords logiques).
    int width;       ///< Largeur de la Hitbox.
    int height;      ///< Hauteur de la Hitbox.
    float dx;        ///< Composante X du vecteur vitesse.
    float dy;        ///< Composante Y du vecteur vitesse.
    bool active;     ///< Si false, l'entité est ignorée (morte/libre pour réutilisation).
    EntityType type; ///< Catégorie de l'entité.

    // Gameplay
    float shoot_timer; ///< Temps restant avant de pouvoir tirer à nouveau (Cooldown).

    // Animation
    float anim_timer; ///< Timer interne pour alterner les sprites.
    int anim_frame;   ///< Index du sprite d'animation (ex: 0=Bras levés, 1=Bras baissés).

    // État Explosion
    bool exploding;      ///< True si l'entité est en train de mourir.
    float explode_timer; ///< Durée restante de l'animation d'explosion.
} Entity;

/**
 * @brief Entité Spéciale : L'OVNI (Mystery Ship).
 * Séparé car il a des règles d'apparition uniques.
 */
typedef struct
{
    float x;                  ///< Position X de l'OVNI.
    float y;                  ///< Position Y de l'OVNI.
    float width;              ///< Largeur de l'OVNI.
    float height;             ///< Hauteur de l'OVNI.
    float dx;                 ///< Vitesse de déplacement horizontale.
    bool active;              ///< Est-il visible à l'écran ?
    bool hasSpawnedThisLevel; ///< Sécurité pour limiter à 1 OVNI par niveau.
    bool exploding;           ///< En cours d'explosion.
    float explode_timer;      ///< Timer explosion.
    EntityType type;          ///< Toujours ENTITY_UFO.
} Ufo;

/**
 * @brief Entité Spéciale : Bouclier (Bunker).
 * Objet statique destructible pixel par pixel (ou par niveau de dégradation).
 */
typedef struct
{
    float x;      ///< Position X du bouclier.
    float y;      ///< Position Y du bouclier.
    float width;  ///< Largeur du bouclier.
    float height; ///< Hauteur du bouclier.
    int health;   ///< Points de vie (0 à 10). Détermine le sprite "abîmé".
    bool active;  ///< True tant que health > 0.
} Shield;

/**
 * @brief Système de drapeaux (Flags) Audio.
 * Pattern "Fire-and-Forget" : Le Modèle demande un son (true), la Vue le joue et reset (false).
 */
typedef struct
{
    // SFX (Sons ponctuels)
    bool play_shoot;            ///< Demande de bruit de tir.
    bool play_invader_killed;   ///< Demande de bruit de mort alien.
    bool play_player_explosion; ///< Demande de bruit de mort joueur.
    bool play_select_sound;     ///< Demande de "Bip" menu.
    bool play_game_over;        ///< Demande de jingle défaite.
    bool play_level_up;         ///< Demande de jingle niveau suivant.

    // Musique / Ambiance
    bool play_beat;   ///< Trigger pour le rythme "Cœur" des aliens qui accélère.
    int beat_index;   ///< Note actuelle du rythme (0, 1, 2, 3).
    bool ufo_loopING; ///< État continu : L'OVNI est présent (Son moteur en boucle).
} SoundState;

/**
 * @brief Structure Principale (God Object).
 * Contient l'intégralité des données du jeu. C'est ce bloc mémoire qui est
 * écrit sur le disque lors d'une sauvegarde (Serialization).
 *
 */
typedef struct
{
    // Machine à états
    GameStateEnum state;          ///< État courant.
    GameStateEnum previous_state; ///< État précédent (pour retour après Pause).

    // --- Les Acteurs (Tableaux statiques pour éviter malloc en jeu) ---
    Entity player;               ///< Le Joueur.
    Entity enemies[MAX_ENEMIES]; ///< Le tableau des envahisseurs.
    Entity bullets[MAX_BULLETS]; ///< Le pool de projectiles.
    Ufo ufo;                     ///< L'OVNI bonus.
    Shield shields[MAX_SHIELDS]; ///< Les bunkers.

    // --- Stats Partie ---
    int score;            ///< Score actuel.
    int lives;            ///< Vies restantes.
    int level;            ///< Niveau de difficulté actuel.
    int normal_max_lives; ///< Seuil pour gagner une vie bonus (ex: tous les 1500 pts).

    // --- IA de Groupe (La Vague) ---
    float enemy_speed_mult; ///< Accélération globale des aliens (plus il en reste peu, plus ils vont vite).
    int direction_enemies;  ///< 1 (Droite) ou -1 (Gauche).
    int drop_direction;     ///< Sens vertical (généralement 1 pour descendre).
    int drop_step_count;    ///< Compteur pour gérer l'animation saccadée de descente.

    // --- Animation Globale ---
    int animation_frame;   ///< Frame globale (0/1) synchronisant tous les aliens.
    float animation_timer; ///< Timer pour battre la mesure de l'animation.

    // --- Timers Divers ---
    float game_over_timer;    ///< Bloque les inputs quelques secondes après la mort.
    float hit_timer;          ///< Temps d'invulnérabilité après un impact.
    float save_success_timer; ///< Temps d'affichage du message de succès sauvegarde.

    // --- Interface & Menus ---
    int menu_selection;                  ///< Index de l'élément sélectionné (0, 1, 2...).
    char input_buffer[MAX_FILENAME_LEN]; ///< Buffer stockant ce que le joueur tape (Sauvegarde).

    // --- Système de Fichiers ---
    char save_files[MAX_SAVE_FILES][256]; ///< Cache des noms de fichiers trouvés dans /saves.
    int save_file_count;                  ///< Nombre de fichiers valides trouvés.
    char current_filename[64];            ///< Nom du fichier actuellement chargé (pour écrasement rapide).
    bool pending_quit;                    ///< Flag demandant la fermeture propre de la boucle principale.

    // --- Audio ---
    SoundState sounds; ///< État des demandes sonores.
    int volume;        ///< Volume global (0-100).
    bool is_muted;     ///< Mode muet.
} GameModel;

// ============================================================================
//                          PROTOTYPES PUBLICS (API)
// ============================================================================

/**
 * @brief Initialise le modèle (Constructeur).
 * Alloue la structure et configure les valeurs par défaut (Niveau 1, Score 0).
 * @return Un pointeur vers le nouveau GameModel.
 */
GameModel *model_init(void);

/**
 * @brief Libère la mémoire (Destructeur).
 */
void model_free(GameModel *model);

/**
 * @brief Met à jour la logique du jeu (Physics & Logic).
 * Appelé à chaque frame.
 */
void model_update(GameModel *model, double dt);

/**
 * @brief Traite une commande abstraite (Input).
 */
void model_handle_input(GameModel *model, GameCommand cmd);

/**
 * @brief Accesseur lecture seule pour le joueur.
 * Permet à la Vue de savoir où dessiner le joueur sans risquer de le modifier.
 */
const Entity *model_get_player(const GameModel *model);

// --- Gestion des Sauvegardes ---

/**
 * @brief Scanne le dossier de sauvegarde.
 * Remplit `model->save_files` avec les noms des fichiers .dat trouvés.
 */
void model_scan_saves(GameModel *model);

/**
 * @brief Sauvegarde binaire.
 * Ecrit l'intégralité de la structure `GameModel` dans un fichier.
 */
void model_save_named(const GameModel *model, const char *filename);

/**
 * @brief Chargement binaire.
 * Lit un fichier .dat et écrase le `GameModel` actuel avec les données lues.
 * @return true si succès, false si erreur lecture.
 */
bool model_load_named(GameModel *model, const char *filename);

#endif // MODEL_H