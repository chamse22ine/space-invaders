/**
 * @file view_sdl.h
 * @brief Implémentation Concrète de la Vue Graphique (SDL3).
 *
 * Ce module expose l'instance de l'interface utilisant la **Simple DirectMedia Layer (SDL3)**.
 * C'est ici que sont définies les structures contenant les "vrais" pointeurs vers
 * les fenêtres, textures et sons.
 *
 * Le fichier regroupe aussi tous les chemins d'accès aux assets (Images/Sons).
 */

#ifndef VIEW_SDL_H
#define VIEW_SDL_H

#include "view_interface.h"

// --- Inclusions nécessaires pour les types SDL3 ---
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>

// ============================================================================
//                        CONSTANTES & CONFIGURATION
// ============================================================================

/** @name Palette de Couleurs
 * Couleurs statiques utilisées pour le texte et les formes géométriques.
 */
///@{
static const SDL_Color COL_WHITE = {255, 255, 255, 255};
static const SDL_Color COL_RED = {255, 50, 50, 255};
static const SDL_Color COL_GREEN = {50, 255, 50, 255};
static const SDL_Color COL_YELLOW = {255, 215, 0, 255};
static const SDL_Color COL_GRAY = {150, 150, 150, 255};
///@}

/** @name Paramètres Fenêtre & HUD */
///@{
#define WIN_WIDTH 1280      ///< Largeur de la fenêtre SDL en pixels.
#define WIN_HEIGHT 768      ///< Hauteur de la fenêtre SDL en pixels.
#define MAX_LIVES_DISPLAY 3 ///< Nombre max de cœurs affichés dans l'interface.
#define HEART_UI_SIZE 45    ///< Taille (carrée) de l'icône cœur en pixels.
#define HEART_SPACING 5     ///< Espace entre les cœurs.
///@}

/** @name Chemins des Assets : Entités */
///@{
#define IMG_PLAYER "assets/aliens/space_player.bmp" ///< Sprite du vaisseau joueur.
#define IMG_UFO "assets/aliens/space_UFO.bmp"       ///< Sprite de l'OVNI bonus.

#define IMG_ENEMY1_A "assets/aliens/alien_A1.bmp" ///< Ennemi Type 1 - Frame A (Pieuvre).
#define IMG_ENEMY1_B "assets/aliens/alien_A2.bmp" ///< Ennemi Type 1 - Frame B (Pieuvre).

#define IMG_ENEMY2_A "assets/aliens/alien_B1.bmp" ///< Ennemi Type 2 - Frame A (Crabe).
#define IMG_ENEMY2_B "assets/aliens/alien_B2.bmp" ///< Ennemi Type 2 - Frame B (Crabe).

#define IMG_ENEMY3_A "assets/aliens/alien_C1.bmp" ///< Ennemi Type 3 - Frame A (Calamar).
#define IMG_ENEMY3_B "assets/aliens/alien_C2.bmp" ///< Ennemi Type 3 - Frame B (Calamar).
///@}

/** @name Chemins des Assets : Environnement & UI */
///@{
#define IMG_BG_MENU "assets/backgrounds/bg_menu_1.bmp"  ///< Fond d'écran du menu principal (variante 1).
#define IMG_BG_MENU_1 "assets/backgrounds/bg_menu.bmp"  ///< Fond d'écran du menu principal (variante 2).
#define IMG_BG_GAME "assets/backgrounds/background.bmp" ///< Fond d'écran pendant le jeu.

#define IMG_HEART_FULL "assets/hearts/heart_full.bmp"   ///< Icône de cœur plein (vie restante).
#define IMG_HEART_EMPTY "assets/hearts/heart_empty.bmp" ///< Icône de cœur vide (vie perdue).

#define FONT_PATH "assets/fonts/LibertinusSerifDisplay-Regular.ttf" ///< Police TrueType utilisée.
#define FONT_SIZE 38                                                ///< Taille de police par défaut.
///@}

/** @name Chemins des Assets : Effets (FX) */
///@{
#define IMG_EXPLOSION_A "assets/explosions/playerExplosionA.bmp" ///< Explosion Frame A.
#define IMG_EXPLOSION_B "assets/explosions/playerExplosionB.bmp" ///< Explosion Frame B.
///@}

// ============================================================================
//                          STRUCTURES DE DONNÉES
// ============================================================================

/**
 * @brief Gestionnaire de Textures.
 * Contient tous les pointeurs `SDL_Texture*` chargés en mémoire GPU.
 */
typedef struct
{
    // --- Entités ---
    SDL_Texture *player; ///< Sprite du vaisseau joueur.
    SDL_Texture *bullet; ///< Sprite générique de balle.
    SDL_Texture *ufo;    ///< Sprite de l'OVNI bonus.

    /** * @brief Tableau des sprites d'ennemis.
     * Dimensions : [3 Types] x [2 Frames d'animation].
     */
    SDL_Texture *enemies[3][2];

    // --- Décors ---
    SDL_Texture *bg_menu;   ///< Fond écran titre.
    SDL_Texture *bg_menu_1; ///< Fond sous-menus.
    SDL_Texture *bg_game;   ///< Fond étoilé du jeu.

    // --- Interface (HUD) ---
    SDL_Texture *heart_full;  ///< Cœur plein (Vie restante).
    SDL_Texture *heart_empty; ///< Cœur vide (Vie perdue).
    SDL_Texture *blur;        ///< Texture pour effet de flou (optionnel).

    // --- Effets Visuels (FX) ---
    SDL_Texture *expl_player[2]; ///< Animation mort joueur (2 frames).
    SDL_Texture *expl_enemy;     ///< Explosion alien (hitmarker).
    SDL_Texture *expl_ufo;       ///< Explosion spéciale OVNI.

    // --- Projectiles & Boucliers ---
    SDL_Texture *missiles[4];    ///< Sprites animés missiles joueur.
    SDL_Texture *projectiles[4]; ///< Sprites animés tirs aliens.
    SDL_Texture *shields[10];    ///< Sprites d'érosion des bunkers (10 états).
} GameTextures;

/**
 * @brief Gestionnaire Audio.
 * Contient les sons (SFX) et musiques chargés via SDL3_mixer.
 */
typedef struct
{
    // --- SFX (Bruitages courts) ---
    MIX_Audio *shoot;     ///< Tir.
    MIX_Audio *killed;    ///< Mort alien.
    MIX_Audio *explosion; ///< Mort joueur.
    MIX_Audio *ufo;       ///< Son brut de l'OVNI.
    MIX_Audio *beat[4];   ///< Les 4 sons du rythme cardiaque des aliens.
    MIX_Audio *game_over; ///< Jingle défaite.
    MIX_Audio *level_up;  ///< Jingle niveau terminé.
    MIX_Audio *select;    ///< Validation menu.

    // --- Pistes Audio (Contrôlables) ---
    MIX_Audio *bg_music_data;  ///< Données brutes de la musique.
    MIX_Track *bg_music_track; ///< Piste de contrôle (Volume, Pause) musique menu.
    MIX_Track *ufo_track;      ///< Piste de contrôle pour la boucle OVNI.
} GameAudio;

/**
 * @brief Contexte Global SDL.
 * Structure "God Object" passée à toutes les fonctions de rendu SDL.
 * Regroupe la fenêtre, le renderer, et les assets chargés.
 */
typedef struct
{
    SDL_Window *window;     ///< La fenêtre OS.
    SDL_Renderer *renderer; ///< Le contexte de rendu 2D (GPU).

    TTF_Font *font;       ///< Police standard.
    TTF_Font *font_title; ///< Police titre (grande).

    GameTextures tex; ///< Conteneur des images.
    GameAudio sfx;    ///< Conteneur des sons.

    MIX_Mixer *mixer; ///< Instance principale du mixeur audio SDL3.

    int ufo_channel; ///< ID du canal audio OVNI (si gestion par canaux).
} SDLContext;

/**
 * @brief Instance concrète de l'interface View pour SDL.
 * Cette variable permet au `main.c` d'utiliser SDL sans connaître les détails ci-dessus.
 */
extern const ViewInterface view_sdl;

#endif // VIEW_SDL_H