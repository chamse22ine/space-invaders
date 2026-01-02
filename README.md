# 🚀 Space Invaders MVC

> Implémentation moderne du classique Space Invaders en C avec architecture MVC et double interface (ncurses/SDL3)

![C](https://img.shields.io/badge/Language-C99-blue)
![ncurses](https://img.shields.io/badge/Graphics-ncurses-blue)
![SDL3](https://img.shields.io/badge/Graphics-SDL3-green)
![License](https://img.shields.io/badge/License-MIT-yellow)

---

## 📋 Table des matières

- [Aperçu](#-aperçu)
- [Description](#-description)
- [Caractéristiques](#-caractéristiques)
- [Architecture MVC](#️-architecture-mvc)
- [Prérequis](#-prérequis)
- [Installation](#-installation)
- [Compilation](#-compilation)
- [Lancement du jeu](#-lancement-du-jeu)
- [Commandes](#️-commandes)
- [Structure du projet](#-structure-du-projet)
- [Système de sauvegarde](#-système-de-sauvegarde)
- [Documentation](#-documentation)
- [Dépannage](#-dépannage)
- [Licence](#-licence)
- [Credits](#-credits)
- [Auteurs](#-auteurs-groupe-7)

---

### 📸 Aperçu

|            Interface SDL3 (Graphique)            |              Interface ncurses (Terminal)               |
| :----------------------------------------------: | :-----------------------------------------------------: |
| ![Mode SDL3](/assets/preview/SDL_Homescreen.png) | ![Mode ncurses](/assets/preview/Ncurses_Homescreen.png) |
| ![Mode SDL3](/assets/preview/SDL_Gamescreen.png) | ![Mode ncurses](/assets/preview/Ncurses_Gamescreen.png) |

---

## 🎯 Description

Space Invaders MVC est une réimplémentation fidèle du jeu d'arcade classique **Space Invaders** , développée en C avec une architecture **Modèle-Vue-Contrôleur** stricte.

Le projet propose **deux modes d'affichage interchangeables** :

- **SDL3** : Interface graphique moderne avec sprites, animations et effets sonores
- **ncurses** : Interface texte ASCII jouable directement dans le terminal

---

## ✨ Caractéristiques

### Gameplay

- ✅ **3 types d'ennemis** avec scores différents (10/20/30 points)
- ✅ **OVNI mystère** apparaissant aléatoirement (100 points + vie bonus)
- ✅ **4 boucliers destructibles** avec 10 états de dégradation
- ✅ **Système de vies** avec indicateur visuel (cœurs en sdl)
- ✅ **Progression par niveaux** avec augmentation de difficulté
- ✅ **Animations** : sprites alternés, explosions, tremblements d'écran
- ✅ **Son** : bruitages tir/explosion, musique de fond cyclique
- ✅ **Menu complet** : Nouveau jeu, Charger, Options, Quitter
- ✅ **Pause en jeu** avec options Sauvegarder/Quitter

### Technique

- 🎮 **Double interface** : SDL3 (graphique) et ncurses (terminal)
- 💾 **Système de sauvegarde** complet avec gestion de fichiers
- 🎵 **Audio SDL_mixer** : bruitages, musiques, boucles
- 🎨 **Sprites animés** : 4 frames pour projectiles, 2 pour ennemis
- ⚡ **Boucle de jeu fixe** à 60 FPS avec delta time (le delta time sert a rendre la vitesse du jeu indépendante de la vitesse du processeur.)
- 🔧 **Responsive** : Adaptation automatique à la taille de la fenêtre/terminal

---

## 🏗️ Architecture MVC

Le projet suit strictement le pattern **Modèle-Vue-Contrôleur** :

```plantext
┌─────────────────────────────────────────────┐
│              MAIN (main.c)                  │
│         Boucle de jeu principale            │
└────────┬─────────────┬──────────────────────┘
         │             │
    ┌────▼─────┐  ┌───▼──────┐
    │ MODÈLE   │  │   VUE    │
    │(model.c) │  │(view_*.c)│
    │          │  │          │
    │ Logique  │  │  Rendu   │
    │ Métier   │  │ Graphique│
    └────▲─────┘  └───▲──────┘
         │            │
    ┌────┴────────────┴─────┐
    │    CONTRÔLEUR         │
    │   (controller.h)      │
    │  Commandes abstraites │
    └───────────────────────┘
```

### 📂 Répartition des responsabilités

#### **Modèle** (`src/model.c`)

- État du jeu (score, vies, niveau)
- Physique des entités (positions, vitesses, collisions)
- Règles métier (game over, level up, spawning UFO/ennemis)
- Système de sauvegarde/chargement binaire
- **⚠️ Ne connaît PAS la vue** : aucune dépendance graphique

#### **Vue** (`src/view_ncurses.c` / `src/view_sdl.c`)

- Rendu à l'écran (sprites, textes, HUD)
- Gestion audio (bruitages, musiques)
- Capture des entrées clavier
- Conversion coordonnées logiques → pixels/caractères
- **⚠️ Ne modifie PAS le modèle** : lecture seule (sauf flags audio)

#### **Contrôleur** (`include/controller.h`)

- Définition des **commandes abstraites** (`CMD_MOVE_LEFT`, `CMD_SHOOT`, etc.)
- Interface entre périphériques bruts et logique métier

---

## 📦 Prérequis

### Dépendances système

```bash
# Ubuntu/Debian
sudo apt install build-essential cmake libncurses-dev
```

```bash
# Fedora
sudo dnf install gcc gcc-c++ make cmake ncurses-devel
```

### Bibliothèques incluses (dossier 3rdParty/)

Les bibliothèques SDL3 sont fournies avec le projet :

- **SDL3** : Rendu graphique
- **SDL3_image** : Chargement d'images BMP
- **SDL3_mixer** : Gestion audio
- **SDL3_ttf** : Rendu de polices

---

## 🔧 Installation

### 1. Clonage du dépôt

```bash
git clone <url-du-repo>
cd space-invaders
```

### 2. Préparation des scripts

```bash
chmod +x build.sh clean.sh
```

---

## 🔨 Compilation

### Option recommandée : Script automatique

```bash
./build.sh
```

Ce script :

1. Compile les bibliothèques SDL3 (si nécessaire)
2. Compile le projet Space Invaders

### Nettoyage complet

```bash
./clean.sh
```

Supprime tous les fichiers compilés (bibliothèques + projet).

### Recompilation rapide

```bash
make
```

---

## 🎮 Lancement du jeu

L'exécutable se trouve dans le dossier `build/` :

```bash
# Mode SDL (graphique, par défaut)
make run-sdl

# Mode ncurses (terminal)
make run-ncurses
```

**💡 Astuce SDL :** Appuyez sur **F11** pour basculer en plein écran.

**⚠️ Mode ncurses :** Taille minimale requise : 80 colonnes × 24 lignes

---

## 🕹️ Commandes

### Menu & Navigation

| Touche                  | Action                               |
| ----------------------- | ------------------------------------ |
| **↑ / ↓**               | Naviguer dans les menus              |
| **← / →**               | Ajuster le volume    (SDL uniquement)|
| **ESPACE** / **ENTRÉE** | Valider la sélection                 |
| **P** / **ÉCHAP**       | Pause / Retour                       |
| **F11**                 | Plein écran (SDL uniquement)         |

### En jeu

| Touche         | Action               |
| -------------- | -------------------- |
| **←/→**        | Déplacer le vaisseau |
| **ESPACE**     | Tirer                |
| **P**/**Echap**| Pause                |
| **Q**          | Quitter              |

### Saisie de texte (sauvegarde)

| Touche        | Action                     |
| ------------- | -------------------------- |
| **A-Z / 0-9** | Caractères alphanumériques |
| **ESPACE**    | Remplacé par `_`           |
| **-**         | Tiret (autorisé)           |
| **BACKSPACE** | Effacer                    |
| **ENTRÉE**    | Valider                    |

---

## 📁 Structure du projet

``` plantext
space-invaders/
│
├── 3rdParty/               # Bibliothèques externes (incluses)
│   ├── SDL3/                 # SDL3 principal
│   ├── SDL3_image/           # Chargement d'images
│   ├── SDL3_mixer/           # Gestion audio
│   └── SDL3_ttf/             # Rendu de texte 
│
├── assets/                 # Ressources graphiques/audio
│   ├── aliens/               # Sprites (A1/A2/A3, UFO,Player)
│   ├── audio/                # Sons (shoot, explosion, UFO, beats)
│   ├── backgrounds/          # Fonds d'écran (menu, jeu)
│   ├── explosions/           # Animations d'explosion
│   ├── fonts/                # Police TTF
│   ├── hearts/               # Icônes de vie (full/empty)
│   ├── missiles/             # Projectiles joueur (4 frames)
│   ├── preview/              # Captures d'écran pour le README
│   ├── projectiles/          # Projectiles ennemis (4 frames)
│   └──  shelter/             # Boucliers (10 états de dégâts)
│
├── build/                 # Fichiers compilés (.o, exécutable)
│
├── docs/                  # Documentation Doxygen générée
│
├── include/               # Headers (.h)
│   ├── common.h             # Constantes globales (GAME_WIDTH, FPS...)
│   ├── controller.h         # Commandes abstraites (GameCommand)
│   ├── model.h              # Structures de données (GameModel, Entity...)
│   ├── utils.h              # Utilitaires (temps, aléatoire)
│   ├── view_interface.h     # Interface abstraite (ViewInterface)
│   ├── view_ncurses.h       # Implémentation ncurses
│   └── view_sdl.h           # Implémentation SDL3
│
├── sauvegardes/            # Dossier des sauvegardes (.dat)
│
├── src/                    # Sources (.c)
│   ├── main.c               # Point d'entrée + boucle de jeu
│   ├── model.c              # Logique métier (update, collisions ...)
│   ├── utils.c              # Implémentation utilitaires
│   ├── view_ncurses.c       # Rendu terminal
│   └── view_sdl.c           # Rendu graphique SDL3
│
├── build.sh                 # Script de build automatique
├── clean.sh                 # Script de nettoyage
├── Doxyfile                 # Configuration Doxygen
├── INSTALL.txt              # Instructions d'installation détaillées
├── Makefile                 # Script de compilation
└── README.md                # Ce fichier
```

---

## 💾 Système de sauvegarde

### Fonctionnement

Les sauvegardes sont stockées dans le dossier `sauvegardes/` au format binaire (`.dat`).

**Contenu sauvegardé :**

- Score actuel
- Nombre de vies
- Niveau en cours
- Position et état des entités (joueur, ennemis, projectiles, UFO)
- État des boucliers (tous les blocs)
- Timers et cooldowns

### Utilisation

#### Créer une sauvegarde

1. Appuyez sur **P** en jeu → Menu Pause
2. Sélectionnez **"Sauvegarder et Quitter"**
3. Choisissez **"Créer nouvelle sauvegarde"**
4. Tapez un nom (ex: `partie1`)
5. Validation → Fichier `sauvegardes/partie1.dat` créé

#### Charger une sauvegarde

1. Menu principal → **"Charger"**
2. Sélectionnez le fichier avec **↑ / ↓**
3. Validation → Le jeu reprend exactement où vous l'aviez quitté

#### Écraser une sauvegarde

- Le système détecte les noms existants et demande confirmation
- Options : Ecraser l'ancien ou Creer une copie (..1) (par exemple sauvegarde1.dat si sauvegarde.dat existe et que vous ne voulez pas écraser la sauvegarde existante)

---

## 📚 Documentation

La documentation du code est générée avec **Doxygen**.

### Génération de la documentation

```bash
doxygen Doxyfile
```

### Consultation

Ouvrez `docs/html/index.html` dans votre navigateur.

---

## 🐛 Dépannage

### Problème : `SDL3 non trouvé` ou erreur de liaison

**Solution :** Utilisez le script de build automatique :

```bash
./build.sh
```

Ce script compile automatiquement toutes les bibliothèques SDL3 depuis le dossier `3rdParty/`.

### Problème : `Terminal trop petit` (ncurses)

**Cause :** La fenêtre terminal fait moins de 80×24 caractères.

**Solution :**

```plantext
Agrandir la fenêtre du terminal
```

### Problème : `Make ou GCC non trouvé`

**Solution :**

```bash
# Ubuntu/Debian
sudo apt install build-essential cmake

# Fedora
sudo dnf groupinstall "Development Tools" && sudo dnf install cmake
```

---

## 📜 Licence

Ce projet est sous licence **MIT**. Voir le fichier [LICENSE](LICENSE) pour plus de détails.

``` plantext
MIT License

Copyright (c) 2025-2026 Chamseddine Adaadour & Quentin Pellosse

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

## 🎵 Credits

### Audio

- Background music: [Game Music Loop 7](https://pixabay.com/sound-effects/game-music-loop-7-145285/) - Licensed under [Licence Pixabay](https://pixabay.com/service/license-summary/) (royalty-free, no attribution required)
- Level UP sound: [Level Up](https://pixabay.com/sound-effects/level-up-enhancement-8-bit-retro-sound-effect-153002/) - Licensed under [Licence Pixabay](https://pixabay.com/service/license-summary/) (royalty-free, no attribution required)
- GameOver sound: [Arcade game over](https://www.youtube.com/watch?v=FVJJKIJWKdc)
- Sprites : [Source](https://www.classicgaming.cc/classics/space-invaders/graphics) - [licence]
- Sound effects: [Source](https://www.classicgaming.cc/classics/space-invaders/sounds) - [licence]

---

## 👤 Auteurs (Groupe 7)

## Chamseddine Adaadour & Quentin Pellosse

- 🌐 GitHub : [@Dwikso](https://github.com/Dwikso) & [@chamse22ine](https://github.com/chamse22ine)
