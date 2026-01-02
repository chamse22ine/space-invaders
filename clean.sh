#!/bin/bash
##
# @file clean.sh
# @brief Script de nettoyage du projet Space Invaders.
#
# Supprime tous les fichiers compilés des dépendances tierces
# et les fichiers objets du projet principal.
#
# @usage ./clean.sh
##

##
# Suppression des builds des bibliothèques tierces
##
rm -rf 3rdParty/SDL3/build
rm -rf 3rdParty/SDL3_image/build
rm -rf 3rdParty/SDL3_mixer/build
rm -rf 3rdParty/SDL3_ttf/build

##
# Nettoyage du projet principal (fichiers objets et exécutable)
##
make clean
