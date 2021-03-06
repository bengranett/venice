#include "main.h"

/*
------------------------
Jean Coupon - Oct 2011
------------------------

Programs that reads a mask file (DS9 type) and a catalogue 
of objects and computes one of the following tasks:
1. Creates a pixelized mask. 
   venice -m mask.reg [OPTIONS]
2. Finds objects inside/outside a mask. 
   venice -m mask.reg -cat file.cat [OPTIONS]
3. Generates a random catalogue of objects inside/outside a mask. 
   venice -m mask.reg -r [OPTIONS]

Modifications:

v 3.04 - Oct 2011
- corrected a bug in flagCat
very rare cases
v 3.01 - Oct 2011
- added "+EPS" in the algorithm to find an 
object within a polygon (was incorrect in some
very rare cases)
v 3.0 - Oct 2011
- polygons are now stored in a tree with leaves
being either empty of polygons of small enough
to contain a few polygons only (currently set 
to the mean size of polygons... this has probably to
be improved).
v 2.2 - Sept 2011
- gives the area with spherical coordinates option
- default output in the standard ouput: stdout
v 2.1 - July 2010
- some precisions added in Makefile and README
- in 'flagCat', the limits of the mask are no
longer used as "default". If no limits are 
definied, an object oustide the mask limits will be
now properly considered as "outside the mask".
v 2.02 - Feb 2nd 2010 
- minor changes (changed some of 
printf formats to work with 64bit machines)
v 2.01 - March 5 2009
- bug corrected in flagcat (wrong ymax)
v 2.0 - Feb 16th 2009
- All parameters put in a single structure "Config".
- File reading improved and simplified.
- Now integrates random catalogue on a sphere.
v 1.02 - Jan 17th 2009
- the option format [-f] can be use in task 2.
- the limits options can be used in task 2.
- README updated.
V 1.01 - April 2nd 2008
- The pixelized mask limits can be definied by the user as well
as the random catalogue limits.
- More explanations added in the README file.

V 1.00 - March 26th 2008
First version.
*/

char MYNAME[100];
//size_t IDERR;
//double EPS=99.00;
FILE *FILE_REG_IN,*FILE_CAT_IN, *FILE_OUT;
int NX,NY,XCOL,YCOL,NPART,FORMAT;
double *LIMITS;


int main(int argc, char **argv)
{
  //Initialization
  srand((unsigned int)time(NULL));
  //EPS = determineMachineEpsilon();
  //IDERR = determineSize_tError();
  Config para;
  
  //Tasks
  switch (readParameters(argc,argv,&para)){
  case 1:
    mask2d(&para);
    break;
  case 2:
    flagCat(&para);
    break;
  case 3:
    randomCat(&para);
    break;
  }
  return EXIT_SUCCESS;
}

/*----------------------------------------------------------------*
 *Main routines                                                   *
 *----------------------------------------------------------------*/

gsl_histogram2d *mask2d(const Config *para){
  /*Return the mask in gsl histogram format and writes the mask in fileOut. 
    The limits are the extrema of the extreme polygons in fileRegIn. 
    The pixel is set to 0 when inside the mask and 1 otherwise.
  */  
  
  double x0[2],x[2],xmin[2],xmax[2];
  int Npolys,poly_id;
  size_t i,j,count,total;
  
  FILE *fileRegIn = fopenAndCheck(para->fileRegInName,"r");
  FILE *fileOut   = fopenAndCheck(para->fileOutName,"w");
  
  //Read the mask file (.reg DS9 file)
  Node *polyTree = readPolygonFileTree(fileRegIn,xmin,xmax);
  Polygon *polys = (Polygon *)polyTree->polysAll;
  Npolys         = polyTree->Npolys;
  
  //Or if the limits are defined by the user:
  if(para->minDefinied[0]) xmin[0] = para->min[0];
  if(para->maxDefinied[0]) xmax[0] = para->max[0];
  if(para->minDefinied[1]) xmin[1] = para->min[1];
  if(para->maxDefinied[1]) xmax[1] = para->max[1];
  
  //Reference point. It must be outside the maxssk;
  x0[0] = xmin[0] - 1.0; x0[1] = xmin[1] - 1.0;
  
  gsl_histogram2d *mask = gsl_histogram2d_alloc(para->nx,para->ny);//Mask.
  gsl_histogram2d_set_ranges_uniform(mask,xmin[0],xmax[0],xmin[1],xmax[1]);//Ranges of the mask.
  
  total = para->nx*para->ny;
  
  fprintf(stderr,"Progress =     ");
  for(i=0; i<mask->nx; i++){
    for(j=0; j<mask->ny; j++){
      count = i*para->ny+j;
      printCount(&count,&total,1000);
      //Center of the pixel.
      x[0] = (mask->xrange[i]+mask->xrange[i+1])/2.0;
      x[1] = (mask->yrange[j]+mask->yrange[j+1])/2.0;
      //1 = outside the mask, 0 = inside the mask
      if(!insidePolygonTree(polyTree,x0,x,&poly_id)) mask->bin[i*mask->ny+j] = 1.0;
      //if(!insidePolygon(polys,Npolys, x0[0], x0[1], x[0], x[1], &poly_id)) mask->bin[i*mask->ny+j] = 1.0;
    }
  }
  fflush(stdout);
  fprintf(stderr,"\b\b\b\b100%%\n");
  
  //Writes the file
  //fprintf(stderr,"Writing %s...\n",para->fileOutName);
  for(j=0; j<mask->ny; j++){
    for(i=0; i<mask->nx; i++){
      fprintf(fileOut,"%f ",mask->bin[i*mask->ny+j]);
    }
    fprintf(fileOut,"\n");
  }
  fclose(fileRegIn);
  fclose(fileOut);
  
  return mask;
}

void flagCat(const Config *para){
  /*Reads fileCatIn and add a flag at the end of the line. 1 is outside
 the mask and 0 is inside the mask. xcol and ycol are the column ids of resp. x coordinate and y coordinate.*/
  double x0[2], x[2], xmin[2], xmax[2];
  size_t i,N,Ncol;
  int Npolys,poly_id,flag;
  char line[NFIELD*NCHAR], item[NFIELD*NCHAR],*str_end;
  
  FILE *fileRegIn = fopenAndCheck(para->fileRegInName,"r");
  FILE *fileCatIn = fopenAndCheck(para->fileCatInName,"r");
  FILE *fileOut   = fopenAndCheck(para->fileOutName,"w");
  
  //Reads the mask file (.reg DS9 file)
  //and construct the array of polygons.
  Node *polyTree = readPolygonFileTree(fileRegIn,xmin,xmax);
  Polygon *polys = (Polygon *)polyTree->polysAll;
  Npolys         = polyTree->Npolys;
  
  //Or if the limits are defined by the user:
  if(para->minDefinied[0]) xmin[0] = para->min[0];
  if(para->maxDefinied[0]) xmax[0] = para->max[0];
  if(para->minDefinied[1]) xmin[1] = para->min[1];
  if(para->maxDefinied[1]) xmax[1] = para->max[1];
  
  //Reference point. It must be outside the mask;
  x0[0] = xmin[0] - 1.0; x0[1] = xmin[1] - 1.0;
  
  N = 0;
  while(fgets(line,NFIELD*NCHAR,fileCatIn) != NULL)
    if(getStrings(line,item," ",&Ncol))  N++;
  rewind(fileCatIn);
  fprintf(stderr,"Nobjects = %zd\n",N);
  
  i = 0;
  fprintf(stderr,"Progress =     ");
  while(fgets(line,NFIELD*NCHAR,fileCatIn) != NULL){
    if(getStrings(line,item," ",&Ncol)){
      i++;
      printCount(&i,&N,1000);
      x[0] = getDoubleValue(item,para->xcol);
      x[1] = getDoubleValue(item,para->ycol);
      
      /*
      flag = 0;
      if(xmin[0] < x[0] && x[0] < xmax[0] && xmin[1] < x[1] && x[1] < xmax[1]){
	if(!insidePolygonTree(polyTree,x0,x,&poly_id)) flag = 1;
      }
      */
      if(flag=0, !insidePolygonTree(polyTree,x0,x,&poly_id)) flag = 1;


      str_end = strstr(line,"\n");//cariage return to the end of the line.
      strcpy(str_end,"\0");       //"end" symbol to the line
      switch (para->format){
      case 1://Only objects outside the mask and inside the user's definied limits
	if(flag) fprintf(fileOut,"%s\n",line);  
	break;
      case 2: //Only objects inside the mask or outside the user's definied limits
	if(!flag) fprintf(fileOut,"%s\n",line);  
	break;
      case 3://All objects with the flag
	fprintf(fileOut,"%s %d\n",line,flag);    
      }
    }
  }
  fflush(stdout);
  fprintf(stderr,"\b\b\b\b100%%\n");
  
  fclose(fileOut);
  fclose(fileCatIn);
  return;
}

void randomCat(const Config *para){
  /*Generates a random catalogue inside the mask (uniform PDF).
    If "all", it puts all objects and add a flag such as: 
    outside the mask:1, inside the mask:0. Otherwise it puts only objects
    outside the mask.*/
  int Npolys,poly_id,flag;
  size_t i;
  double x0[2], x[2], xmin[2], xmax[2];
  gsl_rng *r = randomInitialize(1);
  
  FILE *fileOut = fopenAndCheck(para->fileOutName,"w");
  
  //If no mask is provided, it reads xmin, xmax, ymin and ymax and
  //make a random catalogue with no mask with the format "outside"
  //(all objects written with no flag: x y)...
  if(!strcmp(para->fileRegInName,"\0")){
    fprintf(stderr,"Generating catalogue with no mask...\n");
    xmin[0] = para->min[0];
    xmax[0] = para->max[0];
    xmin[1] = para->min[1];
    xmax[1] = para->max[1];
    fprintf(stderr,"xmin = %f, xmax = %f, ymin = %f, ymax = %f\n",xmin[0],xmax[0],xmin[1],xmax[1]);
    fprintf(stderr,"Progress =     ");
    for(i=0;i<para->npart;i++){
      printCount(&i,&para->npart,1000);
      if(para->coordType == CART){
	x[0] = gsl_ran_flat(r,xmin[0],xmax[0]);
	x[1] = gsl_ran_flat(r,xmin[1],xmax[1]);
	fprintf(fileOut,"%f %f\n",x[0],x[1]);
      }else{
	x[0] = gsl_ran_flat(r,xmin[0],xmax[0]);
	x[1] = gsl_ran_flat(r,sin(xmin[1]*PI/180.0),sin(xmax[1]*PI/180.0));
	fprintf(fileOut,"%f %f\n",x[0], asin(x[1])*180.0/PI);
      }
    }
    fflush(stdout);
    fprintf(stderr,"\b\b\b\b100%%\n");
    return;
  }
  
  //... otherwise it reads the mask file (.reg DS9 file)
  //and construct the array of polygons.
  FILE *fileRegIn = fopenAndCheck(para->fileRegInName,"r");
  Node *polyTree = readPolygonFileTree(fileRegIn,xmin,xmax);
  Polygon *polys = (Polygon *)polyTree->polysAll;
  Npolys         = polyTree->Npolys;
  
  //Or if the limits are defined by the user:
  if(para->minDefinied[0]) xmin[0] = para->min[0];
  if(para->maxDefinied[0]) xmax[0] = para->max[0];
  if(para->minDefinied[1]) xmin[1] = para->min[1];
  if(para->maxDefinied[1]) xmax[1] = para->max[1];
  
  //Reference point. It must be outside the mask;
  x0[0] = xmin[0] - 1.0; x0[1] = xmin[1] - 1.0;
  
  fprintf(stderr,"Creates a random catalogue with N = %zd objects. Format = %d\n",para->npart,para->format);
  fprintf(stderr,"xmin = %f \nxmax = %f \nymin = %f \nymax = %f\n",xmin[0],xmax[0],xmin[1],xmax[1]);
  if(para->coordType == RADEC){
    fprintf(stderr,"Area = %f\n",(xmax[0] - xmin[0])*(sin(xmax[1]*PI/180.0) - sin(xmin[1]*PI/180.0))*180.0/PI);
  }
  fprintf(stderr,"Progress =     ");
  for(i=0;i<para->npart;i++){
    printCount(&i,&para->npart,1000);
    x[0] = gsl_ran_flat(r,xmin[0],xmax[0]);
    x[1] = gsl_ran_flat(r,xmin[1],xmax[1]);
    if(para->coordType == CART){
      x[0] = gsl_ran_flat(r,xmin[0],xmax[0]);
      x[1] = gsl_ran_flat(r,xmin[1],xmax[1]);
    }else{
      x[0] = gsl_ran_flat(r,xmin[0],xmax[0]);
      x[1] = gsl_ran_flat(r,sin(xmin[1]*PI/180.0),sin(xmax[1]*PI/180.0));
      x[1] = asin(x[1])*180.0/PI;
    }
    
    if(flag=0,!insidePolygonTree(polyTree,x0,x,&poly_id)) flag = 1;
    switch (para->format){
    case 1://Only objects outside the mask
      if(flag) fprintf(fileOut,"%f %f\n",x[0],x[1]);
      break;
    case 2: //Only objects inside the mask
      if(!flag) fprintf(fileOut,"%f %f\n",x[0],x[1]);
      break;
    case 3://All objects with the flag
      fprintf(fileOut,"%f %f %d\n",x[0],x[1],flag);
    }
  }
  fflush(stdout);
  fprintf(stderr,"\b\b\b\b100%%\n");

  return;
}


int readParameters(int argc, char **argv, Config *para){
  int i,task,nomask;
    
  //default parameters
  nomask = 1;
  task   = 1;
  para->nx = 512;
  para->ny = 512;
  para->xcol = 1;
  para->ycol = 2;
  para->npart = 1000000;
  para->format = 1;
  para->coordType = CART;
    
  for(i=0;i<2;i++){
    para->minDefinied[i] = 0;
    para->maxDefinied[i] = 0;
    para->min[i] = 0.0;
    para->max[i] = 0.0;
  }
  
  strcpy(para->fileOutName,"");
  strcpy(para->fileCatInName,"\0");
  strcpy(para->fileRegInName,"\0");
  

  for(i=0;i<argc;i++){
    //Help-------------------------------------------------------------------//
    if(!strcmp(argv[i],"-h") || !strcmp(argv[i],"--help") || argc == 1){
      fprintf(stderr,"\n\n                   V E N I C E\n\n");
      fprintf(stderr,"           mask utility program version 3.04 \n\n");
      fprintf(stderr,"Usage: %s -m mask.reg [OPTIONS]\n",argv[0]);
      fprintf(stderr,"    or %s -m mask.reg -cat file.cat [OPTIONS]\n",argv[0]);
      fprintf(stderr,"    or %s -m mask.reg -r [OPTIONS]\n",argv[0]);
      fprintf(stderr,"Notice: 0 means inside the mask, 1 outside\n");
      exit(-1);
    }
    //Polygon file in--------------------------------------------------------//
    if(!strcmp(argv[i],"-m")){
      if(argv[i+1] == NULL){
	fprintf(stderr,"Missing argument after %s\nExiting...\n",argv[i]);
	exit(-1);
      }
      strcpy(para->fileRegInName,argv[i+1]);
      nomask = 0;
    }
    //Input catalogue (if -cat set)------------------------------------------//
    if(!strcmp(argv[i],"-cat")){
      if(argv[i+1] == NULL){
	fprintf(stderr,"Missing argument after %s\nExiting...\n",argv[i]);
	exit(-1);
      }
      strcpy(para->fileCatInName,argv[i+1]);
      task = 2;
    }
    //Random catalogue-------------------------------------------------------//
    if(!strcmp(argv[i],"-r")){
      task = 3;
    }
    //Output file------------------------------------------------------------//
    if(!strcmp(argv[i],"-o")){
      if(argv[i+1] == NULL){
	fprintf(stderr,"Missing argument after %s\nExiting...\n",argv[i]);
	exit(-1);
      }
      strcpy(para->fileOutName,argv[i+1]);
    }
    //Dimensions of the pixel mask--------------------------------------------//
    if(!strcmp(argv[i],"-nx")){
      if(argv[i+1] == NULL){
	fprintf(stderr,"Missing argument after %s\nExiting...\n",argv[i]);
	exit(-1);
      }
      para->nx = atoi(argv[i+1]);
    }
    if(!strcmp(argv[i],"-ny")){
      if(argv[i+1] == NULL){
	fprintf(stderr,"Missing argument after %s\nExiting...\n",argv[i]);
	exit(-1);
      }
      para->ny = atoi(argv[i+1]);
    }
    //Column ids for the coordinates in FILE_CAT_IN--------------------------//
    if(!strcmp(argv[i],"-xcol")){
      if(argv[i+1] == NULL){
	fprintf(stderr,"Missing argument after %s\nExiting...\n",argv[i]);
	exit(-1);
      }
      para->xcol = atoi(argv[i+1]);
    }
    if(!strcmp(argv[i],"-ycol")){
      if(argv[i+1] == NULL){
	fprintf(stderr,"Missing argument after %s\nExiting...\n",argv[i]);
	exit(-1);
      }
      para->ycol = atoi(argv[i+1]);
    }
    //NPART for the random catalogue------------------------------------------//
    if(!strcmp(argv[i],"-npart")){
      if(argv[i+1] == NULL){
	fprintf(stderr,"Missing argument after %s\nExiting...\n",argv[i]);
	exit(-1);
      }
      para->npart = atoi(argv[i+1]);
    }
    //OPTION for the random catalogue-----------------------------------------//
    if(!strcmp(argv[i],"-f")){
      if(argv[i+1] == NULL){
	fprintf(stderr,"Missing argument after %s\nExiting...\n",argv[i]);
	exit(-1);
      }
      if(!strcmp(argv[i+1],"outside")) para->format = 1;
      if(!strcmp(argv[i+1],"inside")) para->format = 2;
      if(!strcmp(argv[i+1],"all")) para->format = 3;
    }
    //Limits for the random catalogue-----------------------------------------//
    if(!strcmp(argv[i],"-xmin")){
      if(argv[i+1] == NULL){
	fprintf(stderr,"Missing argument after %s\nExiting...\n",argv[i]);
	exit(-1);
      }
      para->minDefinied[0] = 1;
      para->min[0] = atof(argv[i+1]);
    }
    if(!strcmp(argv[i],"-xmax")){
      if(argv[i+1] == NULL){
	fprintf(stderr,"Missing argument after %s\nExiting...\n",argv[i]);
	exit(-1);
      }
      para->maxDefinied[0] = 1;
      para->max[0] = atof(argv[i+1]);
    }
    if(!strcmp(argv[i],"-ymin")){
      if(argv[i+1] == NULL){
	fprintf(stderr,"Missing argument after %s\nExiting...\n",argv[i]);
	exit(-1);
      }
      para->minDefinied[1] = 1;
      para->min[1] = atof(argv[i+1]);
    }
    if(!strcmp(argv[i],"-ymax")){
      if(argv[i+1] == NULL){
	fprintf(stderr,"Missing argument after %s\nExiting...\n",argv[i]);
	exit(-1);
      }
      para->maxDefinied[1] = 1;
      para->max[1] = atof(argv[i+1]); 
    }
    if(!strcmp(argv[i],"-coord")) {
      if(!strcmp(argv[i+1],"spher")) para->coordType = RADEC;
      if(!strcmp(argv[i+1],"cart")) para->coordType = CART;
    }
  }
  //If no mask file is provided-----------------------------------------//
  
  if (nomask){
    //Checks if all the limits are definied in this case;
    if(task == 3 && (para->minDefinied[0] + para->minDefinied[1] + para->maxDefinied[0] + para->maxDefinied[1] < 4)) {
      fprintf(stderr,"If you want to generate a random catalogue with no mask,\n");
      fprintf(stderr,"please provide all the coordinate limits:\n");
      fprintf(stderr,"%s -r -xmin value -xmax value -ymin value -ymax value [OPTIONS]\n",argv[0]);
      exit(-1);
    }
    //Checks if the limits are realistics (min < max)
    if(task == 3 && ( para->min[0] > para->max[0] || para->min[1] > para->max[1])){
      fprintf(stderr,"Please put realistic limits (xmin < xmax and ymin < ymax).\n");
      exit(-1);
    }
    if(task != 3){
      fprintf(stderr,"please provide a mask file.\n");
      fprintf(stderr,"Usage: %s -m mask.reg [OPTIONS]\n",argv[0]);
      fprintf(stderr,"    or %s -m mask.reg -cat file.cat [OPTIONS]\n",argv[0]);
      fprintf(stderr,"    or %s -m mask.reg -r [OPTIONS]\n",argv[0]);
      exit(-1); 
    }
  }
  //----------------------------------------------------------------------//

  return task;
}

/*----------------------------------------------------------------*
 *Utils - geometric                                               *
 *----------------------------------------------------------------*/

int insidePolygonTree(Node *polyTree, double x0[2], double x[2], int *poly_id){
  /*Returns 1 if the point (x,y) is inside one of the polygons in
    polys. Returns 0 if the object is oustide of any polygon or outside the 
    mask limits. See insidePolygonTree() for the algorithm explanations.*/
  
  int i,j,k,Ncross,result;
  double s,t,D; 
  
  if(polyTree->Npolys == 0){
    *poly_id = -1;
    return 0;
  }

  if(polyTree->type != LEAF){
    if(x[polyTree->SplitDim] < polyTree->SplitValue){
      result = insidePolygonTree(polyTree->Left, x0, x, poly_id);
    }else{
      result = insidePolygonTree(polyTree->Right, x0, x, poly_id);
    }
  }else{
    Polygon *polys = (Polygon *)polyTree->polysAll;
    for(k=0;k<polyTree->Npolys;k++){
      i = polyTree->poly_id[k];
      if(polys[i].xmin[0] < x[0] && x[0] < polys[i].xmax[0] && polys[i].xmin[1] < x[1] && x[1] < polys[i].xmax[1]){
	//the object is inside the square around the polygon
	Ncross=0;
	for(j=0;j<polys[i].N;j++){
	  if(j<polys[i].N-1){
	    D = (polys[i].x[j+1]-polys[i].x[j])*(x[1]-x0[1])-(polys[i].y[j+1]-polys[i].y[j])*(x[0]-x0[0]);
	    s = ((x[0]-x0[0])*(polys[i].y[j]-x[1])-(x[1]-x0[1])*(polys[i].x[j]-x[0]))/D;
	    t = ((polys[i].x[j]-x[0])*(polys[i].y[j+1]-polys[i].y[j])-(polys[i].y[j]-x[1])*(polys[i].x[j+1]-polys[i].x[j]))/D;
	  }else{
	    D = (polys[i].x[0]-polys[i].x[j])*(x[1]-x0[1])-(polys[i].y[0]-polys[i].y[j])*(x[0]-x0[0]);
	    s = ((x[0]-x0[0])*(polys[i].y[j]-x[1])-(x[1]-x0[1])*(polys[i].x[j]-x[0]))/D;
	    t = ((polys[i].x[j]-x[0])*(polys[i].y[0]-polys[i].y[j])-(polys[i].y[j]-x[1])*(polys[i].x[0]-polys[i].x[j]))/D;
	  } 
	  if(0.0 < s && s < 1.0 + EPS && 0.0 < t && t < 1.0 + EPS) Ncross++;	
	}
	if(GSL_IS_ODD(Ncross)){
	  *poly_id = i;
	  return 1;
	}
      }
    }
    *poly_id = -1;
    return 0;
  }
  
  return result;
}

int insidePolygon(Polygon *polys, int Npolys, double x0, double y0, double x, double y, int *poly_id){
  /*Returns 1 if the point (x,y) is inside one of the polygons in
    polys. The id of the first polygon in which the point is 
    found is also returned in poly_id. If the point is found to be outside 
    all polygons, the function returns 0 and poly_id is set to -1.
    The function first checks if the point is inside the square drawn 
    by the extrema of each polygon ("computationaly" more effecient). 
    If yes, it counts how many times the segment {(x0,y0),(x,y)} crosses 
    the sides of the polygon (Ncross). (x,y) inside the polygon 
    implies Ncross is an odd number if the point (x0,y0) is outside the polygon
    (then the point (x0,y0) must be chosen outside any polygon).*/
  int i,j,Ncross;
  double s,t,D;

  
  for(i=0;i<Npolys;i++){
   
    if(polys[i].xmin[0] < x && x < polys[i].xmax[0] && polys[i].xmin[1] < y && y < polys[i].xmax[1]){
      //the object is inside the square around the polygon
      Ncross=0;
      for(j=0;j<polys[i].N;j++){
	if(j<polys[i].N-1){
	  D = (polys[i].x[j+1]-polys[i].x[j])*(y-y0)-(polys[i].y[j+1]-polys[i].y[j])*(x-x0);
	  s = ((x-x0)*(polys[i].y[j]-y)-(y-y0)*(polys[i].x[j]-x))/D;
	  t = ((polys[i].x[j]-x)*(polys[i].y[j+1]-polys[i].y[j])-(polys[i].y[j]-y)*(polys[i].x[j+1]-polys[i].x[j]))/D;
	}else{
	  D = (polys[i].x[0]-polys[i].x[j])*(y-y0)-(polys[i].y[0]-polys[i].y[j])*(x-x0);
	  s = ((x-x0)*(polys[i].y[j]-y)-(y-y0)*(polys[i].x[j]-x))/D;
	  t = ((polys[i].x[j]-x)*(polys[i].y[0]-polys[i].y[j])-(polys[i].y[j]-y)*(polys[i].x[0]-polys[i].x[j]))/D;
	} 
	if(0.0 < s && s < 1.0 + EPS && 0.0 < t && t < 1.0 + EPS) Ncross++;	
      }
      if(GSL_IS_ODD(Ncross)){
	*poly_id = i;
	return 1;
      }
    }
  }
  *poly_id = -1;
  return 0;
}

Polygon *readPolygonFile(FILE *fileIn, int *Npolys, Node *polyTree){
  
  Polygon *result = malloc(sizeof(Polygon));
  return result;


}

Node *readPolygonFileTree(FILE *fileIn, double xmin[2], double xmax[2]){
  /*Reads the file file_in and returns the polygons tree.*/
  char line[NFIELD*NCHAR], item[NFIELD*NCHAR],*str_begin,*str_end;
  int i,j;
  size_t N, NpolysAll;
  
  NpolysAll = 0;
  //Read the entire file and count the total number of polygons, NpolysAll.
  while(fgets(line,NFIELD*NCHAR,fileIn) != NULL)
    if(strstr(line,"polygon") != NULL) NpolysAll += 1;
  rewind(fileIn);
  Polygon *polysAll = malloc(NpolysAll*sizeof(Polygon));
  
  i=0;
  //Read the file and fill the array with polygons.
  while(fgets(line,NFIELD*NCHAR,fileIn) != NULL){
    if(strstr(line,"polygon") != NULL){
      str_begin = strstr(line,"(")+sizeof(char);
      str_end = strstr(line,")");
      strcpy(str_end,"\n\0");
      strcpy(line,str_begin);
      getStrings(line,item,",",&N);
      //------------------------------//
      //get all coordinates separated by comas.
      polysAll[i].N = N/2;
      if(N/2 > NVERTICES){
	fprintf(stderr,"%s: %zd = too many points for polygon %d (%d maxi). Exiting...\n",MYNAME,N/2,i,NVERTICES);
	exit(EXIT_FAILURE);
      }      
      polysAll[i].id      = i;
      polysAll[i].xmin[0] = atof(item);
      polysAll[i].xmax[0] = atof(item);
      polysAll[i].xmin[1] = atof(item+NCHAR);
      polysAll[i].xmax[1] = atof(item+NCHAR);
      for(j=0;j<N/2;j++){
	polysAll[i].x[j] = atof(item+NCHAR*2*j);
	polysAll[i].y[j] = atof(item+NCHAR*(2*j+1));
	polysAll[i].xmin[0] = MIN(polysAll[i].xmin[0], polysAll[i].x[j]);
	polysAll[i].xmax[0] = MAX(polysAll[i].xmax[0], polysAll[i].x[j]);
	polysAll[i].xmin[1] = MIN(polysAll[i].xmin[1], polysAll[i].y[j]);
	polysAll[i].xmax[1] = MAX(polysAll[i].xmax[1], polysAll[i].y[j]);
      }
      i++;
      //------------------------------//
    }
  }
  if(i==0){
    fprintf(stderr,"%s: 0 polygon found, check input file. Exiting...\n",MYNAME);
    exit(EXIT_FAILURE);
  }
  
  double minArea;
  xmin[0] = polysAll[0].xmin[0];
  xmax[0] = polysAll[0].xmax[0];
  xmin[1] = polysAll[0].xmin[1];
  xmax[1] = polysAll[0].xmax[1];
  minArea = (polysAll[0].xmax[0] - polysAll[0].xmin[0])*(polysAll[0].xmax[1] - polysAll[0].xmin[1]);
  for(i=1;i<NpolysAll;i++){
    xmin[0] = MIN(xmin[0],polysAll[i].xmin[0]);
    xmax[0] = MAX(xmax[0],polysAll[i].xmax[0]);
    xmin[1] = MIN(xmin[1],polysAll[i].xmin[1]);
    xmax[1] = MAX(xmax[1],polysAll[i].xmax[1]);
    minArea += (polysAll[i].xmax[0] - polysAll[i].xmin[0])*(polysAll[i].xmax[1] - polysAll[i].xmin[1]);
  }
  minArea /= 1.0*(double)NpolysAll;
  
  int SplitDim = 0, firstCall = 1;
  return createNode(polysAll,NpolysAll, minArea, SplitDim, xmin, xmax, firstCall); 
}

Node *createNode(Polygon *polys, size_t Npolys, double minArea, int SplitDim, double xmin[2], double xmax[2], int firstCall){
  size_t i,j,SplitDim_new;
  
  //Allocate memory for THIS node
  Node *result = (Node *)malloc(sizeof(Node));
  static size_t countNodes, NpolysAll;
  static void   *root, *polysAll;
  
  //Root & node id
  if(firstCall){
    root         = result;
    countNodes   = 0;
    polysAll     = polys;
    NpolysAll    = Npolys;
  }
  result->root      = root;
  result->id        = countNodes;
  result->Npolys    = Npolys;
  result->NpolysAll = NpolysAll;
  countNodes++;
  
  //Copy address of the complete polygon sample and 
  //save ids of polygons inside the node
  result->polysAll     = polysAll;
  result->poly_id      = (int *)malloc(Npolys*sizeof(int));
  for(i=0;i<Npolys;i++){
    result->poly_id[i] = polys[i].id;
  }  
  
  double area = (xmax[0]-xmin[0])*(xmax[1]-xmin[1]);
  //Leaf: either no polygon or cell smaller than minArea  
  if(result->Npolys == 0 || area < minArea) {
    result->type     = LEAF;
    result->Left     = NULL;
    result->Right    = NULL;
  }else{    
    result->type       = NODE;
    result->SplitDim   = SplitDim;
    result->SplitValue = (xmax[result->SplitDim] + xmin[result->SplitDim])/2.0;

    //Temporary data
    Polygon *polysChild = malloc(Npolys*sizeof(Polygon));
    double xminChild[2], xmaxChild[2];
    for(i=0;i<2;i++){
      xminChild[i] = xmin[i];
      xmaxChild[i] = xmax[i];
    }
    
    //New splitDim for children
    SplitDim++;
    if(SplitDim > 1)  SplitDim = 0;
    
    //"left" -------------------
    //Set new right limit
    xmaxChild[result->SplitDim] = result->SplitValue;
    j=0;
    for(i=0;i<Npolys;i++){
      if(polys[i].xmin[result->SplitDim] < result->SplitValue){
	cpyPolygon(&polysChild[j],&polys[i]);
	j++;
      }
    } 
    result->Left = createNode(polysChild,j,minArea,SplitDim,xminChild,xmaxChild,0);
    
    //"right" -------------------
    //Restore right limit and set new left limit
    xmaxChild[result->SplitDim] = xmax[result->SplitDim];
    xminChild[result->SplitDim] = result->SplitValue;
    j=0;
    for(i=0;i<Npolys;i++){
      if(polys[i].xmax[result->SplitDim] > result->SplitValue){
	cpyPolygon(&polysChild[j],&polys[i]);
	j++;
      }
    } 
    result->Right = createNode(polysChild,j,minArea,SplitDim,xminChild,xmaxChild,0);
    
    free_Polygon(polysChild,Npolys);
  }
  
  result->Nnodes=countNodes;
  
  return result;
}

void free_Polygon(Polygon *polygon, size_t N){
  size_t i;
  for(i=0;i<N;i++){
    //free(polygon->x);
    //free(polygon->y);
    //free(polygon->xmin);
    //free(polygon->xmax);
  }
  free(polygon);
}

void free_Node(Node *node){
  if(node->type == LEAF){
    free(node->poly_id);
    free(node);
  }else{
    free_Node(node->Left);
    free_Node(node->Right);
  }
  return;
}

void cpyPolygon(Polygon *a, Polygon *b){
  /*Copies b into a*/
  size_t i;
  
  a->N  = b->N;
  a->id = b->id;
  for(i=0;i<NVERTICES;i++){
    a->x[i] = b->x[i];
    a->y[i] = b->y[i];
  }
  for(i=0;i<2;i++){
    a->xmin[i] = b->xmin[i];
    a->xmax[i] = b->xmax[i];
  }
}


/*----------------------------------------------------------------*
 *Utils - numeric                                                 *
 *----------------------------------------------------------------*/

gsl_rng *randomInitialize(int time_dependent){
  /*Define here which type of random generator you want. Set 
    time_dependent to 1 if you want the seed to change each time
    you run the program (change every 1 s). */
  
  /*
    WARNING: the time_dependent option actually doesn't work yet :-(, sorry !
    To initialize with a different seed, type :
    tcsh: setenv GSL_RNG_SEED `date +%s`
    bash: export GSL_RNG_SEED=`date +%s`
  */
  //time_dependent option
  //if(time_dependent) system("export GSL_RNG_SEED=20091982");
  //else system("export GSL_RNG_SEED=20091982");
 
  
  //Random number generator
  const gsl_rng_type *T;
  gsl_rng *r;
  gsl_rng_env_setup();
  T = gsl_rng_default;
  r = gsl_rng_alloc(T);
  
  return r;
}


double determineMachineEpsilon()
{
  double u, den;
  
  u = 1.0;
  do {
    u /= 2.0;
    den = 1.0 + u;
  } while(den>1.0);

  return(100.0 * u);
}

size_t determineSize_tError(){
  /*Equals size_t_max in fact.*/
  size_t count=0;
  count--;
  //count = (size_t)(-1);
  return count;
}

FILE *fopenAndCheck(const char *fileName,char *mode){
  /*Checks if fileName exists and opens it. Exits otherwise.;*/
  
  if(!(strcmp(mode,"w")) && !(strcmp(fileName,""))){
    return stdout;
  }
  
  FILE *fileTmp = fopen(fileName,mode);
  
  if (fileTmp == NULL){
    fprintf(stderr,"%s: %s not found. Exiting...\n",MYNAME,fileName);
    exit(EXIT_FAILURE);    
  }
  return fileTmp;
}

int getStrings(char *line, char *strings, char *delimit, size_t *N){
  /*Extract each word/number in line separated by delimit and returns 
    the array of items in strings.*/
  int i,j,begin,length;
  
  if(line == NULL || line[0] == '\n' || line[0] == '#' || line[0] == '\0') return 0;
  
  i = 0;
  j = 0;
  while(line[i] != '\0' && line[i] != '#' && line[i] != '\n'){
    begin = i;
    while(line[i] == *delimit || line[i] == '\t' && (line[i] != '\0' || line[i] != '#' || line[i] != '\n')) i++;
    begin = i;
    while(line[i] != *delimit && line[i] != '\t' && line[i] != '\0' && line[i] != '#' && line[i] != '\n') i++;
    length = i - begin;
    if(length > 0){
      strncpy(strings+NCHAR*j,&line[begin],length);
      strcpy(strings+NCHAR*j+length,"\0");
      j++;
    }
  }
 
  (*N) = j;

  if(*N > 0){
    return 1;
  }else{
    return 0;
  }
}


void printCount(const size_t *count, const size_t *total, const size_t step){
  if((*count)%step == 0){
    fflush(stdout);
    fprintf(stderr,"\b\b\b\b%3.0f%%",100.0*(double)(*count)/(double)(*total));
  }
  return;
}

