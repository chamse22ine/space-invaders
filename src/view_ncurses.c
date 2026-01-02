/**
 * @file view_ncurses.c
 * @brief Implémentation de la vue en mode terminal avec ncurses.
 *
 * Ce fichier gère l'affichage du jeu Space Invaders dans un terminal
 * en utilisant la bibliothèque ncurses pour le rendu ASCII et les couleurs.
 */

#include "view_ncurses.h"
#include <ncurses.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

// ============================================================================
// ASSETS VISUELS
// ============================================================================
static const char *SPRITE_PLAYER = "_^_";
static const char *SPRITE_PLAYER_HIT = "*#*";
static const char *SPRITE_A1 = "/^\\";
static const char *SPRITE_A2 = "/M\\";
static const char *SPRITE_A3 = "/o\\";
static const char *SPRITE_UFO = "<=O=>";

// Caractères pour les boucliers selon les dégâts
static const char SHIELD_FULL = '#';
static const char SHIELD_MED = '+';
static const char SHIELD_LOW = '.';

static const char CHAR_BULLET = '|';

// ============================================================================
// INITIALISATION
// ============================================================================

/**
 * @brief Initialise la bibliothèque ncurses et configure le terminal.
 *
 * Configure le mode cbreak, désactive l'écho, active le clavier étendu,
 * masque le curseur et active le mode non-bloquant. Initialise également
 * les paires de couleurs si le terminal les supporte.
 *
 * @return true si l'initialisation a réussi, false sinon.
 */
static bool ncurses_init(void)
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    nodelay(stdscr, TRUE);

    if (has_colors())
    {
        start_color();
        use_default_colors();

        init_pair(1, COLOR_GREEN, -1);          // Joueur
        init_pair(2, COLOR_RED, -1);            // UFO / Danger
        init_pair(3, COLOR_YELLOW, -1);         // Balles
        init_pair(4, COLOR_BLUE, -1);           // Cadres
        init_pair(5, COLOR_CYAN, -1);           // Bouclier
        init_pair(6, COLOR_MAGENTA, -1);        // Ennemi
        init_pair(7, COLOR_BLACK, COLOR_WHITE); // Sélection (Reste sur fond blanc pour lisibilité)
    }
    return true;
}

/**
 * @brief Ferme proprement la session ncurses.
 *
 * Restaure le terminal dans son état initial avant l'exécution du programme.
 */
static void ncurses_close(void)
{
    endwin();
}

/**
 * @brief Affiche un texte centré horizontalement à une position verticale relative.
 *
 * Calcule automatiquement la position X pour centrer le texte à l'écran
 * et applique la couleur spécifiée.
 *
 * @param y_offset Décalage vertical par rapport au centre de l'écran.
 * @param text Le texte à afficher.
 * @param pair_color L'index de la paire de couleurs ncurses à utiliser.
 */
static void draw_centered(int y_offset, const char *text, int pair_color)
{
    int h, w;
    getmaxyx(stdscr, h, w);
    int len = strlen(text);
    int x = (w / 2) - (len / 2);
    int y = (h / 2) + y_offset;
    if (x < 0)
        x = 0;
    if (y >= 0 && y < h)
    {
        attron(COLOR_PAIR(pair_color));
        mvprintw(y, x, "%s", text);
        attroff(COLOR_PAIR(pair_color));
    }
}

// ============================================================================
// RENDU (RENDER)
// ============================================================================

/**
 * @brief Effectue le rendu complet du jeu dans le terminal.
 *
 * Dessine tous les éléments du jeu selon l'état actuel :
 * - Menu principal avec options
 * - Menu de chargement des sauvegardes
 * - Écran tutoriel avec tableau des points
 * - Zone de jeu avec joueur, ennemis, UFO, boucliers et balles
 * - Menus popup (pause, game over, sauvegarde, confirmation)
 *
 * Le rendu est adapté dynamiquement à la taille du terminal avec un système
 * de mise à l'échelle.
 *
 * @param model Le modèle de jeu contenant l'état actuel à afficher.
 */
static void ncurses_render(const GameModel *model)
{
    erase();
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    if (rows < 20 || cols < 50)
    {
        mvprintw(0, 0, "FENETRE TROP PETITE !");
        refresh();
        return;
    }

    float scale_x = (float)(cols - 2) / (float)GAME_WIDTH;
    float scale_y = (float)(rows - 2) / (float)GAME_HEIGHT;

    attron(COLOR_PAIR(4));
    box(stdscr, 0, 0);
    attroff(COLOR_PAIR(4));

    // --- A. MENU PRINCIPAL ---
    if (model->state == STATE_MENU)
    {
        draw_centered(-6, "=== SPACE INVADERS ===", 1);

        const char *options[] = {"JOUER", "TUTORIEL", "CHARGER", "VOLUME", "QUITTER"};

        for (int i = 0; i < 5; i++)
        {
            char buf[64];
            if (i == 3)
                snprintf(buf, 64, "VOLUME (N/A)");
            else
                strcpy(buf, options[i]);

            int col = (i == model->menu_selection) ? 7 : 0;
            if (i == model->menu_selection)
                attron(COLOR_PAIR(7));
            draw_centered(-2 + (i * 2), buf, col);
            if (i == model->menu_selection)
                attroff(COLOR_PAIR(7));
        }
        refresh();
        return;
    }

    // --- B. CHARGEMENT / TUTO ---
    if (model->state == STATE_LOAD_MENU)
    {
        draw_centered(-8, "=== CHARGER ===", 5);
        if (model->save_file_count == 0)
            draw_centered(0, "Aucune sauvegarde trouvé.", 2);
        else
        {
            for (int i = 0; i < model->save_file_count; i++)
            {
                int col = (i == model->menu_selection) ? 7 : 0;
                if (i == model->menu_selection)
                    attron(COLOR_PAIR(7));
                draw_centered(-4 + i, model->save_files[i], col);
                if (i == model->menu_selection)
                    attroff(COLOR_PAIR(7));
            }
        }
        refresh();
        return;
    }

    if (model->state == STATE_TUTORIAL)
    {
        draw_centered(-9, "=== TABLEAU DES POINTS ===", 6);

        int cx = cols / 2;
        int cy = rows / 2;

        attron(COLOR_PAIR(2));
        mvprintw(cy - 5, cx - 12, "%s", SPRITE_UFO);
        attroff(COLOR_PAIR(2));
        attron(A_BOLD);
        mvprintw(cy - 5, cx - 4, "= 100 PTS + ???");
        attroff(A_BOLD);

        attron(COLOR_PAIR(6));
        mvprintw(cy - 3, cx - 12, " %s ", SPRITE_A1);
        attroff(COLOR_PAIR(6));
        mvprintw(cy - 3, cx - 4, "= 30 PTS");

        attron(COLOR_PAIR(5));
        mvprintw(cy - 1, cx - 12, " %s ", SPRITE_A2);
        attroff(COLOR_PAIR(5));
        mvprintw(cy - 1, cx - 4, "= 20 PTS");

        attron(COLOR_PAIR(1));
        mvprintw(cy + 1, cx - 12, " %s ", SPRITE_A3);
        attroff(COLOR_PAIR(1));
        mvprintw(cy + 1, cx - 4, "= 10 PTS");

        draw_centered(5, "FLECHES : Deplacer", 7);
        draw_centered(6, "ESPACE  : Tirer", 7);

        draw_centered(9, "[ ENTREE POUR RETOUR ]", 4);

        refresh();
        return;
    }

    // --- C. JEU ---
    // HUD
    attron(A_BOLD);
    mvprintw(1, 2, "SCORE: %d", model->score);
    mvprintw(1, cols - 15, "VIES: %d", model->lives);
    mvprintw(1, cols / 2 - 4, "LVL: %d", model->level);
    attroff(A_BOLD);

    // 1. JOUEUR (AVEC EFFET EXPLOSION)
    if (model->player.active)
    {
        int px = (int)(model->player.x * scale_x) + 1;
        int py = (int)(model->player.y * scale_y) + 1;
        if (px < cols - 3 && py < rows - 1)
        {
            if (model->hit_timer > 0)
            {
                if ((int)(model->hit_timer * 5) % 2 == 0)
                {
                    attron(COLOR_PAIR(2));
                    mvprintw(py, px, "%s", SPRITE_PLAYER_HIT);
                    attroff(COLOR_PAIR(2));
                }
            }
            else
            {
                attron(COLOR_PAIR(1));
                mvprintw(py, px, "%s", SPRITE_PLAYER);
                attroff(COLOR_PAIR(1));
            }
        }
    }

    // 2. ENNEMIS
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        const Entity *e = &model->enemies[i];
        if (e->active)
        {
            int ex = (int)(e->x * scale_x) + 1;
            int ey = (int)(e->y * scale_y) + 1;
            int c = 2;
            const char *s = SPRITE_A3;
            if (e->type == ENTITY_ENEMY_TYPE_3)
            {
                c = 6;
                s = SPRITE_A1;
            }
            else if (e->type == ENTITY_ENEMY_TYPE_2)
            {
                c = 5;
                s = SPRITE_A2;
            }

            if (ex > 0 && ex < cols - 3 && ey > 0 && ey < rows - 1)
            {
                attron(COLOR_PAIR(c));
                mvprintw(ey, ex, "%s", (e->exploding ? "*" : s));
                attroff(COLOR_PAIR(c));
            }
        }
    }

    // 3. UFO
    if (model->ufo.active)
    {
        int ux = (int)(model->ufo.x * scale_x) + 1;
        int uy = (int)(model->ufo.y * scale_y) + 1;
        if (ux > -5 && ux < cols)
        {
            attron(COLOR_PAIR(2) | A_BOLD);
            mvprintw(uy, (ux < 1 ? 1 : ux), "%s", (model->ufo.exploding ? "BOOM" : SPRITE_UFO));
            attroff(COLOR_PAIR(2) | A_BOLD);
        }
    }

    // 4. BOUCLIERS (AVEC DÉGÂTS PROGRESSIFS)
    attron(COLOR_PAIR(5));
    for (int i = 0; i < MAX_SHIELDS; i++)
    {
        if (model->shields[i].active)
        {
            int sx = (int)(model->shields[i].x * scale_x) + 1;
            int sy = (int)(model->shields[i].y * scale_y) + 1;
            int sw = (int)(model->shields[i].width * scale_x);
            if (sw < 1)
                sw = 1;
            int sh = (int)(model->shields[i].height * scale_y);
            if (sh < 1)
                sh = 1;

            char c = SHIELD_FULL;
            if (model->shields[i].health <= 3)
                c = SHIELD_LOW;
            else if (model->shields[i].health <= 6)
                c = SHIELD_MED;

            for (int y = 0; y < sh; y++)
                for (int x = 0; x < sw; x++)
                    if (sx + x < cols - 1 && sy + y < rows - 1)
                        mvaddch(sy + y, sx + x, c);
        }
    }
    attroff(COLOR_PAIR(5));

    // 5. BALLES
    attron(COLOR_PAIR(3));
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (model->bullets[i].active)
        {
            int bx = (int)((model->bullets[i].x - 0.8f) * scale_x) + 1;
            int by = (int)(model->bullets[i].y * scale_y) + 1;
            if (bx > 0 && bx < cols - 1 && by > 0 && by < rows - 1)
                mvaddch(by, bx, CHAR_BULLET);
        }
    }
    attroff(COLOR_PAIR(3));

    // --- MENUS POPUP ---

    // PAUSE
    if (model->state == STATE_PAUSED)
    {
        draw_centered(-4, "=== PAUSE ===", 7);
        const char *o[] = {"REPRENDRE", "VOLUME (N/A)", "SAUVEGARDER", "QUITTER"};
        for (int i = 0; i < 4; i++)
        {
            int c = (i == model->menu_selection) ? 7 : 0;
            if (i == model->menu_selection)
                attron(COLOR_PAIR(7));
            draw_centered(-1 + i, o[i], c);
            if (i == model->menu_selection)
                attroff(COLOR_PAIR(7));
        }
    }
    else if (model->state == STATE_GAME_OVER)
    {
        draw_centered(-3, "!!! GAME OVER !!!", 2);

        char sc[32];
        snprintf(sc, 32, "SCORE FINAL: %d", model->score);
        draw_centered(-1, sc, 1);

        const char *o[] = {"SAUVEGARDER SCORE", "REJOUER", "QUITTER"};
        for (int i = 0; i < 3; i++)
        {
            int c = (i == model->menu_selection) ? 7 : 0;
            if (i == model->menu_selection)
                attron(COLOR_PAIR(7));
            draw_centered(2 + (i * 2), o[i], c);
            if (i == model->menu_selection)
                attroff(COLOR_PAIR(7));
        }
    }
    else if (model->state == STATE_SAVE_SELECT)
    {
        draw_centered(-8, "=== CHOISIR EMPLACEMENT ===", 5);

        int col_new = (model->menu_selection == 0) ? 7 : 1;
        if (model->menu_selection == 0)
            attron(COLOR_PAIR(7));
        draw_centered(-4, "[ + ]  NOUVELLE SAUVEGARDE", col_new);
        if (model->menu_selection == 0)
            attroff(COLOR_PAIR(7));

        if (model->save_file_count == 0)
        {
            draw_centered(0, "(Aucun fichier existant)", 4);
        }
        else
        {
            for (int i = 0; i < model->save_file_count; i++)
            {
                int menu_index = i + 1;

                char buf[512];
                snprintf(buf, sizeof(buf), "FICHIER : %s", model->save_files[i]);

                int col = (model->menu_selection == menu_index) ? 7 : 0;
                if (model->menu_selection == menu_index)
                    attron(COLOR_PAIR(7));

                draw_centered(-2 + (i * 1), buf, col);

                if (model->menu_selection == menu_index)
                    attroff(COLOR_PAIR(7));
            }
        }

        draw_centered(rows / 2 - 2, "[ENTREE] Valider   [ECHAP] Retour", 4);
    }
    // SAUVEGARDE
    else if (model->state == STATE_SAVE_INPUT)
    {
        draw_centered(-2, "NOM DE SAUVEGARDE :", 5);
        char buf[64];
        snprintf(buf, 64, "[ %s_ ]", model->input_buffer);
        draw_centered(0, buf, 7);
        draw_centered(2, "(Lettres/Chiffres - ENTREE Valider)", 0);
    }
    else if (model->state == STATE_SAVE_SUCCESS)
    {
        draw_centered(0, "SAUVEGARDE REUSSIE !", 1);
    }
    // CONFIRMATION QUITTER (Classique Oui/Non)
    else if (model->state == STATE_CONFIRM_QUIT)
    {
        draw_centered(-2, "VOULEZ-VOUS QUITTER ?", 2);
        draw_centered(0, (model->menu_selection == 0 ? "> OUI <" : "  OUI  "), (model->menu_selection == 0 ? 7 : 0));
        draw_centered(1, (model->menu_selection == 1 ? "> NON <" : "  NON  "), (model->menu_selection == 1 ? 7 : 0));
    }

    // CONFIRMATION ECRASER (Nouveau style)
    else if (model->state == STATE_OVERWRITE_CONFIRM)
    {
        draw_centered(-4, "CE FICHIER EXISTE DEJA !", 3);

        char buf[64];
        snprintf(buf, 64, "'%s.dat'", model->input_buffer);
        draw_centered(-2, buf, 7);

        const char *opt0 = (model->menu_selection == 0) ? "> ECRASER <" : "  ECRASER  ";
        const char *opt1 = (model->menu_selection == 1) ? "> CREER COPIE (1..) <" : "  CREER COPIE (1..)  ";

        draw_centered(1, opt0, (model->menu_selection == 0 ? 2 : 0));
        draw_centered(3, opt1, (model->menu_selection == 1 ? 7 : 0));
    }
    refresh();
}

// ============================================================================
// GESTION INPUT
// ============================================================================

/**
 * @brief Récupère et traduit les entrées clavier en commandes de jeu.
 *
 * Gère deux modes de saisie :
 * - Mode normal : traduit les touches directionnelles, espace, entrée, etc.
 * - Mode saisie de texte (STATE_SAVE_INPUT) : capture les caractères alphanumériques
 *
 * Correspondances des touches :
 * - Flèches / ZQSD : Navigation et déplacement
 * - Espace : Tir
 * - Entrée : Validation
 * - P / Échap : Pause
 * - Backspace : Effacement (en mode saisie)
 *
 * @param model Le modèle de jeu (utilisé pour connaître l'état et le buffer de saisie).
 * @return La commande GameCommand correspondant à la touche pressée.
 */
static GameCommand ncurses_get_input(GameModel *model)
{
    int ch = getch();
    if (ch == KEY_RESIZE)
        return CMD_NONE;

    if (model->state == STATE_SAVE_INPUT)
    {
        if (ch == '\n' || ch == KEY_ENTER)
            return CMD_RETURN;
        if (ch == 27)
            return CMD_PAUSE;
        if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b')
            return CMD_BACKSPACE;
        if (isalnum(ch) || ch == '-' || ch == '_')
        {
            int len = strlen(model->input_buffer);
            if (len < 19)
            {
                model->input_buffer[len] = (char)ch;
                model->input_buffer[len + 1] = '\0';
            }
        }
        return CMD_NONE;
    }

    switch (ch)
    {
    case KEY_LEFT:
        return (model->state == STATE_PLAYING) ? CMD_MOVE_LEFT : CMD_LEFT;
    case 'q':
        return (model->state == STATE_PLAYING) ? CMD_MOVE_LEFT : CMD_LEFT;
    case KEY_RIGHT:
        return (model->state == STATE_PLAYING) ? CMD_MOVE_RIGHT : CMD_RIGHT;
    case 'd':
        return (model->state == STATE_PLAYING) ? CMD_MOVE_RIGHT : CMD_RIGHT;
    case KEY_UP:
        return CMD_UP;
    case 'z':
        return CMD_UP;
    case KEY_DOWN:
        return CMD_DOWN;
    case 's':
        return CMD_DOWN;
    case ' ':
        return CMD_SHOOT;
    case '\n':
        return CMD_RETURN;
    case KEY_ENTER:
        return CMD_RETURN;
    case 'p':
        return CMD_PAUSE;
    case 27:
        return CMD_PAUSE;
    default:
        return CMD_NONE;
    }
}

const ViewInterface view_ncurses = {
    .init = ncurses_init,
    .close = ncurses_close,
    .render = ncurses_render,
    .get_input = ncurses_get_input};