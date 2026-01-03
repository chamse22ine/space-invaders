##
# @file Makefile
# @brief Script de compilation pour Space Invaders MVC.
#
# @par Cibles principales :
# - all          : Compile le projet
# - run-sdl      : Lance en graphique
# - run-ncurses  : Lance en terminal
# - valgrind-sdl      : Lance SDL avec Valgrind
# - valgrind-ncurses  : Lance ncurses avec Valgrind
# - clean        : Supprime les fichiers compilés (.o, exe)
# - mrproper     : clean + lance clean.sh (grand nettoyage)
# - install-deps : Installe les librairies Linux nécessaires (Ubuntu/Debian)
##

# --- SILENCE MAKE ---
MAKEFLAGS += --no-print-directory

# ============================================================================
#                              CONFIGURATION
# ============================================================================

CC = gcc
EXT_DIR = 3rdParty
TARGET = space_invaders

# ============================================================================
#                           FLAGS DE COMPILATION
# ============================================================================

CFLAGS = -std=c99 -Wall -Wextra -g -Iinclude \
         -I$(EXT_DIR)/SDL3/include \
         -I$(EXT_DIR)/SDL3_image/include \
         -I$(EXT_DIR)/SDL3_ttf/include \
         -I$(EXT_DIR)/SDL3_mixer/include

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
#                         CONFIGURATION VALGRIND
# ============================================================================

VALGRIND = valgrind
VALGRIND_FLAGS = --leak-check=full \
                 --show-leak-kinds=all \
                 --track-origins=yes \
                 --verbose \
                 --log-file=valgrind-output.txt

# Suppressions pour SDL (optionnel, réduit le bruit des fuites internes SDL)
VALGRIND_SUPPRESS = --suppressions=valgrind.supp

# ============================================================================
#                              FICHIERS SOURCE
# ============================================================================

SRC_DIR = src
OBJ_DIR = build
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# ============================================================================
#                           RÈGLES DE CONSTRUCTION
# ============================================================================

all: dirs $(TARGET)

# Compile et lance le script de post-traitement (build.sh)
build:
	@chmod +x build.sh
	@./build.sh

run-ncurses: all
	@./$(TARGET) ncurses

run-sdl: all
	@./$(TARGET) sdl

$(TARGET): $(OBJS)
	@$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	@$(CC) $(CFLAGS) -c $< -o $@

dirs:
	@mkdir -p $(OBJ_DIR)

# ============================================================================
#                            TESTS VALGRIND
# ============================================================================

## @brief Lance la vue ncurses avec Valgrind pour détecter les fuites mémoire
valgrind-ncurses: all
	@echo "--- Lancement de Valgrind (ncurses) ---"
	@$(VALGRIND) $(VALGRIND_FLAGS) ./$(TARGET) ncurses
	@echo "--- Rapport sauvegardé dans valgrind-output.txt ---"

## @brief Lance la vue SDL avec Valgrind pour détecter les fuites mémoire
valgrind-sdl: all
	@echo "--- Lancement de Valgrind (SDL) ---"
	@$(VALGRIND) $(VALGRIND_FLAGS) ./$(TARGET) sdl
	@echo "--- Rapport sauvegardé dans valgrind-output.txt ---"


# ============================================================================
#                              NETTOYAGE
# ============================================================================

# Nettoyage standard (juste les binaires)
clean:
	@rm -rf $(OBJ_DIR) $(TARGET)
	@echo "Fichiers de build supprimés."

# Nettoyage des rapports Valgrind
clean-valgrind:
	@rm -f valgrind-output.txt
	@echo "Rapports Valgrind supprimés."

# Grand nettoyage (binaires + ce que fait ton script clean.sh)
mrproper: clean clean-valgrind
	@chmod +x clean.sh
	@./clean.sh
	@echo "Nettoyage complet (mrproper) effectué."

# ============================================================================
#                        GESTION DES DÉPENDANCES (Linux)
# ============================================================================

## @brief Installe les dépendances système requises (Ubuntu/Debian/Mint).
install-deps:
	@echo "--- Installation des librairies requises (nécessite sudo) ---"
	@sudo apt update
	@sudo apt install -y \
	build-essential git make cmake pkg-config \
	libasound2-dev libpulse-dev \
	libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxinerama-dev libxss-dev libxxf86vm-dev \
	libwayland-dev libxkbcommon-dev libgl1-mesa-dev libegl1-mesa-dev \
	libdbus-1-dev libudev-dev libibus-1.0-dev \
	libfreetype6-dev libharfbuzz-dev \
	libjpeg-dev libpng-dev libtiff-dev libwebp-dev \
	libflac-dev libvorbis-dev libopus-dev libmodplug-dev libmpg123-dev libsndfile1-dev \
	libxtst-dev libncurses-dev libncurses5-dev libncursesw5-dev \
	valgrind
	@echo "--- Toutes les dépendances sont installées ! ---"

.PHONY: all clean clean-valgrind mrproper run-ncurses run-sdl dirs build install-deps \
        valgrind-ncurses valgrind-sdl
