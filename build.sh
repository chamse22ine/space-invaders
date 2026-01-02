#!/bin/bash
##
# @file build.sh
# @brief Script de compilation du projet Space Invaders.
#
# Ce script compile les dépendances tierces (SDL3, SDL3_image, SDL3_mixer, SDL3_ttf)
# si elles ne sont pas déjà compilées, puis compile le projet principal.
#
# @usage ./build.sh
##

# Sauvegarde du répertoire initial
INIT_PATH=$PWD

##
# Compilation de SDL3 (bibliothèque graphique de base)
##
if [ ! -d 3rdParty/SDL3/build ]
then
    cd 3rdParty/SDL3/
    mkdir build
    cd build
    cmake ..
    make -j
fi

##
# Compilation de SDL3_image (chargement d'images BMP, PNG, etc.)
##
if [ ! -d $INIT_PATH/3rdParty/SDL3_image/build ]
then
    cd $INIT_PATH/3rdParty/SDL3_image/
    mkdir build
    cd build
    cmake .. -DSDL3_DIR=../SDL3/build
    make -j
fi

##
# Compilation de SDL3_mixer (gestion audio et sons)
##
if [ ! -d $INIT_PATH/3rdParty/SDL3_mixer/build ]
then
    cd $INIT_PATH/3rdParty/SDL3_mixer/
    mkdir build
    cd build
    cmake .. -DSDL3_DIR=../SDL3/build
    make -j
fi

##
# Compilation de SDL3_ttf (rendu de polices TrueType)
##
if [ ! -d $INIT_PATH/3rdParty/SDL3_ttf/build ]
then
    cd $INIT_PATH/3rdParty/SDL3_ttf/
    mkdir build
    cd build
    cmake .. -DSDL3_DIR=../SDL3/build
    make -j
fi

##
# Compilation du projet principal Space Invaders
##
cd $INIT_PATH
make -j
