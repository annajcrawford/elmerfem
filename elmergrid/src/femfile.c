#include <stdio.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "nrutil.h"
#include "common.h"
#include "femdef.h"
#include "femtools.h"
#include "femtypes.h"
#include "femknot.h"
#include "femfile.h"

#define getline fgets(line,MAXLINESIZE,in) 


char line[MAXLINESIZE];

static int Getrow(char *line1,FILE *io,int upper) 
{
  int i,isend;
  char line0[MAXLINESIZE],*charend;

  for(i=0;i<MAXLINESIZE;i++) 
    line0[i] = ' ';

 newline:

  charend = fgets(line0,MAXLINESIZE,io);
  isend = (charend == NULL);

  if(isend) return(1);

  if(line0[0] == '#' || line0[0] == '!') goto newline;
  if(strstr(line0,"#")) goto newline;

  if(upper) {
    for(i=0;i<MAXLINESIZE;i++) 
      line1[i] = toupper(line0[i]);
  }
  else {
    for(i=0;i<MAXLINESIZE;i++) 
      line1[i] = line0[i];    
  }

  return(0);
}


static void FindPointParents(struct FemType *data,struct BoundaryType *bound,
			    int boundarynodes,int *nodeindx,int *boundindx,int info)
{
  int i,j,k,sideelemtype,elemind,parent,*indx;
  int boundarytype,minboundary,maxboundary,minnode,maxnode,sideelem,elemtype;
  int sideind[MAXNODESD1],elemsides,side,sidenodes,hit,nohits;
  int *elemhits;

  info = TRUE;

  sideelem = 0;
  maxboundary = minboundary = boundindx[1];
  minnode = maxnode = nodeindx[1]; 

  for(i=1;i<=boundarynodes;i++) {
    if(maxboundary < boundindx[i]) maxboundary = boundindx[i];
    if(minboundary > boundindx[i]) minboundary = boundindx[i];
    if(maxnode < nodeindx[i]) maxnode = nodeindx[i];
    if(minnode > nodeindx[i]) minnode = nodeindx[i];
  }

  if(info) {
    printf("Boundary types are in interval [%d, %d]\n",minboundary,maxboundary);
    printf("Boundary nodes are in interval [%d, %d]\n",minnode,maxnode);
  }
  indx = Ivector(1,data->noknots);

  elemhits = Ivector(1,data->noknots);
  for(i=1;i<=data->noknots;i++) elemhits[i] = 0;

  for(elemind=1;elemind<=data->noelements;elemind++) {
    elemtype = data->elementtypes[elemind];
    elemsides = elemtype % 100;
    for(i=0;i<elemsides;i++) 
      elemhits[data->topology[elemind][i]] += 1;
  }
    
 

  for(boundarytype=minboundary;boundarytype <= maxboundary;boundarytype++) {
    int boundfirst,bchits,bcsame,sideelemtype2;
    int sideind2[MAXNODESD1];

    boundfirst = 0;
  
    for(i=1;i<=data->noknots;i++) 
      indx[i] = 0;

    for(i=1;i<=boundarynodes;i++) {
      if(boundindx[i] == boundarytype) 
	indx[nodeindx[i]] = TRUE; 
    }

    for(elemind=1;elemind<=data->noelements;elemind++) {
      elemtype = data->elementtypes[elemind];
      elemsides = elemtype / 100;
      if(elemsides == 8) elemsides = 6;
      else if(elemsides == 5) elemsides = 4;
      else if(elemsides == 6) elemsides = 5;
      
      if(0) printf("ind=%d  type=%d  sides=%d\n",elemind,elemtype,elemsides);
 
      /* Check whether the bc nodes occupy every node in the selected side */
      for(side=0;side<elemsides;side++) {
	GetElementSide(elemind,side,1,data,&sideind[0],&sideelemtype);
	sidenodes = sideelemtype%100;
	
	if(0) printf("sidenodes=%d  side=%d\n",sidenodes,side);
	
	hit = TRUE;
	nohits = 0;
	for(i=0;i<sidenodes;i++) {
	  if(sideind[i] <= 0) { 
	    if(0) printf("sideind[%d] = %d\n",i,sideind[i]);
	    hit = FALSE;
	  }
	  else if(sideind[i] > data->noknots) { 
	    if(0) printf("sideind[%d] = %d (noknots=%d)\n",i,sideind[i],data->noknots);
	    hit = FALSE;
	  }
	  else if(!indx[sideind[i]]) {
	    hit = FALSE;
	  }
	  else {
	    nohits++;
	  }
	}
	
	if(0) {	  
	  printf("******\n");
	  printf("hits=%d  ind=%d  type=%d  sides=%d\n",nohits,elemind,elemtype,elemsides);
	  printf("sidenodes=%d  side=%d\n",sidenodes,side);

	  for(i=0;i<sidenodes;i++)
	    printf("%d  ",sideind[i]);

	  printf("\neleminds %d  %d\n",elemtype,elemind);
	  for(i=0;i<elemtype%100;i++)
	    printf("%d  ",data->topology[elemind][i]);
	  printf("\n");

	  hit = TRUE;
	}	  



	if(hit == TRUE) {

	  /* If all the points belong to more than one element there may be 
	     another parent for the side element */
	  bcsame = FALSE;
	  bchits = TRUE;
	  for(i=0;i<sidenodes;i++) 
	    if(elemhits[sideind[i]] < 2) bchits = FALSE;

	  if(bchits && boundfirst) {
	    for(j=boundfirst;j<=sideelem;j++) {
	      if(bound->parent2[j]) continue;
	      GetElementSide(bound->parent[j],bound->side[j],1,data,&sideind2[0],&sideelemtype2);
	      if(sideelemtype != sideelemtype2) continue;
	      bcsame = 0;
	      for(i=0;i<sidenodes;i++) 
		for(k=0;k<sidenodes;k++)
		  if(sideind[i] == sideind2[k]) bcsame++;

	      if(bcsame == sidenodes) {
		if(data->material[bound->parent[j]] > data->material[elemind]) {
		  bound->parent2[j] = bound->parent[j];
		  bound->side2[j] = bound->side[j];
		  bound->parent[j] = elemind;
		  bound->side[j] = side;
		}
		else {		  
		  bound->parent2[j] = elemind;
		  bound->side2[j] = side;
		}
		goto bcset;
	      }
	    }
	  }
		
	  sideelem += 1;
	  bound->parent[sideelem] = elemind;
	  bound->side[sideelem] = side;
	  bound->parent2[sideelem] = 0;
	  bound->side2[sideelem] = 0;
	  bound->types[sideelem] = boundarytype;

	  if(!boundfirst) boundfirst = sideelem;

	bcset:
	  continue;
	}
      }
    }    
  }
  
  free_Ivector(indx,1,data->noknots);

  if(info) printf("Found %d side elements formed by %d points.\n",
		  sideelem,boundarynodes);

  bound->nosides = sideelem;

  return;
}



int SaveAbaqusInput(struct FemType *data,char *prefix,int info)
/* Saves the grid in a format that can be read by ABAQUS 
   program designed for sructural mechanics. 
   The elementtype is set to be that of thermal conduction. 
   */
{
  int noknots,noelements,nonodes;
  char filename[MAXFILESIZE];
  int i,j;
  FILE *out;

  noknots = data->noknots;
  noelements = data->noelements;
  nonodes = data->maxnodes;

  if(nonodes != 4 && nonodes != 8) {
    printf("SaveAbaqusInput: not designed for %d-node elements\n",nonodes);
    return(1);
  }

  AddExtension(prefix,filename,"inp");
  out = fopen(filename,"w");

  if(info) printf("Saving ABAQUS data to %s.\n",filename);  

  fprintf(out,"*HEADING\n");
  fprintf(out,"Abaqus input file creator by Peter.Raback@csc.fi\n");

  fprintf(out,"*NODE, SYSTEM=");
  if(data->coordsystem == COORD_CART2) fprintf(out,"R\n");
  else if(data->coordsystem == COORD_AXIS) fprintf(out,"C\n");
  else if(data->coordsystem == COORD_POLAR) fprintf(out,"P\n");

  for(i=1; i <= noknots; i++) 
    fprintf(out,"%8d, %12.4le, %12.4le, 0.0\n",i,data->x[i],data->y[i]);

  fprintf(out,"*ELEMENT,TYPE=");
  if(nonodes == 4) fprintf(out,"DC2D4 ");
  else if(nonodes == 8) fprintf(out,"DC2D8 ");
  else printf("SaveAbaqusInput: Not defined for %d-node elements\n",nonodes);

  fprintf(out,",ELSET=SETTI1\n");
  for(i=1;i<=noelements;i++) {
    fprintf(out,"%8d, ",i);
    for(j=1;j<nonodes;j++) 
      fprintf(out,"%6d, ",data->topology[i][j-1]);
    fprintf(out,"%6d\n",data->topology[i][nonodes-1]);
  }

  fprintf(out,"*SOLID SECTION, ELSET=SETTI1, MATERIAL=MAT1\n");

  fprintf(out,"*MATERIAL, NAME=MAT1\n");
  fprintf(out,"*CONDUCTIVITY\n");
  fprintf(out,"1.0\n");

  fprintf(out,"*RESTART, WRITE, FREQUENCY=1\n");
  fprintf(out,"*FILE FORMAT, ASCII\n");

  fprintf(out,"*STEP\n");
  fprintf(out,"*HEAT TRANSFER, STEADY STATE\n");
  fprintf(out,"*END STEP\n");

  fclose(out);

  if(info) printf("Wrote the mesh in ABAQUS restart format.\n");

  return(0);
}



int LoadAbaqusInput(struct FemType *data,struct BoundaryType *bound,
		    char *prefix,int info)
/* Load the grid from a format that can be read by ABAQUS 
   program designed for sructural mechanics. The commands
   understood are only those that IDEAS creates when saving
   results in ABAQUS format.
   */
{
  int noknots,noelements,elemcode,maxnodes,material;
  int mode,allocated,nvalue,maxknot,nosides;
  int boundarytype,boundarynodes;
  int *nodeindx,*boundindx;
  
  char filename[MAXFILESIZE];
  char line[MAXLINESIZE];
  int i,j,*ind;
  FILE *in;
  Real rvalues[MAXDOFS];
  int ivalues[MAXDOFS],ivalues0[MAXDOFS];


  strcpy(filename,prefix);
  if ((in = fopen(filename,"r")) == NULL) {
    AddExtension(prefix,filename,"inp");
    if ((in = fopen(filename,"r")) == NULL) {
      printf("LoadAbaqusInput: opening of the ABAQUS-file '%s' wasn't succesfull !\n",
	     filename);
      return(1);
    }
  }

  printf("Reading input from ABAQUS input file %s.\n",filename);
  InitializeKnots(data);

  allocated = FALSE;
  maxknot = 0;

  /* Because the file format doesn't provide the number of elements
     or nodes the results are read twice but registered only in the
     second time. */
omstart:

  mode = 0;
  maxnodes = 0;
  noknots = 0;
  noelements = 0;
  elemcode = 0;
  boundarytype = 0;
  boundarynodes = 0;
  material = 0;
  ivalues0[0] = ivalues0[1] = 0;

  for(;;) {
    /* getline; */

    if (Getrow(line,in,FALSE)) goto end;

    /* if(!line) goto end; */
    /* if(strstr(line,"END")) goto end; */

    if(strstr(line,"**")) {
      if(info && !allocated) printf("comment: %s",line);
    }
    else if(strrchr(line,'*')) {
      if(strstr(line,"HEAD")) {
	mode = 1;
      }
      else if(strstr(line,"NODE")) {
	if(strstr(line,"SYSTEM=R")) data->coordsystem = COORD_CART2;
	if(strstr(line,"SYSTEM=C")) data->coordsystem = COORD_AXIS;      
	if(strstr(line,"SYSTEM=P")) data->coordsystem = COORD_POLAR;      
	mode = 2;
      }
      else if(strstr(line,"ELEMENT")) {
	material++;
	if(strstr(line,"S3R"))
	  elemcode = 303;
	else if(strstr(line,"2D4") || strstr(line,"SP4") || strstr(line,"AX4") 
		|| strstr(line,"S4") || strstr(line,"CPE4")) 
	  elemcode = 404;
	else if(strstr(line,"2D8") || strstr(line,"AX8"))
	  elemcode = 408;
	else if(strstr(line,"3D8"))
	  elemcode = 808;
	else if(strstr(line,"3D4"))
	  elemcode = 504;
	else printf("Unknown element code: %s\n",line);
	
	if(maxnodes < elemcode%100) maxnodes = elemcode%100;
	mode = 3;
	if(0) printf("Loading elements of type %d starting from element %d.\n",
			elemcode,noelements);
      }
      else if(strstr(line,"BOUNDARY")) {
	boundarytype++;
	mode = 4;
      }
      else if(strstr(line,"CLOAD")) {
	boundarytype++;
	mode = 4;
      }
      else {
	if(!allocated) printf("unknown: %s",line);
	mode = 0;
      }
    }

    else if(mode) {  
      switch (mode) {
	
      case 1:
	if(info) printf("Loading Abacus input file:\n%s",line);
	break;
	
      case 2:
	nvalue = StringToReal(line,rvalues,MAXNODESD2+1,',');
	i = (int)(rvalues[0]+0.5);
	if(i == 0) continue;
	noknots++;
	if(allocated) {
	  ind[i] = noknots;
	  data->x[noknots] = rvalues[1];
	  data->y[noknots] = rvalues[2];
	  data->z[noknots] = rvalues[3];
	}
	else {
	  if(maxknot < i) maxknot = i;
	}
	break;
	
      case 3:
	noelements++;
	if(allocated) {
	  nvalue = StringToInteger(line,ivalues,MAXNODESD2+1,',');
	  data->elementtypes[noelements] = elemcode;
	  for(i=1;i<nvalue;i++)
	    data->topology[noelements][i-1] = ivalues[i];
	  data->material[noelements] = material;
	}
	break;

      case 4:
	nvalue = StringToInteger(line,ivalues,MAXNODESD2+1,',');

	if(ivalues[0] == ivalues0[0] & ivalues[1] != ivalues0[1]) continue;
	ivalues0[0] = ivalues[0];
	ivalues0[1] = ivalues[1];

	boundarynodes++;
	if(allocated) {
	  nodeindx[boundarynodes] = ivalues[0];
	  boundindx[boundarynodes] = boundarytype;
	}
	break;
      default:
	printf("Unknown case: %d\n",mode);
      }      
    }

  }    
  end:


  if(allocated == TRUE) {
    if(info) printf("The mesh was loaded from file %s.\n",filename);

    FindPointParents(data,bound,boundarynodes,nodeindx,boundindx,info);

    /* ABAQUS format does not expect that all numbers are used
       when numbering the elements. Therefore the nodes must
       be renumberred from 1 to noknots. */

    if(noknots != maxknot) {
      if(info) printf("There are %d nodes but maximum index is %d.\n",
		      noknots,maxknot);
      if(info) printf("Renumbering elements\n");
      for(j=1;j<=noelements;j++) 
	for(i=0;i < data->elementtypes[j]%100;i++) 
	  data->topology[j][i] = ind[data->topology[j][i]];
    }

    free_ivector(ind,1,maxknot);
    free_Ivector(nodeindx,1,boundarynodes);
    free_Ivector(boundindx,1,boundarynodes);

    fclose(in);
    return(0);
  }

  rewind(in);
  data->noknots = noknots;
  data->noelements = noelements;
  data->maxnodes = maxnodes;
  data->dim = 3;
  
  if(info) printf("Allocating for %d knots and %d %d-node elements.\n",
		  noknots,noelements,maxnodes);
  AllocateKnots(data);

  nosides = 2*boundarynodes;
  printf("There are %d boundary nodes, thus allocating %d elements\n",
	 boundarynodes,nosides);
  AllocateBoundary(bound,nosides);
  nodeindx = Ivector(1,boundarynodes);
  boundindx = Ivector(1,boundarynodes);

  ind = ivector(1,maxknot);
  for(i=1;i<=maxknot;i++)
    ind[i] = 0;
    
  allocated = TRUE;
  goto omstart;
}


static int ReadAbaqusField(FILE *in,char *buffer,int *argtype,int *argno)
/* This subroutine reads the Abaqus file format and tries to make
   sence out of it. 
   */
{
  int i,val,digits;
  static int maxargno=0,mode=0;

  val = fgetc(in);

  if(val==EOF) return(-1);
  if(val=='\n') val = fgetc(in);

  if(val=='*') {
    if(0) printf("start field\n");
    if((*argno) != maxargno) 
      printf("The previous field was of wrong length, debugging time!\n");
    (*argno) = 0;
    mode = 0;
    val = fgetc(in);
    if(val=='\n') val = fgetc(in);
  }

  if(val=='I') {
    for(i=0;i<2;i++) {
      val = fgetc(in);
      if(val=='\n') val = fgetc(in);	
      buffer[i] = val;
    }
    buffer[2] = '\0';
    digits = atoi(buffer);
    for(i=0;i<digits;i++) {
      val = fgetc(in);
      if(val=='\n') val = fgetc(in);
      buffer[i] = val;
    }
    buffer[digits] = '\0';
    (*argno)++;
    (*argtype) = 1;
    if((*argno) == 1) maxargno = atoi(buffer);
    if((*argno) == 2) mode = atoi(buffer);
  }   
  else if(val=='D') {
    for(i=0;i<22;i++) {
      val = fgetc(in);
      if(val=='\n') val = fgetc(in);
      if(val=='D') val = 'E';
      buffer[i] = val;
    }
    buffer[22] = '\0';
    (*argno)++;
    (*argtype) = 2;
  }
  else if(val=='A') {
    for(i=0;i<8;i++) {
      val = fgetc(in);
      if(val=='\n') val = fgetc(in);
      buffer[i] = val;
    }
    buffer[8] = '\0';
    (*argno)++;
    (*argtype) = 3;
  }
  else {
    buffer[0] = val;
    buffer[1] = '\0';
    (*argtype) = 0;
  }
  return(mode);
}



int LoadAbaqusOutput(struct FemType *data,char *prefix,int info)
/* Load the grid from a format that can be read by ABAQUS 
   program designed for sructural mechanics. 
   */
{
  int knotno,elemno,elset,secno;
  int argtype,argno;
  int mode,allocated;
  char filename[MAXFILESIZE];
  char buffer[MAXLINESIZE];
  int i,j,prevdog,nodogs;
  int ignored;
  FILE *in;
  int *indx;


  AddExtension(prefix,filename,"fil");

  if ((in = fopen(filename,"r")) == NULL) {
    printf("LoadAbaqusOutput: opening of the Abaqus-file '%s' wasn't succesfull !\n",
	   filename);
    return(1);
  }
  if(info) printf("Reading input from ABAQUS output file %s.\n",filename);
  InitializeKnots(data);

  allocated = FALSE;
  mode = 0;
  knotno = 0;
  elemno = 0;
  nodogs = 0;
  prevdog = 0;
  argno = 0;
  elset = 0;
  secno = 0;
  ignored = 0;
  data->maxnodes = 9;
  data->dim = 3;

  for(;;) {

    mode = ReadAbaqusField(in,buffer,&argtype,&argno);
    if(0) printf("%d %d: buffer: %s\n",argtype,argno,buffer);

    switch (mode) {

    case -1:
      goto jump;
      
    case 0:
      break;

    case 1921:
      /* General info */
      if(argno == 3) printf("Reading output file for Abaqus %s\n",buffer);
      else if(argno == 4) printf("Created on %s",buffer);
      else if(argno == 5) printf("%s",buffer);
      else if(argno == 6) printf("%s\n",buffer);
      else if(argno == 7) data->noelements = atoi(buffer);
      else if(argno == 8 && allocated == FALSE) {
	data->noknots = atoi(buffer);
	allocated = TRUE;
	AllocateKnots(data);
	indx = Ivector(0,2 * data->noknots);
	for(i=1;i<=2*data->noknots;i++)
	  indx[i] = 0;
      }
      break;
      
    case 1900:
      /* Element definition */
      if(argno == 3) elemno = atoi(buffer);
      else if(argno == 4) {
	if(strstr(buffer,"2D4") || strstr(buffer,"SP4") || strstr(buffer,"AX4")) 
	  data->elementtypes[elemno] = 404;
	else if(strstr(buffer,"2D8") || strstr(buffer,"AX8") || strstr(buffer,"S8R5"))
	  data->elementtypes[elemno] = 408;
	else if(strstr(buffer,"3D8"))
	  data->elementtypes[elemno] = 808;
	else printf("Unknown element code: %s\n",buffer);
      } 
      else if(argno >= 5)
	data->topology[elemno][argno-5] = atoi(buffer);
      break;

    case 1901:
      /* Node definition */
      if(argno == 3) {
	knotno++;
	if(atoi(buffer) > 2*data->noknots) 
	  printf("LoadAbaqusOutput: allocate more space for indx.\n");
	else 
	  indx[atoi(buffer)] = knotno;
      }
      if(argno == 4) sscanf(buffer,"%le",&(data->x[knotno]));
      if(argno == 5) sscanf(buffer,"%le",&(data->y[knotno]));
      if(argno == 6) sscanf(buffer,"%le",&(data->z[knotno]));
      break;

    case 1933:
      /* Element set */
      if(argno == 3) { 
	elset++;
	strcpy(data->materialname[elset],buffer);
      }
    case 1934:
      /* Element set continuation */
      if(argno > 3) {
	elemno = atoi(buffer);
	data->material[elemno] = elset;
      }
      break;

    case 2001:
      /* Just ignore */
      break;

    case 1:
      if(argno == 3) knotno = indx[atoi(buffer)];
      if(argno == 5) secno = atoi(buffer);
      break;

    case 2:
      if(prevdog != mode) {
	prevdog = mode;
	nodogs++;
	CreateVariable(data,nodogs,1,0.0,"Temperature",FALSE);
      }
      break;

    /* Read vectors in nodes in elements */
    case 11:
      if(prevdog != mode) {
	prevdog = mode;
	nodogs++;
	CreateVariable(data,nodogs,3,0.0,"Stress",FALSE);
      }
    case 12:
      if(prevdog != mode) {
	prevdog = mode;
	nodogs++;
	CreateVariable(data,nodogs,3,0.0,"Invariants",FALSE);
      }
      if(secno==1 && argno == 3) sscanf(buffer,"%le",&(data->dofs[nodogs][3*knotno-2]));
      if(secno==1 && argno == 4) sscanf(buffer,"%le",&(data->dofs[nodogs][3*knotno-1]));
      if(secno==1 && argno == 5) sscanf(buffer,"%le",&(data->dofs[nodogs][3*knotno]));
      break;

    /* Read vectors in nodes. */ 
    case 101:
      if(prevdog != mode) {
	prevdog = mode;
	nodogs++;
	CreateVariable(data,nodogs,3,0.0,"Displacement",FALSE);
      }
    case 102:
      if(prevdog != mode) {
	prevdog = mode;
	nodogs++;
	CreateVariable(data,nodogs,3,0.0,"Velocity",FALSE);
      }
      if(argno == 3) knotno = indx[atoi(buffer)]; 
      if(argno == 4) sscanf(buffer,"%le",&(data->dofs[nodogs][3*knotno-2]));
      if(argno == 5) sscanf(buffer,"%le",&(data->dofs[nodogs][3*knotno-1]));
      if(argno == 6) sscanf(buffer,"%le",&(data->dofs[nodogs][3*knotno]));
      break;

    default:
      if(ignored != mode) {
	printf("Record %d was ignored!\n",mode);
	ignored = mode;
      }
      break;
    }
  }

jump:

  if(info) printf("Renumbering elements\n");
  for(j=1;j<=data->noelements;j++) 
    for(i=0;i < data->elementtypes[j]%100;i++) 
      data->topology[j][i] = indx[data->topology[j][i]];

  free_ivector(indx,0,2*data->noknots);

  fclose(in);

  if(info) printf("LoadAbacusInput: results were loaded from file %s.\n",filename);

  return(0);
}




static void ReorderFidapNodes(struct FemType *data,int element,int nodes,int typeflag) 
{
  int i,j,oldtopology[MAXNODESD2],*topology,elementtype,dim;
  int order808[]={1,2,4,3,5,6,8,7};
  int order408[]={1,3,5,7,2,4,6,8};
  int order306[]={1,3,5,2,4,6};
  int order203[]={1,3,2};
  int order605[]={1,2,4,3,5};

  dim = data->dim;
  if(typeflag > 10) dim -= 1;

  data->elementtypes[element] = 101*nodes;
  topology = data->topology[element];

  if(dim == 1) {
    if(nodes == 3) {
      data->elementtypes[element] = 203;
      for(i=0;i<nodes;i++) 
	oldtopology[i] = topology[i];
      for(i=0;i<nodes;i++) 
	topology[i] = oldtopology[order203[i]-1];            
    }
  }
  else if(dim == 2) {
    if(nodes == 6) {
      data->elementtypes[element] = 306;
      for(i=0;i<nodes;i++) 
	oldtopology[i] = topology[i];
      for(i=0;i<nodes;i++) 
	topology[i] = oldtopology[order306[i]-1];      
    }
    else if(nodes == 8) {
      data->elementtypes[element] = 408;
      for(i=0;i<nodes;i++) 
	oldtopology[i] = topology[i];
      for(i=0;i<nodes;i++) 
	topology[i] = oldtopology[order408[i]-1];      
    }
  }
  else if(dim == 3) {
    if(nodes == 4) {
      data->elementtypes[element] = 504;      
    }
    else if(nodes == 5) {
      data->elementtypes[element] = 605;      
      for(i=0;i<nodes;i++) 
	oldtopology[i] = topology[i];
      for(i=0;i<nodes;i++) 
	topology[i] = oldtopology[order605[i]-1];
    }
    else if(nodes == 6) {
      data->elementtypes[element] = 706;      
    }
    else if(nodes == 8) {
      for(i=0;i<nodes;i++) 
	oldtopology[i] = topology[i];
      for(i=0;i<nodes;i++) 
	topology[i] = oldtopology[order808[i]-1];
    }
    else {
      printf("Unknown Fidap elementtype with %d nodes.\n",nodes);
    }

  }
  else printf("ReorderFidapNodes: unknown dimension (%d)\n",data->dim);
}


int SaveFidapOutput(struct FemType *data,char *prefix,int info,
		    int vctrs,Real *vect1, ...)
/* This procedure saves the solution in a form that is understood by 
   the CFD program Fidap. The routine that reads this file seems
   to be stupidly place-dependent. The best manual for this 
   subroutine is provided in the appendix E of FIDAP users manual.
   */
{
  int noknots,noelements,dim,nonodes;
  int i,j,no,nogroup,material,cellelem;
  int elemcodes[MAT_MAXNUMBER+1],mat[MAT_MAXNUMBER+1];
  FILE *out;
  Real *dofs[MAXDOFS];
  char filename[MAXFILESIZE];
  va_list ap;

  if(!data->created) {
    printf("You tried to save points that were never created.\n");
    return(1);
  }

  printf("Saving results in FIDAP neutral format.\n");

  noknots = data->noknots;
  noelements = data->noelements;
  dim = data->dim;

  if(vctrs > 3) {
    printf("SaveSolutionFidap: Maximum of 3 d.o.f.\n");
    vctrs = 3;
  }

  /* Read the list of pointers to vectors to be saved. */
  if(vctrs > 0) {
    va_start(ap,vect1);
    dofs[1] = vect1;
    for(i=2;i<=vctrs;i++)
      dofs[i] = va_arg(ap,Real*);
    va_end(ap);
  }

  /* Create groups by placing the same materials to same groups. */
  nogroup = 0;
  for(i=1;i<=MAT_MAXNUMBER;i++) 
    elemcodes[i] = mat[i] = 0;
  for(j=1;j <= noelements;j++)  {
    material = data->material[j];
    if(material > 0 && material<=MAT_MAXNUMBER) mat[material] += 1;
    elemcodes[material] = data->elementtypes[j];
  } 
  for(i=1;i<=MAT_MAXNUMBER;i++) 
    if(mat[i] > 0) nogroup++;

  AddExtension(prefix,filename,"fidap");
  out = fopen(filename,"w");

  /* Control information */
  fprintf(out,"** FIDAP NEUTRAL FILE\n");
  fprintf(out,"Fidap input file creator by Peter.Raback@csc.fi\n");
  fprintf(out,"VERSION %7.2f\n",7.52); /* Fidap version */
  fprintf(out," 1 Dec 96    12:00:00\n");
  fprintf(out,"%15s%15s%15s%15s%15s\n","NO. OF NODES",
	  "NO. ELEMENTS","NO. ELT GROUPS","NDFCD","NDFVL");
  fprintf(out,"%15d%15d%15d%15d%15d\n",noknots,noelements,nogroup,dim,dim);
  fprintf(out,"%15s%15s%15s%15s\n",
	  "STEADY/TRANS","TURB. FLAG","FREE SURF FLAG","COMPR. FLAG");
  fprintf(out,"%15d%15d%15d%15d\n",0,0,0,0);
  fprintf(out,"%s\n","TEMPERATURE/SPECIES FLAGS");
  fprintf(out,"%s\n"," 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
  fprintf(out,"%s\n","PRESSURE FLAGS - IDCTS, IPENY MPDF");
  fprintf(out,"%10d%10d%10d\n",0,0,1);

  fprintf(out,"NODAL COORDINATES\n");
  for(i=1; i <= noknots; i++) 
    fprintf(out,"%10d%20.10le%20.10le\n",i,data->y[i],data->x[i]);

  /* Boundary and initial conditions */
  fprintf(out,"BOUNDARY CONDITIONS\n");
  fprintf(out,"%10d%10d%10d%20.10e\n",0,0,5,0.0);

  fprintf(out,"ELEMENT GROUPS\n");
  nogroup = 0;
  for(no=1;no<=MAT_MAXNUMBER;no++)
    if(mat[no] > 0) {
      nonodes = elemcodes[no]%100;
      nogroup++;
      cellelem = mat[no];
      fprintf(out,"GROUP:    %5d ELEMENTS:%10d NODES:   %10d GEOMETRY:%5d TYPE:%4d\n",
	      nogroup,cellelem,nonodes,1,1);
      fprintf(out,"ENTITY NAME:   %s%d\n","material",nogroup);

      for(j=1;j <= noelements;j++)  {
	if(data->material[j] == no) {    
	  fprintf(out,"%8d\n",j);
	  for(i=0;i < nonodes;i++)
	    fprintf(out,"%8d",data->topology[j][i]);
	  fprintf(out,"\n");
	}
      } 
    }    

  fprintf(out,"TIMESTEP: %5d TIME:     %15.7e INCRMNT: %15.7e\n",1,1.0,1.0);

  fprintf(out,"VELOCITY\n");            
  if(vctrs < 2) 
    for(i=1;i<=2*noknots;i++) {
      fprintf(out,"%16.9le",0.0);
      if(i%5==0) fprintf(out,"\n");
    }  
  else 
    for(i=1;i<=2*noknots;i++) {
      fprintf(out,"%16.9le",dofs[1][i]);
      if((2*i-1)%5 == 0) fprintf(out,"\n");
      fprintf(out,"%16.9le",dofs[2][i]);
      if((2*i)%5 == 0) fprintf(out,"\n");   
  }
  if((2*noknots)%5 != 0) fprintf(out,"\n");

  fprintf(out,"TEMPERATURE\n");

  if(vctrs == 2) 
    for(i=1;i<=noknots;i++) {
      fprintf(out,"%16.9le",0.0);
      if(i%5==0) fprintf(out,"\n");
    }
  else 
    for(i=1;i<=noknots;i++) {
      fprintf(out,"%16.9le",dofs[vctrs][i]);
      if(i%5==0) fprintf(out,"\n");
    }
  if(noknots%5 != 0) fprintf(out,"\n");
  fprintf(out,"ENDOFTIMESTEP\n");       

  fclose(out);
  if(info) printf("Results were saved in FIDAP neutral format to file %s.\n",filename);

  return(0);
}




int LoadFidapInput(struct FemType *data,char *prefix,int info)
/* Load the grid from a format that can be read by FIDAP 
   program designed for fluid mechanics. 

   Still under implementation
   */
{
  int noknots,noelements,dim,novel,elemcode,maxnodes;
  int mode,allocated,nvalue,maxknot,totelems,entity,maxentity;
  char filename[MAXFILESIZE];
  char line[MAXLINESIZE],*cp,entityname[MAXLINESIZE],entitylist[100][MAXLINESIZE];
  int i,j,k,*ind,geoflag,typeflag;
  int **topology;
  FILE *in;
  Real rvalues[MAXDOFS];
  Real *vel,*temp;
  int ivalues[MAXDOFS];
  int nogroups,knotno,nonodes,elemno;
  char *isio;

  AddExtension(prefix,filename,"fidap");

  if ((in = fopen(filename,"r")) == NULL) {
    AddExtension(prefix,filename,"FDNEUT");
    if ((in = fopen(filename,"r")) == NULL) {
      printf("LoadFidapInput: opening of the Fidap-file '%s' wasn't succesfull !\n",
	     filename);
      return(1);
    }
  }
 
  InitializeKnots(data);

  entity = 0;
  mode = 0;
  noknots = 0;
  noelements = 0;
  dim = 0;
  elemcode = 0;
  maxnodes = 4;
  totelems = 0;

  for(;;) {
    isio = getline;

    if(!isio) goto end;
    if(!line) goto end;
    if(line=="") goto end;
    if(strstr(line,"END")) goto end;

    /* Control information */
    if(strstr(line,"FIDAP NEUTRAL FILE")) mode = 1;
    else if(strstr(line,"NO. OF NODES")) mode = 2;
    else if(strstr(line,"TEMPERATURE/SPECIES FLAGS")) mode = 3;
    else if(strstr(line,"PRESSURE FLAGS")) mode = 4;
    else if(strstr(line,"NODAL COORDINATES")) mode = 5;
    else if(strstr(line,"BOUNDARY CONDITIONS")) mode = 6;
    else if(strstr(line,"ELEMENT GROUPS")) mode = 7;
    else if(strstr(line,"GROUP:")) mode = 8;
    else if(strstr(line,"VELOCITY")) mode = 10;
    else if(strstr(line,"TEMPERATURE")) mode = 11;
    else if(0) printf("unknown: %s",line);
    
    switch (mode) {

    case 1: 
      if(info) printf("Loading FIDAP input file:\n");
      getline;
      if(info) printf("Name of the case: %s",line);
      mode = 0;
      break;

    case 2:
      getline;   
      if(info) printf("reading the header info\n");
      sscanf(line,"%d%d%d%d%d",&noknots,&noelements,
	     &nogroups,&dim,&novel);
      data->noknots = noknots;
      data->noelements = noelements;
      data->maxnodes = maxnodes;
      data->dim = dim;

      mode = 0;
      break;
      
    case 5:
      if(info) printf("Allocating for %d knots and %d %d-node elements.\n",
		      noknots,noelements,maxnodes);
      AllocateKnots(data);
      if(info) printf("reading the nodes\n");
      for(i=1;i<=noknots;i++) {
	getline;
	if (dim == 2)
	  sscanf(line,"%d%le%le",&knotno,
		 &(data->x[i]),&(data->y[i]));
	else if(dim==3) 
	  sscanf(line,"%d%le%le%le",&knotno,
		 &(data->x[i]),&(data->y[i]),&(data->z[i]));
      }
      break;
      
    case 8: 
      {
	int val,group,elems,nodes;
	char *cp;

	i=0;
	do val=line[i++];
	while(val!=':');i++;
	sscanf(&line[i],"%d",&group);
	
	do val=line[i++];
	while(val!=':');i++;
	sscanf(&line[i],"%d",&elems);
	
	do val=line[i++];
	while(val!=':');i++;
	sscanf(&line[i],"%d",&nodes);

	do val=line[i++];
	while(val!=':');i++;
	sscanf(&line[i],"%d",&geoflag);

	do val=line[i++];
	while(val!=':');i++;
	sscanf(&line[i],"%d",&typeflag);
	
	getline;
	i=0;
	do val=line[i++];
	while(val!=':');i++;
	sscanf(&line[i],"%s",entityname);

	if(nodes > maxnodes) {
	  if(info) printf("Allocating a %d-node topology matrix\n",nodes);
	  topology = Imatrix(1,noelements,0,nodes-1);
	  if(totelems > 0) {
	    for(j=1;j<=totelems;j++) 
	      for(i=0;i<data->elementtypes[j] % 100;i++)
		topology[j][i] = data->topology[j][i];
	  }
	  free_Imatrix(data->topology,1,data->noelements,0,data->maxnodes-1);
	  data->maxnodes = maxnodes = nodes;
	  data->topology = topology;
	}

	if(info) printf("reading %d element topologies with %d nodes for %s\n",
			elems,nodes,entityname);

	for(entity=1;entity<=maxentity;entity++) {
	  k = strcmp(entityname,entitylist[entity]);
	  if(k == 0) break;
	}

	if(entity > maxentity) {
	  maxentity++;
	  strcpy(entitylist[entity],entityname);
	}

	for(i=totelems+1;i<=totelems+elems;i++) {
	  getline;

	  cp = line;
	  j = next_int(&cp);
          for(j=0;j<nodes;j++)
	    data->topology[i][j] = next_int(&cp);

	  ReorderFidapNodes(data,i,nodes,typeflag);

	  if(entity) data->material[i] = entity;
	  else data->material[i] = group;
	}
	totelems += elems;
      }
    mode = 0;
    break;
      
    case 10:
      if(info) printf("reading the velocity field\n");
      CreateVariable(data,2,dim,0.0,"Velocity",FALSE);
      vel = data->dofs[2];
      for(j=1;j<=noknots;j++) {
	getline;
	if(dim==2) 
	  sscanf(line,"%le%le",&(vel[2*j-1]),&(vel[2*j]));
	if(dim==3) 
	  sscanf(line,"%le%le%le",&(vel[3*j-2]),&(vel[3*j-1]),&(vel[3*j]));
      }
      mode = 0;
      break;
      
    case 11:

      if(info) printf("reading the temperature field\n");
      CreateVariable(data,1,1,0.0,"Temperature",FALSE);
      temp = data->dofs[1];
      for(j=1;j<=noknots;j++) {
	getline;
	sscanf(line,"%le",&(temp[j]));
      }      
      mode = 0;
      break;
      
    default:      
      break;
    }
  }    

end:

  /* Renumber the nodes */
  maxknot = 0;
  for(i=1;i<=noelements;i++) 
    for(j=0;j < data->elementtypes[i]%100;j++) 
      if(data->topology[i][j] > maxknot) maxknot = data->topology[i][j];

  if(maxknot > noknots) {
    if(info) printf("renumbering the nodes from 1 to %d\n",noknots);

    ind = ivector(1,maxknot);
    for(i=1;i<=maxknot;i++)
      ind[i] = 0;

    for(i=1;i<=noelements;i++) 
      for(j=0;j < data->elementtypes[i]%100;j++) 
	ind[data->topology[i][j]] = data->topology[i][j];
    i=0;
    for(j=1;j<=noknots;j++) {
      i++;
      while(ind[i]==0) i++;
      ind[i] = j;
    } 
    for(i=1;i<=noelements;i++) 
      for(j=0;j < data->elementtypes[i]%100;j++) 
	data->topology[i][j] = ind[data->topology[i][j]];
  }

  fclose(in);
  return(0);
}



static void ReorderAnsysNodes(struct FemType *data,int *oldtopology,
			      int element,int dim,int nodes) 
{
  int i,j,*topology,elementtype;
  int order820[]={1,2,3,4,5,6,7,8,9,10,11,12,17,18,19,20,13,14,15,16};
  int order504[]={1,2,3,5};
  int order306[]={1,2,3,5,6,8};
  int order510[]={1,2,3,5,9,10,12,17,18,19};
  int order613[]={1,2,3,4,5,9,10,11,12,17,18,19,20};
    
  if(dim == 3) {
    if(nodes == 20) {
      if(oldtopology[2] == oldtopology[3]) elementtype = 510;
      else if(oldtopology[4] == oldtopology[5]) elementtype = 613;
      else elementtype = 820;
    }
    if(nodes == 8) {
      if(oldtopology[2] == oldtopology[3]) elementtype = 504;
      else if(oldtopology[4] == oldtopology[5]) elementtype = 605;
      else elementtype = 808;
    }
    if(nodes == 4) elementtype = 504;
    if(nodes == 10) elementtype = 510;
  }
  else if(dim == 2) {
    if(nodes == 9) elementtype = 408;
    if(nodes == 8) {
      if(oldtopology[3] == oldtopology[6]) 
	elementtype = 306;
      else
	elementtype = 408;
    }
    if(nodes == 4) elementtype = 404;
    if(nodes == 10) elementtype = 310;
    if(nodes == 6) elementtype = 306;
    if(nodes == 3) elementtype = 303;
  }
  else if(dim == 1) {
    if(nodes == 4) elementtype = 204;
    if(nodes == 3) elementtype = 203;
    if(nodes == 2) elementtype = 202;
  }

  if(!elementtype) {
    printf("Unknown elementtype in element %d (%d nodes, %d dim).\n",
	   element,nodes,dim);
  }

  data->elementtypes[element] = elementtype;
  topology = data->topology[element];

  switch (elementtype) {
 
  case 820:
    for(i=0;i<elementtype%100;i++) 
      topology[i] = oldtopology[order820[i]-1];
    break;

  case 504:
    for(i=0;i<elementtype%100;i++) 
      topology[i] = oldtopology[order504[i]-1];
    break;


  case 510:
    for(i=0;i<elementtype%100;i++) {
      if(oldtopology[2] == oldtopology[3]) 
	topology[i] = oldtopology[order510[i]-1];
      else 
	topology[i] = oldtopology[i];
    }	
    break;

  case 605:
    for(i=0;i<elementtype%100;i++) {
      topology[i] = oldtopology[i];
    }	
    break;

  case 613:
    for(i=0;i<elementtype%100;i++) {
      if(oldtopology[4] == oldtopology[5]) 
	topology[i] = oldtopology[order613[i]-1];
      else 
	topology[i] = oldtopology[i];
    }	
    break;

  case 306:
    for(i=0;i<elementtype%100;i++) {
      if(oldtopology[3] == oldtopology[6]) 
	topology[i] = oldtopology[order306[i]-1];    
      else 
	topology[i] = oldtopology[i];
    }
    break;

  default:
    for(i=0;i<elementtype%100;i++) 
      topology[i] = oldtopology[i];
    
  }  

}



int LoadAnsysInput(struct FemType *data,struct BoundaryType *bound,
		   char *prefix,int info)
/* This procedure reads the FEM mesh as written by Ansys. */
{
  int noknots,noelements,nosides,elemcode,sidetype,currenttype;
  int elementtypes,sideind[MAXNODESD1],tottypes,elementtype;
  int maxindx,*indx,*revindx,topology[100];
  int i,j,k,l,imax,grp,dummyint,*nodeindx,*boundindx,boundarynodes,maxnodes;
  int noansystypes,*ansysdim,*ansysnodes,*ansystypes,boundarytypes;
  Real x,y,z,r;
  FILE *in;
  char *cp,line[MAXLINESIZE],filename[MAXFILESIZE],
    text[MAXNAMESIZE],directoryname[MAXFILESIZE];

#if 0
  sprintf(directoryname,"%s",prefix);
  mkdir(directoryname,0700);
  chdir(directoryname);

  if(info) printf("Loading mesh in Ansys format from directory %s.\n",
		  directoryname);
#endif


  sprintf(filename,"%s.header",prefix);
  if ((in = fopen(filename,"r")) == NULL) {
    printf("LoadAnsysInput: The opening of the header-file %s failed!\n",
	   filename);
    return(1);
  }
  else 
    printf("Calculating Ansys elementtypes from %s\n",filename);

  for(i=0;getline;i++);
  fclose(in);

  noansystypes = i-1;
  printf("LoadAnsysInput: There seems to be %d elementytypes in file %s.\n",
	 noansystypes,filename);
  
  ansysdim = Ivector(1,noansystypes);
  ansysnodes = Ivector(1,noansystypes);
  ansystypes = Ivector(1,noansystypes);
  
  in = fopen(filename,"r");
  
  for(i=0;i<=noansystypes;i++) {
    Real dummy1,dummy2,dummy3;
    getline;

    sscanf(line,"%le %le %le",&dummy1,&dummy2,&dummy3);

    if(i==0) {
      noelements = dummy1+0.5;
      noknots = dummy2+0.5;
      boundarytypes = dummy3+0.5;
    }
    else {
      ansysdim[i] = dummy1+0.5;
      ansysnodes[i] = dummy2+0.5;
      ansystypes[i] = dummy3+0.5;
    }
  }
  fclose(in);

  printf("Ansys file has %d elements, %d nodes and %d boundary types.\n",
	 noelements,noknots,boundarytypes);
  


  sprintf(filename,"%s.node",prefix);
  if ((in = fopen(filename,"r")) == NULL) {
    printf("LoadAnsysInput: The opening of the nodes-file %s failed!\n",
	   filename);
    return(1);
  }
  else 
    printf("Calculating Ansys nodes from %s\n",filename);

  for(i=0;getline;i++);
  fclose(in);

  printf("LoadAnsysInput: There seems to be %d nodes in file %s.\n",
	 i,filename);
  if(i != noknots) printf("Conflicting number of nodes %d vs %d!\n",
			  i,noknots);

#if 0
  sprintf(filename,"%s.elem",prefix);
  if ((in = fopen(filename,"r")) == NULL) {
    printf("LoadAnsysInput: The opening of the element-file %s failed!\n",
	   filename);
    return(2);
  }
  else 
    printf("Calculating Ansys elements from %s\n",filename);

  for(i=0;getline;i++);
  fclose(in);
  
  noelements = i;
  if(elementtype%100 > 8) noelements /= 2;

  printf("LoadAnsysInput: There seems to be %d elements in file %s.\n",
	 noelements,filename);
#endif


  InitializeKnots(data);

  /* Even 2D elements may form a 3D object! */
  data->dim = 3;
  data->maxnodes = 1;

  for(i=1;i<=noansystypes;i++) {
    if(ansysdim[i] > data->dim) data->dim = ansysdim[i];
    if(ansysnodes[i] > data->maxnodes) data->maxnodes = ansysnodes[i];
  }    
  if(data->maxnodes < 8) data->maxnodes = 8;

  data->noknots = noknots;
  data->noelements = noelements;

  if(info) printf("Allocating for %d nodes and %d elements with max. %d nodes in %d-dim.\n",
		  noknots,noelements,data->maxnodes,data->dim);
  AllocateKnots(data);

  indx = Ivector(1,noknots);

  for(i=1;i<=noknots;i++) 
    indx[i] = 0;

  sprintf(filename,"%s.node",prefix);
  if ((in = fopen(filename,"r")) == NULL) {
    printf("LoadAnsysInput: The opening of the nodes-file %s failed!\n",
	   filename);
    return(3);
  }
  else 
    printf("Loading %d Ansys nodes from %s\n",noknots,filename);

  for(i=1;i<=noknots;i++) {
    getline; cp=line;

    indx[i] = next_int(&cp);

    x = next_real(&cp);
    if(!cp) x = y = z = 0.0;
    else {
      y = next_real(&cp);
      if(!cp) y = z = 0.0;
      else if(data->dim == 3) {
	z = next_real(&cp);
	if(!cp) z = 0.0;
      }
    }
    data->x[i] = x;
    data->y[i] = y;
    if(data->dim == 3) data->z[i] = z;

#if 0
    if(i != indx[i]) {
      printf("Indexes differ: i=%d indx=%d\n",i,indx[i]);
      printf("line: %s",line);
    }
    if(0 && data->dim==3) printf("i:%d  ind:%d  x:%.3lg  y:%.3lg  z:%.3lg\n",
	   i,indx[i],data->x[i],data->y[i],data->z[i]);
    if(0 && data->dim==2) printf("i:%d  ind:%d  x:%.3lg  y:%.3lg\n",
	   i,indx[i],data->x[i],data->y[i]);
#endif
  }
  fclose(in);


  
  /* reorder the indexes */
  maxindx = indx[1];
  for(i=1;i<=noknots;i++) 
    if(indx[i] > maxindx) maxindx = indx[i];
  revindx = Ivector(0,maxindx);

  if(maxindx > noknots) {
    printf("There are %d nodes with indexes up to %d.\n",
	   noknots,maxindx);
    for(i=1;i<=maxindx;i++) 
      revindx[i] = 0;
    for(i=1;i<=noknots;i++) 
      revindx[indx[i]] = i;
  }
  else {
    for(i=1;i<=noknots;i++) 
      revindx[i] = i;
  }


  sprintf(filename,"%s.elem",prefix);
  if ((in = fopen(filename,"r")) == NULL) {
    printf("LoadAnsysInput: The opening of the element-file %s failed!\n",
	   filename);
    return(4);
  }
  else 
    printf("Loading %d Ansys elements from %s\n",noelements,filename);

  for(j=1;j<=noelements;j++) {
    getline; cp=line;
    for(i=0;i<8;i++)
      topology[i] = revindx[next_int(&cp)];

    data->material[j] = next_int(&cp);

    currenttype = next_int(&cp);
    
    for(k=1;k<=noansystypes;k++) 
      if(ansystypes[k] == currenttype) break;
    if(ansystypes[k] != currenttype) k=1;

    if(ansysnodes[k] > 8) {
      getline; cp=line;

      if(ansysnodes[k] == 10 && topology[2] != topology[3])
	imax = 10;
      else 
	imax = 20;

      for(i=8;i<imax;i++) 
	topology[i] = revindx[next_int(&cp)];
    }

#if 0
    printf("topology %3d : ",j);
    for(i=0;i<imax;i++) 
      printf("%3d ",topology[i]);
    printf("\n");
#endif

    ReorderAnsysNodes(data,&topology[0],j,ansysdim[k],ansysnodes[k]);

#if 0
    if(j<10) {
      printf("elem=%d dim=%d nodes=%d\n",j,ansysdim[k],ansysnodes[k]);
      for(i=0;i<ansysnodes[k];i++) {
	l = data->topology[j][i],
	printf("i=%d ind=%d x=%.3lg y=%.3lg z=%.3lg\n",
	       i,l,data->x[l],data->y[l],data->z[l]);
      }	  
    }
#endif
  }      



  fclose(in);

  sprintf(filename,"%s.boundary",prefix);
  printf("Calculating nodes in file %s\n",filename);
  if ((in = fopen(filename,"r")) == NULL) {
    printf("LoadAnsysInput: The opening of the boundary-file %s failed!\n",
	   filename);
    return(5);
  }

  j = 0;
  for(i=0;getline;i++)
    if(!strstr(line,"Boundary")) j++;
  fclose(in);

  boundarynodes = j;
  nosides = 2*boundarynodes;

  if(info) printf("There are %d boundary nodes, thus allocating %d elements\n",
		  boundarynodes,nosides);
  AllocateBoundary(bound,nosides);
  nodeindx = Ivector(1,boundarynodes);
  boundindx = Ivector(1,boundarynodes);


  j++;

  if ((in = fopen(filename,"r")) == NULL) {
    printf("LoadAnsysInput: The opening of the boundary-file %s failed!\n",
	   filename);
  }

  if(info) printf("Loading Ansys boundary from %s\n",filename);

  for(i=1;i<=boundarynodes;i++) 
    nodeindx[i] = boundindx[i] = 0;

  j = 0;
  for(i=0;getline;i++) {
    if(strstr(line,"Boundary")) {
      sscanf(line,"%d",&sidetype);
    }
    else {
      j++;
      sscanf(line,"%d",&k);
      nodeindx[j] = revindx[k];
      if(!nodeindx[j]) printf("The boundary %dth node %d is not in index list\n",j,k);
      boundindx[j] = sidetype;
    }
  }
  printf("Found %d nodes on boundary %s\n",j,filename);
  fclose(in);
    
  FindPointParents(data,bound,boundarynodes,nodeindx,boundindx,info);

  free_Ivector(boundindx,1,boundarynodes);
  free_Ivector(nodeindx,1,boundarynodes);

#if 0
  chdir("..");
#endif

  return(0);
}




static int FindFemlabParents(struct FemType *data,struct BoundaryType *bound,
			    int nosides,int sidenodes,int *boundindx,int **nodeindx)
{
  int i,j,k,l,sideelemtype,elemind,parent,*indx,*twosided;
  int boundarytype,minboundary,maxboundary,sideelem;
  int sideind[MAXNODESD1],elemsides,side,hit,sameelem,same,nosame;

  if(nosides < 1) return(0);

  maxboundary = minboundary = boundindx[1];

  for(i=1;i<=nosides;i++) {
    if(maxboundary < boundindx[i]) maxboundary = boundindx[i];
    if(minboundary > boundindx[i]) minboundary = boundindx[i];
  }

  printf("Boundary types are in interval [%d, %d]\n",minboundary,maxboundary);

  twosided = Ivector(1,nosides);
  indx = Ivector(1,data->noknots);
  sideelem = 0;
  sameelem = 0;

  for(boundarytype=minboundary;boundarytype <= maxboundary;boundarytype++) {
  
    for(i=1;i<=data->noknots;i++) 
      indx[i] = 0;

    for(i=1;i<=nosides;i++) {
      if(boundindx[i] == boundarytype) 
	for(j=1;j<=sidenodes;j++) 
	  indx[nodeindx[i][j]] = TRUE; 
    }

    for(i=1;i<=nosides;i++) 
      twosided[i] = 0;

    for(elemind=1;elemind<=data->noelements;elemind++) {
      elemsides = data->elementtypes[elemind] / 100;
      if(elemsides == 8) elemsides = 6;
      else if(elemsides == 6) elemsides = 5;
      else if(elemsides == 5) elemsides = 4;
      
      if(0) printf("elem=%d\n",elemind);
 
      for(side=0;side<elemsides;side++) {
	GetElementSide(elemind,side,1,data,&sideind[0],&sideelemtype);
	if(sidenodes != sideelemtype%100) printf("FindFemlabParents: bug?\n");
	
	if(0) printf("sidenodes=%d  side=%d\n",sidenodes,side);
	
	hit = TRUE;
	for(i=0;i<sidenodes;i++) {
	  if(sideind[i] <= 0) { 
	    printf("sideind[%d] = %d\n",i,sideind[i]);
	    hit = FALSE;
	  }
	  else if(sideind[i] > data->noknots) { 
	    printf("sideind[%d] = %d (noknots=%d)\n",
		   i,sideind[i],data->noknots);
	    hit = FALSE;
	  }
	  else if(!indx[sideind[i]]) hit = FALSE;
	}
	
	if(hit == TRUE) {

	  same = FALSE;
	  for(i=1;i<=nosides;i++) {
	    if(boundindx[i] == boundarytype) {
	      nosame = 0;
	      for(k=0;k<sidenodes;k++) 
		for(l=1;l<=sidenodes;l++) 
		  if(sideind[k] == nodeindx[i][l]) nosame++;
	      if(nosame == sidenodes) {
		same = TRUE;
		goto foundsame;
	      }
	    }
	  }


	foundsame:

	  if(same && i<=nosides && twosided[i]) {
	    sameelem += 1;
	    bound->parent2[twosided[i]] = elemind;
	    bound->side2[twosided[i]] = side;		  
	  }
	  else {
	    sideelem += 1;
	    if(same) twosided[i] = sideelem;
	    bound->parent[sideelem] = elemind;
	    bound->side[sideelem] = side;
	    bound->parent2[sideelem] = 0;
	    bound->side2[sideelem] = 0;
	    bound->types[sideelem] = boundarytype;
	  }
	  
	  same = FALSE;
	}
      }
    }    
  }

  if(sameelem) printf("Found %d side elements that have two parents.\n",sameelem);

  if(sideelem == nosides) 
    printf("Found correctly %d side elements.\n",sideelem);
  else {
    printf("Found %d side elements, should have found %d\n",sideelem,nosides);
    bound->nosides = sideelem;
  }

  free_Ivector(indx,1,data->noknots);
  free_Ivector(twosided,1,nosides);

  return(0);
}




int LoadFemlabMesh(struct FemType *data,struct BoundaryType *bound,
		   char *prefix,int info)
/* This procedure reads the FEM mesh as written by Femlab and 
   a special Matlab routine . */
{
  int noknots,noelements,nosides,elemcode,sideelem,sidetype,currenttype;
  int elementtypes,sideind[MAXNODESD1],tottypes,elementtype;
  int i,j,k,l,imax,grp,dummyint,**nodeindx,*boundindx,boundarynodes,maxnodes;
  int dim,nonodes,sidenodes;
  Real x,y,z,dummy;
  FILE *in;
  char *cp,line[MAXLINESIZE],filename[MAXFILESIZE],
    text[MAXNAMESIZE],directoryname[MAXFILESIZE];

  sprintf(filename,"%s.header",prefix);
  if ((in = fopen(filename,"r")) == NULL) {
    printf("LoadFemlabInput: The opening of the header-file %s failed!\n",
	   filename);
    return(1);
  }

  for(i=0;getline;i++);
  fclose(in);

  
  in = fopen(filename,"r");
  
  getline;
  sscanf(line,"%le",&dummy);
  dim = dummy+0.5;

  getline;
  sscanf(line,"%le",&dummy);
  noknots = dummy+0.5;
  
  getline;
  sscanf(line,"%le",&dummy);
  noelements = dummy+0.5;
  
  getline;
  sscanf(line,"%le",&dummy);
  nosides = dummy+0.5;
  
  getline;
  sscanf(line,"%le",&dummy);
  nonodes = dummy+0.5;

  fclose(in);

  printf("Femlab (%d-dim) file has %d %d-node elements, %d nodes and %d sides.\n",
	 dim,noelements,nonodes,noknots,nosides);
  

  sprintf(filename,"%s.node",prefix);
  if ((in = fopen(filename,"r")) == NULL) {
    printf("LoadFemlabInput: The opening of the nodes-file %s failed!\n",
	   filename);
    return(1);
  }

  InitializeKnots(data);

  /* Even 2D elements may form a 3D object! */
  data->dim = 3;
  data->maxnodes = nonodes;
  data->noknots = noknots;
  data->noelements = noelements;
  AllocateKnots(data);


  sprintf(filename,"%s.node",prefix);
  if ((in = fopen(filename,"r")) == NULL) {
    printf("LoadFemlabInput: The opening of the nodes-file %s failed!\n",
	   filename);
    return(3);
  }
  else 
    printf("Loading %d Femlab nodes from %s\n",noknots,filename);

  for(i=1;i<=noknots;i++) {
    getline; cp=line;

    x = next_real(&cp);
    y = next_real(&cp);
    z = next_real(&cp);

    data->x[i] = x;
    data->y[i] = y;
    data->z[i] = z;
  }
  fclose(in);



  sprintf(filename,"%s.elem",prefix);
  if ((in = fopen(filename,"r")) == NULL) {
    printf("LoadFemlabInput: The opening of the element-file %s failed!\n",
	   filename);
    return(4);
  }
  else 
    printf("Loading %d Femlab elements from %s\n",noelements,filename);


  for(j=1;j<=noelements;j++) {
    getline; cp=line;

    for(i=0;i<nonodes;i++) {
      dummy = next_real(&cp)+0.5;
      data->topology[j][i] = (int)(dummy);
    }
    dummy = next_real(&cp)+0.5;
    data->material[j] = (int)(dummy);

    if(nonodes == 3) data->elementtypes[j] = 303;
    else if(nonodes == 4) data->elementtypes[j] = 504;
  }      
  fclose(in);



  sprintf(filename,"%s.boundary",prefix);
  if ((in = fopen(filename,"r")) == NULL) {
    printf("LoadFemlabInput: The opening of the boundary-file %s failed!\n",
	   filename);
    return(5);
  }
  else printf("Reading Femlab boundaries from file %s\n",filename);

  AllocateBoundary(bound,nosides);

  if(nonodes == 3) 
    sidenodes = 2;
  else if(nonodes == 4) 
    sidenodes = 3;

  nodeindx = Imatrix(1,nosides,1,sidenodes);
  boundindx = Ivector(1,nosides);

  j = 0;
  for(j=1;j<=nosides;j++) {
    getline; cp=line;

    for(i=1;i<=sidenodes;i++) {
      dummy = next_real(&cp)+0.5;
      nodeindx[j][i] = (int)(dummy);
    }
    dummy = next_real(&cp)+0.5;
    boundindx[j] = (int)(dummy);
  }
  fclose(in);

  FindFemlabParents(data,bound,nosides,sidenodes,boundindx,nodeindx);
  free_Imatrix(nodeindx,1,nosides,1,sidenodes);
  free_Ivector(boundindx,1,nosides);

  return(0);
}



int LoadFemlab3Mesh(struct FemType *data,struct BoundaryType *bound,
		   char *prefix,int info)
     /* This procedure reads the FEM mesh as written by Femlab3 */
{
  int noknots,noelements,nosides,elemcode,sideelem,sidetype,currenttype;
  int elementtypes,sideind[MAXNODESD1],tottypes,elementtype;
  int i,j,k,l,imax,grp,dummyint,**nodeindx,*boundindx,boundarynodes,maxnodes;
  int dim,nonodes,sidenodes,mode;
  Real x,y,z,dummy;
  FILE *in;
  char *cp,line[MAXLINESIZE],filename[MAXFILESIZE],
    text[MAXNAMESIZE],directoryname[MAXFILESIZE];


  sprintf(filename,"%s",prefix);
  if ((in = fopen(filename,"r")) == NULL) {
    sprintf(filename,"%s.p",prefix);
    if ((in = fopen(filename,"r")) == NULL) {
      printf("LoadFemlabInput: The opening of the header-file %s failed!\n",
	     filename);
      return(1);
    }
  }

  mode = 0;
  for(;;) {
    getline;
    
    if(strstr(line,"Coordinates")) {
      mode = 1;
      noknots = 0;
      getline;
    }

    if(strstr(line,"Elements")) {
      mode = 2;
      noelements= 0;
      getline;
    }
    if(strstr(line,"Data")) {
      mode = 3;
    }

    if(mode == 1) noknots = noknots + 1;
    if(mode == 2) noelements = noelements + 1;
    if(mode == 3) break;
  }
  fclose(in);

  printf("Femlab file has %d elements and %d nodes.\n",noelements,noknots);

  in = fopen(filename,"r");


  InitializeKnots(data);

  /* Even 2D elements may form a 3D object! */
  data->dim = 2;
  data->maxnodes = 3;
  data->noknots = noknots;
  data->noelements = noelements;
  AllocateKnots(data);

  mode = 0;
  for(;;) {
    getline;
    
    if(strstr(line,"Coordinates")) {
      mode = 1;
      noknots = 0;
      getline;
    }

    if(strstr(line,"Elements")) {
      mode = 2;
      noelements = 0;
      getline;
    }
    if(strstr(line,"Data")) {
      mode = 3;
    }

    if(mode == 1) {
      noknots = noknots + 1;
      cp=line;
      x = next_real(&cp);
      y = next_real(&cp);
      data->x[noknots] = x;
      data->y[noknots] = y;      
    }
    if(mode == 2) {
      cp = line;
      noelements = noelements + 1;
      for(i=0;i<3;i++) {
	data->topology[noelements][i] = next_int(&cp);
      }      
      data->elementtypes[noelements] = 303;
      data->material[noelements] = 1;      
    }
    if(mode == 3) break;
  }
  fclose(in);

  return(0);
}



static void ReorderFieldviewNodes(struct FemType *data,int *oldtopology,
				  int element,int dim,int nodes) 
{
  int i,j,*topology,elementtype;
  int order808[]={1,2,4,3,5,6,8,7};
  int order706[]={1,4,6,2,3,5};
  int order404[]={1,2,3,4};
    
  if(dim == 3) {
    if(nodes == 8) elementtype = 808;
    if(nodes == 6) elementtype = 706;
    if(nodes == 4) elementtype = 504;
  }
  else if(dim == 2) {
    if(nodes == 4) elementtype = 404;
    if(nodes == 3) elementtype = 303;
  }

  if(!elementtype) {
    printf("Unknown elementtype in element %d (%d nodes, %d dim).\n",
	   element,nodes,dim);
  }

  data->elementtypes[element] = elementtype;
  topology = data->topology[element];

  for(i=0;i<elementtype%100;i++) {
    if(elementtype == 808) topology[i] = oldtopology[order808[i]-1];
    else if(elementtype == 706) topology[i] = oldtopology[order706[i]-1];
    else if(elementtype == 404) topology[i] = oldtopology[order404[i]-1];
    else topology[i] = oldtopology[i];
  }
}




int LoadFieldviewInput(struct FemType *data,char *prefix,int info)
/* Load the grid from a format that can be read by FieldView
   program by PointWise. This is a suitable format to read files created
   by GridGen. */
{
  int noknots,noelements,dim,novel,elemcode,maxnodes;
  int mode,allocated,nvalue,maxknot,totelems,entity,maxentity;
  char filename[MAXFILESIZE];
  char line[MAXLINESIZE],*cp,entityname[MAXLINESIZE],entitylist[100][MAXLINESIZE];
  int i,j,k,*ind,geoflag,typeflag;
  FILE *in;
  Real *vel,*temp,x,y,z;
  int nogroups,knotno,nonodes,elemno,maxindx,sidenodes;
  char *isio;
  int nobound,nobulk,maxsidenodes,*boundtypes,**boundtopos,*boundnodes,*origtopology;

  if ((in = fopen(prefix,"r")) == NULL) {
    AddExtension(prefix,filename,"dat");
    if ((in = fopen(filename,"r")) == NULL) {
      printf("LoadFieldviewInput: opening of the Fieldview-file '%s' wasn't succesfull !\n",
	     filename);
      return(1);
    }
  }
 
  InitializeKnots(data);
  data->dim = 3;
  data->created = TRUE;

  entity = 0;
  mode = 0;
  noknots = 0;
  noelements = 0;
  elemcode = 0;

  maxnodes = 8;
  maxsidenodes = 4;
  maxindx = 0;
  sidenodes = 0;

  data->maxnodes = maxnodes;

  totelems = 0;

  for(;;) {
    
    if(mode == 0) {
      isio = getline;
      
      if(!isio) goto end;
      if(!line) goto end;
      if(line=="") goto end;
      if(strstr(line,"END")) goto end;
      
      /* Control information */
      if(strstr(line,"FIELDVIEW")) mode = 1;
      else if(strstr(line,"CONSTANTS")) mode = 2;
      else if(strstr(line,"GRIDS")) mode = 3;
      else if(strstr(line,"Boundary Table")) mode = 4;
      else if(strstr(line,"Variable Names")) mode = 5;
      else if(strstr(line,"Nodes")) mode = 6;
      else if(strstr(line,"Boundary Faces")) mode = 7;
      else if(strstr(line,"Elements")) mode = 8;
      else if(strstr(line,"Variables")) mode = 9;
      else if(0) printf("unknown: %s",line);
    }      

    switch (mode) {

    case 1: 
      printf("This is indeed a Fieldview input file.\n");
      mode = 0;
      break;
      
    case 2:
      if(0) printf("Constants block\n");
      mode = 0;
      break;

    case 3:
      if(0) printf("Grids block\n");
      mode = 0;
      break;

    case 4:
      if(0) printf("Boundary Table\n");
      mode = 0;
      break;

    case 5:
      if(0) printf("Variable names\n");
      mode = 0;
      break;

    case 6:

      getline;   
      sscanf(line,"%d",&noknots);
      data->noknots = noknots;

      if(info) printf("Loading %d node coordinates\n",noknots);
      
      data->x = Rvector(1,noknots);
      data->y = Rvector(1,noknots);
      data->z = Rvector(1,noknots);
      
      for(i=1;i<=noknots;i++) {
	getline;
	sscanf(line,"%le%le%le",&x,&y,&z);
	data->x[i] = x;
	data->y[i] = y;
	data->z[i] = z;
      }
      mode = 0;
      break;
      
    case 7:
      
      getline;   
      sscanf(line,"%d",&nobound);

      if(info) printf("Loading %d boundary element definitions\n",nobound);

      boundtypes = Ivector(1,nobound);
      boundtopos = Imatrix(1,nobound,0,maxsidenodes-1);
      boundnodes = Ivector(1,nobound);
      
      for(i=1;i<=nobound;i++) {
	getline; cp=line;
	
	boundtypes[i]= next_int(&cp);
	maxsidenodes = next_int(&cp);

	for(j=0;j<maxsidenodes && cp;j++) 
	  boundtopos[i][j] = next_int(&cp);

	boundnodes[i] = j;
      }
      mode = 0;
      break;
      
      
    case 8: 

      if(info) printf("Loading bulk element definitions\n");

      if(maxsidenodes == 4) noelements = noknots + nobound;
      else noelements = 6*noknots + nobound;

      origtopology = Ivector(0,maxnodes-1);
      data->topology = Imatrix(1,noelements,0,maxnodes-1);
      data->material = Ivector(1,noelements);
      data->elementtypes = Ivector(1,noelements);
      
      for(i=0;;) {
	getline; cp=line;

	if(strstr(line,"Variables")) mode = 9;
	if(mode != 8) break;
	
	i++;

	k = next_int(&cp);

	if(k == 2) maxnodes = 8;
	else if(k == 1) maxnodes = 4;

	data->material[i]= next_int(&cp);

	for(j=0;j<maxnodes && cp;j++) {
	  origtopology[j] = next_int(&cp);
	  if(maxindx < origtopology[j]) maxindx = origtopology[j];
	}	

	ReorderFieldviewNodes(data,origtopology,i,3,j);

	if(nobulk+nobound == noelements+1) 
	  printf("Too few elements (%d) were allocated!!\n",noelements);

      }
      nobulk = i;

      printf("Found %d bulk elements\n",nobulk);
      if(nobulk+nobound > noelements) printf("Too few elements (%d) were allocated!!\n",noelements);
      printf("Allocated %.4lg %% too many elements\n",
	     noelements*100.0/(nobulk+nobound)-100.0);


      for(i=1;i<=nobound;i++) {
	ReorderFieldviewNodes(data,boundtopos[i],i+nobulk,2,boundnodes[i]);
	data->material[i+nobulk] = boundtypes[i];
      }

      data->noelements = noelements = nobulk + nobound;

      mode = 0;
      break;


    case 9: 

      printf("Variables\n");
      
      mode = 0;
      break;
      
    default:      
      break;
    }
  }    
  
end:

  if(maxindx != noknots) 
    printf("The maximum index %d differs from the number of nodes %d !\n",maxindx,noknots);
  
  return(0);
}




int LoadTriangleInput(struct FemType *data,struct BoundaryType *bound,
		      char *prefix,int info)
/* This procedure reads the mesh assuming Triangle format
   */
{
  int noknots,noelements,maxnodes,elematts,nodeatts,nosides,dim;
  int sideind[MAXNODESD1],elemind[MAXNODESD2],tottypes,elementtype,bcmarkers;
  int i,j,k,dummyint,*boundnodes;
  FILE *in;
  char *cp,line[MAXLINESIZE],elemfile[MAXFILESIZE],nodefile[MAXFILESIZE];


  if(info) printf("Loading mesh in Triangle format from file %s.*\n",prefix);

  sprintf(nodefile,"%s.node",prefix);
  if ((in = fopen(nodefile,"r")) == NULL) {
    printf("LoadElmerInput: The opening of the nodes file %s failed!\n",nodefile);
    return(1);
  }
  else 
    printf("Loading nodes from file %s\n",nodefile);

  getline;
  sscanf(line,"%d %d %d %d",&noknots,&dim,&nodeatts,&bcmarkers);
  fclose(in);

  if(dim != 2) {
    printf("LoadTriangleInput assumes that the space dimension is 2, not %d.\n",dim);
    return(2);
  }

  sprintf(elemfile,"%s.ele",prefix);
  if ((in = fopen(elemfile,"r")) == NULL) {
    printf("LoadElmerInput: The opening of the element file %s failed!\n",elemfile);
    return(3);
  }
  else 
    printf("Loading elements from file %s\n",elemfile);

  getline;
  sscanf(line,"%d %d %d",&noelements,&maxnodes,&elematts);
  fclose(in);


  InitializeKnots(data);
  data->dim = dim;
  data->maxnodes = maxnodes;
  data->noelements = noelements;
  data->noknots = noknots;
  elementtype = 300 + maxnodes;

  if(info) printf("Allocating for %d knots and %d elements.\n",noknots,noelements);
  AllocateKnots(data);

  boundnodes = Ivector(1,noknots);
  for(i=1;i<=noknots;i++) 
    boundnodes[i] = 0;

  in = fopen(nodefile,"r");
  getline;
  for(i=1; i <= noknots; i++) {
    getline;
    cp = line;
    j = next_int(&cp);
    if(j != i) printf("LoadTriangleInput: nodes i=%d j=%d\n",i,j);
    data->x[i] = next_real(&cp);
    data->y[i] = next_real(&cp);
    for(j=0;j<nodeatts;j++) {
      next_real(&cp);
    }
    if(bcmarkers > 0) 
      boundnodes[i] = next_int(&cp);
  }
  fclose(in);

  in = fopen(elemfile,"r");
  getline;
  for(i=1; i <= noelements; i++) {
    getline;
    cp = line;
    data->elementtypes[i] = elementtype;
    j = next_int(&cp);
    if(j != i) printf("LoadTriangleInput: elem i=%d j=%d\n",i,j);
    for(j=0;j<3;j++)
      data->topology[i][j] = next_int(&cp);
    if(maxnodes == 6) {
      data->topology[i][4] = next_int(&cp);
      data->topology[i][5] = next_int(&cp);
      data->topology[i][3] = next_int(&cp);
    }
    data->material[i] = 1;
  }
  fclose(in);

  FindNewBoundaries(data,bound,boundnodes,0,info);

  printf("Succesfully read the mesh from the Triangle input file.\n");

  return(0);
}




int LoadMeditInput(struct FemType *data,struct BoundaryType *bound,
		   char *prefix,int info)
/* This procedure reads the mesh assuming Medit format
   */
{
  int noknots,noelements,maxnodes,elematts,nodeatts,nosides,dim;
  int sideind[MAXNODESD1],elemind[MAXNODESD2],tottypes,elementtype,bcmarkers;
  int i,j,k,dummyint,*boundnodes,allocated;
  FILE *in;
  char *cp,line[MAXLINESIZE],elemfile[MAXFILESIZE],nodefile[MAXFILESIZE];


  sprintf(nodefile,"%s.mesh",prefix);
  if(info) printf("Loading mesh in Medit format from file %s\n",prefix);

  if ((in = fopen(nodefile,"r")) == NULL) {
    printf("LoadElmerInput: The opening of the mesh file %s failed!\n",nodefile);
    return(1);
  }

  allocated = FALSE;
  maxnodes = 0;

allocate:

  if(allocated) {
    InitializeKnots(data);
    data->dim = dim;
    data->maxnodes = maxnodes;
    data->noelements = noelements;
    data->noknots = noknots;
    elementtype = 300 + maxnodes;

    if(info) printf("Allocating for %d knots and %d elements.\n",noknots,noelements);
    AllocateKnots(data);
    in = fopen(nodefile,"r");
  }


  for(;;) {
    if(Getrow(line,in,TRUE)) goto end;
    if(!line) goto end;
    if(strstr(line,"END")) goto end;
    
    if(strstr(line,"DIMENSION")) {
      if(Getrow(line,in,TRUE)) goto end;
      cp = line;
      dim = next_int(&cp);
      printf("dim = %d %s",dim,line);
    }
    else if(strstr(line,"VERTICES")) {
      printf("verts: %s",line);

      if(Getrow(line,in,TRUE)) goto end;
      cp = line;
      noknots = next_int(&cp);      

      printf("noknots = %d %s",noknots,line);

      for(i=1; i <= noknots; i++) {
	getline;
#if 0
	printf("i=%d line=%s",i,line);
#endif
	if(allocated) {
	  cp = line;
#if 0
	  printf("cp = %s",cp);
#endif

	  data->x[i] = next_real(&cp);
	  data->y[i] = next_real(&cp);
	  if(dim > 2) data->z[i] = next_real(&cp);
	}
      }
    }
    else if(strstr(line,"TRIANGLES")) {
      if(Getrow(line,in,TRUE)) goto end;
      cp = line;
      noelements = next_int(&cp);      

      printf("noelements = %d %s",noelements,line);

      elementtype = 303;
      if(maxnodes < 3) maxnodes = 3;

      for(i=1; i <= noelements; i++) {
	getline;
	if(allocated) {
	  cp = line;
	  data->elementtypes[i] = elementtype;
	  for(j=0;j<3;j++)
	    data->topology[i][j] = next_int(&cp);
	  data->material[i] = next_int(&cp);
	}
      }
    }
#if 0
    else printf("unknown command: %s",line);
#endif
  }    

end:
  fclose(in);

printf("ALLOCATED=%d\n",allocated);

  if(!allocated) {
    allocated = TRUE;
    goto allocate;
  }

  printf("Succesfully read the mesh from the Medit input file.\n");

  return(0);
}




int LoadGidInput(struct FemType *data,struct BoundaryType *bound,
		 char *prefix,int info)
/* Load the grid from GID mesh format */
{
  int noknots,noelements,elemcode,maxnodes,material,foundsame;
  int mode,allocated,nvalue,maxknot,nosides,sideelemtype;
  int boundarytype,materialtype,boundarynodes,side,parent,elemsides;
  int dim, elemnodes, elembasis, elemtype, bulkdone, usedmax,hits;
  int minbulk,maxbulk,minbound,maxbound,label,debug;
  int *usedno, **usedelem;  
  char filename[MAXFILESIZE],line[MAXLINESIZE],*cp;
  int i,j,k,l,n,ind,inds[MAXNODESD2],sideind[MAXNODESD1];
  FILE *in;
  Real x,y,z;

  debug = FALSE;

  strcpy(filename,prefix);
  if ((in = fopen(filename,"r")) == NULL) {
    AddExtension(prefix,filename,"msh");
    if ((in = fopen(filename,"r")) == NULL) {
      printf("LoadAbaqusInput: opening of the GID-file '%s' wasn't succesfull !\n",
	     filename);
      return(1);
    }
  }

  printf("Reading mesh from GID file %s.\n",filename);
  InitializeKnots(data);

  allocated = FALSE;
  maxknot = 0;

  /* Because the file format doesn't provide the number of elements
     or nodes the results are read twice but registered only in the
     second time. */

  minbulk = minbound = 1000;
  maxbulk = maxbound = 0;

omstart:

  mode = 0;
  maxnodes = 0;
  noknots = 0;
  noelements = 0;
  elemcode = 0;
  boundarytype = 0;
  boundarynodes = 0;
  material = 0;
  nosides = 0;
  bulkdone = FALSE;

  for(;;) {

    if(Getrow(line,in,FALSE)) goto end;
    if(!line) goto end;

    if(strstr(line,"MESH")) {
      if(debug) printf("MESH\n");

      if(strstr(line,"dimension 3")) 
	dim = 3;
      else if(strstr(line,"dimension 2")) 
	dim = 2;
      else printf("Unknown dimension\n");
      
      if(strstr(line,"ElemType Tetrahedra"))
	elembasis = 500;
      else if(strstr(line,"ElemType Triangle"))
	elembasis = 300;
      else if(strstr(line,"ElemType Linear"))
	elembasis = 200;
      else printf("Unknown elementtype: %s\n",line);

      if(strstr(line,"Nnode 4"))
	elemnodes = 4;
      else if(strstr(line,"Nnode 3"))
	elemnodes = 3;
      else if(strstr(line,"Nnode 2"))
	elemnodes = 2;
      else printf("Unknown elementnode: %s\n",line);

      if(elemnodes > maxnodes) maxnodes = elemnodes;      
      elemtype = elembasis + elemnodes;
      mode = 0;
      
      if(debug) printf("dim=%d elemtype=%d\n",dim,elemtype);
    }
    else if(strstr(line,"Coordinates")) {
      if(debug) printf("Start coords\n");
      mode = 1;
    }
    else if(strstr(line,"end coordinates")) {
      if(debug) printf("End coords\n");
      mode = 0;
    }
    else if(strstr(line,"Elements")) {
      if(bulkdone) { 
	if(debug) printf("Start boundary elems\n");
	mode = 3;
	boundarytype++;
      }
      else {
	if(debug) printf("Start bulk elems\n");	
	mode = 2;
      }
    }
    else if(strstr(line,"end elements")) {
      if(debug) printf("End elems\n");

      mode = 0;
      if(!bulkdone && allocated) {
	usedno = Ivector(1,data->noknots);

	for(j=1;j<=data->noknots;j++)
	  usedno[j] = 0;

	for(j=1;j<=data->noelements;j++) {
	  n = data->elementtypes[j] % 100; 
	  for(i=0;i<n;i++) {
	    ind = data->topology[j][i];
	    usedno[data->topology[j][i]] += 1;
	  }
	}

	usedmax = 0;
	for(i=1;i<=data->noknots;i++)
	  if(usedno[i] > usedmax) usedmax = usedno[i];

	for(j=1;j<=data->noknots;j++)
	  usedno[j] = 0;

	usedelem = Imatrix(1,data->noknots,1,usedmax);
	for(j=1;j<=data->noknots;j++)
	  for(i=1;i<=usedmax;i++)
	    usedelem[j][i] = 0;

	for(j=1;j<=data->noelements;j++) {
	  n = data->elementtypes[j] % 100;
	  for(i=0;i<n;i++) {
	    ind = data->topology[j][i];
	    usedno[ind] += 1;
	    k = usedno[ind];
	    usedelem[ind][k] = j;
	  }
	}
      }
      bulkdone = TRUE;
    }
    else if(!mode) {
      if(debug) printf("mode: %d %s\n",mode,line);
    }
    else if(mode) {  
      switch (mode) {
	
      case 1:
	cp = line;
	ind = next_int(&cp);
	if(ind > noknots) noknots = ind;

	x = next_real(&cp);
	y = next_real(&cp);
	if(dim == 3) z = next_real(&cp);
	
	if(allocated) {
	  data->x[ind] = x;
	  data->y[ind] = y;
	  if(dim == 3) data->z[ind] = z;
	}
	break;

      case 2:
	cp = line;
	ind = next_int(&cp);
	if(ind > noelements) noelements = ind;
	
	for(i=0;i<elemnodes;i++) {
	  k = next_int(&cp);
	  if(allocated) {
	    data->topology[ind][i] = k;
	    data->elementtypes[ind] = elemtype;
	    data->material[ind] = 1;
	  }
	}
	label = next_int(&cp);
	
	if(allocated) {
	  if(label) { 
	    materialtype = label-minbulk+1;
	  }
	  else if(maxbound) {
	    materialtype = maxbulk-minbulk + 2;
	  }
	  else { 
	    materialtype = 1;
	  }
	  data->material[ind] = materialtype;
	}
	else {
	  if(label > maxbulk) maxbulk = label;
	  if(label < minbulk) minbulk = label;	  
	}

	break;
	
      case 3:
	cp = line;
	ind = next_int(&cp);
	nosides++;

	for(i=0;i<elemnodes;i++) {
	  k = next_int(&cp);
	  inds[i] = k;
	}
	label = next_int(&cp);
	
	if(!allocated) {
	  if(label) {
	    if(label > maxbound) maxbound = label;
	    if(label < minbound) minbound = label;
	  }
	}

	if(allocated) {
	  
	  if(label) { 
	    boundarytype = label-minbound+1;
	  }
	  else if(maxbound) {
	    boundarytype = maxbound-minbound + 2;
	  }
	  else { 
	    boundarytype = 1;
	  }

	  foundsame = FALSE;
	  for(i=1;i<=usedno[inds[0]];i++) {
	    parent = usedelem[inds[0]][i];

	    elemsides = data->elementtypes[parent] % 100;
	    
	    for(side=0;side<elemsides;side++) {

	      GetElementSide(parent,side,1,data,&sideind[0],&sideelemtype);

	      if(elemnodes != sideelemtype%100) printf("LoadGidMesh: bug?\n");
	      
	      hits = 0;
	      for(j=0;j<elemnodes;j++) 
		for(k=0;k<elemnodes;k++) 
		  if(sideind[j] == inds[k]) hits++;

	      if(hits < elemnodes) continue;
	      
	      if(!foundsame) {
		foundsame++;
		bound->parent[nosides] = parent;
		bound->side[nosides] = side;
		bound->parent2[nosides] = 0;
		bound->side2[nosides] = 0;
		bound->types[nosides] = boundarytype;	  
	      }
	      else if(foundsame == 1) {
		if(parent == bound->parent[nosides]) continue;
		foundsame++;
		bound->parent2[nosides] = parent;
		bound->side2[nosides] = side;		  
	      }
	      else if(foundsame > 1) {
		printf("Boundary %d has more than 2 parents\n",nosides);
	      }
	    }
	  }
	  if(!foundsame) {
	    printf("Did not find parent for side %d\n",nosides);
	    nosides--;
	  }
	  else {
	    if(debug) printf("Parent of side %d is %d\n",nosides,bound->parent[nosides]);
	  }
	}
      }
    }
  }

end:

  if(!allocated) {
    rewind(in);
    data->noknots = noknots;
    data->noelements = noelements;
    data->maxnodes = maxnodes;
    data->dim = dim;
    
    if(info) {
      printf("Allocating for %d knots and %d %d-node elements.\n",
	     noknots,noelements,maxnodes);
      printf("Initial material indexes are at interval %d to %d.\n",minbulk,maxbulk);
    }  
    AllocateKnots(data);

    if(info) {
      printf("Allocating %d boundary elements\n",nosides);
      printf("Initial boundary indexes are at interval %d to %d.\n",minbound,maxbound);
    }
    AllocateBoundary(bound,nosides);

    bound->nosides = nosides;
    bound->created = TRUE;
    
    nosides = 0;
    bulkdone = FALSE;
    boundarytype = 0;

    allocated = TRUE;
    goto omstart;
  }

  bound->nosides = nosides;
  free_Ivector(usedno,1,data->noknots);
  free_Imatrix(usedelem,1,data->noknots,1,usedmax);

  if(info) printf("The mesh was loaded from file %s.\n",filename);
  return(0);
}



static int GmshToElmerType(int gmshtype)
{
  int elmertype;

  switch (gmshtype) {
      
  case 1:        
    elmertype = 202;
    break;
  case 2:        
    elmertype = 303;
    break;
  case 3:        
    elmertype = 404;
    break;
  case 4:        
    elmertype = 504;
    break;
  case 5:        
    elmertype = 808;
    break;
  case 6:        
    elmertype = 706;
    break;
  case 7:        
    elmertype = 605;
    break;
  case 8:        
    elmertype = 203;
    break;
  case 9:        
    elmertype = 306;
    break;
  case 10:        
    elmertype = 409;
    break;
  case 11:        
    elmertype = 510;
    break;
  case 12:        
    elmertype = 827;
    break;
  case 15:        
    elmertype = 101;
    break;

  default:
    printf("Gmsh element %d does not have an Elmer counterpart!\n",gmshtype);
  }

  return(elmertype);
}





int LoadGmshInput(struct FemType *data,struct BoundaryType *bound,
		   char *prefix,int info)
/* This procedure reads the mesh assuming Gmsh format */
{
  int noknots,noelements,maxnodes,elematts,nodeatts,nosides,dim;
  int sideind[MAXNODESD1],elemind[MAXNODESD2],tottypes,elementtype,bcmarkers;
  int i,j,k,dummyint,*boundnodes,allocated,*revindx,maxindx;
  int elemno, gmshtype, regphys, regelem, elemnodes,maxelemtype,elemdim;
  FILE *in;
  char *cp,line[MAXLINESIZE],filename[MAXFILESIZE];


  sprintf(filename,"%s",prefix);
  if ((in = fopen(filename,"r")) == NULL) {
    sprintf(filename,"%s.msh",prefix);
    if ((in = fopen(filename,"r")) == NULL) {
      printf("LoadElmerInput: The opening of the mesh file %s failed!\n",filename);
      return(1);
    }
  }
  if(info) printf("Loading mesh in Gmsh format from file %s\n",filename);

  allocated = FALSE;
  dim = 3;
  maxnodes = 0;
  maxindx = 0;
  maxelemtype = 0;

allocate:

  if(allocated) {
    InitializeKnots(data);
    data->dim = dim;
    data->maxnodes = maxnodes;
    data->noelements = noelements;
    data->noknots = noknots;

    if(info) printf("Allocating for %d knots and %d elements.\n",noknots,noelements);
    AllocateKnots(data);

    if(maxindx > noknots) {
      revindx = Ivector(1,maxindx);
      for(i=1;i<=maxindx;i++) revindx[i] = 0;
    }
    in = fopen(filename,"r");
  }


  for(;;) {
    if(Getrow(line,in,TRUE)) goto end;
    if(!line) goto end;
    if(strstr(line,"END")) goto end;

    if(strstr(line,"$NOD")) {
      
      getline;
      cp = line;
      noknots = next_int(&cp);

      for(i=1; i <= noknots; i++) {
	getline;
	cp = line;

	j = next_int(&cp);
	if(allocated) {
	  if(maxindx > noknots) revindx[j] = i;
	  data->x[i] = next_real(&cp);
	  data->y[i] = next_real(&cp);
	  if(dim > 2) data->z[i] = next_real(&cp);
	}
	else {
	  maxindx = MAX(j,maxindx);
	}
      }
      getline;
      if(!strstr(line,"$ENDNOD")) {
	printf("NOD section should end to string ENDNOD\n");
	printf("%s\n",line);
      }
    }
    
    if(strstr(line,"$ELM")) {
      getline;
      cp = line;
      noelements = next_int(&cp);

      for(i=1; i <= noelements; i++) {
	getline;
	
	cp = line;
	elemno = next_int(&cp);
	gmshtype = next_int(&cp);
	regphys = next_int(&cp);
	regelem = next_int(&cp);
	elemnodes = next_int(&cp);

	if(allocated) {
	  elementtype = GmshToElmerType(gmshtype);
	  maxelemtype = MAX(maxelemtype,elementtype);
	  data->elementtypes[i] = elementtype;
	  data->material[i] = regphys;

	  if(elemnodes != elementtype%100) {
	    printf("Conflict in elementtypes %d and number of nodes %d!\n",
		   elementtype,elemnodes);
	  }	  
	  for(j=0;j<elemnodes;j++)
	    data->topology[i][j] = next_int(&cp);
	}
	else {
	  maxnodes = MAX(elemnodes,maxnodes);
	}

      }
      getline;
      if(!strstr(line,"$ENDELM")) 
	printf("NOD section should end to string ENDELM\n");
    }

  }

 end:

  fclose(in);

  if(!allocated) {
    allocated = TRUE;
    goto allocate;
  }

  if(maxindx > noknots) {
    printf("Renumbering the Gmsh nodes from %d to %d\n",maxindx,noknots);

    for(i=1; i <= noelements; i++) {
      elementtype = data->elementtypes[i];
      elemnodes = elementtype % 100; 

      for(j=0;j<elemnodes;j++) {
	k = data->topology[i][j];
	if(k <= 0 || k > maxindx) 
	  printf("index out of bounds %d\n",k);
	else if(revindx[k] <= 0) 
	  printf("unkonwn node %d %d in element %d\n",k,revindx[k],i);
	else 
	  data->topology[i][j] = revindx[k];
      }      
    }
    free_Ivector(revindx,1,maxindx);
  }

  ElementsToBoundaryConditions(data,bound,info);
  ReorderTypes(data,bound,info);
 
  printf("Succesfully read the mesh from the Gmsh input file.\n");

  return(0);
}




int SaveFastcapInput(struct FemType *data,
		     struct BoundaryType *bound,char *prefix,int decimals,int info)
/* Saves the mesh in a form that may be used as input 
   in Fastcap calculations. 
   */
#define MAXELEMENTTYPE 827
{
  int noknots,noelements,quads,triangles,elemtype,maxelemtype,fail;
  int sideelemtype,nodesd1,nodesd2;
  int i,j,k,l;
  int ind[MAXNODESD1];
  FILE *out;
  char filename[MAXFILESIZE], outstyle[MAXFILESIZE];

  if(!data->created) {
    printf("You tried to save points that were never created.\n");
    return(1);
  }

  noelements = data->noelements;
  noknots = data->noknots;

  sprintf(filename,"%s%s",prefix,".fc");
  sprintf(outstyle,"%%.%dlg %%.%dlg %%.%dlg ",decimals,decimals,decimals);

  if(info) printf("Saving mesh in Fastcap format to filename %s.\n",filename);

  out = fopen(filename,"w");
  if(out == NULL) {
    printf("opening of file was not successful\n");
    return(2);
  }

  quads = 0;
  triangles = 0;
  maxelemtype = 0;
  for(i=1;i<=noelements;i++) {
    elemtype = data->elementtypes[i];
    maxelemtype = MAX(elemtype,maxelemtype);
  }
  
  if(maxelemtype < 500) {    
    if(info) printf("Saving Fastcap mesh using bulk elements.\n");

    fprintf(out,"0 Elmer mesh of %d elements, bulk elements\n",data->noelements);
 
    for(i=1;i<=noelements;i++) {
      elemtype = data->elementtypes[i];

      nodesd2 = elemtype%100;
      if(nodesd2 == 4) {
	quads++;
	fprintf(out,"Q %d ",data->material[i]);
	for(j=0;j < nodesd2;j++) {
	  k = data->topology[i][j];
	  fprintf(out,outstyle,data->x[k],data->y[k],data->z[k]);      
	}
	fprintf(out,"\n");
      }
      if(nodesd2 == 3) {
	triangles++;
	fprintf(out,"T %d ",data->material[i]);
	for(j=0;j < nodesd2;j++) {
	  k = data->topology[i][j];
	  fprintf(out,outstyle,data->x[k],data->y[k],data->z[k]);      
	}
	fprintf(out,"\n");
      }
    }
  }
  else {
    if(info) printf("Saving Fastcap mesh using boundary elements.\n");

    fprintf(out,"0 Elmer mesh of %d elements, boundary elements\n",data->noelements);
   
    for(j=0;j < MAXBOUNDARIES;j++) {
      
      if(bound[j].created == FALSE) continue;
      if(bound[j].nosides == 0) continue;
      
      for(i=1; i <= bound[j].nosides; i++) {
	
	GetElementSide(bound[j].parent[i],bound[j].side[i],bound[j].normal[i],data,ind,&sideelemtype); 

	nodesd2 = sideelemtype%100;
	if(nodesd2 == 4) {
	  quads++;
	  fprintf(out,"Q %d ",bound[j].types[i]);
	  for(k=0;k < nodesd2;k++) {
	    l = ind[k];
	    fprintf(out,outstyle,data->x[l],data->y[l],data->z[l]);      
	  }
	  fprintf(out,"\n");
	}
	if(nodesd2 == 3) {
	  triangles++;
	  fprintf(out,"T %d ",bound[j].types[i]);
	  for(k=0;k < nodesd2;k++) {
	    l = ind[k];
	    fprintf(out,outstyle,data->x[l],data->y[l],data->z[l]);      
	  }
	  fprintf(out,"\n");
	}
		
      }	
    }
  }

  if(info) {
    printf("Fastcap mesh saved successfully\n");
    printf("There are %d quadrilateral and %d triangular panels\n",quads,triangles);
  }

  fclose(out);

  return(0);
}
