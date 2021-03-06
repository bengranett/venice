#ifndef MAIN_H
#define MAIN_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_histogram.h>
#include <gsl/gsl_histogram2d.h>

#define FAILURE 0
#define SUCCESS 1

#define PI 3.14159265358979323846
#define TWOPI 6.283185307179586476925287

#define EPS 1.0e-14
#define INF 1.0e30
#define ODD 0
#define EVEN 1
#define LEAF 0
#define NODE 1
#define RADEC 0
#define CART 1

#define MAX(x,y) ((x) > (y)) ? (x) : (y)
#define MIN(x,y) ((x) < (y)) ? (x) : (y)
#define ABS(a) ((a) < 0 ? -(a) : (a))
#define PARITY(a) (a)%2 ? ODD : EVEN

#define NFIELD 100
#define NCHAR 20
#define NVERTICES 100

#define getDoubleValue(array,col)  atof(array+NCHAR*(col-1))
#define getIntValue(array,col)  atoi(array+NCHAR*(col-1))
#define getCharValue(array,col) array+NCHAR*(col-1)
#define getLine(array,i) array+NFIELD*NCHAR*i

#define MAX(x,y) ((x) > (y)) ? (x) : (y)
#define MIN(x,y) ((x) < (y)) ? (x) : (y)
#define ABS(a) ((a) < 0 ? -(a) : (a))
#define SWAP(a,b) {swap = (a); (a) = (b); (b) = swap;}


//char MYNAME[100];
//size_t IDERR;
//double EPS=99.00;

/*----------------------------------------------------------------*
 *New types                                                       *
 *----------------------------------------------------------------*/


typedef struct Config
{
  char fileRegInName[1000];
  char fileCatInName[1000];
  char fileOutName[1000];
  int nx,ny,format;
  int xcol,ycol;
  int coordType;
  size_t npart;
  double min[2];
  double max[2];
  int minDefinied[2];
  int maxDefinied[2];
} Config;


typedef struct Complex
{
    double re;
    double im;
} Complex;

typedef struct Polygon
{
  int N, id;
  double x[NVERTICES];
  double y[NVERTICES];
  double xmin[2];
  double xmax[2];
} Polygon;


typedef struct Node
{
  int type, *root, id, *polysAll, SplitDim;
  double SplitValue;
  size_t Nnodes, Npolys, NpolysAll;
  int *poly_id;
  void *Left, *Right;
} Node;


/*----------------------------------------------------------------*
 *Global variables                                                *
 *----------------------------------------------------------------*/
//FILE *FILE_REG_IN,*FILE_CAT_IN, *FILE_OUT;
//int NX,NY,XCOL,YCOL,NPART,FORMAT;
//double *LIMITS;
/*----------------------------------------------------------------*
 *Main routines                                                   *
 *----------------------------------------------------------------*/

gsl_histogram2d *mask2d(const Config *para);
void flagCat(const Config *para);
void randomCat(const Config *para);
int readParameters(int argc, char **argv, Config *para);
void free_Polygon(Polygon *polygon, size_t N);
void free_Node(Node *node);
void cpyPolygon(Polygon *a, Polygon *b);

/*----------------------------------------------------------------*
 *Utils - geometric                                               *
 *----------------------------------------------------------------*/

int insidePolygon(Polygon *p,int Npoly, double x0,double y0,double x,double y, int *poly_id);
int insidePolygonTree(Node *polyTree, double x0[2], double x[2], int *poly_id);
Polygon *readPolygonFile(FILE *fileIn, int *Npolys, Node *polyTree);
Node *readPolygonFileTree(FILE *fileIn, double xmin[2], double xmax[2]);
Node *createNode(Polygon *polys, size_t Npolys, double minArea, int SplitDim, double xmin[2], double xmax[2], int firstCall);

/*----------------------------------------------------------------*
 *Utils - numeric                                                 *
 *----------------------------------------------------------------*/

double determineMachineEpsilon();
size_t determineSize_tError();
gsl_rng *randomInitialize(int time_dependent);
FILE *fopenAndCheck(const char *filename,char *mode);
int getStrings(char *line, char *strings, char *delimit, size_t *N);
void printCount(const size_t *count, const size_t *total,  const size_t step);


#endif	/* MAIN_H */
