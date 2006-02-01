/*  
   ElmerGrid - A simple mesh generation and manipulation utility  
   Copyright (C) 1995- , CSC - Scientific Computing Ltd.   

   Author:  Peter R�back
   Email:   Peter.Raback@csc.fi
   Address: CSC - Scientific Computing Ltd.
            Keilaranta 14
            02101 Espoo, Finland

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/* --------------------:  feminfo.c  :--------------------------

   These functions provide the user of the program information 
   about how the mesh was created and what the resulting sparse 
   matrix will be alike and also present the results of the 
   calculations in various ways. These subroutines don't affect 
   the operation and results of the program and can thus be 
   omitted if not needed. 
   */
   
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>

#include "common.h"
#include "nrutil.h"
#include "femdef.h"
#include "femtypes.h"
#include "femsolve.h"
#include "femmesh.h"
#include "femknot.h"
#include "feminfo.h"


char line[MAXLINESIZE];

int Getline(char *line1,FILE *io) 
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

  if(strstr(line0,"subcell boundaries")) goto newline;
  if(strstr(line0,"material structure")) goto newline;
  if(strstr(line0,"mode")) goto newline;
  if(strstr(line0,"type")) goto newline;

  for(i=0;i<MAXLINESIZE;i++) 
    line1[i] = toupper(line0[i]);

  return(0);
}


int GetCommand(char *line1,char *line2,FILE *io) 
{
  int i,j,isend,empty;
  char line0[MAXLINESIZE],*charend;

 newline:

  for(i=0;i<MAXLINESIZE;i++) 
    line2[i] = line1[i] = line0[i] = ' ';

  charend = fgets(line0,MAXLINESIZE,io);
  isend = (charend == NULL);

  if(isend) return(1);

  empty = TRUE;
  for(i=1;i<20;i++) if(line0[0] != ' ') empty = FALSE;
  if(empty) goto newline;

  if(line0[0] == '*' ||  line0[0] == '#' || line0[0] == '!' || line0[0] == '\n') goto newline;

  j = 0;
  for(i=0;i<MAXLINESIZE;i++) {
    if(line0[i] == '=') {
      j = i;
      break;
    }
    line1[i] = toupper(line0[i]);
  }

  /* After these commands there will be no nextline even though there is no equality sign */
  if(strstr(line1,"END")) return(0);
  if(strstr(line1,"NEW MESH")) return(0);


  if(j) {
    for(i=j+1;i<MAXLINESIZE;i++) 
      line2[i-j-1] = line0[i];      
  }
  else {
  newline2:
    charend = fgets(line2,MAXLINESIZE,io);
    isend = (charend == NULL);
    if(isend) return(2);
    if(line2[0] == '*' || line2[0] == '#' || line2[0] == '!') goto newline2;
  }
  
  return(0);
}


int SaveCellInfo(struct GridType *grid,struct CellType *cell,
		 char *prefix,int info)
/* Save some (int) values for each cell in structure CellType. 
   The resulting matrix may be used to check that the division to 
   subcells is as wanted.
   */
{
  int i; 
  FILE *out;
  char filename[MAXFILESIZE];

  AddExtension(prefix,filename,"cell");
  out = fopen(filename,"w");

  fprintf(out,"%-6s %-6s %-6s %-6s %-6s %-6s %-6s %-6s %-6s\n",
	  "1st","2nd","last","level","center","mat","xlin","ylin","num");
  for(i=1;i<=grid->nocells;i++) {
    fprintf(out,"%-6d %-6d %-6d %-6d %-6d %-6d %-6d %-6d %-6d\n",
	    cell[i].left1st,cell[i].left2nd,cell[i].leftlast,
	    cell[i].levelwidth,cell[i].leftcenter,
	    cell[i].material,cell[i].xlinear,cell[i].ylinear,cell[i].numbering);
  }
  fclose(out);

  if(info) printf("The cell information was saved to file %s.\n",filename);

  return(0);
}



int SaveBoundary(struct FemType *data,struct BoundaryType *bound,
		 char *prefix,int info)
/* This function saves the given boundary to an ascii-file.
   The data is saved in format [x1 x2 y1 y2 v1 v2].
   */
{
  int i,j,k,sideelemtype;
  FILE *out;
  char filename[MAXFILESIZE];
  int sideind[MAXNODESD1]; 

  if(!bound->created) {
    printf("SaveBoundary: You tried to save a nonexisting boundary.\n");
    return(1);
  }
  if(bound->nosides == 0) return(0);

  AddExtension(prefix,filename,"bound");
  out = fopen(filename,"w");

  for(i=1; i <= bound->nosides; i++) {

    GetElementSide(bound->parent[i],bound->side[i],bound->normal[i],data,sideind,&sideelemtype);

    fprintf(out,"%-12.4le %-12.4le %-12.4le %-12.4le ",
	    data->x[sideind[0]],data->x[sideind[1]],
	    data->y[sideind[0]],data->y[sideind[1]]);
    for(k=0;k<MAXVARS;k++) 
      if(bound->evars[k]) {
	if(bound->points[k] == 1) 
	  fprintf(out,"%-10.4le ",bound->vars[k][i]);
      }		
    fprintf(out,"\n");
  }

  fclose(out);

  if(info) printf("Boundary information was saved to file %s.\n",filename);

  return(0);
}


int SaveBoundariesChain(struct FemType *data,struct BoundaryType *bound,
			char *prefix,int info)
/* This function saves the given boundary to an ascii-file.
   The data is saved in format [x y v].
   This may be used particularly for boundaries that 
   are continues i.e. the element sides constitute a full chain. 
   */
{
  int i,j,k,ind,length,col;
  FILE *out;
  char filename[MAXFILESIZE];
  char filename2[MAXFILESIZE];

  for(j=0;j<MAXBOUNDARIES;j++) 
    if(bound[j].created && bound[j].nosides >0) {

      CreateBoundaryChain(data,&bound[j],info);
      length = bound[j].chainsize;
      if(length < 2) continue;

      sprintf(filename,"%s%d%s\0",prefix,j,".side");
      out = fopen(filename,"w");

      for(i=0;i<=length;i++) {
	ind = bound[j].chain[i];
	fprintf(out,"%-10.4le %-10.4le %-6d ",
		data->x[ind],data->y[ind],ind);
	for(k=0;k<MAXVARS;k++) 
	  if(bound[j].evars[k]) {
	    if(bound[j].points[k] == 0)
	      fprintf(out,"%-10.4le ",bound[j].vars[k][i]);
	    else if(bound[j].points[k] == 1) {
	      if(i==0)
		fprintf(out,"%-10.4le ",bound[j].vars[k][1]);		
	      else if(i==length)
		fprintf(out,"%-10.4le ",bound[j].vars[k][length]);		
	      else
		fprintf(out,"%-10.4le ",
			0.5*(bound[j].vars[k][i]+bound[j].vars[k][i+1]));	
	    }
	  }

	for(k=0;k<MAXDOFS;k++) {
	  if(data->edofs[k] == 1) 
	    fprintf(out,"%-10.4le  ",data->dofs[k][ind]);
	  if(data->edofs[k] == 2) 
	    fprintf(out,"%-10.4le  %-10.4le  ",
		    data->dofs[k][2*ind-1],data->dofs[k][2*ind]);
	}
	
	fprintf(out,"\n");
      }
      fclose(out);

      sprintf(filename2,"%s%d%s\0",prefix,j,".sidetxt");
      out = fopen(filename2,"w");
      fprintf(out,"Degrees of freedom in file %s are as follows:\n",filename);
      if(bound->coordsystem == COORD_CART2) {
	fprintf(out,"col1: X coordinate\n");
	fprintf(out,"col2: Y coordinate\n");
      }
      else if(bound->coordsystem == COORD_AXIS) {
	fprintf(out,"col1: R coordinate\n");
	fprintf(out,"col2: Z coordinate\n");
      }
      else if(bound->coordsystem == COORD_POLAR) {
	fprintf(out,"col1: R coordinate\n");
	fprintf(out,"col2: F coordinate\n");
      }
      fprintf(out,"col3: node indices\n");
      col = 3;
      for(k=0;k<MAXVARS;k++) 
	if(bound[j].evars[k] && bound[j].points[k] <= 1) 
	  fprintf(out,"col%d: %s\n",++col,bound[j].varname[k]);	  
      for(k=0;k<MAXDOFS;k++) {
	if(data->edofs[k] == 1) 
	  fprintf(out,"col%d: %s\n",++col,data->dofname[k]);	  
	if(data->edofs[k] == 2) {
	  fprintf(out,"col%d: %s1\n",++col,data->dofname[k]);	  
	  fprintf(out,"col%d: %s2\n",++col,data->dofname[k]);	  
	}
      }
      fclose(out);

      if(info) printf("Boundary info was saved to files %s and %s.\n",
		      filename,filename2);
    }

  return(0);
}



int SaveBoundaryForm(struct FemType *data,struct CellType *cell,
		     char* prefix,int info)
/* Saves the form of the boundary as given by function GetSideInfo 
   in form [x,y,index]. 
   */
{
  int sideknots,elemno,side,more,elemind[2],sideelemtype;
  int no,sideind[MAXNODESD1];
  FILE *out;
  char filename[MAXFILESIZE];

  if(data->created == FALSE) {
    printf("SaveBoundaryForm: stucture FemType not created\n");
    return(1);
  }

  sideknots = 0;
  more = FALSE;

  AddExtension(prefix,filename,"boundary");
  out = fopen(filename,"w");

  /* Go through all pairs of points and save them into a matrix. */
  for(no=1; no <= data->nocells; no++)
    for(side=0; side < 4; side++) 
      if(cell[no].material != cell[no].boundary[side]) {
        elemno = 0; 
        do { 
          elemno++;
	  sideknots++;
          more = GetSideInfo(cell,no,side,elemno,elemind);
	  GetElementSide(elemind[0],side,1,data,sideind,&sideelemtype);
	  
	  fprintf(out,"%-12.4le %-12.4le %-12.4le %-12.4le\n",
		  data->x[sideind[0]],data->x[sideind[1]],
		  data->y[sideind[0]],data->y[sideind[1]]);
        } while(more);
      }

  fclose(out);

  if(info) printf("%d boundaries between materials were saved to file %s.\n",
		  sideknots,filename);
  return(0);
}


int SaveBoundaryLine(struct FemType *data,int direction,
		     Real c0,char* prefix,int info)
/* Saves the nodes forming a vertical or a horizontal line. 
   The format is [x y v1 v2 v3 ...].
   */
{
  int k,no,points;
  FILE *out;
  char filename[MAXFILESIZE];
  Real c,c1,eps;

  if(data->created == FALSE) {
    printf("SaveBoundaryLine: stucture FemType not created\n");
    return(1);
  }

  eps = 1.0e-6;
  points = 0;

  AddExtension(prefix,filename,"line");
  out = fopen(filename,"w");

  /* Go through all pairs of points and save them into amatrix. */
  
  c1 = data->x[1];
  for(no=1; no <= data->noknots; no++) {
    if(direction > 0) 
      c = data->x[no];
    else
      c = data->y[no];
    if(fabs(c-c0) < fabs(c1-c0))
      c1 = c;
  }
  for(no=1; no <= data->noknots; no++) {
    if(direction > 0) 
      c = data->x[no];
    else
      c = data->y[no];
    if(fabs(c-c1) < eps) {
      if(direction > 0) 
	fprintf(out,"%-12.7le %-12.7le ",c,data->y[no]);
      else 
	fprintf(out,"%-12.7le %-12.7le ",data->x[no],c);	
      for(k=0;k<MAXDOFS;k++) {
	if(data->edofs[k] == 1) 
	  fprintf(out,"%-12.7le ",data->dofs[k][no]);
	if(data->edofs[k] == 2) 
	  fprintf(out,"%-12.7le %-12.7le ",
		  data->dofs[k][2*no-1],data->dofs[k][2*no]);
      }	
      fprintf(out,"\n");
      points++;
    }
  }
  if(info) printf("Line (c=%.3lg) with %d nodes was saved to file %s.\n",
		  c1,points,filename);
  
  fclose(out);
  
  return(0);
}


int SaveSubcellForm(struct FemType *data,struct CellType *cell, 
		    char* prefix,int info)
/* Saves the form of the boundary as given by function GetSideInfo 
   in form [x,y,index]. 
   */
{
  int more,sideknots,elemno,elemind[2],side,i;
  int no,sideind[MAXNODESD1],nosidenodes,sidelemtype;
  FILE *out;
  char filename[MAXFILESIZE];

  sideknots = 0;
  more = FALSE;

  if(data->created == FALSE) {
    printf("SaveSubcellForm: stucture FemType not created\n");
    return(1);
  }

  AddExtension(prefix,filename,"subcell");
  out = fopen(filename,"w");

  /* Go through all pairs of points and save them into amatrix. */
  for(no=1; no <= data->nocells; no++)
    for(side=0; side < 4; side++) 
      if(cell[no].boundary[side]) {
        elemno = 0; 
        do { 
          elemno++;
	  sideknots++;
          more = GetSideInfo(cell,no,side,elemno,elemind);
	  GetElementSide(elemind[0],side,1,data,sideind,&sidelemtype);
	  nosidenodes = sidelemtype%100;
	  for(i=0;i<nosidenodes;i++) 
	    fprintf(out,"%-12.4le %-12.4le %-8d\n",
		    data->x[sideind[i]],data->y[sideind[i]],sideind[i]);
        } while(more);
      }
  fclose(out);

  if(info) printf("There are %d sideknots in the elements.\n",sideknots);
  if(info) printf("The positions of the sideknots were saved in file %s.\n",filename);

  return(0);
}



int SaveViewFactors(struct FemType *data,struct BoundaryType *bound,
		    char *prefix,int info)
/* This function saves the view factors and side information
   to an external file.
   */
{
  int i,j,ind[MAXNODESD1],sidelemtype;
  FILE *out;
  char filename[MAXFILESIZE];

  if(!bound->created) {
    printf("SaveViewFactors: boundary not created.\n");
    return(1);
  }
  if(bound->nosides == 0) {
    printf("SaveViewFactors: no sides on boundary.\n");
    return(2);
  }
  if(!bound->vfcreated) {
    printf("SaveViewFactors: view factors not created.\n");
    return(3);
  }

  AddExtension(prefix,filename,"vf");
  out = fopen(filename,"w");

  fprintf(out,"%d\n",bound->nosides);
  for(i=1; i <= bound->nosides; i++) { 
    GetElementSide(bound->parent[i],bound->side[i],bound->normal[i],
		   data,ind,&sidelemtype);
    fprintf(out,"%-14.6le %-14.6le %-14.6le %-14.6le %-8d %-8d\n",
	    data->x[ind[0]],data->x[ind[1]],
	    data->y[ind[0]],data->y[ind[1]],
	    ind[0],ind[1]);
  }
  for(j=1; j <= bound->nosides; j++) { 
    for(i=1; i <= bound->nosides; i++) 
      fprintf(out,"%-14.6le ",bound->vf[j][i]); 
    fprintf(out,"\n");
  }

  fclose(out);

  if(info) printf("View factors for %d sides were saved to file %s.\n",
		  bound->nosides,filename);
  return(0);
}



int LoadViewFactors(struct FemType *data,struct BoundaryType *bound,
		    char *prefix,int info)
/* This function loads the view factors 
   from an external file.
   */
#define MAXERROR 1.0e-3
{
  int i,j,sides,i1,i2,ind[MAXNODESD1],sidelemtype;
  FILE *in;
  char filename[MAXFILESIZE];
  Real x0,x1,y0,y1;

  if(!bound->created  ||  bound->nosides == 0) {
    printf("You tried to load nonexisting view factors.\n");
    return(1);
  }

  AddExtension(prefix,filename,"vf");

  if ((in = fopen(filename,"r")) == NULL) {
    printf("The opening of the file '%s' wasn't succesfull!\n",filename);
    return(2);
  }

  Getline(line,in); 
  sscanf(line,"%d",&sides);
  if(sides != bound->nosides) {
    printf("Number of sides differs, %d vs. %d\n",sides,bound->nosides);
    fclose(in);
    return(3);
  }

  for(i=1; i <= bound->nosides; i++) { 
    Getline(line,in); 
    sscanf(line,"%le%le%le%le%d%d",&x0,&x1,&y0,&y1,&i1,&i2);

    GetElementSide(bound->parent[i],bound->side[i],bound->normal[i],data,ind,&sidelemtype);

    if(fabs(x0 - data->x[ind[0]])>MAXERROR || fabs(x1 - data->x[ind[1]])>MAXERROR) {
      printf("Mismatch in x-direction of side %d of %d.\n",i,bound->nosides);
      fclose(in);
      return(4);
    }
    if(fabs(y0 - data->y[ind[0]])>MAXERROR || fabs(y1-data->y[ind[1]])>MAXERROR) {
      printf("Mismatch in y-direction of side %d of %d.\n",i,bound->nosides);
      fclose(in);
      return(5);
    }
  }
  for(j=1; j <= bound->nosides; j++)  
    for(i=1; i <= bound->nosides; i++) 
      fscanf(in,"%le",&bound->vf[j][i]);

  fclose(in);

  if(info) printf("View factors for %d sides were loaded from file %s.\n",
		  bound->nosides,filename);
  return(0);
}



int SaveClosureFactors(struct BoundaryType *bound,char *prefix,int info)
/* Save the factors that couple the knots inside a closure 
   together. 
   */
{
  int sides;
  char file1[MAXFILESIZE],file2[MAXFILESIZE],file3[MAXFILESIZE];

  if(!bound->created) {
    printf("SaveClosureFactors: boundary not created.\n");
    return(1);
  }
  if(!bound->vfcreated  &&  !bound->gfcreated) {
    printf("SaveClosureFactors: no closure factors created.\n");
    return(2);
  }

  sides = bound->nosides;
  if(sides == 0) return(0);

  AddExtension(prefix,file1,"avf");
  AddExtension(prefix,file2,"vf");
  AddExtension(prefix,file3,"gvf");

  SaveRealVector(bound->areas,1,sides,file1);
  if(info) printf("The side areas were saved to file %s.\n",file1);

  if(bound->vfcreated) {
    SaveRealMatrix(bound->vf,1,sides,1,sides,file2);
    if(info) printf("The view factors were saved to file %s.\n",file2);
  }
  if(bound->gfcreated) {
    SaveRealMatrix(bound->gf,1,sides,1,sides,file3);
    if(info) printf("The Gebhart factors were saved to file %s.\n",file3);
  }
  return(0);
}





int SaveElmergrid(struct GridType *grid,int nogrids,char *prefix,int info)
{
  int res,sameline,maxsameline;
  int i,j,dim;
  FILE *out;
  char filename[MAXFILESIZE];

  AddExtension(prefix,filename,"grd");
  out = fopen(filename,"w");
  dim = grid->dimension;
  if(grid->coordsystem == COORD_CART1) dim = 1;

  j = 0;
  sameline = TRUE;
  maxsameline = 6;
  if(grid->xcells > maxsameline) sameline = FALSE;
  if(dim >= 2 && grid->ycells > maxsameline) sameline = FALSE;
  if(dim >= 3 && grid->zcells > maxsameline) sameline = FALSE;
  
  fprintf(out,"***** ElmerGrid input file for structured grid generation *****\n");
  fprintf(out,"Version = 210903\n");

  fprintf(out,"Coordinate System = ");
  if(grid->coordsystem == COORD_AXIS)
    fprintf(out,"2D Axisymmetric\n");
  else if(grid->coordsystem == COORD_POLAR)
    fprintf(out,"2D Polar\n");
  else 
    fprintf(out,"Cartesian %dD\n",dim);
 
  fprintf(out,"Subcell Divisions in %dD = ",dim);
  if(dim >= 1) fprintf(out,"%d ",grid->xcells);
  if(dim >= 2) fprintf(out,"%d ",grid->ycells);
  if(dim >= 3) fprintf(out,"%d ",grid->zcells);
  fprintf(out,"\n");

  fprintf(out,"Subcell Limits 1 %s",sameline ? "= ":"\n  ");
  for(i=0;i <= grid->xcells;i++) 
    fprintf(out,"%.5lg ",grid->x[i]); 
  fprintf(out,"\n");
    
  if(dim >= 2) {
    fprintf(out,"Subcell Limits 2 %s",sameline ? "= ":"\n  ");
    for(i=0;i <= grid->ycells;i++) 
      fprintf(out,"%.5lg ",grid->y[i]); 
    fprintf(out,"\n");
  }
  
  if(dim >= 3) {
    fprintf(out,"Subcell Limits 3 %s",sameline ? "= ":"\n  ");
    for(i=0;i <= grid->zcells;i++) 
      fprintf(out,"%.5lg ",grid->z[i]); 
    fprintf(out,"\n");
  }  

  fprintf(out,"Material Structure in %dD\n",dim==1 ? 1:2);  
  for(j=grid->ycells;j>=1;j--) {
    fprintf(out,"  ");
    for(i=1;i<=grid->xcells;i++) 
      fprintf(out,"%-5d",grid->structure[j][i]);
    fprintf(out,"\n");
  }
  fprintf(out,"End\n");

  if(grid->mappings > 0) {
    fprintf(out,"Geometry Mappings\n");
    fprintf(out,"! mode  line  limits(2)   Np  params(Np)\n");
    for(i=0;i<grid->mappings;i++) {
      fprintf(out,"  %-5d %-5d %-7.5lg %-7.5lg %-3d ",
	      grid->mappingtype[i],grid->mappingline[i],
	      grid->mappinglimits[2*i],grid->mappinglimits[2*i+1],
	      grid->mappingpoints[i]);
      for(j=0;j<grid->mappingpoints[i];j++) 
	fprintf(out,"%.4lg ",grid->mappingparams[i][j]);
      fprintf(out,"\n");
    }
    fprintf(out,"End\n");
  }

  j = 0;
  if(grid[j].rotate) {
    fprintf(out,"Revolve Blocks = %d\n",grid[j].rotateblocks);
    fprintf(out,"Revolve Radius = %-8.3lg\n",grid[j].rotateradius2);
    if(fabs(grid[j].rotateimprove-1.0) > 1.0e-10)
      fprintf(out,"Revolve Improve = %-8.3lg\n",grid[j].rotateimprove);
    
  }
  if(grid[j].rotatecurve) {
    fprintf(out,"Revolve Curve Direct = %-8.3lg\n",grid[j].curvezet);
    fprintf(out,"Revolve Curve Radius = %-8.3lg\n",grid[j].curverad);
    fprintf(out,"Revolve Curve Angle = %-8.3lg\n",grid[j].curveangle);
  }

  if(grid[j].coordsystem == COORD_POLAR) {
    fprintf(out,"Polar Radius = %.3lg\n",grid[j].polarradius);
  } 

  for(j=0;j<nogrids;j++) {
    
    if(j>0) fprintf(out,"\nStart New Mesh\n");
  
    fprintf(out,"Materials Interval = %d %d\n",
	    grid[j].firstmaterial,grid[j].lastmaterial);
  
    if(dim == 3) {
      fprintf(out,"Extruded Structure\n");
      fprintf(out,"! %-8s %-8s %-8s\n","1stmat", "lastmat","newmat");
      for(i=1;i<=grid[j].zcells;i++) 
	fprintf(out,"  %-8d %-8d %-8d\n",
		grid[j].zfirstmaterial[i],grid[j].zlastmaterial[i],
		grid[j].zmaterial[i]); 
      fprintf(out,"End\n");    
    }

    if(grid[j].noboundaries > 0) {
      fprintf(out,"Boundary Definitions\n");
      fprintf(out,"! %-8s %-8s %-8s\n","type","out","int"); 
      for(i=0;i<grid[j].noboundaries;i++)
	fprintf(out,"  %-8d %-8d %-8d %-8d\n",
		grid[j].boundtype[i],grid[j].boundext[i],
		grid[j].boundint[i], grid[j].boundsolid[i]);
      fprintf(out,"End\n");
    }

    if(grid->numbering == NUMBER_XY)
      fprintf(out,"Numbering = Horizontal\n");
    if(grid->numbering == NUMBER_YX)
      fprintf(out,"Numbering = Vertical\n");
        
    fprintf(out,"Element Degree = %d\n",grid[j].elemorder);
    fprintf(out,"Element Innernodes = %s\n",grid[j].elemmidpoints ? "True" : "False");
    fprintf(out,"Triangles = %s\n",grid[j].triangles ? "True" : "False");
    if(grid[j].autoratio) 
      fprintf(out,"Surface Elements = %d\n",grid[j].wantedelems);
    if(dim == 3 && grid[j].wantedelems3d) 
      fprintf(out,"Volume Elements = %d\n",grid[j].wantedelems3d);
    if(dim == 3 && grid[j].wantednodes3d) 
      fprintf(out,"Volume Nodes = %d\n",grid[j].wantednodes3d);

    if(dim == 2)
      fprintf(out,"Coordinate Ratios = %-8.3lg\n",grid[j].xyratio);
    if(dim == 3)
      fprintf(out,"Coordinate Ratios = %-8.3lg %-8.3lg\n",
	      grid[j].xyratio,grid[j].xzratio);
 
    fprintf(out,"Minimum Element Divisions = %d",grid[j].minxelems);
    if(dim >= 2) fprintf(out," %d",grid[j].minyelems);
    if(dim >= 3) fprintf(out," %d",grid[j].minzelems);
    fprintf(out,"\n");

    fprintf(out,"Element Ratios 1 %s",sameline ? "= ":"\n  ");
    for(i=1;i<=grid[j].xcells;i++) 
      fprintf(out,"%.3lg ",grid[j].xexpand[i]); 
    fprintf(out,"\n");
    if(dim >= 2) {
      fprintf(out,"Element Ratios 2 %s",sameline ? "= ":"\n  ");
      for(i=1;i<=grid[j].ycells;i++) 
	fprintf(out,"%.3lg ",grid[j].yexpand[i]); 
      fprintf(out,"\n");
    }
    if(dim >= 3) {
      fprintf(out,"Element Ratios 3 %s",sameline ? "= ":"\n  ");
      for(i=1;i<=grid[j].zcells;i++) 
	fprintf(out,"%.3lg ",grid[j].zexpand[i]); 
      fprintf(out,"\n");
    }

    if(grid[j].autoratio) {
      fprintf(out,"Element Densities 1 %s",sameline ? "= ":"\n  ");
      for(i=1;i<=grid[j].xcells;i++) 
	fprintf(out,"%.3lg ",grid[j].xdens[i]); 
      fprintf(out,"\n");
      if(dim >= 2) {
	fprintf(out,"Element Densities 2 %s",sameline ? "= ":"\n  ");
	for(i=1;i<=grid[j].ycells;i++) 
	  fprintf(out,"%.3lg ",grid[j].ydens[i]); 
	fprintf(out,"\n");
      }
      if(dim >= 3) {       
	fprintf(out,"Element Densities 3 %s",sameline ? "= ":"\n  ");
	for(i=1;i<=grid[j].zcells;i++) 
	  fprintf(out,"%.3lg ",grid[j].zdens[i]); 
	fprintf(out,"\n");
      }
    }
    else {
      fprintf(out,"Element Divisions 1 %s",sameline ? "= ":"\n  ");
      for(i=1;i<=grid[j].xcells;i++) 
	fprintf(out,"%d ",grid[j].xelems[i]); 
      fprintf(out,"\n");
      if(dim >= 2) {
	fprintf(out,"Element Divisions 2 %s",sameline ? "= ":"\n  ");
	for(i=1;i<=grid[j].ycells;i++) 
	  fprintf(out,"%d ",grid[j].yelems[i]); 
	fprintf(out,"\n");
      }
      if(dim >= 3) {       
	fprintf(out,"Element Divisions 3 %s",sameline ? "= ":"\n  ");
	for(i=1;i<=grid[j].zcells;i++) 
	  fprintf(out,"%d ",grid[j].zelems[i]); 
	fprintf(out,"\n");
      }
    }
    

  }

  if(info) printf("The Elmergrid input was saved to file %s.\n",filename);
  fclose(out);

  return(0);
}




int LoadElmergridOld(struct GridType **grid,int *nogrids,char *prefix,int info) 
{
  char filename[MAXFILESIZE];
  FILE *in;
  int i,j,k,l,error=0;
  struct GridType grid0;
  Real scaling;
  char *cp;
  int mode,noknots,noelements,dim,axisymmetric;
  int elemcode,maxnodes,totelems,nogrids0;
  int minmat,maxmat;

  AddExtension(prefix,filename,"grd");
  if ((in = fopen(filename,"r")) == NULL) {
    printf("LoadElmergrid: opening of the file '%s' wasn't succesfull !\n",filename);
    return(1);
  }

  if(info) printf("Loading the geometry from file '%s'.\n",filename);

  InitGrid(grid[*nogrids]);
  k = *nogrids;
  nogrids0 = *nogrids;

  mode = 0;
  noknots = 0;
  noelements = 0;
  dim = 0;
  axisymmetric = FALSE;
  elemcode = 0;
  maxnodes = 4;
  totelems = 0;
  scaling = 1.0;



  Getline(line,in);
  for(;;) {
    if(Getline(line,in)) goto end;
    if(!line) goto end;
    if(strstr(line,"END")) goto end;
    if(strstr(line,"RESULTS")) goto end;

    /* Control information */
    if(strstr(line,"VERSION")) mode = 1;
    else if(strstr(line,"GEOMETRY")) mode = 2;
    else if(strstr(line,"MAPPINGS IN")) mode = 31;
    else if(strstr(line,"MAPPINGS OUT")) mode = 32;
    else if(strstr(line,"MAPPINGS")) mode = 3;
    else if(strstr(line,"NUMBERING")) mode = 4;
    else if(strstr(line,"MESHING")) mode = 5;
    else if(strstr(line,"ELEMENTS")) mode = 6;
    else if(strstr(line,"ELEMENT NUMBER")) mode = 29;
    else if(strstr(line,"NODES")) mode = 7;
    else if(strstr(line,"TRIANGLE")) mode = 8;
    else if(strstr(line,"SQUARE")) mode = 17;
    else if(strstr(line,"COORDINATE RATIO"))  mode = 10;
    else if(strstr(line,"MATERIALS")) mode = 11;
    else if(strstr(line,"LAYERED ST")) mode = 12;
    else if(strstr(line,"ELEMENT RAT")) mode = 13;
    else if(strstr(line,"ELEMENT DENS")) mode = 14;
    else if(strstr(line,"ELEMENT MINIMUM")) mode = 27;
    else if(strstr(line,"BOUNDARY COND")) mode = 15;
    else if(strstr(line,"ELEMENTTYPE") || strstr(line,"ELEMENTCODE")) mode = 16;
    else if(strstr(line,"ROTATE")) mode = 20;
    else if(strstr(line,"ROTRAD")) mode = 21;
    else if(strstr(line,"ROTBLOCK")) mode = 22;
    else if(strstr(line,"ROTIMP")) mode = 24;
    else if(strstr(line,"ROTCURVE")) mode = 25;
    else if(strstr(line,"REDUCE ELEMENT")) mode = 26;
    else if(strstr(line,"SCALING")) mode = 23;
    else if(strstr(line,"LAYERED BO")) mode = 28;
    else if(strstr(line,"POLAR RADIUS")) mode = 30;


    switch (mode) {
    case 1: 
      printf("Loading Elmergrid file: %s\n",line);
      mode = 0;
      break;
      
    case 2:
      grid[k]->dimension = 2;
      if(strstr(line,"CARTES") && strstr(line,"1D")) {
	grid[k]->coordsystem = COORD_CART1;
	grid[k]->dimension = 1;
      }
      else if(strstr(line,"CARTES") && strstr(line,"2D")) 
	grid[k]->coordsystem = COORD_CART2;
      else if(strstr(line,"AXIS") && strstr(line,"2D")) 
	grid[k]->coordsystem = COORD_AXIS;
      else if(strstr(line,"POLAR") && strstr(line,"2D")) 
	grid[k]->coordsystem = COORD_POLAR;
      else if(strstr(line,"CARTES") && strstr(line,"3D")) {
	grid[k]->coordsystem = COORD_CART3;
	grid[k]->dimension = 3;
      }
      else printf("Unknown coordinate system: %s\n",line);
      printf("Defining the coordinate system (%d-DIM).\n",grid[k]->dimension);

      Getline(line,in);

      if(grid[k]->dimension == 1) {
	sscanf(line,"%d",&(*grid)[k].xcells);
	grid[k]->ycells = 1;	
      }
      if(grid[k]->dimension == 2) 
	sscanf(line,"%d %d",&(*grid)[k].xcells,&(*grid)[k].ycells);
      if(grid[k]->dimension == 3) 
	sscanf(line,"%d %d %d",&(*grid)[k].xcells,&(*grid)[k].ycells,&(*grid)[k].zcells);      
      if(grid[k]->xcells >= MAXCELLS || grid[k]->ycells >= MAXCELLS || 
	 grid[k]->zcells >= MAXCELLS) {
	printf("LoadGrid: Too many subcells [%d %d %d] vs. %d:\n",
	       grid[k]->xcells,grid[k]->ycells,grid[k]->zcells,MAXCELLS);
      }
      
      if(grid[k]->dimension == 1) {
	printf("Loading [%d] subcell intervals in 1D\n",
	       grid[k]->xcells);
      }
      else if(grid[k]->dimension == 2) {
	printf("Loading [%d %d] subcell intervals in 2D\n",
	       grid[k]->xcells,grid[k]->ycells);   
      } else {
	printf("Loading [%d %d %d] subcell intervals in 3D\n",
	       grid[k]->xcells,grid[k]->ycells,grid[k]->zcells);   
      }


      for(j=1;j<=grid[k]->dimension;j++) {
	Getline(line,in);
	cp=line;

	if(j==1) for(i=0;i<=grid[k]->xcells;i++) grid[k]->x[i] = next_real(&cp);
	if(j==2) for(i=0;i<=grid[k]->ycells;i++) grid[k]->y[i] = next_real(&cp);
	if(j==3) for(i=0;i<=grid[k]->zcells;i++) grid[k]->z[i] = next_real(&cp);
      }

      printf("Loading material structure\n");

      for(j=grid[k]->ycells;j>=1;j--) {
	
	Getline(line,in);
	cp=line;
	
	for(i=1;i<=grid[k]->xcells;i++) 
	  grid[k]->structure[j][i] = next_int(&cp);
      }

      minmat = maxmat = grid[k]->structure[1][1];
      for(j=grid[k]->ycells;j>=1;j--) 
	for(i=1;i<=grid[k]->xcells;i++) {
	  if(minmat > grid[k]->structure[j][i])
	    minmat = grid[k]->structure[j][i];
	  if(maxmat < grid[k]->structure[j][i])
	    maxmat = grid[k]->structure[j][i];
	}      
      if(minmat < 0) 
	printf("LoadElmergrid: please use positive material indices.\n");
      if(maxmat > MAXMATERIALS) 
	printf("LoadElmergrid: material indices larger to %d may create problems.\n",
	       MAXMATERIALS);
      mode = 0;
      break;

    case 3:
    case 31:
    case 32:

      sscanf(line,"%d",&l);

      for(i=grid[k]->mappings;i<grid[k]->mappings+l;i++) {
	Getline(line,in);
	cp=line; 

	grid[k]->mappingtype[i] = next_int(&cp);
	if(mode == 32) grid[k]->mappingtype[i] += 50*SGN(grid[k]->mappingtype[i]);

	grid[k]->mappingline[i] = next_int(&cp);
	grid[k]->mappinglimits[2*i] = next_real(&cp);
	grid[k]->mappinglimits[2*i+1] = next_real(&cp);
	grid[k]->mappingpoints[i] = next_int(&cp);
	grid[k]->mappingparams[i] = Rvector(0,grid[k]->mappingpoints[i]);
	for(j=0;j<grid[k]->mappingpoints[i];j++) 
	  grid[k]->mappingparams[i][j] = next_real(&cp);
      }
      
      printf("Loaded %d geometry mappings\n",l);
      grid[k]->mappings += l;

      mode = 0;
      break;
      
    case 4: /* NUMBERING */
      if(strstr(line,"HORIZ")) grid[k]->numbering = NUMBER_XY;
      if(strstr(line,"VERTI")) grid[k]->numbering = NUMBER_YX;
      mode = 0;
      break;

    case 5: /* MESHING */
      if((*nogrids) >= MAXCASES) {
	printf("There are more grids than was allocated for!\n"); 
	printf("Ignoring meshes starting from %d\n.",(*nogrids)+1);
	goto end;
      }
      (*nogrids)++;
      printf("Loading element meshing no %d\n",*nogrids);
      k = *nogrids - 1;	           
      if(k > nogrids0) (*grid)[k] = (*grid)[k-1];	 
      mode = 0;
      break;

    case 6: /* ELEMENTS */
      sscanf(line,"%d",&(*grid)[k].wantedelems);
      mode = 0;
      break;

    case 7: /* NODES */
      sscanf(line,"%d",&(*grid)[k].nonodes);      
      
      (*grid)[k].elemmidpoints = FALSE;
      if((*grid)[k].nonodes == 4) 
	(*grid)[k].elemorder = 1;
      if((*grid)[k].nonodes == 8) 
	(*grid)[k].elemorder = 2;
      if((*grid)[k].nonodes == 16) 
	(*grid)[k].elemorder = 3;

      if((*grid)[k].nonodes == 9) { 
	(*grid)[k].elemorder = 2;
	(*grid)[k].elemmidpoints = TRUE;
      }
      if((*grid)[k].nonodes == 12) { 
	(*grid)[k].elemorder = 3;
	(*grid)[k].elemmidpoints = TRUE;
      }


      mode = 0;
      break;

    case 8: /* TRIANGLES */
      (*grid)[k].triangles = TRUE;
      mode = 0;
      break;

    case 17: /* SQUARES */
      (*grid)[k].triangles = FALSE;
      mode = 0;
      break;

    case 16: /* ELEMENTTYPE and ELEMENTCODE */
      sscanf(line,"%d",&elemcode);
      if(elemcode/100 == 2) {
	(*grid)[k].triangles = FALSE;      
	(*grid)[k].nonodes = elemcode%100;
      }
      else if(elemcode/100 == 4) {
	(*grid)[k].triangles = FALSE;      
	(*grid)[k].nonodes = elemcode%100;
      }
      else if(elemcode/100 == 3) {  
	(*grid)[k].triangles = TRUE;      
	if(elemcode%100 == 3)       (*grid)[k].nonodes = 4;
	else if(elemcode%100 == 6)  (*grid)[k].nonodes = 9;
	else if(elemcode%100 == 10) (*grid)[k].nonodes = 16;	
      }

      (*grid)[k].elemmidpoints = FALSE;
      if((*grid)[k].nonodes == 4) 
	(*grid)[k].elemorder = 1;
      if((*grid)[k].nonodes == 8) 
	(*grid)[k].elemorder = 2;
      if((*grid)[k].nonodes == 16) 
	(*grid)[k].elemorder = 3;

      if((*grid)[k].nonodes == 9) { 
	(*grid)[k].elemorder = 2;
	(*grid)[k].elemmidpoints = TRUE;
      }
      if((*grid)[k].nonodes == 12) { 
	(*grid)[k].elemorder = 3;
	(*grid)[k].elemmidpoints = TRUE;
      }

      mode = 0;
      break;

    case 10: /* COORDINATE RATIO */
      if((*grid)[k].dimension == 2) 
	sscanf(line,"%le",&(*grid)[k].xyratio);
      if((*grid)[k].dimension == 3) 
	sscanf(line,"%le %le",&(*grid)[k].xyratio,&(*grid)[k].xzratio);      
      mode = 0;
      break;

    case 11: /* MATERIALS */
      sscanf(line,"%d %d",&(*grid)[k].firstmaterial,&(*grid)[k].lastmaterial);      
      mode = 0;
      break;

    case 12: /* LAYERES */
      for(i=1;i<=(*grid)[k].zcells;i++) {
	Getline(line,in);
	sscanf(line,"%d %d %d\n",
		&(*grid)[k].zfirstmaterial[i],&(*grid)[k].zlastmaterial[i],&(*grid)[k].zmaterial[i]); 
      }
      mode = 0;
      break;

    case 13: /* ELEMENT RATIOS */
      printf("Loading element ratios\n");

      for (j=1;j<=(*grid)[k].dimension;j++) {
	Getline(line,in);
	cp = line;

	if(j==1) for(i=1;i<=(*grid)[k].xcells;i++) (*grid)[k].xexpand[i] = next_real(&cp);
	if(j==2) for(i=1;i<=(*grid)[k].ycells;i++) (*grid)[k].yexpand[i] = next_real(&cp);
	if(j==3) for(i=1;i<=(*grid)[k].zcells;i++) (*grid)[k].zexpand[i] = next_real(&cp);
      }
      mode = 0;
      break;

    case 29: /* ELEMENT NUMBER */
      printf("Loading element numbers\n");

      for (j=1;j<=(*grid)[k].dimension;j++) {
	Getline(line,in);
	cp = line;
	if(j==1) for(i=1;i<=(*grid)[k].xcells;i++) (*grid)[k].xelems[i] = next_int(&cp);
	if(j==2) for(i=1;i<=(*grid)[k].ycells;i++) (*grid)[k].yelems[i] = next_int(&cp);
	if(j==3) for(i=1;i<=(*grid)[k].zcells;i++) (*grid)[k].zelems[i] = next_int(&cp);
      }
      (*grid)[k].autoratio = 0;
      mode = 0;
      break;

    case 27: /* ELEMENT MINIMUM */
      printf("Loading minimum number of elements\n");
      if((*grid)[k].dimension == 1) 
	sscanf(line,"%d",&(*grid)[k].minxelems);
      if((*grid)[k].dimension == 2) 
	sscanf(line,"%d %d",&(*grid)[k].minxelems,&(*grid)[k].minyelems);
      if((*grid)[k].dimension == 3) 
	sscanf(line,"%d %d %d",&(*grid)[k].minxelems,&(*grid)[k].minyelems,&(*grid)[k].minzelems);
      mode = 0;
      break;

    case 14: /* ELEMENT DENSITIES */
      printf("Loading element densities\n");
      for (j=1;j<=(*grid)[k].dimension;j++) {
	Getline(line,in);
	cp = line;

	if(j==1) for(i=1;i<=(*grid)[k].xcells;i++) (*grid)[k].xdens[i] = next_real(&cp);
	if(j==2) for(i=1;i<=(*grid)[k].ycells;i++) (*grid)[k].ydens[i] = next_real(&cp);
	if(j==3) for(i=1;i<=(*grid)[k].zcells;i++) (*grid)[k].zdens[i] = next_real(&cp);
      }
      mode = 0;
      break;

    case 15: /* BOUNDARY CONDITIONS */
      sscanf(line,"%d",&(*grid)[k].noboundaries);
      printf("Loading %d boundary conditions\n",(*grid)[k].noboundaries);

      for(i=0;i<(*grid)[k].noboundaries;i++) {
	Getline(line,in);
	sscanf(line,"%d %d %d %d",
	       &(*grid)[k].boundtype[i],&(*grid)[k].boundext[i],
	       &(*grid)[k].boundint[i],&(*grid)[k].boundsolid[i]);
      }  
      mode = 0;
      break;

    case 20: /* ROTATE */
      (*grid)[k].rotate = TRUE;
      mode = 0;
      break;

    case 21: /* ROTRAD */
      sscanf(line,"%le",&(*grid)[k].rotateradius2);
      mode = 0;
      break;

    case 22: /* ROTBLOCK */
      sscanf(line,"%d",&(*grid)[k].rotateblocks);
      if(0) printf("Reading blocks %d\n",(*grid)[k].rotateblocks);
      mode = 0;
      break;

    case 24: /* ROTIMP */
      sscanf(line,"%le",&(*grid)[k].rotateimprove);
      mode = 0;
      break;

    case 30: /* POLAR RADIUS */
      sscanf(line,"%le",&(*grid)[k].polarradius);
      mode = 0;
      break;

    case 25: /* ROTCURVE */
      (*grid)[k].rotatecurve = TRUE;
      sscanf(line,"%le%le%le",&(*grid)[k].curvezet,
	     &(*grid)[k].curverad,&(*grid)[k].curveangle);
      mode = 0;
      break;

    case 26: /* REDUCE ELEMENT */
      sscanf(line,"%d%d",&(*grid)[k].reduceordermatmin,
	     &(*grid)[k].reduceordermatmax);
      mode = 0;
      break;

    case 28: /* LAYERED BO */
      sscanf(line,"%d",&(*grid)[k].layeredbc);
      mode = 0;
      break;

    case 23: /* SCALING */
      sscanf(line,"%le",&scaling);
      for(i=0;i<=(*grid)[k].xcells;i++) (*grid)[k].x[i] *= scaling;
      if((*grid)[k].dimension > 1) 
	for(i=0;i<=(*grid)[k].ycells;i++) (*grid)[k].y[i] *= scaling;
      if((*grid)[k].dimension == 3) 
	for(i=0;i<=(*grid)[k].ycells;i++) (*grid)[k].z[i] *= scaling;

      (*grid)[k].rotateradius2 *= scaling;
      (*grid)[k].curverad *= scaling;
      (*grid)[k].curvezet *= scaling;
      mode = 0;
      break;

    default:
      printf("Unknown case: %s",line);
    }

  }

end:

  if(info) printf("Found %d divisions for grid\n",*nogrids);

  for(k=nogrids0;k < (*nogrids) && k<MAXCASES;k++) {
    SetElementDivision(&(*grid)[k],info);
  }


  fclose(in);
  return(error);
}



int LoadElmergrid(struct GridType **grid,int *nogrids,char *prefix,int info) 
{
  char filename[MAXFILESIZE];
  char command[MAXLINESIZE],params[MAXLINESIZE];
  FILE *in;
  int i,j,k,l,error=0;
  struct GridType grid0;
  char *cp;
  int noknots,noelements,dim,axisymmetric;
  int elemcode,maxnodes,totelems,nogrids0,minmat,maxmat;
  long code;
  Real raid;

  AddExtension(prefix,filename,"grd");
  if ((in = fopen(filename,"r")) == NULL) {
    printf("LoadElmergrid: opening of the file '%s' wasn't succesfull !\n",filename);
    return(1);
  }

  if(info) printf("Loading the geometry from file '%s'.\n",filename);

  InitGrid(grid[*nogrids]);
  k = *nogrids;
  nogrids0 = *nogrids;

  noknots = 0;
  noelements = 0;
  dim = 0;
  axisymmetric = FALSE;
  elemcode = 0;
  maxnodes = 4;
  totelems = 0;

  for(;;) {
    if(GetCommand(command,params,in)) {
      printf("Reached the end of command file\n");
      goto end;
    }    

    /* Control information */
    if(strstr(command,"VERSION")) {
      if(strstr(command,"080500")) {
	printf("Loading old version of Elmergrid file.\n");
	i = LoadElmergridOld(grid,nogrids,prefix,info);
	return(i);
      }
      else {
	sscanf(params,"%ld",&code);
	if(code == 210903) 
	  printf("Loading ElmerGrid file version: %d\n",code);
	else {
	  printf("Unknown ElmerGrid file version: %d\n",code);
	  return(2);
	}
      }
      *nogrids += 1;
    }      
    
    else if(strstr(command,"COORDINATE SYSTEM")) {
      for(i=0;i<MAXLINESIZE;i++) params[i] = toupper(params[i]);
      grid[k]->dimension = 2;
      if(strstr(params,"CARTESIAN 1D")) {
	grid[k]->coordsystem = COORD_CART1;
	grid[k]->dimension = 1;
      }
      else if(strstr(params,"CARTESIAN 2D")) 
	grid[k]->coordsystem = COORD_CART2;
      else if(strstr(params,"AXISYMMETRIC")) 
	grid[k]->coordsystem = COORD_AXIS;
      else if(strstr(params,"POLAR"))
	grid[k]->coordsystem = COORD_POLAR;
      else if(strstr(params,"CARTESIAN 3D")) {
	grid[k]->coordsystem = COORD_CART3;
	grid[k]->dimension = 3;
      }
      else printf("Unknown coordinate system: %s\n",params);
      printf("Defining the coordinate system (%d-DIM).\n",grid[k]->dimension);
    }
    
    else if(strstr(command,"SUBCELL DIVISIONS")) {
      if(grid[k]->dimension == 1) {
	sscanf(params,"%d",&(*grid)[k].xcells);
	grid[k]->ycells = 1;	
      }
      else if(grid[k]->dimension == 2) 
	sscanf(params,"%d %d",&(*grid)[k].xcells,&(*grid)[k].ycells);
      else if(grid[k]->dimension == 3) 
	sscanf(params,"%d %d %d",&(*grid)[k].xcells,&(*grid)[k].ycells,&(*grid)[k].zcells);      
      if(grid[k]->xcells >= MAXCELLS || grid[k]->ycells >= MAXCELLS || grid[k]->zcells >= MAXCELLS) {
	printf("LoadElmergrid: Too many subcells [%d %d %d] vs. %d:\n",
	       grid[k]->xcells,grid[k]->ycells,grid[k]->zcells,MAXCELLS);
      }
    }
    
    else if(strstr(command,"MINIMUM ELEMENT DIVISION")) {
      printf("Loading minimum number of elements\n");
      if((*grid)[k].dimension == 1) 
	sscanf(params,"%d",&(*grid)[k].minxelems);
      if((*grid)[k].dimension == 2) 
	sscanf(params,"%d %d",&(*grid)[k].minxelems,&(*grid)[k].minyelems);
      if((*grid)[k].dimension == 3) 
	sscanf(params,"%d %d %d",&(*grid)[k].minxelems,&(*grid)[k].minyelems,&(*grid)[k].minzelems);
    }      
    
    else if(strstr(command,"SUBCELL LIMITS 1")) {
      printf("Loading [%d] subcell limits in X-direction\n",grid[k]->xcells+1);
      cp = params;
      for(i=0;i<=grid[k]->xcells;i++) grid[k]->x[i] = next_real(&cp);
    }    
    else if(strstr(command,"SUBCELL LIMITS 2")) {
      printf("Loading [%d] subcell limits in Y-direction\n",grid[k]->ycells+1);
      cp = params;
      for(i=0;i<=grid[k]->ycells;i++) grid[k]->y[i] = next_real(&cp);
    }      
    else if(strstr(command,"SUBCELL LIMITS 3")) {
      printf("Loading [%d] subcell limits in Z-direction\n",grid[k]->zcells+1);
      cp = params;
      for(i=0;i<=grid[k]->zcells;i++) grid[k]->z[i] = next_real(&cp);
    }

    else if(strstr(command,"SUBCELL SIZES 1")) {
      printf("Loading [%d] subcell sizes in X-direction\n",grid[k]->xcells);
      cp = params;
      for(i=1;i<=grid[k]->xcells;i++) grid[k]->x[i] = next_real(&cp);
      for(i=1;i<=grid[k]->xcells;i++) grid[k]->x[i] = grid[k]->x[i-1] + grid[k]->x[i];
    }      
    else if(strstr(command,"SUBCELL SIZES 2")) {
      printf("Loading [%d] subcell sizes in Y-direction\n",grid[k]->ycells);
      cp = params;
      for(i=1;i<=grid[k]->ycells;i++) grid[k]->y[i] = next_real(&cp);
      for(i=1;i<=grid[k]->ycells;i++) grid[k]->y[i] = grid[k]->y[i-1] + grid[k]->y[i];
    }      
    else if(strstr(command,"SUBCELL SIZES 3")) {
      printf("Loading [%d] subcell sizes in Z-direction\n",grid[k]->zcells);
      cp = params;
      for(i=1;i<=grid[k]->zcells;i++) grid[k]->z[i] = next_real(&cp);
      for(i=1;i<=grid[k]->zcells;i++) grid[k]->z[i] = grid[k]->z[i-1] + grid[k]->z[i];
    }

    else if(strstr(command,"SUBCELL ORIGIN 1")) {
      for(i=0;i<MAXLINESIZE;i++) params[i] = toupper(params[i]);
      if(strstr(params,"CENTER")) {
	raid = 0.5 * (grid[k]->x[0] + grid[k]->x[grid[k]->xcells]);
      }
      else if(strstr(params,"LEFT")) {
	raid = grid[k]->x[0];
      }
      else if(strstr(params,"RIGHT")) {
	raid = grid[k]->x[grid[k]->xcells];
      }
      else {
	cp = params;
	raid = next_real(&cp);
      }
      for(i=0;i<=grid[k]->xcells;i++) grid[k]->x[i] -= raid;
    }
    else if(strstr(command,"SUBCELL ORIGIN 2")) {
      for(i=0;i<MAXLINESIZE;i++) params[i] = toupper(params[i]);
      if(strstr(params,"CENTER")) {
	raid = 0.5 * (grid[k]->y[0] + grid[k]->y[grid[k]->ycells]);
      }
      else if(strstr(params,"LEFT")) {
	raid = grid[k]->y[0];
      }
      else if(strstr(params,"RIGHT")) {
	raid = grid[k]->y[grid[k]->ycells];
      }
      else {
	cp = params;
	raid = next_real(&cp);
      }      
      for(i=0;i<=grid[k]->ycells;i++) grid[k]->y[i] -= raid;
    }
    else if(strstr(command,"SUBCELL ORIGIN 3")) {
      for(i=0;i<MAXLINESIZE;i++) params[i] = toupper(params[i]);
      if(strstr(params,"CENTER")) {
	raid = 0.5 * (grid[k]->z[0] + grid[k]->z[grid[k]->zcells]);
      }
      else if(strstr(params,"LEFT")) {
	raid = grid[k]->z[0];
      }
      else if(strstr(params,"RIGHT")) {
	raid = grid[k]->z[grid[k]->zcells];
      }
      else {
	cp = params;
	raid = next_real(&cp);
      }
      for(i=0;i<=grid[k]->zcells;i++) grid[k]->z[i] -= raid;      
    }

    else if(strstr(command,"MATERIAL STRUCTURE")) {
      printf("Loading material structure\n");
      
      for(j=grid[k]->ycells;j>=1;j--) {
	if(j < grid[k]->ycells) Getline(params,in);
	cp=params;
	for(i=1;i<=grid[k]->xcells;i++) 
	  grid[k]->structure[j][i] = next_int(&cp);
      }      
      minmat = maxmat = grid[k]->structure[1][1];
      for(j=grid[k]->ycells;j>=1;j--) 
	for(i=1;i<=grid[k]->xcells;i++) {
	  if(minmat > grid[k]->structure[j][i])
	    minmat = grid[k]->structure[j][i];
	  if(maxmat < grid[k]->structure[j][i])
	    maxmat = grid[k]->structure[j][i];
	}      
      if(minmat < 0) 
	printf("LoadElmergrid: please use positive material indices.\n");
      if(maxmat > MAXMATERIALS) 
	printf("LoadElmergrid: material indices larger to %d may create problems.\n",
	       MAXMATERIALS);
    }
    else if(strstr(command,"MATERIALS INTERVAL")) {
      sscanf(params,"%d %d",&(*grid)[k].firstmaterial,&(*grid)[k].lastmaterial);      
    }
     
    else if(strstr(command,"REVOLVE")) {
      if(strstr(command,"REVOLVE RADIUS")) {
	(*grid)[k].rotate = TRUE;
	sscanf(params,"%le",&(*grid)[k].rotateradius2);
      }
      else if(strstr(command,"REVOLVE BLOCKS")) {
	(*grid)[k].rotate = TRUE;
	sscanf(params,"%d",&(*grid)[k].rotateblocks);
      }
      else if(strstr(command,"REVOLVE IMPROVE")) {
	(*grid)[k].rotate = TRUE;
	sscanf(params,"%le",&(*grid)[k].rotateimprove);
      }
      else if(strstr(command,"REVOLVE RADIUS")) {
	sscanf(params,"%le",&(*grid)[k].polarradius);
      }
      else if(strstr(command,"REVOLVE CURVE DIRECT")) {
	(*grid)[k].rotatecurve = TRUE;
	sscanf(params,"%le",&(*grid)[k].curvezet);
      }
      else if(strstr(command,"REVOLVE CURVE RADIUS")) {
	(*grid)[k].rotatecurve = TRUE;
	sscanf(params,"%le",&(*grid)[k].curverad);
      }
      else if(strstr(command,"REVOLVE CURVE ANGLE")) {
	(*grid)[k].rotatecurve = TRUE;
	sscanf(params,"%le",&(*grid)[k].curveangle);
      }
    }

    else if(strstr(command,"REDUCE ORDER INTERVAL")) {
      sscanf(params,"%d%d",&(*grid)[k].reduceordermatmin,
	     &(*grid)[k].reduceordermatmax);
    }
    
    else if(strstr(command,"BOUNDARY DEFINITION")) {
      printf("Loading boundary conditions\n");
      
      for(i=0;i<MAXBOUNDARIES;i++) {
	if(i>0) Getline(params,in);
	for(j=0;j<MAXLINESIZE;j++) params[j] = toupper(params[j]);
	if(strstr(params,"END")) break;
	sscanf(params,"%d %d %d %d",
	       &(*grid)[k].boundtype[i],&(*grid)[k].boundext[i],
	       &(*grid)[k].boundint[i],&(*grid)[k].boundsolid[i]);
      }  
      printf("Found %d boundaries\n",i);
      (*grid)[k].noboundaries = i;
    }
    
    else if(strstr(command,"LAYERED BOUNDARIES")) {
      for(i=0;i<MAXLINESIZE;i++) params[i] = toupper(params[i]);
      if(strstr(params,"TRUE")) (*grid)[k].layeredbc = 1;
      if(strstr(params,"FALSE")) (*grid)[k].layeredbc = 0;
    }
    
    else if(strstr(command,"NUMBERING")) {
      for(i=0;i<MAXLINESIZE;i++) params[i] = toupper(params[i]);
      if(strstr(params,"HORIZONATAL")) (*grid)[k].numbering = NUMBER_XY;
      if(strstr(params,"VERTICAL")) (*grid)[k].numbering = NUMBER_YX;
    }
    
    else if(strstr(command,"ELEMENT DEGREE")) {
      sscanf(params,"%d",&(*grid)[k].elemorder);
    }
    
    else if(strstr(command,"ELEMENT INNERNODES")) {
      for(i=0;i<MAXLINESIZE;i++) params[i] = toupper(params[i]);
      if(strstr(params,"TRUE")) (*grid)[k].elemmidpoints = TRUE;
      if(strstr(params,"FALSE")) (*grid)[k].elemmidpoints = FALSE;
    }
    else if(strstr(command,"ELEMENTTYPE") || strstr(command,"ELEMENTCODE")) {
      sscanf(params,"%d",&elemcode);
    }
    
    else if(strstr(command,"TRIANGLES")) {
      for(i=0;i<MAXLINESIZE;i++) params[i] = toupper(params[i]);
      if(strstr(params,"TRUE")) (*grid)[k].triangles = TRUE;
      if(strstr(params,"FALSE")) (*grid)[k].triangles = FALSE;
    }
    
    else if(strstr(command,"PLANE ELEMENTS")) {
      sscanf(params,"%d",&(*grid)[k].wantedelems);
    }
    else if(strstr(command,"SURFACE ELEMENTS")) {
      sscanf(params,"%d",&(*grid)[k].wantedelems);
    }
    
    else if(strstr(command,"VOLUME ELEMENTS")) {
      sscanf(params,"%d",&(*grid)[k].wantedelems3d);
    }
    else if(strstr(command,"VOLUME NODES")) {
      sscanf(params,"%d",&(*grid)[k].wantednodes3d);
    }

    else if(strstr(command,"COORDINATE RATIO")) {
      if((*grid)[k].dimension == 2) 
	sscanf(params,"%le",&(*grid)[k].xyratio);
      if((*grid)[k].dimension == 3) 
	sscanf(params,"%le %le",&(*grid)[k].xyratio,&(*grid)[k].xzratio);      
    }
    
    else if(strstr(command,"ELEMENT RATIOS 1")) {
      cp = params;
      for(i=1;i<=(*grid)[k].xcells;i++) (*grid)[k].xexpand[i] = next_real(&cp);
    }
    else if(strstr(command,"ELEMENT RATIOS 2")) {
      cp = params;
      for(i=1;i<=(*grid)[k].ycells;i++) (*grid)[k].yexpand[i] = next_real(&cp);
    }
    else if(strstr(command,"ELEMENT RATIOS 3")) {
      cp = params;
      for(i=1;i<=(*grid)[k].zcells;i++) (*grid)[k].zexpand[i] = next_real(&cp);
    }
    
    else if(strstr(command,"ELEMENT DENSITIES 1")) {
      cp = params;
      for(i=1;i<=(*grid)[k].xcells;i++) (*grid)[k].xdens[i] = next_real(&cp);
    }
    else if(strstr(command,"ELEMENT DENSITIES 2")) {
      cp = params;
      for(i=1;i<=(*grid)[k].ycells;i++) (*grid)[k].ydens[i] = next_real(&cp);
    }
    else if(strstr(command,"ELEMENT DENSITIES 3")) {
      cp = params;
      for(i=1;i<=(*grid)[k].zcells;i++) (*grid)[k].zdens[i] = next_real(&cp);
    }
    
    else if(strstr(command,"ELEMENT DIVISIONS 1")) {
      cp = params;
      for(i=1;i<=(*grid)[k].xcells;i++) (*grid)[k].xelems[i] = next_int(&cp);
      (*grid)[k].autoratio = 0;
    }
    else if(strstr(command,"ELEMENT DIVISIONS 2")) {
      cp = params;
      for(i=1;i<=(*grid)[k].ycells;i++) (*grid)[k].yelems[i] = next_int(&cp);
      (*grid)[k].autoratio = 0;
    }
    else if(strstr(command,"ELEMENT DIVISIONS 3")) {
      cp = params;
      for(i=1;i<=(*grid)[k].zcells;i++) (*grid)[k].zelems[i] = next_int(&cp);
      (*grid)[k].autoratio = 0;
    }
    
    else if(strstr(command,"EXTRUDED STRUCTURE")) {
      for(i=1;i<=(*grid)[k].zcells;i++) {
	if(i>1) Getline(params,in);
	sscanf(params,"%d %d %d\n",
	       &(*grid)[k].zfirstmaterial[i],&(*grid)[k].zlastmaterial[i],&(*grid)[k].zmaterial[i]); 
      }
    }
    
    else if(strstr(command,"GEOMETRY MAPPINGS")) {     
      for(i=0;i<MAXLINESIZE;i++) params[i] = toupper(params[i]);
      for(i=grid[k]->mappings;i<MAXMAPPINGS;i++) {
	if(i>grid[k]->mappings) Getline(params,in);

	if(strstr(params,"END")) break;
	cp=params; 
	grid[k]->mappingtype[i] = next_int(&cp);	
#if 0
	grid[k]->mappingtype[i] += 50*SGN(grid[k]->mappingtype[i]);
#endif
	grid[k]->mappingline[i] = next_int(&cp);
	grid[k]->mappinglimits[2*i] = next_real(&cp);
	grid[k]->mappinglimits[2*i+1] = next_real(&cp);
	grid[k]->mappingpoints[i] = next_int(&cp);
	grid[k]->mappingparams[i] = Rvector(0,grid[k]->mappingpoints[i]);
	for(j=0;j<grid[k]->mappingpoints[i];j++) 
	  grid[k]->mappingparams[i][j] = next_real(&cp);
      }      
      printf("Loaded %d geometry mappings\n",i);
      grid[k]->mappings = i;      
    }

    else if(strstr(command,"END") ) {      
      printf("End of field\n");
    }
      
    else if(strstr(command,"START NEW MESH")) {
      if((*nogrids) >= MAXCASES) {
	printf("There are more grids than was allocated for!\n"); 
	printf("Ignoring meshes starting from %d\n.",(*nogrids)+1);
	goto end;
      }
      (*nogrids)++;
      printf("\nLoading element meshing no %d\n",*nogrids);
      k = *nogrids - 1;	           
      if(k > nogrids0) (*grid)[k] = (*grid)[k-1];	 
    }

    else {
      printf("Unknown command: %s",command);
    }
  }

end:

  if(info) printf("Found %d divisions for grid\n",*nogrids);
  
  for(k=nogrids0;k < (*nogrids) && k<MAXCASES;k++) {

    if(elemcode == 0) {
      if((*grid)[k].dimension == 1) {
	(*grid)[k].nonodes = (*grid)[k].elemorder + 1;
      }
      else if((*grid)[k].elemmidpoints == FALSE) {
	(*grid)[k].nonodes = 4 * (*grid)[k].elemorder;
      }					
      else {
	if((*grid)[k].elemorder == 2) (*grid)[k].nonodes = 9;
	if((*grid)[k].elemorder == 3) (*grid)[k].nonodes = 16;	
      }
    }
    else if(elemcode/100 == 2) {
      (*grid)[k].triangles = FALSE;      
      (*grid)[k].nonodes = elemcode%100;
    }
    else if(elemcode/100 == 4) {
      (*grid)[k].triangles = FALSE;      
      (*grid)[k].nonodes = elemcode%100;
    }
    else if(elemcode/100 == 3) {  
      (*grid)[k].triangles = TRUE;      
      if(elemcode%100 == 3)       (*grid)[k].nonodes = 4;
      else if(elemcode%100 == 6)  (*grid)[k].nonodes = 9;
      else if(elemcode%100 == 10) (*grid)[k].nonodes = 16;	
    }    
  }

  for(k=nogrids0;k < (*nogrids) && k<MAXCASES;k++) {
    SetElementDivision(&(*grid)[k],info);
  }

  fclose(in);
  return(error);
}



int SaveGridToGridMapping(struct CellType *cell1, struct GridType *grid1, 
			  struct CellType *cell2, struct GridType *grid2,
			  char *prefix)
/* Creates a mapping between two grids with a similar geometry, 
   but different number of elements. Note that even if the the mapping is 
   possible even from 8- and 9-node elements only the four corner elements
   are used for the mapping. 
   */
{
  int xcell,ycell,i,i1,j1,i2,j2,no1,no2,hit;
  int ind1[MAXNODESD2],ind2[MAXNODESD2],nonodes1,nonodes2;
  int **mapi;
  Real **mapw;
  Real epsilon = 1.0e-20;
  Real coord1[DIM*MAXNODESD2],coord2[DIM*MAXNODESD2],x2,y2,rx,ry;
  char filename[MAXFILESIZE];
  FILE *out;

  nonodes1 = grid1->nonodes;
  nonodes2 = grid2->nonodes;

  if((nonodes1!=4  &&  nonodes1!=8 && nonodes1!=9) ||
     (nonodes2!=4  &&  nonodes2!=8 && nonodes2!=9)) {
    printf("SaveGridToGridMapping: not defined for all element types.\n");
    return(1);
  }

  AddExtension(prefix,filename,"map");
  out = fopen(filename,"w");

  mapi = Imatrix(0,3,1,grid2->noknots);
  mapw = Rmatrix(0,3,1,grid2->noknots);

  for(i=0;i<3;i++)
    for(i2=1;i2<=grid2->noknots;i2++) {
      mapi[i][i2] = 0;
      mapw[i][i2] = 0.0;
    }

  for(xcell=1;xcell<=MAXCELLS;xcell++)
    for(ycell=1;ycell<=MAXCELLS;ycell++) 

      /* Go through cells that are common to both grids. */
      if( (no1 = grid1->numbered[ycell][xcell]) && 
	  (no2= grid2->numbered[ycell][xcell]) ) {

	j1 = 1;
	i1 = 1;

        for(j2=1; j2 <= cell2[no2].yelem; j2++) 
          for(i2=1; i2 <= cell2[no2].xelem; i2++) {
	    GetElementCoordinates(&(cell2)[no2],i2,j2,coord2,ind2);

	    for(i=0;i<nonodes2;i++) {

	      if(mapi[0][ind2[i]] != 0) continue;

	      x2 = coord2[i];
	      y2 = coord2[i+nonodes2];

	      do {
		hit = TRUE;
		GetElementCoordinates(&(cell1)[no1],i1,j1,coord1,ind1);
		if(coord1[TOPRIGHT+nonodes1]+epsilon < y2 && j1< cell1[no1].yelem) {
		  j1++;
		  hit = FALSE;
		}
		else if(coord1[BOTRIGHT+nonodes1]-epsilon > y2 && j1>1) {
		  j1--;
		  hit = FALSE;
		}
		if(coord1[TOPRIGHT]+epsilon < x2 && i1< cell1[no1].xelem) {
		  i1++;
		  hit = FALSE;
		}
		else if(coord1[TOPLEFT]-epsilon > x2 && i1>1) {
		  i1--;
		  hit = FALSE;
		}


	      } while(hit == FALSE);
      
	      rx = (coord1[BOTRIGHT]-x2) 
		/ (coord1[BOTRIGHT]-coord1[BOTLEFT]);
	      ry = (coord1[TOPLEFT+nonodes1]-y2) 
		/ (coord1[TOPLEFT+nonodes1]-coord1[BOTLEFT+nonodes1]);

	      rx = MIN(rx,1.0);
	      rx = MAX(rx,0.0);
	      ry = MIN(ry,1.0);
	      ry = MAX(ry,0.0);

	      mapi[0][ind2[i]] = ind1[0];
	      mapi[1][ind2[i]] = ind1[1];
	      mapi[2][ind2[i]] = ind1[2];
	      mapi[3][ind2[i]] = ind1[3];

	      mapw[0][ind2[i]] = rx*ry;
	      mapw[1][ind2[i]] = (1.-rx)*ry;
	      mapw[2][ind2[i]] = (1.-rx)*(1.-ry);
	      mapw[3][ind2[i]] = rx*(1.-ry);
	    }
	  }
      }
  
  for(i2=1;i2<=grid2->noknots;i2++) {
    fprintf(out,"%-8d ",i2);
    for(i=0;i<4;i++) {
      fprintf(out,"%-5d ",mapi[i][i2]);
      fprintf(out,"%-10.5le ",mapw[i][i2]);
    }
    fprintf(out,"\n");
  }

  fclose(out);

  free_Imatrix(mapi,0,3,1,grid2->noknots);
  free_Rmatrix(mapw,0,3,1,grid2->noknots);

  printf("Saved mapping for %d knots to %d knots to %s.\n",
	 grid1->noknots,grid2->noknots,filename);
	 
#if DEBUG
  printf("The data was copied from a grid to another.\n");
#endif
  return(0);
}



int ShowCorners(struct FemType *data,int variable,Real offset)
{
  int i,ind,unknowns;
  Real *solution;

  if(data->nocorners < 1) 
    return(1);

  unknowns = data->edofs[variable];
  if(unknowns == 0) return(2);

  solution = data->dofs[variable];

  printf("Variable %s at free corners:\n",data->dofname[variable]);
  for(i=1;i<=data->nocorners;i++) {
    ind = data->topology[data->corners[2*i-1]][data->corners[2*i]];
    if(data->order[variable][(ind-1)*unknowns+1])
       printf("\t%-2d: %-7.1lf  at (%.1lf , %.1lf )\n",
	      i,solution[(ind-1)*unknowns+1]-offset,
	      data->x[ind],data->y[ind]);
  }
  return(0);
}





