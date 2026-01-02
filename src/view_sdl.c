/**
 * @file view_sdl.c
 * @brief Implémentation de la vue graphique avec SDL3.
 *
 * Ce fichier gère l'affichage du jeu Space Invaders en mode graphique
 * en utilisant SDL3 pour le rendu, SDL_ttf pour les polices et SDL_mixer pour l'audio.
 */

#include "view_sdl.h"
#include <stdio.h>
#include <stdlib.h>

// ============================================================================
// 1. CONFIGURATION & GLOBALES
// ============================================================================

// Instance unique du contexte
static SDLContext ctx = {0};

// Facteurs de mise à l'échelle (Model -> Pixels)
static float SCALE_X = 1.0f;
static float SCALE_Y = 1.0f;
// ============================================================================
// 2. FONCTIONS UTILITAIRES (HELPERS DE BASE)
// ============================================================================

/**
 * @brief Charge une image depuis un fichier et crée une texture SDL.
 *
 * Gère automatiquement la transparence pour les images sans canal alpha
 * en utilisant le noir (0,0,0) comme couleur clé.
 *
 * @param path Chemin vers le fichier image (BMP, PNG, etc.).
 * @return Pointeur vers la texture SDL créée, ou NULL en cas d'échec.
 */
static SDL_Texture *load_texture(const char *path)
{
    SDL_Surface *surface = IMG_Load(path);
    if (!surface)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Missing: %s", path);
        return NULL;
    }
    if (SDL_GetPixelFormatDetails(surface->format)->bits_per_pixel < 32)
    {
        const SDL_PixelFormatDetails *d = SDL_GetPixelFormatDetails(surface->format);
        Uint32 key = SDL_MapRGB(d, NULL, 0, 0, 0);
        SDL_SetSurfaceColorKey(surface, true, key);
    }
    SDL_Texture *tex = SDL_CreateTextureFromSurface(ctx.renderer, surface);
    SDL_DestroySurface(surface);
    if (tex)
        SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
    return tex;
}

/**
 * @brief Affiche du texte à une position donnée avec la police par défaut.
 *
 * @param text Le texte à afficher.
 * @param x Position horizontale en pixels.
 * @param y Position verticale en pixels.
 * @param color Couleur du texte (SDL_Color).
 */
static void draw_text(const char *text, int x, int y, SDL_Color color)
{
    if (!ctx.font || !text || !text[0])
        return;
    SDL_Surface *s = TTF_RenderText_Blended(ctx.font, text, 0, color);
    if (!s)
        return;
    SDL_Texture *t = SDL_CreateTextureFromSurface(ctx.renderer, s);
    if (t)
    {
        SDL_FRect r = {(float)x, (float)y, (float)s->w, (float)s->h};
        SDL_RenderTexture(ctx.renderer, t, NULL, &r);
        SDL_DestroyTexture(t);
    }
    SDL_DestroySurface(s);
}

/**
 * @brief Affiche du texte centré horizontalement avec une police spécifique.
 *
 * @param text Le texte à afficher.
 * @param y Position verticale en pixels.
 * @param color Couleur du texte (SDL_Color).
 * @param font Police TTF à utiliser pour le rendu.
 */
static void draw_text_centered(const char *text, int y, SDL_Color color, TTF_Font *font)
{
    if (!font || !text || !text[0])
        return;
    SDL_Surface *s = TTF_RenderText_Blended(font, text, 0, color);
    if (!s)
        return;
    SDL_Texture *t = SDL_CreateTextureFromSurface(ctx.renderer, s);
    if (t)
    {
        float x = (WIN_WIDTH - s->w) / 2.0f;
        SDL_FRect r = {x, (float)y, (float)s->w, (float)s->h};
        SDL_RenderTexture(ctx.renderer, t, NULL, &r);
        SDL_DestroyTexture(t);
    }
    SDL_DestroySurface(s);
}

/**
 * @brief Dessine une entité du jeu avec mise à l'échelle et décalage optionnel.
 *
 * Convertit les coordonnées du modèle de jeu en pixels écran
 * et applique un décalage pour les effets de tremblement.
 *
 * @param tex Texture SDL de l'entité.
 * @param x Position X dans le modèle de jeu.
 * @param y Position Y dans le modèle de jeu.
 * @param w Largeur dans le modèle de jeu.
 * @param h Hauteur dans le modèle de jeu.
 * @param shake_x Décalage horizontal pour l'effet de tremblement.
 * @param shake_y Décalage vertical pour l'effet de tremblement.
 */
static void draw_entity_scaled(SDL_Texture *tex, float x, float y, float w, float h, int shake_x, int shake_y)
{
    if (!tex)
        return;
    SDL_FRect dst = {(x * SCALE_X) + shake_x, (y * SCALE_Y) + shake_y, w * SCALE_X, h * SCALE_Y};
    SDL_RenderTexture(ctx.renderer, tex, NULL, &dst);
}

/**
 * @brief Dessine un overlay semi-transparent noir sur tout l'écran.
 *
 * Utilisé pour assombrir le fond lors des menus et popups.
 *
 * @param alpha Niveau d'opacité (0 = transparent, 255 = opaque).
 */
static void draw_overlay(int alpha)
{
    SDL_SetRenderDrawBlendMode(ctx.renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ctx.renderer, 0, 0, 0, alpha);
    SDL_RenderFillRect(ctx.renderer, NULL);
    SDL_SetRenderDrawBlendMode(ctx.renderer, SDL_BLENDMODE_NONE);
}

// ============================================================================
// 3. FONCTIONS DE RENDU INTERMÉDIAIRES
// ============================================================================

/**
 * @brief Met à jour l'état audio selon l'état du jeu.
 *
 * Gère la lecture des sons et musiques :
 * - Pause/reprise selon l'état du jeu
 * - Volume et mute
 * - Sons ponctuels (tir, explosion, etc.)
 * - Musique de fond et boucle UFO
 *
 * @param model Le modèle de jeu en lecture seule.
 * @param mutable_model Pointeur modifiable pour réinitialiser les flags sonores.
 */
static void update_audio_state(const GameModel *model, GameModel *mutable_model)
{
    if (!ctx.mixer)
        return;
    bool game_frozen = (model->state == STATE_PAUSED || model->state == STATE_CONFIRM_QUIT ||
                        model->state == STATE_SAVE_SELECT || model->state == STATE_SAVE_INPUT);

    if (game_frozen)
        MIX_PauseAllTracks(ctx.mixer);
    else if (model->state == STATE_PLAYING || model->state == STATE_SAVE_SUCCESS)
        MIX_ResumeAllTracks(ctx.mixer);

    MIX_SetMasterGain(ctx.mixer, model->is_muted ? 0.0f : (float)model->volume / 100.0f);

    bool in_menu = (model->state == STATE_MENU || model->state == STATE_TUTORIAL ||
                    model->state == STATE_LOAD_MENU || model->state == STATE_GAME_OVER);

    if (ctx.sfx.bg_music_track)
    {
        if (in_menu && !game_frozen)
        {
            if (!MIX_TrackPlaying(ctx.sfx.bg_music_track))
                MIX_PlayTrack(ctx.sfx.bg_music_track, 0);
            if (MIX_TrackPaused(ctx.sfx.bg_music_track))
                MIX_ResumeTrack(ctx.sfx.bg_music_track);
        }
        else if (MIX_TrackPlaying(ctx.sfx.bg_music_track))
            MIX_PauseTrack(ctx.sfx.bg_music_track);
    }

    if (model->sounds.play_select_sound)
    {
        MIX_PlayAudio(ctx.mixer, ctx.sfx.select);
        mutable_model->sounds.play_select_sound = false;
    }
    if (model->sounds.play_game_over)
    {
        MIX_PlayAudio(ctx.mixer, ctx.sfx.game_over);
        mutable_model->sounds.play_game_over = false;
    }
    if (model->sounds.play_level_up)
    {
        MIX_PlayAudio(ctx.mixer, ctx.sfx.level_up);
        mutable_model->sounds.play_level_up = false;
    }

    if (!game_frozen && model->state == STATE_PLAYING)
    {
        if (model->sounds.play_shoot)
        {
            MIX_PlayAudio(ctx.mixer, ctx.sfx.shoot);
            mutable_model->sounds.play_shoot = false;
        }
        if (model->sounds.play_invader_killed)
        {
            MIX_PlayAudio(ctx.mixer, ctx.sfx.killed);
            mutable_model->sounds.play_invader_killed = false;
        }
        if (model->sounds.play_player_explosion)
        {
            MIX_PlayAudio(ctx.mixer, ctx.sfx.explosion);
            mutable_model->sounds.play_player_explosion = false;
        }
        if (model->sounds.play_beat)
        {
            MIX_PlayAudio(ctx.mixer, ctx.sfx.beat[model->sounds.beat_index]);
            mutable_model->sounds.play_beat = false;
        }

        if (ctx.sfx.ufo_track)
        {
            if (model->sounds.ufo_loopING)
            {
                if (!MIX_TrackPlaying(ctx.sfx.ufo_track))
                    MIX_PlayTrack(ctx.sfx.ufo_track, 0);
                if (MIX_TrackPaused(ctx.sfx.ufo_track))
                    MIX_ResumeTrack(ctx.sfx.ufo_track);
            }
            else if (MIX_TrackPlaying(ctx.sfx.ufo_track))
                MIX_PauseTrack(ctx.sfx.ufo_track);
        }
    }
}

/**
 * @brief Dessine l'interface utilisateur en jeu (HUD).
 *
 * Affiche le score, le niveau et les vies restantes (cœurs).
 * Les cœurs bonus au-delà de la limite normale sont colorés en or.
 *
 * @param model Le modèle de jeu contenant les informations à afficher.
 */
static void draw_hud(const GameModel *model)
{
    char buf[64];
    snprintf(buf, 64, "SCORE: %d   NIVEAU: %d", model->score, model->level);
    draw_text(buf, 20, 20, COL_WHITE);

    int start_x = WIN_WIDTH - 20;
    int max_draw = (model->lives > MAX_LIVES_DISPLAY) ? model->lives : MAX_LIVES_DISPLAY;
    for (int i = 0; i < max_draw; i++)
    {
        int cx = start_x - ((i + 1) * (HEART_UI_SIZE + 5));
        SDL_Texture *t = (i < model->lives) ? ctx.tex.heart_full : ctx.tex.heart_empty;
        SDL_FRect r = {(float)cx, 20, HEART_UI_SIZE, HEART_UI_SIZE};
        if (t)
        {
            if (i >= MAX_LIVES_DISPLAY)
                SDL_SetTextureColorMod(t, 255, 215, 0);
            else
                SDL_SetTextureColorMod(t, 255, 255, 255);
            SDL_RenderTexture(ctx.renderer, t, NULL, &r);
        }
    }
}

/**
 * @brief Dessine le monde de jeu complet.
 *
 * Affiche tous les éléments du jeu avec effets visuels :
 * - Fond de jeu
 * - Joueur (avec clignotement rouge quand touché)
 * - Ennemis (avec animations et couleurs par type)
 * - UFO (avec effet d'explosion)
 * - Boucliers (avec états de dégradation)
 * - Balles (joueur et ennemis)
 * - Effet de tremblement d'écran quand le joueur est touché
 *
 * @param model Le modèle de jeu contenant l'état du monde.
 */
static void draw_game_world(const GameModel *model)
{
    int sx = 0, sy = 0;
    if (model->state == STATE_PLAYING && model->hit_timer > 0)
    {
        sx = (rand() % 11) - 5;
        sy = (rand() % 11) - 5;
    }

    SDL_RenderTexture(ctx.renderer, ctx.tex.bg_game, NULL, NULL);

    if (model->player.active)
    {
        SDL_Texture *t = ctx.tex.player;
        if (model->hit_timer > 0)
        {
            t = ctx.tex.expl_player[(int)(model->hit_timer * 10) % 2];
            SDL_SetTextureColorMod(t, 255, 100, 100);
        }
        else
            SDL_SetTextureColorMod(t, 0, 255, 0);
        draw_entity_scaled(t, model->player.x, model->player.y, model->player.width, model->player.height, sx, sy);
        SDL_SetTextureColorMod(t, 255, 255, 255);
    }

    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        const Entity *e = &model->enemies[i];
        if (!e->active)
            continue;
        SDL_Texture *t = e->exploding ? ctx.tex.expl_enemy : NULL;
        if (!t)
        {
            int idx = e->type - ENTITY_ENEMY_TYPE_1;
            if (idx < 0)
                idx = 0;
            if (idx > 2)
                idx = 2;
            t = ctx.tex.enemies[idx][model->animation_frame];
            if (idx == 0)
                SDL_SetTextureColorMod(t, 0, 255, 255);
            if (idx == 1)
                SDL_SetTextureColorMod(t, 255, 165, 0);
            if (idx == 2)
                SDL_SetTextureColorMod(t, 255, 50, 50);
        }
        draw_entity_scaled(t, e->x, e->y, e->width, e->height, sx, sy);
        if (t)
            SDL_SetTextureColorMod(t, 255, 255, 255);
    }

    if (model->ufo.active)
    {
        SDL_Texture *t = model->ufo.exploding ? ctx.tex.expl_ufo : ctx.tex.ufo;
        if (!model->ufo.exploding)
            SDL_SetTextureColorMod(t, 255, 0, 255);
        draw_entity_scaled(t, model->ufo.x, model->ufo.y, model->ufo.width, model->ufo.height, sx, sy);
        SDL_SetTextureColorMod(t, 255, 255, 255);
    }

    for (int i = 0; i < MAX_SHIELDS; i++)
    {
        if (!model->shields[i].active)
            continue;
        int idx = 10 - model->shields[i].health;
        if (idx < 0)
            idx = 0;
        if (idx > 9)
            idx = 9;
        SDL_Texture *t = ctx.tex.shields[idx];
        SDL_SetTextureColorMod(t, 0, 255, 0);
        draw_entity_scaled(t, model->shields[i].x, model->shields[i].y, model->shields[i].width, model->shields[i].height, sx, sy);
        SDL_SetTextureColorMod(t, 255, 255, 255);
    }

    for (int i = 0; i < MAX_BULLETS; i++)
    {
        const Entity *b = &model->bullets[i];
        if (!b->active)
            continue;
        SDL_Texture *t = (b->type == ENTITY_BULLET_PLAYER) ? ctx.tex.missiles[b->anim_frame] : ctx.tex.projectiles[b->anim_frame];
        draw_entity_scaled(t, b->x, b->y, 1.0f, 1.0f, sx, sy);
    }
}

// ============================================================================
// 4. FONCTIONS PRINCIPALES (INTERFACE)
// ============================================================================

/**
 * @brief Initialise SDL et charge toutes les ressources graphiques et audio.
 *
 * Configure la fenêtre, le renderer, charge les textures (sprites, fonds,
 * explosions, boucliers), les polices et les sons.
 *
 * @return true si l'initialisation a réussi, false sinon.
 */
static bool sdl_init(void)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO))
        return false;
    if (!TTF_Init())
        return false;
    if (!MIX_Init())
        SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "Mix_Init");

    ctx.window = SDL_CreateWindow("Space Invaders", WIN_WIDTH, WIN_HEIGHT, SDL_WINDOW_RESIZABLE);
    ctx.renderer = SDL_CreateRenderer(ctx.window, NULL);
    if (!ctx.window || !ctx.renderer)
        return false;

    SDL_SetRenderLogicalPresentation(ctx.renderer, WIN_WIDTH, WIN_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    SCALE_X = (float)WIN_WIDTH / GAME_WIDTH;
    SCALE_Y = (float)WIN_HEIGHT / GAME_HEIGHT;

    SDL_AudioSpec spec = {SDL_AUDIO_S16, 2, 44100};
    ctx.mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);

    ctx.font = TTF_OpenFont(FONT_PATH, FONT_SIZE);
    ctx.font_title = TTF_OpenFont(FONT_PATH, 64);

    ctx.tex.player = load_texture("assets/aliens/space_player.bmp");
    ctx.tex.heart_full = load_texture("assets/hearts/heart_full.bmp");
    ctx.tex.heart_empty = load_texture("assets/hearts/heart_empty.bmp");
    ctx.tex.bg_menu = load_texture("assets/backgrounds/bg_menu_1.bmp");
    ctx.tex.bg_menu_1 = load_texture("assets/backgrounds/bg_menu.bmp");
    ctx.tex.bg_game = load_texture("assets/backgrounds/background.bmp");
    ctx.tex.ufo = load_texture("assets/aliens/space_UFO.bmp");
    ctx.tex.expl_ufo = load_texture("assets/explosions/ufoExplosion.bmp");
    ctx.tex.expl_enemy = load_texture("assets/explosions/enemyExplosion.bmp");
    ctx.tex.expl_player[0] = load_texture("assets/explosions/playerExplosionA.bmp");
    ctx.tex.expl_player[1] = load_texture("assets/explosions/playerExplosionB.bmp");

    const char *ef[3][2] = {{"assets/aliens/alien_A1.bmp", "assets/aliens/alien_A2.bmp"}, {"assets/aliens/alien_B1.bmp", "assets/aliens/alien_B2.bmp"}, {"assets/aliens/alien_C1.bmp", "assets/aliens/alien_C2.bmp"}};
    for (int t = 0; t < 3; t++)
        for (int f = 0; f < 2; f++)
            ctx.tex.enemies[t][f] = load_texture(ef[t][f]);

    for (int i = 0; i < 4; i++)
    {
        char p1[64], p2[64];
        snprintf(p1, 64, "assets/missiles/missile_%d.bmp", i + 1);
        snprintf(p2, 64, "assets/projectiles/projectileA_%d.bmp", i + 1);
        ctx.tex.missiles[i] = load_texture(p1);
        ctx.tex.projectiles[i] = load_texture(p2);
    }

    ctx.tex.shields[0] = load_texture("assets/shelter/shelter_full.bmp");
    for (int i = 1; i <= 9; i++)
    {
        char p[64];
        snprintf(p, 64, "assets/shelter/shelterDamaged_%d.bmp", i);
        ctx.tex.shields[i] = load_texture(p);
    }

    if (ctx.mixer)
    {
        ctx.sfx.shoot = MIX_LoadAudio(ctx.mixer, "assets/audio/shootSound.wav", true);
        ctx.sfx.killed = MIX_LoadAudio(ctx.mixer, "assets/audio/invaderKilledSound.wav", true);
        ctx.sfx.explosion = MIX_LoadAudio(ctx.mixer, "assets/audio/explosionSound.wav", true);
        ctx.sfx.ufo = MIX_LoadAudio(ctx.mixer, "assets/audio/ufoSound.wav", true);
        ctx.sfx.game_over = MIX_LoadAudio(ctx.mixer, "assets/audio/gameOverSound.wav", true);
        ctx.sfx.level_up = MIX_LoadAudio(ctx.mixer, "assets/audio/levelUpSound.wav", true);
        ctx.sfx.select = MIX_LoadAudio(ctx.mixer, "assets/audio/selectSound.wav", true);
        ctx.sfx.bg_music_data = MIX_LoadAudio(ctx.mixer, "assets/audio/menuSound.wav", true);

        for (int i = 0; i < 4; i++)
        {
            char p[64];
            snprintf(p, 64, "assets/audio/fastinvader%d.wav", i + 1);
            ctx.sfx.beat[i] = MIX_LoadAudio(ctx.mixer, p, true);
        }
        if (ctx.sfx.bg_music_data)
        {
            ctx.sfx.bg_music_track = MIX_CreateTrack(ctx.mixer);
            if (ctx.sfx.bg_music_track)
                MIX_SetTrackAudio(ctx.sfx.bg_music_track, ctx.sfx.bg_music_data);
        }
        if (ctx.sfx.ufo)
        {
            ctx.sfx.ufo_track = MIX_CreateTrack(ctx.mixer);
            if (ctx.sfx.ufo_track)
                MIX_SetTrackAudio(ctx.sfx.ufo_track, ctx.sfx.ufo);
        }
    }
    return true;
}

/**
 * @brief Libère toutes les ressources SDL et ferme proprement.
 *
 * Détruit les textures, les pistes audio, les polices,
 * le renderer et la fenêtre, puis quitte SDL.
 */
static void sdl_close(void)
{
    if (ctx.sfx.ufo_track)
        MIX_DestroyTrack(ctx.sfx.ufo_track);
    if (ctx.sfx.bg_music_track)
        MIX_DestroyTrack(ctx.sfx.bg_music_track);
    if (ctx.sfx.shoot)
        MIX_DestroyAudio(ctx.sfx.shoot);
    if (ctx.sfx.killed)
        MIX_DestroyAudio(ctx.sfx.killed);
    if (ctx.sfx.explosion)
        MIX_DestroyAudio(ctx.sfx.explosion);
    if (ctx.sfx.ufo)
        MIX_DestroyAudio(ctx.sfx.ufo);
    if (ctx.sfx.game_over)
        MIX_DestroyAudio(ctx.sfx.game_over);
    if (ctx.sfx.level_up)
        MIX_DestroyAudio(ctx.sfx.level_up);
    if (ctx.sfx.select)
        MIX_DestroyAudio(ctx.sfx.select);
    if (ctx.sfx.bg_music_data)
        MIX_DestroyAudio(ctx.sfx.bg_music_data);
    for (int i = 0; i < 4; i++)
        if (ctx.sfx.beat[i])
            MIX_DestroyAudio(ctx.sfx.beat[i]);
    if (ctx.mixer)
        MIX_DestroyMixer(ctx.mixer);

    SDL_DestroyTexture(ctx.tex.player);
    SDL_DestroyTexture(ctx.tex.bullet);
    SDL_DestroyTexture(ctx.tex.ufo);
    SDL_DestroyTexture(ctx.tex.bg_menu);
    SDL_DestroyTexture(ctx.tex.bg_menu_1);
    SDL_DestroyTexture(ctx.tex.bg_game);
    SDL_DestroyTexture(ctx.tex.heart_full);
    SDL_DestroyTexture(ctx.tex.heart_empty);
    SDL_DestroyTexture(ctx.tex.blur);
    SDL_DestroyTexture(ctx.tex.expl_enemy);
    SDL_DestroyTexture(ctx.tex.expl_ufo);
    SDL_DestroyTexture(ctx.tex.expl_player[0]);
    SDL_DestroyTexture(ctx.tex.expl_player[1]);

    for (int t = 0; t < 3; t++)
        for (int f = 0; f < 2; f++)
            SDL_DestroyTexture(ctx.tex.enemies[t][f]);
    for (int i = 0; i < 4; i++)
    {
        SDL_DestroyTexture(ctx.tex.missiles[i]);
        SDL_DestroyTexture(ctx.tex.projectiles[i]);
    }
    for (int i = 0; i < 10; i++)
        SDL_DestroyTexture(ctx.tex.shields[i]);

    if (ctx.font)
        TTF_CloseFont(ctx.font);
    if (ctx.font_title)
        TTF_CloseFont(ctx.font_title);
    if (ctx.renderer)
        SDL_DestroyRenderer(ctx.renderer);
    if (ctx.window)
        SDL_DestroyWindow(ctx.window);
    TTF_Quit();
    SDL_Quit();
}

/**
 * @brief Effectue le rendu complet d'une frame.
 *
 * Dessine l'écran approprié selon l'état du jeu :
 * - STATE_PLAYING : monde de jeu + HUD
 * - STATE_MENU : menu principal avec options
 * - STATE_PAUSED : jeu en arrière-plan + menu pause
 * - STATE_GAME_OVER : écran de fin avec score
 * - STATE_CONFIRM_QUIT : popup de confirmation
 * - STATE_SAVE_* : interfaces de sauvegarde
 * - STATE_LOAD_MENU : menu de chargement
 * - STATE_TUTORIAL : écran d'aide
 *
 * @param model Le modèle de jeu contenant l'état à afficher.
 */
static void sdl_render(const GameModel *model)
{
    update_audio_state(model, (GameModel *)model);
    SDL_SetRenderDrawColor(ctx.renderer, 0, 0, 0, 255);
    SDL_RenderClear(ctx.renderer);

    if (model->state == STATE_PLAYING)
    {
        draw_game_world(model);
        draw_hud(model);
    }
    else if (model->state == STATE_MENU)
    {
        SDL_RenderTexture(ctx.renderer, ctx.tex.bg_menu, NULL, NULL);
        draw_text_centered("SPACE INVADERS", WIN_HEIGHT / 4, COL_GREEN, ctx.font_title);
        const char *opts[] = {"JOUER", "TUTORIEL", "CHARGER", "VOLUME", "QUITTER"};
        for (int i = 0; i < 5; i++)
        {
            SDL_Color c = (i == model->menu_selection) ? COL_YELLOW : COL_GRAY;
            char buf[64];
            if (i == 3)
            {
                if (model->is_muted)
                    snprintf(buf, 64, "VOLUME: [MUTE]");
                else
                {
                    char b[11] = {0};
                    int n = model->volume / 10;
                    for (int k = 0; k < 10; k++)
                        b[k] = (k < n) ? '|' : '-';
                    snprintf(buf, 64, "VOLUME: [%s] %d%%", b, model->volume);
                }
            }
            else
                snprintf(buf, 64, (i == model->menu_selection) ? "> %s <" : "%s", opts[i]);
            draw_text_centered(buf, WIN_HEIGHT / 2 + i * 50, c, ctx.font);
        }
    }
    else if (model->state == STATE_PAUSED)
    {
        draw_game_world(model);
        draw_overlay(180);
        draw_text_centered("PAUSE", WIN_HEIGHT / 4, COL_WHITE, ctx.font_title);
        const char *opts[] = {"REPRENDRE", "VOLUME", "SAUVEGARDER ET QUITTER", "QUITTER SANS SAUVEGARDER"};
        for (int i = 0; i < 4; i++)
        {
            SDL_Color c = (i == model->menu_selection) ? COL_YELLOW : COL_GRAY;
            char buf[128];
            if (i == 1)
                snprintf(buf, 64, model->is_muted ? "SON: OFF" : "SON: < %d%% >", model->volume);
            else
                snprintf(buf, 64, (i == model->menu_selection) ? "> %s <" : "%s", opts[i]);
            draw_text_centered(buf, WIN_HEIGHT / 2 - 50 + i * 60, c, ctx.font);
        }
    }
    else if (model->state == STATE_GAME_OVER)
    {
        if (ctx.tex.bg_menu_1)
            SDL_RenderTexture(ctx.renderer, ctx.tex.bg_menu_1, NULL, NULL);
        draw_overlay(200);
        draw_text_centered("GAME OVER", WIN_HEIGHT / 4, COL_RED, ctx.font_title);
        char s[32];
        snprintf(s, 32, "Score Final: %d", model->score);
        draw_text_centered(s, WIN_HEIGHT / 2 - 50, COL_WHITE, ctx.font);
        const char *opts[] = {"SAUVEGARDER", "REJOUER", "QUITTER"};
        for (int i = 0; i < 3; i++)
        {
            SDL_Color c = (i == model->menu_selection) ? COL_WHITE : COL_GRAY;
            char b[64];
            snprintf(b, 64, (i == model->menu_selection) ? "> %s <" : "%s", opts[i]);
            draw_text_centered(b, WIN_HEIGHT / 2 + 30 + i * 60, c, ctx.font);
        }
    }
    else if (model->state == STATE_CONFIRM_QUIT)
    {
        if (model->previous_state == STATE_PLAYING || model->previous_state == STATE_PAUSED)
        {
            draw_game_world(model);
            draw_overlay(230);
        }
        else
        {
            if (ctx.tex.bg_menu_1)
                SDL_RenderTexture(ctx.renderer, ctx.tex.bg_menu_1, NULL, NULL);
            draw_overlay(200);
        }
        draw_text_centered("ATTENTION !", WIN_HEIGHT / 3, COL_RED, ctx.font_title);
        if (model->previous_state == STATE_PLAYING || model->previous_state == STATE_PAUSED)
            draw_text_centered("Progression non sauvegardee !", WIN_HEIGHT / 3 + 60, COL_WHITE, ctx.font);
        else
            draw_text_centered("Voulez-vous quitter le jeu ?", WIN_HEIGHT / 3 + 60, COL_WHITE, ctx.font);
        draw_text_centered("Confirmer ?", WIN_HEIGHT / 3 + 90, COL_GRAY, ctx.font);
        const char *opts[] = {"OUI, QUITTER", "NON, RETOUR", "SAUVEGARDER ET QUITTER"};
        for (int i = 0; i < 3; i++)
        {
            SDL_Color c = (i == model->menu_selection) ? COL_YELLOW : COL_GRAY;
            char b[64];
            snprintf(b, 64, (i == model->menu_selection) ? "> %s <" : "%s", opts[i]);
            draw_text_centered(b, WIN_HEIGHT / 2 + 50 + i * 60, c, ctx.font);
        }
    }
    else if (model->state == STATE_SAVE_SELECT || model->state == STATE_LOAD_MENU || model->state == STATE_SAVE_INPUT || model->state == STATE_OVERWRITE_CONFIRM)
    {
        if (ctx.tex.bg_menu_1)
            SDL_RenderTexture(ctx.renderer, ctx.tex.bg_menu_1, NULL, NULL);
        draw_overlay(200);
        if (model->state == STATE_SAVE_SELECT)
        {
            draw_text_centered("CHOISIR L'EMPLACEMENT", 80, COL_YELLOW, ctx.font_title);
            draw_text_centered((model->menu_selection == 0) ? "> CREER NOUVELLE <" : " CREER NOUVELLE ", 180, (model->menu_selection == 0) ? COL_GREEN : COL_GRAY, ctx.font);
            for (int i = 0; i < model->save_file_count; i++)
            {
                char b[256];
                snprintf(b, 256, (i + 1 == model->menu_selection) ? "> %s <" : "%s", model->save_files[i]);
                draw_text_centered(b, 230 + i * 45, (i + 1 == model->menu_selection) ? COL_WHITE : COL_GRAY, ctx.font);
            }
        }
        else if (model->state == STATE_LOAD_MENU)
        {
            draw_text_centered("CHARGER UNE PARTIE", 100, COL_GREEN, ctx.font_title);
            if (model->save_file_count == 0)
                draw_text_centered("AUCUNE SAUVEGARDE TROUVE", WIN_HEIGHT / 2, COL_RED, ctx.font);
            for (int i = 0; i < model->save_file_count; i++)
            {
                char b[256];
                snprintf(b, 256, (i == model->menu_selection) ? "> %s <" : "%s", model->save_files[i]);
                draw_text_centered(b, 200 + i * 40, (i == model->menu_selection) ? COL_WHITE : COL_GRAY, ctx.font);
            }
        }
        else if (model->state == STATE_SAVE_INPUT)
        {
            draw_text_centered("NOM DE LA SAUVEGARDE :", WIN_HEIGHT / 2 + 20, COL_YELLOW, ctx.font_title);
            char b[128];
            snprintf(b, 128, "%s_", model->input_buffer);
            draw_text_centered(b, WIN_HEIGHT / 2 + 100, COL_WHITE, ctx.font);
            draw_text_centered("(Entree: Valider)", WIN_HEIGHT / 2 + 150, COL_GRAY, ctx.font);
        }
        else
        {
            SDL_SetRenderDrawColor(ctx.renderer, 0, 0, 0, 200);
            SDL_FRect overlay = {0, 0, WIN_WIDTH, WIN_HEIGHT};
            SDL_RenderFillRect(ctx.renderer, &overlay);

            draw_text_centered("CE FICHIER EXISTE DEJA !", WIN_HEIGHT / 2 - 100, (SDL_Color){255, 165, 0, 255}, ctx.font);

            char buf[128];
            snprintf(buf, sizeof(buf), "Fichier : '%s.dat'", model->input_buffer);
            draw_text_centered(buf, WIN_HEIGHT / 2 - 50, COL_WHITE, ctx.font);

            SDL_Color col0 = (model->menu_selection == 0) ? (SDL_Color){255, 0, 0, 255} : (SDL_Color){128, 128, 128, 255};
            const char *txt0 = (model->menu_selection == 0) ? "> ECRASER L'ANCIEN <" : "  ECRASER L'ANCIEN  ";
            draw_text_centered(txt0, WIN_HEIGHT / 2 + 30, col0, ctx.font);

            SDL_Color col1 = (model->menu_selection == 1) ? (SDL_Color){0, 255, 0, 255} : (SDL_Color){128, 128, 128, 255};
            const char *txt1 = (model->menu_selection == 1) ? "> CREER UNE COPIE (1..) <" : "  CREER UNE COPIE (1..)  ";
            draw_text_centered(txt1, WIN_HEIGHT / 2 + 80, col1, ctx.font);
        }
    }
    else if (model->state == STATE_TUTORIAL)
    {
        if (ctx.tex.bg_menu_1)
            SDL_RenderTexture(ctx.renderer, ctx.tex.bg_menu_1, NULL, NULL);
        draw_overlay(200);
        draw_text_centered("COMMENT JOUER ?", 50, (SDL_Color){0, 255, 255, 255}, ctx.font_title);
        draw_text_centered("Fleches : Se Deplacer", 130, COL_WHITE, ctx.font);
        draw_text_centered("Espace : Tirer", 180, COL_WHITE, ctx.font);
        struct
        {
            SDL_Texture *t;
            const char *d;
            SDL_Color c;
        } tuts[] = {{ctx.tex.enemies[0][0], "= 10 PTS", {0, 255, 255, 255}}, {ctx.tex.enemies[1][0], "= 20 PTS", {255, 165, 0, 255}}, {ctx.tex.enemies[2][0], "= 30 PTS", {255, 50, 50, 255}}, {ctx.tex.ufo, "= 100 PTS + ???", {255, 0, 255, 255}}};
        for (int i = 0; i < 4; i++)
        {
            SDL_SetTextureColorMod(tuts[i].t, tuts[i].c.r, tuts[i].c.g, tuts[i].c.b);
            SDL_FRect r = {WIN_WIDTH / 2 - 80, 325 + i * 60, 40, (i == 3) ? 20 : 40};
            SDL_RenderTexture(ctx.renderer, tuts[i].t, NULL, &r);
            SDL_SetTextureColorMod(tuts[i].t, 255, 255, 255);
            char b[32];
            snprintf(b, 32, "%s", tuts[i].d);
            draw_text(b, WIN_WIDTH / 2 - 20, 320 + i * 60, tuts[i].c);
        }
        draw_text_centered("(Appuyez sur Entree pour retour)", WIN_HEIGHT - 50, COL_GRAY, ctx.font);
    }
    else if (model->state == STATE_SAVE_SUCCESS)
    {
        SDL_SetRenderDrawColor(ctx.renderer, 0, 0, 0, 255);
        SDL_RenderClear(ctx.renderer);
        draw_text_centered("SAUVEGARDE REUSSIE !", WIN_HEIGHT / 2 - 40, COL_GREEN, ctx.font_title);
        draw_text_centered("Le jeu va se fermer...", WIN_HEIGHT / 2 + 40, COL_WHITE, ctx.font);
    }
    SDL_RenderPresent(ctx.renderer);
}

/**
 * @brief Récupère et traduit les événements SDL en commandes de jeu.
 *
 * Gère plusieurs modes :
 * - Mode saisie (STATE_SAVE_INPUT) : capture les caractères alphanumériques
 * - Mode jeu : détection continue des touches pour le mouvement fluide
 * - Mode menu : événements ponctuels pour la navigation
 *
 * Supporte également le plein écran (F11) et la fermeture de fenêtre.
 *
 * @param model Le modèle de jeu (pour connaître l'état et le buffer de saisie).
 * @return La commande GameCommand correspondant à l'entrée utilisateur.
 */
static GameCommand sdl_get_input(GameModel *model)
{
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_EVENT_QUIT)
            return CMD_EXIT;
        if (model->state == STATE_SAVE_INPUT && e.type == SDL_EVENT_KEY_DOWN)
        {
            SDL_Keycode k = e.key.key;
            if (k == SDLK_RETURN || k == SDLK_KP_ENTER)
                return CMD_RETURN;
            if (k == SDLK_ESCAPE)
                return CMD_PAUSE;
            if (k == SDLK_BACKSPACE)
            {
                int l = strlen(model->input_buffer);
                if (l > 0)
                    model->input_buffer[l - 1] = '\0';
            }
            else if (strlen(model->input_buffer) < MAX_FILENAME_LEN - 1)
            {
                char c = 0;
                if (k >= SDLK_A && k <= SDLK_Z)
                    c = (char)k;
                if (k >= SDLK_0 && k <= SDLK_9)
                    c = (char)k;
                if (k == SDLK_MINUS)
                    c = '-';
                if (k == SDLK_SPACE)
                    c = '_';
                if (c)
                {
                    int l = strlen(model->input_buffer);
                    model->input_buffer[l] = c;
                    model->input_buffer[l + 1] = '\0';
                }
            }
            return CMD_NONE;
        }
        if (e.type == SDL_EVENT_KEY_DOWN)
        {
            switch (e.key.key)
            {
            case SDLK_ESCAPE:
                return CMD_PAUSE;
            case SDLK_P:
                return CMD_PAUSE;
            case SDLK_Q:
                return CMD_EXIT;
                break;
            case SDLK_UP:
                return CMD_UP;
            case SDLK_DOWN:
                return CMD_DOWN;
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                return CMD_RETURN;
            case SDLK_SPACE:
                return CMD_SHOOT;
            case SDLK_LEFT:
                return (model->state == STATE_PLAYING) ? CMD_MOVE_LEFT : CMD_LEFT;
            case SDLK_RIGHT:
                return (model->state == STATE_PLAYING) ? CMD_MOVE_RIGHT : CMD_RIGHT;
            case SDLK_F11:
                SDL_SetWindowFullscreen(ctx.window, !(SDL_GetWindowFlags(ctx.window) & SDL_WINDOW_FULLSCREEN));
                break;
            }
        }
    }
    if (model->state == STATE_PLAYING)
    {
        const bool *s = SDL_GetKeyboardState(NULL);
        if (s[SDL_SCANCODE_LEFT])
            return CMD_MOVE_LEFT;
        if (s[SDL_SCANCODE_RIGHT])
            return CMD_MOVE_RIGHT;
        if (s[SDL_SCANCODE_SPACE])
            return CMD_SHOOT;
    }
    return CMD_NONE;
}

const ViewInterface view_sdl = {.init = sdl_init, .close = sdl_close, .render = sdl_render, .get_input = sdl_get_input};