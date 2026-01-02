##
# @file Makefile
# @brief Script de compilation pour Space Invaders MVC.
#
# Ce Makefile compile le projet Space Invaders avec support SDL3 et ncurses.
# Les bibliothèques SDL3 sont liées depuis le dossier 3rdParty/.
#
# @par Cibles principales :
# - all        : Compile le projet (par défaut)
# - run-sdl    : Compile et lance en mode graphique SDL3
# - run-ncurses: Compile et lance en mode terminal
# - clean      : Supprime les fichiers compilés
#
# @par Utilisation :
# @code
#   make            # Compile le projet
#   make run-sdl    # Lance en mode SDL3
#   make clean      # Nettoie les fichiers générés
# @endcode
##

# ============================================================================
#                              CONFIGURATION
# ============================================================================

## @brief Compilateur C utilisé.
CC = gcc

## @brief Dossier contenant les bibliothèques externes (SDL3, etc.).
EXT_DIR = 3rdParty

## @brief Nom de l'exécutable généré.
TARGET = space_invaders

# ============================================================================
#                           FLAGS DE COMPILATION
# ============================================================================

## @brief Options de compilation.
# - C99 standard
# - Warnings activés (-Wall -Wextra)
# - Symboles de debug (-g)
# - Chemins d'inclusion pour les headers
CFLAGS = -std=c99 -Wall -Wextra -g -Iinclude \
         -I$(EXT_DIR)/SDL3/include \
         -I$(EXT_DIR)/SDL3_image/include \
         -I$(EXT_DIR)/SDL3_ttf/include \
         -I$(EXT_DIR)/SDL3_mixer/include

## @brief Options d'édition des liens.
# - Bibliothèques : math, ncurses, SDL3 et extensions
# - RPATH pour trouver les .so à l'exécution
LDFLAGS = -lm -lncurses \
          -L$(EXT_DIR)/SDL3_image/build -lSDL3_image \
          -L$(EXT_DIR)/SDL3_ttf/build -lSDL3_ttf \
          -L$(EXT_DIR)/SDL3_mixer/build -lSDL3_mixer \
          -L$(EXT_DIR)/SDL3/build -lSDL3 \
          -Wl,-rpath,$(abspath $(EXT_DIR)/SDL3_image/build) \
          -Wl,-rpath,$(abspath $(EXT_DIR)/SDL3_ttf/build) \
          -Wl,-rpath,$(abspath $(EXT_DIR)/SDL3_mixer/build) \
          -Wl,-rpath,$(abspath $(EXT_DIR)/SDL3/build)

# ============================================================================
#                              FICHIERS SOURCE
# ============================================================================

## @brief Dossier des fichiers sources (.c).
SRC_DIR = src

## @brief Dossier des fichiers objets (.o).
OBJ_DIR = build

## @brief Liste automatique de tous les fichiers sources.
SRCS = $(wildcard $(SRC_DIR)/*.c)

## @brief Liste des fichiers objets correspondants.
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# ============================================================================
#                           RÈGLES DE CONSTRUCTION
# ============================================================================

## @brief Cible par défaut : compile le projet.
all: dirs $(TARGET)

## @brief Compile et lance le jeu en mode terminal (ncurses).
run-ncurses: all
	@./$(TARGET) ncurses

## @brief Compile et lance le jeu en mode graphique (SDL3).
run-sdl: all
	@./$(TARGET) sdl

## @brief Édition des liens : génère l'exécutable final.
$(TARGET): $(OBJS)
	@$(CC) $(OBJS) -o $@ $(LDFLAGS)

## @brief Compilation des fichiers sources en objets.
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	@$(CC) $(CFLAGS) -c $< -o $@

## @brief Crée le dossier de build si nécessaire.
dirs:
	@mkdir -p $(OBJ_DIR)

## @brief Supprime tous les fichiers générés.
clean:
	@rm -rf $(OBJ_DIR) $(TARGET)


.PHONY: all clean run-ncurses run-sdl dirs