/***********************************************************************
*
*       ELMER, A Computational Fluid Dynamics Program.
*
*       Copyright 1st April 1995 - , Center for Scientific Computing,
*                                    Finland.
*
*       All rights reserved. No part of this program may be used,
*       reproduced or transmitted in any form or by any means
*       without the written permission of CSC.
*
*                Address: Center for Scientific Computing
*                         Tietotie 6, P.O. BOX 405
*                         02101 Espoo, Finland
*                         Tel.     +358 0 457 2001
*                         Telefax: +358 0 457 2302
*                         EMail:   Jari.Jarvinen@csc.fi
************************************************************************/

/***********************************************************************
Program:    ELMER Front
Module:     ecif_solverControl.h
Language:   C++
Date:       05.09.01
Version:    1.00
Author(s):  Martti Verho
Revisions:

Abstract:   A Base class solver control (info output etc.) parameter

************************************************************************/

#ifndef _ECIF_SOLVER_CONTROL_
#define _ECIF_SOLVER_CONTROL_

#include "ecif_parameter.h"


// ****** SolverControl class ******
class SolverControl : public Parameter{
public:
  SolverControl();
  SolverControl(int pid);
  SolverControl(int pid, char* data_string, char* param_name);
  int getLastId() {return last_id;}
  void setLastId(int lid) {last_id = lid;}
  const char* getGuiName() { return "SolverControl"; }
  const char* getArrayName() { return "SolverControl"; }
  const char* getEmfName() { return "SolverControl"; }
  const char* getSifName() { return "SolverControl"; }
  ecif_parameterType getParameterType() { return ECIF_SOLVER_CONTROL; }
  static void initClass(Model* model);
  virtual ostream& output_sif(ostream& out, short indent_size, short indent_level, SifOutputControl& soc);
  void setName(char* param_name);
protected:
  static int last_id;
  static Model* model;
};


#endif
