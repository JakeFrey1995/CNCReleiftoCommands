#include <stdio.h>
#include <stdlib.h>
#include "bmp.h"

void writerow(FILE* outptr, long width, float spp, float spyu, float spi, int side, int row[]);

int main(int argc, char *argv[]){
    
    //make sure command line has 2 arguments
    if (argc != 3){
        printf("usage error");
        return 1;
    }
    
    //initialize infile and outfile 
    char *infile = argv[1];
    char *outfile = argv[2];
    
    
    //open infile
    FILE *inptr = fopen(infile, "r");
    if (inptr == NULL){
        printf("could not open %s", infile);
        return 1;
    }
    
    //open outfile
    FILE *outptr = fopen(outfile, "w");
    if (outptr == NULL){
        fclose("inptr");
        printf("could not create %s", outfile);
        return 1;
    }
   
    //read file header
    BITMAPFILEHEADER bf;
    fread(&bf, sizeof(BITMAPFILEHEADER), 1, inptr);
    
    //read info header
    BITMAPINFOHEADER bi;
    fread(&bi, sizeof(BITMAPINFOHEADER), 1, inptr);
    
    //make sure infile is a 24-bit bitmap
    if (bf.bfType != 0x4d42 || bf.bfOffBits != 54 || bi.biSize != 40 || 
        bi.biBitCount != 24 || bi.biCompression != 0)
    {
        fclose(outptr);
        fclose(inptr);
        fprintf(stderr, "not a bitmap");
        return 1;
    }
    
    //intitialize and prompt for and receive dimensional data
    float depth, width, diameter;
    printf("enter the depth of the piece in inches\n");
    scanf("%f", &depth);
    printf("enter the width of the piece in inches\n");
    scanf("%f", &width);
    
    //find steps per pixel and steps per y unit
    float spi = 159; //steps per inch (should be 159)
    float ppi = bi.biWidth/width; //pixel per inch 
    float spp = spi/ppi; //steps per pixel (should be spi/ppi)
    float yupi = 256/depth; //y unit per inch
    float spyu = spi/yupi; //steps per y unit (should be spi/yupi)
    
    //find padding
    int padding = (4 - (bi.biWidth * sizeof(RGBTRIPLE)) % 4) % 4;
    
    //variables for flipping side
    int side = 0;
    
    //this portion will iterate over the entire image and make a relief array
    //iterate over height of bitmap
    for (int i = 0; i < bi.biHeight; i++){
        //temp storage for row
        int row[bi.biWidth];
        //iterate over height of bitmap
        for (int j = 0; j < bi.biWidth; j++){
            
            //temp storage for pixel
            RGBTRIPLE triple;
            
            //read pixel to triple
            fread(&triple, sizeof(RGBTRIPLE), 1, inptr);
            row[j] = (triple.rgbtBlue + triple.rgbtGreen + triple.rgbtRed)/3;
        }
        //skip padding
        fseek(inptr, padding, SEEK_CUR);
        
        //send row to write function
        writerow(outptr, bi.biWidth, spp, spyu, spi, side, row);
        if (side == 0){
            side = 1;
        }
        else if (side == 1){
            side = 0;
        }
        
    }
        
    
    
    return 0;
}


void writerow(FILE* outptr, long width, float spp, float spyu, float spi, int side, int row[]){
    //variables used for choosing tool path
    int h1, h2;
    int count = 0;
    short axis, dir;
    float steps; 
    
    if (side == 0){
        
        //moves tool to initial y height
        h1 = row[0];
        axis = 2;
        dir = 0;
        steps = (255-h1)*spyu + .5*spi;
        fwrite(&axis, sizeof(short), 1, outptr);
        fwrite(&dir, sizeof(short), 1, outptr);
        fwrite(&steps, sizeof(float), 1, outptr);
        
        for (int i = 1; i < width; i++){
            h2 = row[i];
            if (h2 == h1){
                if (i == width - 1){
                    axis = 1;
                    dir = 1;
                    steps = (count + 1)*spp;
                    fwrite(&axis, sizeof(short), 1, outptr);
                    fwrite(&dir, sizeof(short), 1, outptr);
                    fwrite(&steps, sizeof(float), 1, outptr);
                }
                count++;
                h1 = h2;
            }
            if (h2 > h1){
                axis = 1;
                dir = 1;
                steps = count*spp;
                fwrite(&axis, sizeof(short), 1, outptr);
                fwrite(&dir, sizeof(short), 1, outptr);
                fwrite(&steps, sizeof(float), 1, outptr);
                axis = 2;
                dir = 1;
                steps = (h2 - h1)*spyu;
                fwrite(&axis, sizeof(short), 1, outptr);
                fwrite(&dir, sizeof(short), 1, outptr);
                fwrite(&steps, sizeof(float), 1, outptr);
                axis = 1;
                dir = 1;
                steps = spp;
                fwrite(&axis, sizeof(short), 1, outptr);
                fwrite(&dir, sizeof(short), 1, outptr);
                fwrite(&steps, sizeof(float), 1, outptr);
                count = 0;
            }
            if (h2 < h1){
                axis = 1;
                dir = 1;
                steps = (count + 1)*spp;
                fwrite(&axis, sizeof(short), 1, outptr);
                fwrite(&dir, sizeof(short), 1, outptr);
                fwrite(&steps, sizeof(float), 1, outptr);
                axis = 2;
                dir = 0;
                steps = (h1 - h2)*spyu;
                fwrite(&axis, sizeof(short), 1, outptr);
                fwrite(&dir, sizeof(short), 1, outptr);
                fwrite(&steps, sizeof(float), 1, outptr);
                count = 0;
            }
            h1 = h2;
        }
        axis = 2;
        dir = 1;
        steps = (255 - h2)*spyu + .5*spi;
        fwrite(&axis, sizeof(short), 1, outptr);
        fwrite(&dir, sizeof(short), 1, outptr);
        fwrite(&steps, sizeof(float), 1, outptr);
        axis = 3;
        dir = 1;
        steps = spp;
        fwrite(&axis, sizeof(short), 1, outptr);
        fwrite(&dir, sizeof(short), 1, outptr);
        fwrite(&steps, sizeof(float), 1, outptr);
    }
    
    if (side == 1){
        
        //moves tool to initial y height
        h1 = row[width - 1];
        axis = 2;
        dir = 0;
        steps = (255-h1)*spyu + .5*spi;
        fwrite(&axis, sizeof(short), 1, outptr);
        fwrite(&dir, sizeof(short), 1, outptr);
        fwrite(&steps, sizeof(float), 1, outptr);
        
        for (int i = width - 2; i > -1; i--){
            h2 = row[i];
            if (h2 == h1){
                if (i == 0){
                    axis = 1;
                    dir = 0;
                    steps = (count + 1)*spp;
                    fwrite(&axis, sizeof(short), 1, outptr);
                    fwrite(&dir, sizeof(short), 1, outptr);
                    fwrite(&steps, sizeof(float), 1, outptr);
                }
                count++;
                h1 = h2;
            }
            if (h2 > h1){
                axis = 1;
                dir = 0;
                steps = count*spp;
                fwrite(&axis, sizeof(short), 1, outptr);
                fwrite(&dir, sizeof(short), 1, outptr);
                fwrite(&steps, sizeof(float), 1, outptr);
                axis = 2;
                dir = 1;
                steps = (h2 - h1)*spyu;
                fwrite(&axis, sizeof(short), 1, outptr);
                fwrite(&dir, sizeof(short), 1, outptr);
                fwrite(&steps, sizeof(float), 1, outptr);
                axis = 1;
                dir = 0;
                steps = spp;
                fwrite(&axis, sizeof(short), 1, outptr);
                fwrite(&dir, sizeof(short), 1, outptr);
                fwrite(&steps, sizeof(float), 1, outptr);
                count = 0;
            }
            if (h2 < h1){
                axis = 1;
                dir = 0;
                steps = (count + 1)*spp;
                fwrite(&axis, sizeof(short), 1, outptr);
                fwrite(&dir, sizeof(short), 1, outptr);
                fwrite(&steps, sizeof(float), 1, outptr);
                axis = 2;
                dir = 0;
                steps = (h1 - h2)*spyu;
                fwrite(&axis, sizeof(short), 1, outptr);
                fwrite(&dir, sizeof(short), 1, outptr);
                fwrite(&steps, sizeof(float), 1, outptr);
                count = 0;
            }
            h1 = h2;
        }
        axis = 2;
        dir = 1;
        steps = (255 - h2)*spyu + .5*spi;
        fwrite(&axis, sizeof(short), 1, outptr);
        fwrite(&dir, sizeof(short), 1, outptr);
        fwrite(&steps, sizeof(float), 1, outptr);
        axis = 3;
        dir = 1;
        steps = spp;
        fwrite(&axis, sizeof(short), 1, outptr);
        fwrite(&dir, sizeof(short), 1, outptr);
        fwrite(&steps, sizeof(float), 1, outptr);
    }
}