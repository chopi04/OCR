/*
    OCR.c

    Recherche du chiffre correspondant depuis Cell.bin

    Axel Chopard, Daniel Jenelten, Kishan Parbez

    20.12.2023
*/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#pragma warning(disable : 4996)



#define DigitBitmapHeight 24
#define DigitBitmapWidth 16


//contrôle la taille de la cellule (largeur et hauteur)
void CheckSizeCell(int width, int height, FILE* f_in) {
    if ((width < 16) || (width > 100)) {
        fprintf(stderr, "incorrect width");
        fclose(f_in);
        exit(-1);
    }
    if ((height < 24) || (height > 100)) {
        fprintf(stderr, "incorrect height");
        fclose(f_in);
        exit(-1);
    }
}


//obtenir le bit de Cell.bin pour une certaine position (ligne, colonne)
unsigned char GetCellBit(unsigned char* cell, unsigned int width, unsigned char line, unsigned char col) {
    int coord;
    coord = (line * width) + col;
    return (unsigned char)cell[coord];
}


//obtenir le bit du bitmap pour une certaine position (ligne, colonne)
unsigned int GetDigitBitmapBit(int16_t digits[][DigitBitmapHeight], unsigned int line, unsigned int col, unsigned int num) {
    unsigned int bitvalue;
    unsigned int mask;
    mask = 1 << (DigitBitmapWidth - 1 - col);
    bitvalue = (digits[num][line] & mask) >> (DigitBitmapWidth - 1 - col);
    return bitvalue;
}


//ouvrir les seuils depuis argc
int main(int argc, const char* argv[])
{
    //initialiser les bitmaps
    int16_t DigitBitmap[9][DigitBitmapHeight] =
    {
        {0, 0, 96, 224, 224, 992, 4064, 4064, 224, 224, 224, 224, 224, 224, 224, 224, 224, 224, 224, 224, 224, 224, 0, 0},
        {0, 0, 496, 2044, 4092, 7710, 7182, 14350, 14351, 14, 30, 60, 124, 496, 2016, 3968, 7680, 7168, 14336, 16382, 16382, 16382, 0, 0},
        {0, 0, 992, 4088, 8188, 7196, 14350, 14350, 14350, 28, 252, 504, 508, 30, 14, 14, 14350, 14350, 14350, 15422, 8188, 4080, 0, 0},
        {0, 0, 56, 120, 120, 248, 504, 440, 952, 1848, 3640, 3640, 7224, 14392, 14392, 16383, 16383, 56, 56, 56, 56, 56, 0, 0 },
        {0, 0, 4092, 4092, 4092, 3072, 7168, 7168, 7168, 8176, 8184, 8188, 7198, 14, 14, 15, 14, 14350, 14350, 15932, 8188, 4080, 0, 0},
        {0, 0, 496, 2044, 4092, 7710, 7182, 7174, 14336, 14832, 15352, 16380, 15902, 15374, 14350, 14343, 14343, 14350, 7182, 7708, 4092, 2032, 0, 0},
        {0, 0, 16382, 16383, 16383, 14, 28, 28, 56, 112, 112, 224, 224, 448, 448, 960, 896, 896, 1920, 1792, 1792, 1792, 0, 0, },
        {0, 0, 992, 4088, 8188, 7198, 7182, 14350, 7182, 7196, 4092, 4088, 8188, 15390, 14350, 14350, 14350, 14350, 15374, 7710, 8188, 2040, 128, 0, },
        {0, 0, 992, 4088, 8188, 7196, 14350, 14350, 14350, 14350, 14350, 15390, 7742, 8190, 2030, 14, 14, 14364, 14364, 7224, 8184, 4080, 0, 0, }
    };


    if (argc != 12) {
        fprintf(stderr, "Not the right number of parameters in the command line");
        return -1;
    }

    //initialiser width et height
    unsigned int width;
    unsigned int height;

    //Ouverture de Cell.bin et contrôle de l'ouverture
    FILE* f_in;

    const char* file_name = argv[0];

    f_in = fopen(file_name, "rb");

    if (f_in == NULL) {
        perror("Error for opening Cell.bin\n");
        return -1;
    }


    //Lecture de Cell.bin

    //width et height + contrôle de lecture
    int r1 = fread(&width, sizeof(unsigned int), 1, f_in);
    if (r1 != 1) {
        fprintf(stderr, "Error reading width");
        fclose(f_in);
        return -1;
    }
    int r2 = fread(&height, sizeof(unsigned int), 1, f_in);
    if (r2 != 1) {
        fprintf(stderr, "Error reading height");
        fclose(f_in);
        return -1;
    }

    //contrôler les valeurs
    CheckSizeCell(width, height, f_in);


    size_t SizeCell = width * height;

    //création de la mémoire dynamique cell (à la taille de SizeCell)
    unsigned char* cell;
    cell = (unsigned char*)malloc(SizeCell * sizeof(unsigned char));

    //contrôle de la création du malloc
    if (cell == NULL) {
        fprintf(stderr, "Error in initialising the malloc\n");
        fclose(f_in);
        return -1;
    }

    //mettre le contenu de Cell.bin dans la mémoire dynamique cell
    int size = width * height;

    int NbPixelCell = fread(cell, sizeof(unsigned char), (size + 1), f_in);

    //contrôler qu'il y ait bien le nombre correct de pixel dans Cell.bin (width*height)
    if (NbPixelCell < size) {
        fprintf(stderr, "Not enough pixel in Cell.bin");
        fclose(f_in);
        return -1;
    }
    if (NbPixelCell > size) {
        fprintf(stderr, "Too much pixel in Cell.bin");
        fclose(f_in);
        return -1;
    }


    //initialiser les valeurs pour l'étude du chiffre dans "Cell.bin"
    int pixelIdCount = 0;
    int pixelWhite = 0;

    double thresholdBitMap[10] = { 0 }; // = { 0.98, 0.91, 0.78, 0.78, 0.85, 0.78, 0.78, 0.90, 0.76, 0.75 };

    double ratio[10] = { 0 };
    int digitPixelId[9] = { 0 };
    int analysedNumb = 0;

    
    //initialiser thresholdBitmap
    for (int i = 2; i < argc; i++) {
        thresholdBitMap[i - 2] = (atof(argv[i]) / 100);

        if ((thresholdBitMap[i - 2] > 1.0) || (thresholdBitMap[i - 2] < 0.05)) {
            fprintf(stderr, "threshold value out of boundary\n");
            fclose(f_in);
            return -1;
        }

    }
    
    //ratio pour la case blanche
    for (int i = 0; i < size; i++) {
        if (cell[i] == 0) pixelWhite++;
    }
    ratio[0] = (double)pixelWhite / size;

    //teste si c'est une case blanche
    if (ratio[0] > thresholdBitMap[0]) {
        analysedNumb = 0;

        //sauter la recherche des autres chiffres, vu que c'est une case blanche
        goto Afteranalysing;
    }


    //test des chiffres pour les bitmaps de 1 à 9
    for (unsigned char n = 0; n < 9; n++) {

        //essayer le bitmap à chaque emplacement possible dans "Cell.bin"
        for (unsigned char i = 0; i < (height - DigitBitmapHeight); i++) {
            for (unsigned char j = 0; j < (width - DigitBitmapWidth); j++) {
                pixelIdCount = 0;

                //comparer le bitmap avec "Cell.bin"
                for (unsigned char x = 0; x < DigitBitmapHeight; x++) {
                    for (unsigned char y = 0; y < DigitBitmapWidth; y++) {

                        //comptage des pixels identiques pour la position du BitMap
                        if (GetCellBit(cell, width, (i + x), (j + y)) == GetDigitBitmapBit(DigitBitmap, x, y, n)) pixelIdCount++;
                    }
                }
                //determination du nombre max de pixel identique en fonction de la position du BitMap
                if (pixelIdCount > digitPixelId[n]) digitPixelId[n] = pixelIdCount;

            }
        }
    }

    //calcul des ratios et détermination si un chiffre est éligible (car dépassant le seuil)
    int possibleNum[9] = { 0 };
    for (int i = 1; i < 10; i++) {
        ratio[i] = (double)digitPixelId[i - 1] / (DigitBitmapHeight * DigitBitmapWidth);
        if (ratio[i] > thresholdBitMap[i]) possibleNum[i - 1] = 1;
    }


    //détermination du chiffre en prenant le ratio le plus haut si plus qu'un chiffre est éligible
    int sumJ = 0;
    for (int j = 0; j < 9; j++) {

        sumJ += possibleNum[j];

        if (sumJ == 1 && possibleNum[j] == 1) analysedNumb = (j + 1);

        if (possibleNum[j] == 1 && ratio[j + 1] > ratio[analysedNumb]) analysedNumb = (j + 1);

    }

    // si aucun nombre n'est éligible alors la case est vide
    if (sumJ == 0) analysedNumb = 0;


    //correspondance pour le goto
Afteranalysing:;


    //Ouverture et mise a zéro ou création de CellValue.txt
    FILE* f_out;

    f_out = fopen("CellValue.txt", "w");

    if (f_out == NULL) {
        perror("Error for opening CellValue.txt\n");
        return -1;
    }

    //Écriture du résultat sur le fichier "CellValue.txt"
    fprintf(f_out, "d:'%d', %.4lf%% \n", analysedNumb, 100 * ratio[analysedNumb]);

    free(cell);


    //fermeture des fichiers
    fclose(f_in);
    fclose(f_out);

    //fin du code C
    return 0;
}
