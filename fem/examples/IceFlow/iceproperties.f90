!/******************************************************************************
! *
! *       ELMER, A Computational Fluid Dynamics Program.
! *
! *       Copyright 1st April 1995 - , Center for Scientific Computing,
! *                                    Finland.
! *
! *       All rights reserved. No part of this program may be used,
! *       reproduced or transmitted in any form or by any means
! *       without the written permission of CSC.
! *
! *****************************************************************************/
!
!/******************************************************************************
!*
! ******************************************************************************
! *
! *                    Author:  Juha Ruokolainen
! *
! *                    Address: Center for Scientific Computing
! *                                Tietotie 6, P.O. BOX 405
! *                                  02101 Espoo, Finland
! *
! *
!/******************************************************************************
! *
! *  iceproperties.f90  material parameters and physical models for ice flow
! *
! *
! *       Module Author:           Thomas Zwinger
! *       Address:                 Center for Scientific Computing
! *                                Tietotie 6, P.O. BOX 405
! *                                  02101 Espoo, Finland
! *                                  Tel. +358 0 457 2723
! *                                Telefax: +358 0 457 2183
! *                                EMail: Thomas.Zwinger@csc.fi
! *
! *       Modified by:             Thomas Zwinger
! *
! *       Date of modification: 11/11/2005
! *
! *****************************************************************************/
!
!
!
!

!*********************************************************************************************************************************
!*
!*  basal slip coefficient as a function of temperature
!*
!*********************************************************************************************************************************
FUNCTION basalSlip( Model, Node, dummyArgument ) RESULT(basalSlipCoefficient)
!-----------------------------------------------------------
  USE DefUtils
!-----------------------------------------------------------
  IMPLICIT NONE
!-----------------------------------------------------------
  !external variables
  TYPE(Model_t), TARGET :: Model
  INTEGER :: Node
  REAL(KIND=dp) :: dummyArgument, basalSlipCoefficient
  !internal variables
  TYPE(Element_t), POINTER :: CurrentElementAtBeginning, BoundaryElement, ParentElement
  TYPE(ValueList_t), POINTER :: ParentMaterial, BC
  TYPE(Variable_t), POINTER :: varTemperature, varPressure
  INTEGER :: N, NBoundary, NParent, BoundaryElementNode, ParentElementNode, &
       i, DIM, other_body_id, body_id, material_id, istat, NSDOFs
  REAL(KIND=dp) :: TempHom, ThermalCoefficient
  REAL(KIND=dp), ALLOCATABLE :: Temperature(:),PressureMeltingPoint(:),&
        TemperateSlipCoefficient(:)
  CHARACTER(LEN=MAX_NAME_LEN) :: TempName
  LOGICAL ::  GotIt, stat

  !---------------
  ! Initialization
  !---------------
  basalSlipCoefficient = 1.0D30 ! high value - > no slip by default
  CurrentElementAtBeginning => Model % CurrentElement 
  N = Model % MaxElementNodes 
  ALLOCATE(Temperature(N), &
       TemperateSlipCoefficient(N),&  
       PressureMeltingPoint(N),&
       STAT = istat)
  IF (istat /= 0) THEN
     CALL FATAL('iceproperties (basalSlip)','Allocations failed')
  END IF

  Temperature = 0.0D00
  TemperateSlipCoefficient = 0.0D00
  PressureMeltingPoint = 0.0D00

  !-----------------------------------------------------------------
  ! get some information upon active boundary element and its parent
  !-----------------------------------------------------------------
  BoundaryElement => Model % CurrentElement
  IF ( .NOT. ASSOCIATED(BoundaryElement) ) THEN
     CALL FATAL('iceproperties (basalMelting)','No boundary element found')
  END IF
  other_body_id = BoundaryElement % BoundaryInfo % outbody
  IF (other_body_id < 1) THEN ! only one body in calculation
     ParentElement => BoundaryElement % BoundaryInfo % Right
     IF ( .NOT. ASSOCIATED(ParentElement) ) ParentElement => BoundaryElement % BoundaryInfo % Left
  ELSE ! we are dealing with a body-body boundary and asume that the normal is pointing outwards
     ParentElement => BoundaryElement % BoundaryInfo % Right
     IF (ParentElement % BodyId == other_body_id) ParentElement => BoundaryElement % BoundaryInfo % Left
  END IF
  ! just to be on the save side, check again
  IF ( .NOT. ASSOCIATED(ParentElement) ) THEN
     WRITE(Message,'(A,I10,A)')&
          'Parent Element for Boundary element no. ',&
          BoundaryElement % ElementIndex, ' not found'
     CALL FATAL('iceproperties (basalMelting)',Message)
  END IF  
  Model % CurrentElement => ParentElement
  body_id = ParentElement % BodyId
  material_id = ListGetInteger(Model % Bodies(body_id) % Values, 'Material', GotIt)
  ParentMaterial => Model % Materials(material_id) % Values
  IF ((.NOT. ASSOCIATED(ParentMaterial)) .OR. (.NOT. GotIt)) THEN
     WRITE(Message,'(A,I10,A,I10)')&
          'No material values found for body no ', body_id,&
          ' under material id ', material_id
     CALL FATAL('iceproperties (basalMelting)',Message)
  END IF
  ! number of nodes and node in elements
  NBoundary = BoundaryElement % Type % NumberOfNodes
  NParent = ParentElement % Type % NumberOfNodes
  DO BoundaryElementNode=1,Nboundary
     IF ( Node == BoundaryElement % NodeIndexes(BoundaryElementNode) ) EXIT
  END DO
  DO ParentElementNode=1,NParent
     IF ( Node == ParentElement % NodeIndexes(ParentElementNode) ) EXIT
  END DO
  !-------------------------
  ! Get Temperature Field
  !-------------------------
  TempName =  GetString(ParentMaterial ,'Temperature Name', GotIt)
  IF (.NOT.GotIt) THEN
     CALL FATAL('iceproperties (basalSlip)','Keyword >Temperature Name< not found')
  ELSE
     WRITE(Message,'(a,a)') 'Variable Name for temperature: ', TempName
     CALL INFO('iceproperties (basalSlip)',Message,Level=12)
  END IF
  VarTemperature => VariableGet( Model % Variables, TempName, .TRUE. )
  IF ( ASSOCIATED( VarTemperature ) ) THEN
     Temperature(1:NParent) = VarTemperature % Values(VarTemperature % Perm(ParentElement % NodeIndexes))
  ELSE
     CALL FATAL('iceproperties (basalSlip)','No Temperature Variable found')
  END IF
  !-------------------------
  ! Get Pressure Melting Point
  !-------------------------
  PressureMeltingPoint(1:NParent) =&
       ListGetReal( ParentMaterial, TRIM(TempName) // ' Upper Limit',&
       NParent, ParentElement % NodeIndexes, GotIt)
  IF (.NOT. GotIt) THEN
     WRITE(Message,'(a,a,a)') 'No entry for ', TRIM(TempName) // ' Upper Limit', ' found'
     CALL FATAL('iceproperties (basalMelting)',Message)
  END IF
  !-------------------------------------------------------------------
  ! get slip coefficient if temperature reached pressure melting point
  !-------------------------------------------------------------------
  Model % CurrentElement => BoundaryElement
  BC => GetBC()

  IF (.NOT.ASSOCIATED(BC)) THEN
     CALL FATAL('iceproperties (basalSlip)','No Boundary Condition associated')
  ELSE
     TemperateSlipCoefficient(1:NBoundary) = GetReal(BC, 'Temperate Slip Coefficient', GotIt)
     IF (.NOT. GotIt) THEN
        CALL WARN('iceproperties (basalSlip)','Keyword >Temperate Slip Coefficient< not found')
        CALL WARN('iceproperties (basalSlip)','Asuming Default 5.63D08 [kg /(s m^2)]')
        TemperateSlipCoefficient(1:NBoundary) = 5.63D08
     END IF
     ThermalCoefficient = GetConstReal(BC, 'Thermal Coefficient', GotIt)
     IF (.NOT. GotIt) THEN
        CALL WARN('iceproperties (basalSlip)','Keyword >Thermal Coefficient< not found')
        CALL WARN('iceproperties (basalSlip)','Asuming Default 1 [1/K]')
        ThermalCoefficient =  1.0D00
     END IF
  END IF
  !------------------------------
  ! check homologous temperature
  !------------------------------
  TempHom = MIN(Temperature(ParentElementNode) - PressureMeltingPoint(ParentElementNode),0.0D00)
  
!  PRINT *, 'Thom =', TempHom,' = ', Temperature(ParentElementNode), '-', PressureMeltingPoint(ParentElementNode)
  basalSlipCoefficient = TemperateSlipCoefficient(BoundaryElementNode)*EXP(-1.0D00*TempHom*ThermalCoefficient) 
  !------------------------------
  ! clean up
  !------------------------------
  DEALLOCATE(Temperature, TemperateSlipCoefficient, PressureMeltingPoint)
END FUNCTION basalSlip

!*********************************************************************************************************************************
!*
!*  basal melting rate as a function of internal and external heat flux and latent heat
!*
!*********************************************************************************************************************************
FUNCTION basalMelting( Model, Node, dummyArgument ) RESULT(basalMeltingRate)
!-----------------------------------------------------------
  USE DefUtils
!-----------------------------------------------------------
  IMPLICIT NONE
!-----------------------------------------------------------
  !external variables
  TYPE(Model_t), TARGET :: Model
  INTEGER :: Node
  REAL(KIND=dp) :: dummyArgument, basalMeltingRate
  !internal variables
  TYPE(Element_t), POINTER :: CurrentElementAtBeginning, BoundaryElement, ParentElement
  TYPE(Nodes_t) :: Nodes
  TYPE(ValueList_t), POINTER :: ParentMaterial, BC
  TYPE(Variable_t), POINTER :: varTemperature, varPressure
  INTEGER :: N, NBoundary, NParent, BoundaryElementNode, ParentElementNode, &
       i, DIM, other_body_id, body_id, material_id, istat, NSDOFs
  REAL(KIND=dp) :: U, V, W, gradTemperature(3),  Normal(3), Gravity(3),&
       grav, InternalHeatFlux, HeatFlux, SqrtElementMetric, pressure
  REAL(KIND=dp), ALLOCATABLE :: Basis(:),dBasisdx(:,:), ddBasisddx(:,:,:),&
       LatentHeat(:), HeatConductivity(:), Density(:), Temperature(:),&
       ExternalHeatFlux(:), ClausiusClapeyron(:),PressureMeltingPoint(:)
  REAL(KIND=dp), POINTER :: Work(:,:)
  CHARACTER(LEN=MAX_NAME_LEN) :: TempName
  LOGICAL ::  FirstTime = .TRUE., GotIt, stat
!----------------------------------------------------------  
  CurrentElementAtBeginning => Model % CurrentElement
  !--------------------------------
  ! Allocations
  !--------------------------------
  DIM = CoordinateSystemDimension()
  N = Model % MaxElementNodes 
  ALLOCATE(Nodes % x(N), Nodes % y(N), Nodes % z(N),&
       Basis(N), dBasisdx(N,3), ddBasisddx(N,3,3), &
       LatentHeat( N ),&
       HeatConductivity( N ),&
       PressureMeltingPoint( N ),&
       Density( N ),&
       Temperature( N ),&
       ExternalHeatFlux( N ),&
       ClausiusClapeyron( N),&
       STAT = istat)
  IF (istat /= 0) THEN
     CALL FATAL('iceproperties (basalMelting)','Allocations failed')
  END IF
  !-----------------------------------------------------------------
  ! get some information upon active boundary element and its parent
  !-----------------------------------------------------------------
  BoundaryElement => Model % CurrentElement
  IF ( .NOT. ASSOCIATED(BoundaryElement) ) THEN
     CALL FATAL('iceproperties (basalMelting)','No boundary element found')
  END IF
  other_body_id = BoundaryElement % BoundaryInfo % outbody
  IF (other_body_id < 1) THEN ! only one body in calculation
     ParentElement => BoundaryElement % BoundaryInfo % Right
     IF ( .NOT. ASSOCIATED(ParentElement) ) ParentElement => BoundaryElement % BoundaryInfo % Left
  ELSE ! we are dealing with a body-body boundary and asume that the normal is pointing outwards
     ParentElement => BoundaryElement % BoundaryInfo % Right
     IF (ParentElement % BodyId == other_body_id) ParentElement => BoundaryElement % BoundaryInfo % Left
  END IF
  ! just to be on the save side, check again
  IF ( .NOT. ASSOCIATED(ParentElement) ) THEN
     WRITE(Message,'(A,I10,A)')&
          'Parent Element for Boundary element no. ',&
          BoundaryElement % ElementIndex, ' not found'
     CALL FATAL('iceproperties (basalMelting)',Message)
  END IF
  body_id = ParentElement % BodyId
  material_id = ListGetInteger(Model % Bodies(body_id) % Values, 'Material', GotIt)
  ParentMaterial => Model % Materials(material_id) % Values
  IF ((.NOT. ASSOCIATED(ParentMaterial)) .OR. (.NOT. GotIt)) THEN
     WRITE(Message,'(A,I10,A,I10)')&
          'No material values found for body no ', body_id,&
          ' under material id ', material_id
     CALL FATAL('iceproperties (basalMelting)',Message)
  END IF
  !-------------------------------------------
  ! Get normal of the boundary element at node
  !-------------------------------------------
  Nboundary = BoundaryElement % Type % NumberOfNodes
  DO BoundaryElementNode=1,Nboundary
     IF ( Node == BoundaryElement % NodeIndexes(BoundaryElementNode) ) EXIT
  END DO
  U = BoundaryElement % Type % NodeU(BoundaryElementNode)
  V = BoundaryElement % Type % NodeV(BoundaryElementNode)
  Nodes % x(1:Nboundary) = Model % Nodes % x(BoundaryElement % NodeIndexes)
  Nodes % y(1:Nboundary) = Model % Nodes % y(BoundaryElement % NodeIndexes)
  Nodes % z(1:Nboundary) = Model % Nodes % z(BoundaryElement % NodeIndexes)
  Normal = NormalVector( BoundaryElement, Nodes, U, V,.TRUE. )
  ! ----------------------------------
  ! Get information on parent element
  ! ----------------------------------
  NParent = ParentElement % Type % NumberOfNodes
  DO ParentElementNode=1,NParent
     IF ( Node == ParentElement % NodeIndexes(ParentElementNode) ) EXIT
  END DO
  U = ParentElement % Type % NodeU(ParentElementNode)
  V = ParentElement % Type % NodeV(ParentElementNode)
  W = ParentElement % Type % NodeW(ParentElementNode)
  Nodes % x(1:NParent) = Model % Nodes % x(ParentElement % NodeIndexes)
  Nodes % y(1:NParent)  = Model % Nodes % y(ParentElement % NodeIndexes)
  Nodes % z(1:NParent)  = Model % Nodes % z(ParentElement % NodeIndexes)
  stat = ElementInfo( ParentElement,Nodes,U,V,W,SqrtElementMetric, &
       Basis,dBasisdx,ddBasisddx,.FALSE.,.FALSE. )
  !-------------------------
  ! Get Temperature Field
  !-------------------------
  TempName =  GetString(ParentMaterial ,'Temperature Name', GotIt)
  IF (.NOT.GotIt) THEN
     CALL FATAL('iceproperties (basalMelting)','No Temperature Name found')
  ELSE
     WRITE(Message,'(a,a)') 'Variable Name for temperature: ', TempName
     CALL INFO('iceproperties (basalMelting)',Message,Level=12)
  END IF
  VarTemperature => VariableGet( Model % Variables, TempName, .TRUE. )
  IF ( ASSOCIATED( VarTemperature ) ) THEN
     Temperature(1:NParent) = VarTemperature % Values(VarTemperature % Perm(ParentElement % NodeIndexes))
  ELSE
     CALL FATAL('iceproperties (basalMelting)','No Temperature Variable found')
  END IF
  !-------------------------
  ! Get Pressure Melting Point
  !-------------------------
  Model % CurrentElement => ParentElement
  PressureMeltingPoint(1:NParent) =&
       ListGetReal( ParentMaterial, TRIM(TempName) // ' Upper Limit',&
       NParent, ParentElement % NodeIndexes)
  IF (.NOT. GotIt) THEN
     WRITE(Message,'(a,a,a)') 'No entry for ', TRIM(TempName) // ' Upper Limit', ' found'
     CALL FATAL('iceproperties (basalMelting)',Message)
  END IF
  !---------------------------------------
  ! Limit Temperature to physical values
  !--------------------------------------
  DO i=1,NParent
     Temperature(i) = MIN(Temperature(i), PressureMeltingPoint(i))
  END DO
  !-------------------------
  ! Get Temperature Gradient
  !-------------------------
  gradTemperature = 0.0D00
  DO i=1,DIM
     gradTemperature(i) = SUM(dBasisdx(1:NParent,i)*Temperature(1:NParent))
  END DO
  !-------------------------
  ! Get material parameters
  !-------------------------
  LatentHeat(1:NParent) = ListGetReal(ParentMaterial, 'Latent Heat', NParent, ParentElement % NodeIndexes, GotIt)
  IF (.NOT. GotIt) THEN
     CALL FATAL('iceproperties (basalMelting)','No value for Latent Heat found')
  END IF
  HeatConductivity(1:NParent) = ListGetReal(Model % Materials(material_id) % Values, &
       TRIM(TempName) // ' Heat Conductivity', NParent, ParentElement % NodeIndexes, GotIt)
  IF (.NOT. GotIt) THEN
     CALL FATAL('iceproperties (basalMelting)','No value for Heat Conductivity found')
  END IF
  Density(1:NParent) = ListGetReal( ParentMaterial, 'Density', NParent, ParentElement % NodeIndexes)
  IF (.NOT. GotIt) THEN
     CALL FATAL('iceproperties (basalMelting)','No value for Density found')
  END IF
  ClausiusClapeyron(1:NParent) = & 
       ListGetReal( ParentMaterial, 'Clausius Clapeyron', NParent, ParentElement % NodeIndexes)
  IF (.NOT. GotIt) THEN
     CALL FATAL('iceproperties (basalMelting)','No value for Clausius Clapeyron parameter found')
  END IF

  Model % CurrentElement => BoundaryElement

  IF (Temperature(ParentElementNode) .GE. PressureMeltingPoint(ParentElementNode)) THEN
     !----------------------------
     ! compute internal heat flux
     !----------------------------
     InternalHeatFlux = -1.0D00 * HeatConductivity(ParentElementNode) * SUM(gradTemperature(1:DIM)*Normal(1:DIM))
     !-------------------------
     ! get external heat flux
     !------------------------
     BC => GetBC()
     ExternalHeatFlux = 0.0D00
     IF (other_body_id < 1) THEN ! we are dealing with an external heat flux
        ExternalHeatFlux(1:NBoundary) = GetReal(BC, TRIM(TempName) // ' Heat Flux', GotIt)
        IF (.NOT. GotIt) THEN
           CALL INFO('iceproperties (basalMelting)','No external heat flux given', Level=4)
        END IF
     ELSE ! we are dealing with a heat conducting body on the other side 
        CALL FATAL('iceproperties (basalMelting)','Interface condition not implemented!')
     END IF

     HeatFlux = ExternalHeatFlux(BoundaryElementNode) + InternalHeatFlux

     IF (HeatFlux <= 0.0D00) THEN
        WRITE(Message,'(A, i7, A, e10.4, A, e10.4, A, e10.4, A)') &
             'Heatflux towards temperate boundary node', Node, ': ', &
             HeatFlux, ' = ', ExternalHeatFlux(BoundaryElementNode), ' + ', InternalHeatFlux, &
             ' < 0!'
        CALL INFO('iceproperties (basalMelting)',Message,level=9)
        BasalMeltingRate = 0.0D00
     ELSE
     ! basal melting rate (volume/time and area) == normal velocity    
        BasalMeltingRate =  HeatFlux/&
             (LatentHeat(ParentElementNode) * Density(ParentElementNode))
     END IF
  !----------------------------------------------
  ! T < T_m no basal melting flux (cold ice base) 
  !----------------------------------------------
  ELSE 
     BasalMeltingRate = 0.0D00
  END IF
  !----------------------------------------------
  ! clean up before leaving
  !----------------------------------------------
  DEALLOCATE( Nodes % x, Nodes % y, Nodes % z,&
       Basis, dBasisdx, ddBasisddx, &
       LatentHeat,&
       HeatConductivity,&
       ClausiusClapeyron,&
       Density,&
       PressureMeltingPoint,&
       Temperature,&
       ExternalHeatFlux)
  Model % CurrentElement => CurrentElementAtBeginning 
END FUNCTION basalMelting

!*********************************************************************************************************************************
!*
!*  projecting vertical geothermal heat flux to boundary normal
!*
!*********************************************************************************************************************************
FUNCTION getNormalFlux( Model, Node, dummyArgument ) RESULT(NormalFlux)
!-----------------------------------------------------------
  USE DefUtils
!-----------------------------------------------------------
  IMPLICIT NONE
!-----------------------------------------------------------
  !external variables
  TYPE(Model_t), TARGET :: Model
  INTEGER :: Node
  REAL(KIND=dp) :: dummyArgument, NormalFlux
  !internal variables
  TYPE(Element_t), POINTER :: BoundaryElement
  TYPE(Nodes_t) :: Nodes
  TYPE(ValueList_t), POINTER :: BC
  INTEGER :: N, NBoundary, BoundaryElementNode, i, DIM, body_id, istat
  REAL(KIND=dp) :: U, V, W, Normal(3), Gravity(3), direction(3),&
       HeatFlux, SqrtElementMetric
  REAL(KIND=dp), ALLOCATABLE ::  ExternalHeatFlux(:)
  REAL(KIND=dp), POINTER :: Work(:,:)
  LOGICAL ::  FirstTime = .TRUE., GotIt, stat
!-----------------------------------------------------------
  !--------------------------------
  ! Allocations
  !--------------------------------
  DIM = CoordinateSystemDimension()
  N = Model % MaxElementNodes 
  ALLOCATE(Nodes % x(N), Nodes % y(N), Nodes % z(N),&
       ExternalHeatFlux( N ),&
       STAT = istat)
  IF (istat /= 0) THEN
     CALL FATAL('iceproperties (normalFlux)','Allocations failed')
  END IF

  !-----------------------------------------------------------------
  ! get some information upon active boundary element and its parent
  !-----------------------------------------------------------------
  BoundaryElement => Model % CurrentElement
  IF ( .NOT. ASSOCIATED(BoundaryElement) ) THEN
     CALL FATAL('iceproperties (normalFlux)','No boundary element found')
  END IF
  !-------------------------------------------
  ! Get normal of the boundary element at node
  !-------------------------------------------
  Nboundary = BoundaryElement % Type % NumberOfNodes
  DO BoundaryElementNode=1,Nboundary
     IF ( Node == BoundaryElement % NodeIndexes(BoundaryElementNode) ) EXIT
  END DO
  U = BoundaryElement % Type % NodeU(BoundaryElementNode)
  V = BoundaryElement % Type % NodeV(BoundaryElementNode)
  Nodes % x(1:Nboundary) = Model % Nodes % x(BoundaryElement % NodeIndexes)
  Nodes % y(1:Nboundary) = Model % Nodes % y(BoundaryElement % NodeIndexes)
  Nodes % z(1:Nboundary) = Model % Nodes % z(BoundaryElement % NodeIndexes)
  Normal = NormalVector( BoundaryElement, Nodes, U, V,.TRUE. )
  !-------------------------------
  ! get gravitational acceleration
  !-------------------------------
  Work => ListGetConstRealArray( Model % Constants,'Gravity',GotIt)
  IF ( GotIt ) THEN
     Gravity = Work(1:3,1)
  ELSE
     Gravity = 0.0D00
     CALL INFO('iceproperties (normalFlux)','No vector for Gravity (Constants) found', level=1)
     IF (DIM == 1) THEN
        Gravity(1) = -1.0D00
        CALL INFO('iceproperties (normalFlux)','setting direction to -1', level=1)
     ELSE IF (DIM == 2) THEN
        Gravity    =  0.00D0
        Gravity(2) = -1.0D00
        CALL INFO('iceproperties (normalFlux)','setting direction to (0,-1)', level=1)
     ELSE
        Gravity    =  0.00D00
        Gravity(3) = -1.0D00
        CALL INFO('iceproperties (normalFlux)','setting direction to (0,0,-1)', level=1)
     END IF
  END IF
  !------------------------
  ! get external heat flux
  !------------------------
  BC => GetBC()
  ExternalHeatFlux = 0.0D00  
  ExternalHeatFlux(1:NBoundary) = GetReal(BC, 'External Heat Flux', GotIt)
  IF (.NOT. GotIt) THEN
     CALL INFO('iceproperties (normalFlux)','No external heat flux given', Level=4)
  END IF
  !--------------------------------------------------------
  ! compute normal component of vertically aligned heatflux
  !--------------------------------------------------------
  NormalFlux = ExternalHeatFlux(BoundaryElementNode) * ABS(SUM(Gravity(1:DIM)*Normal(1:DIM)))
  !----------------------------------------------
  ! clean up before leaving
  !----------------------------------------------
  DEALLOCATE( Nodes % x,&
       Nodes % y,&
       Nodes % z,&
       ExternalHeatFlux)
END FUNCTION getNormalFlux

!*********************************************************************************************************************************
!*
!* heat conductivity of ice as a function of temperature (K):  k = c_1 * exp(c_2 * T[K]); c_2 < 0 
!*
!*********************************************************************************************************************************
FUNCTION getHeatConductivity( Model, N, temperature ) RESULT(conductivity)
  USE types
  USE CoordinateSystems
  USE SolverUtils
  USE ElementDescription
!-----------------------------------------------------------
  IMPLICIT NONE
!------------ external variables ---------------------------
  TYPE(Model_t) :: Model
  INTEGER :: N
  REAL(KIND=dp) :: temperature, conductivity
!------------ internal variables----------------------------
  TYPE(ValueList_t), POINTER :: Material
  INTEGER :: nMax,i,j,body_id,material_id,elementNodes,nodeInElement,istat
  REAL (KIND=dp), ALLOCATABLE :: conductivityExponentFactor(:), conductivityFactor(:)
  LOGICAL :: FirstTime = .TRUE., GotIt
!------------ remember this -------------------------------
  Save FirstTime, conductivityExponentFactor, conductivityFactor
  !-------------------------------------------
  ! Allocations 
  !------------------------------------------- 
  IF (FirstTime) THEN
     nMax = Model % MaxElementNodes
     ALLOCATE(conductivityExponentFactor(nMax),&
          conductivityFactor(nMax),&
          STAT=istat)
     IF ( istat /= 0 ) THEN
        CALL FATAL('iceproperties (getHeatConductivity)','Memory allocation error, Aborting.')
     END IF
     FirstTime = .FALSE.
     CALL INFO('iceproperties (getHeatConductivity)','Memory allocation done', level=3)
  END IF
  !-------------------------------------------
  ! get element properties
  !-------------------------------------------   
  IF ( .NOT. ASSOCIATED(Model % CurrentElement) ) THEN
     CALL FATAL('iceproperties (getHeatConductivity)', 'Model % CurrentElement not associated')
  END IF
  body_id = Model % CurrentElement % BodyId
  material_id = ListGetInteger(Model % Bodies(body_id) % Values, 'Material', GotIt)
  elementNodes = Model % CurrentElement % Type % NumberOfNodes
  IF (.NOT. GotIt) THEN
     WRITE(Message,'(a,I2,a,I2,a)') 'No material id for current element of node ',n,', body ',body_id,' found'
     CALL FATAL('iceproperties (getHeatConductivity)', Message)
  END IF
  DO nodeInElement=1,elementNodes
     IF ( N == Model % CurrentElement % NodeIndexes(nodeInElement) ) EXIT
  END DO
  Material => Model % Materials(material_id) % Values
  !-------------------------------------------
  ! get material properties
  !-------------------------------------------
  conductivityExponentFactor(1:elementNodes) = ListGetReal( Material,'Conductivity Exponent Factor', elementNodes, &
       Model % CurrentElement % NodeIndexes, GotIt )
  IF (.NOT. GotIt) THEN
     WRITE(Message,'(a,I2,a,I2,a)') 'No Conductivity Exponent Factor found in Material ', &
          material_id,' for node ', n, '.setting E=1'
     CALL FATAL('iceproperties (getHeatConductivity)', Message)
  END IF
  conductivityFactor(1:elementNodes) = ListGetReal( Material,'Conductivity Factor', elementNodes, &
       Model % CurrentElement % NodeIndexes, GotIt )
  IF (.NOT. GotIt) THEN
     WRITE(Message,'(a,I2,a,I2,a)') 'No Conductivity Factor found in Material ', material_id,' for node ', n, '.setting E=1'
     CALL FATAL('iceproperties (getHeatConductivity)', Message)
  END IF
  !-------------------------------------------
  ! compute heat conductivity
  !-------------------------------------------
  conductivity = conductivityFactor(nodeInElement)*EXP(conductivityExponentFactor(nodeInElement)*temperature)
END FUNCTION getHeatConductivity

!*********************************************************************************************************************************
!*
!* heat capacity of ice as a function of temperature (K):  k = c_1 + c_2 * T[C];
!*
!*********************************************************************************************************************************
FUNCTION getHeatCapacity( Model, N, temperature ) RESULT(capacity)
  USE types
  USE CoordinateSystems
  USE SolverUtils
  USE ElementDescription
!-----------------------------------------------------------
  IMPLICIT NONE
!------------ external variables ---------------------------
  TYPE(Model_t) :: Model
  INTEGER :: N
  REAL(KIND=dp) :: temperature, capacity
!------------ internal variables----------------------------
  REAL(KIND=dp) :: celsius

  !-------------------------------------------
  ! compute celsius temperature and limit it 
  ! to 0 deg
  !-------------------------------------------  
  celsius = MIN(temperature - 2.7316D02,0.0d00)
  !-------------------------------------------
  ! compute heat capacity
  !-------------------------------------------  
  capacity = 2.1275D03 + 7.253D00*celsius
END FUNCTION getHeatCapacity

!****************************************************************************************************************
!*
!* viscosity factor as a function of homologous temperature
!*
!****************************************************************************************************************
FUNCTION getViscosityFactor( Model, n, temperature ) RESULT(visFact)
  USE types
  USE CoordinateSystems
  USE SolverUtils
  USE ElementDescription
  USE DefUtils
!-----------------------------------------------------------
  IMPLICIT NONE
!------------ external variables ---------------------------
  TYPE(Model_t) :: Model
  INTEGER :: n
  REAL(KIND=dp) :: temperature, visFact
!------------ internal variables----------------------------
  TYPE(ValueList_t), POINTER :: Material
  INTEGER :: DIM,nMax,i,j,body_id,material_id,elementNodes,nodeInElement,istat
  REAL(KIND=dp) ::&
       rateFactor, aToMinusOneThird, gasconst, temphom
  REAL(KIND=dp), POINTER :: Hwrk(:,:,:)
  REAL (KIND=dp), ALLOCATABLE :: activationEnergy(:,:), arrheniusFactor(:,:),&
       enhancementFactor(:), viscosityExponent(:), PressureMeltingPoint(:)
  LOGICAL :: FirstTime = .TRUE., GotIt
  CHARACTER(LEN=MAX_NAME_LEN) :: TempName
!------------ remember this -------------------------------
  Save DIM, FirstTime, gasconst, activationEnergy, arrheniusFactor,&
       enhancementFactor, viscosityExponent, Hwrk, PressureMeltingPoint
!-----------------------------------------------------------
  !-----------------------------------------------------------
  ! Read in constants from SIF file and do some allocations
  !-----------------------------------------------------------
  IF (FirstTime) THEN
     ! inquire coordinate system dimensions  and degrees of freedom from NS-Solver
     ! ---------------------------------------------------------------------------
     DIM = CoordinateSystemDimension()
     ! inquire minimum temperature
     !------------------------- 
     gasconst = ListGetConstReal( Model % Constants,'Gas Constant',GotIt)
     IF (.NOT. GotIt) THEN
        gasconst = 8.314D00 ! m-k-s
        WRITE(Message,'(a,e10.4,a)') 'No entry for Gas Constant (Constants) in input file found. Setting to ',&
             gasconst,' (J/mol)'
        CALL INFO('iceproperties (getViscosityFactor)', Message, level=4)
     END IF
     nMax = Model % MaxElementNodes
     ALLOCATE(activationEnergy(2,nMax),&
          arrheniusFactor(2,nMax),&
          enhancementFactor(nMax),&
          PressureMeltingPoint( nMax ),&
          viscosityExponent(nMax),&
          STAT=istat)
     IF ( istat /= 0 ) THEN
        CALL Fatal('iceproperties (getViscosityFactor)','Memory allocation error, Aborting.')
     END IF
     NULLIFY( Hwrk )
     FirstTime = .FALSE.
     CALL Info('iceproperties (getViscosityFactor)','Memory allocations done', Level=3)
  END IF
  !-------------------------------------------
  ! get element properties
  !-------------------------------------------   
  body_id = Model % CurrentElement % BodyId
  material_id = ListGetInteger(Model % Bodies(body_id) % Values, 'Material', GotIt)
  elementNodes = Model % CurrentElement % Type % NumberOfNodes
  IF (.NOT. GotIt) THEN
     WRITE(Message,'(a,I2,a,I2,a)') 'No material id for current element of node ',n,', body ',body_id,' found'
     CALL FATAL('iceproperties (getViscosityFactor)', Message)
  END IF
  DO nodeInElement=1,elementNodes
     IF ( N == Model % CurrentElement % NodeIndexes(nodeInElement) ) EXIT
  END DO
  Material => Model % Materials(material_id) % Values
  IF (.NOT.ASSOCIATED(Material)) THEN 
     WRITE(Message,'(a,I2,a,I2,a)') 'No Mterial for current element of node ',n,', body ',body_id,' found'
     CALL FATAL('iceproperties (getViscosityFactor)',Message)
  END IF
  !-------------------------------------------
  ! get material properties
  !-------------------------------------------
  ! activation energies
  !--------------------
  CALL ListGetRealArray( Material,'Activation Energies',Hwrk,elementNodes, &
       Model % CurrentElement % NodeIndexes, GotIt )
  IF (.NOT. GotIt) THEN
     WRITE(Message,'(a,I2,a,I2)') 'No Value for Activation Energy  found in Material ', material_id,' for node ', n
     CALL FATAL('iceproperties (getViscosityFactor)',Message)
  END IF
  IF ( SIZE(Hwrk,2) == 1 ) THEN
     DO i=1,MIN(3,SIZE(Hwrk,1))
        activationEnergy(i,1:elementNodes) = Hwrk(i,1,1:elementNodes)
     END DO
  ELSE
     WRITE(Message,'(a,I2,a,I2)') 'Incorrect array size for Activation Energy in Material ', material_id,' for node ', n
     CALL FATAL('iceproperties (getViscosityFactor)',Message)
  END IF
  ! Arrhenius Factors
  !------------------
  CALL ListGetRealArray( Material,'Arrhenius Factors',Hwrk,elementNodes, &
       Model % CurrentElement % NodeIndexes, GotIt )
  IF (.NOT. GotIt) THEN
     WRITE(Message,'(a,I2,a,I2)') 'No Value for Arrhenius Factors  found in Material ', material_id,' for node ', n
     CALL FATAL('iceproperties (getViscosityFactor)',Message)
  END IF
  IF ( SIZE(Hwrk,2) == 1 ) THEN
     DO i=1,MIN(3,SIZE(Hwrk,1))
        arrheniusFactor(i,1:elementNodes) = Hwrk(i,1,1:elementNodes)
     END DO
  ELSE
     WRITE(Message,'(a,I2,a,I2)') 'Incorrect array size for Arrhenius Factors in Material ', material_id,' for node ', n
     CALL FATAL('iceproperties (getViscosityFactor)',Message)
  END IF
  ! Enhancement Factor
  !-------------------
  enhancementFactor(1:elementNodes) = ListGetReal( Material,'Enhancement Factor', elementNodes, &
       Model % CurrentElement % NodeIndexes, GotIt )
  IF (.NOT. GotIt) THEN
     enhancementFactor(1:elementNodes) = 1.0D00
     WRITE(Message,'(a,I2,a,I2,a)') 'No Enhancement Factor found in Material ', material_id,' for node ', n, '.setting E=1'
     CALL INFO('iceproperties (getViscosityFactor)', Message, level=9)
  END IF
  ! Viscosity Exponent
  !-------------------
  viscosityExponent(1:elementNodes) = ListGetReal( Material,'Viscosity Exponent', elementNodes, &
       Model % CurrentElement % NodeIndexes, GotIt )
  IF (.NOT. GotIt) THEN
     viscosityExponent(1:elementNodes) = 1.0D00/3.0D00
     WRITE(Message,'(a,I2,a,I2,a)') 'No Viscosity Exponent found in Material ', material_id,' for node ', n, '.setting k=1/3'
     CALL INFO('iceproperties (getViscosityFactor)', Message, level=9)
  END IF
  ! Pressure Melting Point and homologous temperature
  !--------------------------------------------------
  TempName =  GetString(Material ,'Temperature Name', GotIt)
  IF (.NOT.GotIt) CALL FATAL('iceproperties (getViscosityFactor)','No Temperature Name found')
  PressureMeltingPoint(1:elementNodes) =&
       ListGetReal( Material, TRIM(TempName) // ' Upper Limit',&
       elementNodes, Model % CurrentElement % NodeIndexes, GotIt )
  IF (.NOT.GotIt) THEN
     temphom = 0.0d00
     WRITE(Message,'(A,A,A,i3,A)') 'No entry for ',TRIM(TempName) // ' Upper Limit',&
          ' found in material no. ', material_id,'. Using 273.16 K.'
     CALL WARN('iceproperties (getViscosityFactor)',Message)
  ELSE
     temphom = MIN(temperature - PressureMeltingPoint(nodeInElement), 0.0d00)
  END IF
  !-------------------------------------------
  ! homologous Temperature is below 10 degrees
  !-------------------------------------------
  IF (temphom < -1.0D01) THEN
     i=1
     !-------------------------------------------
     ! homologous Temperature is above 10 degrees
     !-------------------------------------------
  ELSE
     i=2
  END IF
  rateFactor =&
       arrheniusFactor(i,nodeInElement)*exp(-1.0D00*activationEnergy(i,nodeInElement)/(gasconst*(2.7316D02 + temphom)))
  visFact = 0.5*(enhancementFactor(nodeInElement)&
       *rateFactor)**(-1.0e00*viscosityExponent(nodeInElement))
!  PRINT *, activationEnergy(i,nodeInElement), temphom, gasconst
!  PRINT *, 'ratefact=', rateFactor,' = ', arrheniusFactor(i,nodeInElement),&
! '*', exp(-1.0D00*activationEnergy(i,nodeInElement)/(gasconst*(2.7316D02 + temphom)))

!  PRINT *, 'viscfact=',visFact, '= 1/2*(',&
!       '*',enhancementFactor(nodeInElement),&
!       '*',rateFactor,&
!       ')**(-',viscosityExponent(nodeInElement),')'
END FUNCTION getViscosityFactor

!*********************************************************************************************************************************
RECURSIVE SUBROUTINE getTotalViscosity(Model,Solver,Timestep,TransientSimulation)
  USE DefUtils
  USE Materialmodels
!-----------------------------------------------------------
  IMPLICIT NONE
!------------ external variables ---------------------------
  TYPE(Model_t)  :: Model
  TYPE(Solver_t), TARGET :: Solver
  LOGICAL :: TransientSimulation
  REAL(KIND=dp) :: Timestep
!------------------------------------------------------------------------------
!    Local variables
!------------------------------------------------------------------------------
  TYPE(GaussIntegrationPoints_t) :: IP
  TYPE(ValueList_t), POINTER :: Material, SolverParams
  TYPE(Variable_t), POINTER :: TotViscSol
  TYPE(Element_t), POINTER :: Element
  TYPE(Nodes_t) :: Nodes
  INTEGER :: DIM,nMax,i,p,q,t,N,body_id,material_id,nodeInElement,istat
  INTEGER, POINTER :: TotViscPerm(:)
  REAL(KIND=dp) ::  LocalDensity, LocalViscosity,effVisc,detJ,Norm
  REAL(KIND=dp), POINTER :: Hwrk(:,:,:)
  REAL(KIND=dp), POINTER :: TotVisc(:)
  REAL (KIND=dp), ALLOCATABLE ::  Ux(:), Uy(:), Uz(:), Density(:), Viscosity(:),&
       STIFF(:,:),FORCE(:),Basis(:),dBasisdx(:,:),ddBasisddx(:,:,:)
  LOGICAL :: FirstTime = .TRUE., GotIt, stat, limitation
!------------ remember this -------------------------------
  Save DIM, FirstTime, Nodes, Density, Ux, Uy, Uz, Viscosity, STIFF, FORCE, Basis,dBasisdx,ddBasisddx
  

  IF (FirstTime) THEN
     ! inquire coordinate system dimensions  and degrees of freedom from NS-Solver
     ! ---------------------------------------------------------------------------
     DIM = CoordinateSystemDimension()
     nMax = Model % MaxElementNodes 
     ALLOCATE(Ux(nMax),&
          Uy(nMax),&
          Uz(nMax),&
          Density(nMax),&
          Viscosity(nMax),&
          FORCE(nMax), &
          STIFF(nMax,nMax),&
          Nodes % x(nMax), &
          Nodes % y(nMax), &
          Nodes % z(nMax), &
          Basis(nMax),&
          dBasisdx(nMax,3),&
          ddBasisddx(nMax,3,3),&
          STAT=istat)
     IF ( istat /= 0 ) THEN
        CALL Fatal('iceproperties (getTotalViscosity)','Memory allocation error, Aborting.')
     END IF
     NULLIFY( Hwrk )
     FirstTime = .FALSE.
     CALL Info('iceproperties (getTotalViscosity)','Memory allocations done', Level=3)
  END IF

  CALL DefaultInitialize()

  DO t=1,Solver % NumberOfActiveElements 
     !-------------------------------------------
     ! get element properties
     !------------------------------------------- 
     Element => GetActiveElement(t)
     N = GetElementNOFNodes()
     body_id = Element % BodyId
     material_id = ListGetInteger(Model % Bodies(body_id) % Values, 'Material', GotIt)
     IF (.NOT. GotIt) THEN
        WRITE(Message,'(a,I2,a,I2,a)') &
             'No material id for current element of node ',n,', body ',body_id,' found'
        CALL FATAL('iceproperties (getTotalViscosity)', Message)
     
     END IF

     !-----------------------------------------
     ! get Material properties
     !-----------------------------------------
     Material => GetMaterial()
     IF (.NOT.ASSOCIATED(Material)) THEN 
        WRITE(Message,'(a,I2,a,I2,a)') &
             'No Material for current element of node ',n,', body ',body_id,' found'
        CALL FATAL('iceproperties (getTotalViscosit)',Message)
     END IF
     Density = ListGetReal( Material,'Density', N, Element % NodeIndexes, GotIt )
     IF (.NOT. GotIt) THEN
        WRITE(Message,'(a,I2,a,I2)') 'No Value for Density found in Material ', material_id,' for node ', n
        CALL FATAL('iceproperties (getTotalViscosity)',Message)
     END IF
     Viscosity = ListGetReal( Material,'Viscosity', N, Element % NodeIndexes, GotIt )
     IF (.NOT. GotIt) THEN
        WRITE(Message,'(a,I2,a,I2)') 'No Value for Viscosity found in Material ', material_id,' for node ', n
        CALL FATAL('iceproperties (getTotalViscosity)',Message)
     END IF

     !-----------------------------------------
     ! Velocity Field
     !----------------------------------------
     Ux = 0.0d00
     Uy = 0.0d00
     Uz = 0.0d00
  
     CALL GetScalarLocalSolution( Ux, 'Velocity 1')
     IF (DIM>1) THEN
        CALL GetScalarLocalSolution( Uy, 'Velocity 2')
        IF (DIM == 3) CALL GetScalarLocalSolution( Uz, 'Velocity 3')
     END IF
     

     STIFF = 0.0d00
     FORCE = 0.0d00

     IP = GaussPoints( Element )
     CALL GetElementNodes( Nodes )

     DO i=1,IP % n
        stat = ElementInfo( Element, Nodes, IP % U(i), IP % V(i), &
             IP % W(i),  detJ, Basis, dBasisdx, ddBasisddx, .FALSE. )
        !
        ! get local Material parameters at Gauss-point
        !
        LocalDensity = SUM(Density(1:n)*Basis(1:n))
        LocalViscosity = SUM(Viscosity(1:n)*Basis(1:n))
        !
        ! get effective Viscosity at Integration point
        !
        effVisc = EffectiveViscosity( LocalViscosity, LocalDensity,&
             Ux, Uy, Uz, &
             Element, Nodes, N, N,&
             IP % U(i), IP % V(i), IP % W(i))
         IF (effVisc .le. 0.0E00) THEN
           WRITE(Message,'(A,i10,A,i10,A,e13.3)')&
                'effective viscosity for Gauss point no. ', i, ' in element no. ', t,' is negative:', effVisc
           CALL WARN('iceproperties (getTotalViscosity)',Message)
        END IF
        DO p=1,n
           FORCE(p) = FORCE(p) + IP % S(i) * DetJ * effVisc * Basis(p)
           DO q=1,n
              STIFF(p,q) = STIFF(p,q) + IP % S(i) * detJ * Basis(q)*Basis(p)
           END DO
        END DO
     END DO
     CALL DefaultUpdateEquations( STIFF, FORCE )
  END DO
  CALL DefaultFinishAssembly()
!   CALL DefaultDirichletBCs()
  Norm = DefaultSolve()
  SolverParams => GetSolverParams()
  limitation = GetLogical( SolverParams,'Positive Values',GotIt )
  IF (.NOT. GotIt) limitation = .FALSE.
  IF (limitation) THEN
     CALL INFO('iceproperties (getTotalViscosity)','Results limited to positive values',Level=1)
     TotViscSol => Solver % Variable
     TotViscPerm  => TotViscSol % Perm
     TotVisc => TotViscSol % Values
     DO i= 1,Solver % Mesh % NumberOfNodes
        TotVisc(i) = MAX(TotVisc(i),0.0D00)
     END DO
  END IF
END SUBROUTINE getTotalViscosity

!*********************************************************************************************************************************
!*
!* total capacity (heat capacity * density) as a function of Kelvin temperature
!*
!*********************************************************************************************************************************
FUNCTION getCapacity( Model, N, Temp ) RESULT(capacity)
  USE types
  USE CoordinateSystems
  USE SolverUtils
  USE ElementDescription
!-----------------------------------------------------------
  IMPLICIT NONE
!------------ external variables ---------------------------
  TYPE(Model_t) :: Model
  INTEGER :: N
  REAL(KIND=dp) :: Temp, capacity
!------------ internal variables----------------------------
  REAL(KIND=dp) :: celsius
  REAL(KIND=dp), ALLOCATABLE :: density(:)
  INTEGER :: istat, elementNodes,nodeInElement,material_id,body_id,nmax
  LOGICAL :: FirstTime=.TRUE.,GotIt
  TYPE(Element_t), POINTER :: Element
  TYPE(ValueList_t),POINTER :: Material

  SAVE FirstTime, density

  IF (FirstTime) THEN
     nMax = Model % MaxElementNodes
     ALLOCATE(density(nMax),&
          STAT=istat)
     IF ( istat /= 0 ) THEN
        CALL Fatal('iceproperties (getCapacity)','Memory allocation error, Aborting.')
     END IF
     FirstTime = .FALSE.
     CALL Info('iceproperties (getCapacity)','Memory allocations done', Level=3)
  END IF

  !-------------------------------------------
  ! get element properties
  !-------------------------------------------   
  body_id = Model % CurrentElement % BodyId
  material_id = ListGetInteger(Model % Bodies(body_id) % Values, 'Material', GotIt)
  elementNodes = Model % CurrentElement % Type % NumberOfNodes
  IF (.NOT. GotIt) THEN
     WRITE(Message,'(a,I2,a,I2,a)') 'No material id for current element of node ',n,', body ',body_id,' found'
     CALL FATAL('iceproperties (getCapacity)', Message)
  END IF
  DO nodeInElement=1,elementNodes
     IF ( N == Model % CurrentElement % NodeIndexes(nodeInElement) ) EXIT
  END DO
  Material => Model % Materials(material_id) % Values
  !-------------------------------------------
  ! get density
  !-------------------------------------------
  density(1:elementNodes) = ListGetReal( Material,'Density', elementNodes, &
       Model % CurrentElement % NodeIndexes, GotIt )
  IF (.NOT. GotIt) THEN
     WRITE(Message,'(a,I2,a,I2)') 'No Value for Activation Energy  found in Material ', material_id,' for node ', n
     CALL FATAL('iceproperties (getCapacity)',Message)
  END IF


  !-------------------------------------------
  ! compute heat capacity
  !-------------------------------------------  
  capacity = 2.1275D03 *density(nodeInElement) + 7.253D00*celsius
END FUNCTION getCapacity

!*********************************************************************************************************************************
!*
!* viscosity as a function of the viscosity factor
!*
!*********************************************************************************************************************************
FUNCTION getCriticalShearRate( Model, n, temperature ) RESULT(critShear)
   USE types
   USE CoordinateSystems
   USE SolverUtils
   USE ElementDescription
!-----------------------------------------------------------
   IMPLICIT NONE
!------------ external variables ---------------------------
   TYPE(Model_t) :: Model
   INTEGER :: n
   REAL(KIND=dp) :: critShear, temperature
!------------ internal variables----------------------------
   TYPE(Element_t), POINTER :: Element
   TYPE(ValueList_t),POINTER :: Material
   INTEGER :: DIM, body_id, material_id
   REAL(KIND=dp) ::&
        visFact, cuttofViscosity, power, rateFactor, aToMinusOneThird,&
        activationEnergy, gasconst, enhancementFactor
   LOGICAL :: GotIt, FirstTime = .TRUE. 
!------------ remember this -------------------------------
   Save DIM, FirstTime
!-----------------------------------------------------------
! Read in constants from SIF file
!-----------------------------------------------------------
   IF (FirstTime) THEN
      ! inquire coordinate system dimensions  and degrees of freedom from NS-Solver
      ! ---------------------------------------------------------------------------
      DIM = CoordinateSystemDimension()
      gasconst = ListGetConstReal( Model % Constants,'Gas Constant',GotIt)
      IF (.NOT. GotIt) THEN
         gasconst = 8.314 ! m-k-s
         WRITE(Message,'(a,e10.4,a)') 'No entry for Gas Constant (Constants) in input file found. Setting to ',&
              gasconst,' (J/mol)'
         CALL INFO('iceproperties (getCriticalShearRate)', Message, level=9)
      END IF
      FirstTime = .FALSE.
   END IF
   ! inquire Model parameters
   !-------------------------
   Element => Model % CurrentElement
   body_id = Element % BodyId
   material_id = ListGetInteger( Model % Bodies(body_id) % Values,&
        'Material', Gotit, minv=1, maxv=Model % NumberOFMaterials)
   Material => Model % Materials(material_id) % Values
   power=  ListGetConstReal( Material, 'Viscosity Exponent', Gotit)
   IF (.NOT.Gotit) THEN
      CALL FATAL('iceproperties (getCriticalShearRate)', 'Viscosity Exponent not found')
   ELSE
      WRITE(Message,'(a,e10.4)') 'Viscosity Exponent =', power
      CALL INFO('iceproperties (getCriticalShearRate)', Message, level=9)
   END IF
   cuttofViscosity =  ListGetConstReal(Material, 'Cutoff Viscosity', Gotit)
   IF (.NOT.Gotit) THEN
      CALL FATAL('iceproperties (getCriticalShearRate)', 'Cutoff Viscosity not found')
   ELSE
      WRITE(Message,'(a,e10.4)') 'Cutoff Viscosity =', cuttofViscosity
      CALL INFO('iceproperties (getCriticalShearRate)', Message, level=9) 
   END IF

   ! get viscosity factor for local node
   ! -----------------------------------
!   visFact = ListGetReal(Material, 'Viscosity Factor',1, n, GotIt)
!   IF (.NOT.Gotit) THEN
!      WRITE(Message,'(a,i4,a)') 'Viscosity Factor for point no. ', n, ' not found' 
!      CALL FATAL('iceproperties (getCriticalShearRate)', 'Cutoff Viscosity not found'Message)
!   ELSE
!      WRITE(Message,'(a,i4,a,e10.4)') 'Viscosity Factor for point no. ',&
!           n, ' = ', visFact
!      CALL INFO('iceproperties (getCriticalShearRate)', Message, level=4) 
!   END IF

   IF (temperature < 263.15) THEN
      activationEnergy = 6.0e04 ! m-k-s
      aToMinusOneThird = 4.42577e16
      enhancementFactor = 1.0e00
   ELSE 
      activationEnergy = 1.39e05
      aToMinusOneThird = 2.62508e09
      enhancementFactor = 1.0e00
   END IF
   rateFactor = exp(-activationEnergy/(gasconst*temperature))
   visFact = 0.5*aToMinusOneThird*(enhancementFactor*rateFactor)**(-1.0e00/3.00e00)
   IF (visFact .NE. 0.0e0) THEN
      critShear = (cuttofViscosity/visFact)**(1.0e0/(1.0e0 - power))
   ELSE
      critShear = 0.0d0
   END IF
END FUNCTION getCriticalShearRate

!******************************************************************************************************
!* 
!* limits temperature to pressure melting point and adds homologous temperature as variable
!* 
!******************************************************************************************************
SUBROUTINE HomologousTemperatureSolver( Model,Solver,dt,TransientSimulation )
  USE CoordinateSystems
  USE Types
  USE Lists
  USE SolverUtils
  USE ElementDescription
  USE Integration
!------------------------------------------------------------------------------
  IMPLICIT NONE
!------------------------------------------------------------------------------
!    external variables
!------------------------------------------------------------------------------
  TYPE(Model_t), TARGET :: Model
  TYPE(Solver_t), TARGET :: Solver
  REAL(KIND=dp) :: dt
  LOGICAL :: TransientSimulation
!------------------------------------------------------------------------------
!    Local variables
!------------------------------------------------------------------------------
  CHARACTER(LEN=MAX_NAME_LEN)  :: EquationName
  TYPE(Nodes_t) :: ElementNodes
  TYPE(Element_t),POINTER :: CurrentElement
  TYPE(Variable_t), POINTER :: FlowSol, TempSol
  TYPE(ValueList_t),POINTER :: Material, Equation
  INTEGER :: NMAX, NSDOFs, DIM, n, i, j, elementNumber, istat, material_id, body_id
  INTEGER, POINTER ::&
       HomolTempPerm(:), FlowPerm(:), TempPerm(:), NodeIndexes(:)
  REAL(KIND=dp), POINTER ::&
       Work(:,:), HomolTemp(:), PrevHomolTemp(:,:),&
       FlowSolution(:), PrevFlowSol(:,:), TempSolution(:), PrevTempSol(:,:)
  REAL(KIND=dp), ALLOCATABLE :: &
       ClausiusClapeyron(:), Pressure(:), density(:), pressureMeltingPoint(:), temperature(:)
  REAL(KIND=dp) :: &
       Gravity(3), grav
  LOGICAL :: AllocationsDone = .FALSE., GotIt
!-----------------------------------------------------------------------------
!      remember these variables
!----------------------------------------------------------------------------- 
  SAVE ElementNodes, AllocationsDone, pressure, density, &
       pressureMeltingPoint, temperature, ClausiusClapeyron
!-----------------------------------------------------------------------------
!      get some parameters
!-----------------------------------------------------------------------------
  DIM = CoordinateSystemDimension()
!------------------------------------------------------------------------------
!    Allocate some permanent storage, this is done first time only
!------------------------------------------------------------------------------
  IF ( .NOT. AllocationsDone ) THEN
     NMAX = Model % MaxElementNodes
     ALLOCATE( ElementNodes % x( NMAX ),    &
          ElementNodes % y( NMAX ),    &
          ElementNodes % z( NMAX ),    &
          density( NMAX ), &
          pressure( NMAX ), &
          temperature( NMAX), &
          ClausiusClapeyron( NMAX ),&
          pressureMeltingPoint( NMAX ),&
          STAT=istat )
     IF ( istat /= 0 ) THEN
        CALL Fatal('iceproperties (HomologousTemperatureSolver)','Memory allocation error, Aborting.')
     END IF
     CALL Info('iceproperties (HomologousTemperatureSolver)','Memory allocations done', Level=3)
     AllocationsDone = .TRUE.     
  END IF
!------------------------------------------------------------------------------
!    Get variables for the solution
!------------------------------------------------------------------------------
  EquationName = ListGetString( Solver % Values, 'Equation' )
  HomolTemp     => Solver % Variable % Values     ! Nodal values
  HomolTempPerm => Solver % Variable % Perm       ! Permutations
  PrevHomolTemp  => Solver % Variable % PrevValues ! Nodal values
!------------------------------------------------------------------------------
!    Pointers from NS-Solver and Heat-Solver
!------------------------------------------------------------------------------
  FlowSol => VariableGet( Solver % Mesh % Variables, 'Flow Solution', .TRUE. )
  IF ( ASSOCIATED( FlowSol ) ) THEN
     FlowPerm     => FlowSol % Perm
     NSDOFs       =  FlowSol % DOFs
     FlowSolution => FlowSol % Values
     PrevFlowSol => FlowSol % PrevValues
  ELSE
     CALL FATAl('iceproperties (HomologousTemperatureSolver)', 'No Flow Solution associated.')
  END IF
  WRITE(Message,'(A,I1,A,I1)') 'DIM=', DIM, 'NSDOFs=', NSDOFs 
  CALL Info( 'iceproperties (HomologousTemperatureSolver)', Message, level=4) 
  TempSol => VariableGet( Solver % Mesh % Variables, 'Temperature', .TRUE. )
  IF ( ASSOCIATED( TempSol ) ) THEN
     TempPerm     => TempSol % Perm
     TempSolution => TempSol % Values
     PrevTempSol => TempSol % PrevValues
  ELSE
     CALL Info('iceproperties (HomologousTemperatureSolver)', 'No variable for temperature associated.', Level=4)
  END IF
  WRITE(Message,'(a,i1,a,i1)') 'DIM=', DIM, ', NSDOFs=', NSDOFs
  CALL Info( 'iceproperties (HomologousTemperatureSolver)', Message, level=4) 
!-----------------------------------------------------------------------------
! loop all elements
!-----------------------------------------------------------------------------
  DO elementNumber=1,Solver % NumberOfActiveElements
     !-------------------------------
     ! get some information on element
     !-------------------------------
     CurrentElement => Solver % Mesh % Elements(Solver % ActiveElements(elementNumber)) 
     body_id = CurrentElement % BodyId
     material_id = ListGetInteger( Model % Bodies(body_id) % Values, 'Material' )
     Material => Model % Materials(material_id) % Values
     NodeIndexes => CurrentElement % NodeIndexes
     n = CurrentElement % TYPE % NumberOfNodes
     !--------------
     ! get pressures
     !--------------
     IF ( ASSOCIATED( FlowSol ) ) THEN
        DO i=1,n
           j = NSDOFs*FlowPerm(NodeIndexes(i))
           Pressure(i) = FlowSolution(j)
        END DO
     ELSE
        CALL Fatal( 'iceproperties (HomologousTemperatureSolver)', 'No flow solution associated') 
     END IF
     !----------------
     ! get temperature
     !----------------
     IF ( ASSOCIATED( TempSol ) ) THEN
        DO i=1,n
           j =TempPerm(NodeIndexes(i))
           Temperature(i) = TempSolution(j)
        END DO
     ELSE
        CALL Fatal( 'iceproperties (HomologousTemperatureSolver)', 'No solution for heat equation associated') 
     END IF
     !---------------------------------
     ! get Clausius Clapeyron parameter
     !---------------------------------
     ClausiusClapeyron(1:n) = ListGetReal(Material, 'Clausius Clapeyron', n, NodeIndexes, GotIt)
     IF (.NOT. GotIt) THEN
        WRITE(Message,'(A,I2,A)') 'No value for Clausius Clapeyron in Material ,',material_id,' found'
        CALL FATAL('iceproperties (HomologousTemperatureSolver)',Message)
     END IF
     !------------
     ! get density
     !------------
     density(1:n) = ListGetReal(Material, 'density', n, NodeIndexes, GotIt)
     IF (.NOT. GotIt) THEN
        WRITE(Message,'(A,I2,A)') 'No value for density in Material',material_id,' found'
        CALL FATAL('iceproperties (HomologousTemperatureSolver)',Message)
     END IF
     !-------------------------------
     ! get gravitational acceleration
     !-------------------------------
     Work => ListGetConstRealArray( Model % Constants,'Gravity',GotIt)
     IF ( GotIt ) THEN
        Gravity = Work(1:3,1)*Work(4,1)
     ELSE
        CALL INFO('iceproperties (HomologousTemperatureSolver)','No value for Gravity (Constants) found', level=1)
        IF (DIM == 1) THEN
           Gravity(1) = -9.81D0
           CALL INFO('iceproperties (HomologousTemperatureSolver)','setting to -9.81', level=1)
        ELSE IF (DIM == 2) THEN
           Gravity    =  0.00D0
           Gravity(2) = -9.81D0
           CALL INFO('iceproperties (HomologousTemperatureSolver)','setting to (0,-9.81)', level=1)
        ELSE
           Gravity    =  0.00D0
           Gravity(3) = -9.81D0
           CALL INFO('iceproperties (HomologousTemperatureSolver)','setting to (0,0,-9.81)', level=1)
        END IF
     END IF
     IF (DIM == 1) THEN
        grav = ABS(Gravity(1))
     ELSE IF (DIM == 2) THEN
        grav = ABS(Gravity(2))
     ELSE
        grav = ABS(Gravity(3))
     END IF         
     !-------------------------------    
     ! compute homologous temperature (that's why we are here!)
     !-------------------------------
     DO i=1,n
        IF (density(i)*grav > 0.0D00) THEN
           pressureMeltingPoint(i) =  2.7316D02 - ClausiusClapeyron(i)*Pressure(i)/(density(i)*grav)
           HomolTemp(HomolTempPerm(NodeIndexes(i))) = Temperature(i) - pressureMeltingPoint(i)
        ELSE
           WRITE(Message,'(A,E14.5,A,E14.5,A,E14.5)')&
                'negative value for nominator density*grav=',density(i)*grav,'=',density(i),'*',grav 
           CALL FATAL('iceproperties (HomologousTemperatureSolver)',Message)
        END IF
        !----------------------------------------------
        ! limit solution:
        ! do not allow homologue temperatures above 0.0
        ! i.e., temperatures above pressure melting 
        !       points
        !----------------------------------------------
        IF (HomolTemp(HomolTempPerm(NodeIndexes(i))) > 0.0D00) THEN        
           HomolTemp(HomolTempPerm(NodeIndexes(i))) = 0.0D00
           TempSolution(TempPerm(NodeIndexes(i))) = pressureMeltingPoint(i)
           WRITE(Message,'(A,I4,A,E14.5,A,E14.5)')&
                'Corrected temperature at point ',TempPerm(NodeIndexes(i)),' from ', Temperature(i),&
                'to', TempSolution(TempPerm(NodeIndexes(i)))
           CALL INFO('iceproperties (HomologousTemperatureSolver)',Message,level=4)
        END IF
     END DO
  END DO
END SUBROUTINE HomologousTemperatureSolver


!*********************************************************************************************************************************
!*
!* density  as a function of the position; Special and dirty workaround for crater2d_* cases -DO NOT USE ELSEWHERE!!
!*
!*********************************************************************************************************************************
FUNCTION getDensity( Model, Node, Depth ) RESULT(density)
  USE types
  USE CoordinateSystems
  USE SolverUtils
  USE ElementDescription
!-----------------------------------------------------------
  IMPLICIT NONE
!------------ external variables ---------------------------
  TYPE(Model_t) :: Model
  INTEGER :: Node
  REAL(KIND=dp) :: Depth, density
!------------ internal variables----------------------------
  TYPE(ValueList_t), POINTER :: Material
  INTEGER :: i,j
  REAL (KIND=dp) :: XCoord, Height, porosity

  INTERFACE
     FUNCTION getPorosity( Model, Node, Depth ) RESULT(porosity)
       USE types
!       REAL(KIND=dp) :: getPorosity 
       !------------ external variables ---------------------------
       TYPE(Model_t) :: Model
       INTEGER :: Node
       REAL(KIND=dp) :: Depth, porosity
     END FUNCTION getPorosity
  END INTERFACE

  porosity = getPorosity(Model, Node, Depth)
  density = 9.18D02 * porosity

END FUNCTION getDensity


!*********************************************************************************************************************************
!*
!* viscosity factor (due to porosity); Special and dirty workaround for crater2d_* cases -DO NOT USE ELSEWHERE!!
!*
!*********************************************************************************************************************************
FUNCTION getViscosity( Model, Node, Depth ) RESULT(viscosity)
  USE types
  USE CoordinateSystems
  USE SolverUtils
  USE ElementDescription
!-----------------------------------------------------------
  IMPLICIT NONE
!------------ external variables ---------------------------
  TYPE(Model_t) :: Model
  INTEGER :: Node
  REAL(KIND=dp) :: Depth, viscosity
  INTERFACE
     FUNCTION getPorosity( Model, Node, Depth ) RESULT(porosity)
       USE types
!       REAL(KIND=dp) :: getPorosity 
       !------------ external variables ---------------------------
       TYPE(Model_t) :: Model
       INTEGER :: Node
       REAL(KIND=dp) :: Depth, porosity
     END FUNCTION getPorosity
  END INTERFACE

  viscosity = getPorosity(Model, Node, Depth)
END FUNCTION getViscosity



!*********************************************************************************************************************************
!*
!* heat conductivity factor 
!*
!*********************************************************************************************************************************
FUNCTION getHeatConductivityFactor( Model, Node, Depth ) RESULT(heatCondFact)
  USE types
  USE CoordinateSystems
  USE SolverUtils
  USE ElementDescription
!-----------------------------------------------------------
  IMPLICIT NONE
!------------ external variables ---------------------------
  TYPE(Model_t) :: Model
  INTEGER :: Node
  REAL(KIND=dp) :: Depth, heatCondFact
  REAL(KIND=dp) :: porosity
  INTERFACE
     FUNCTION getPorosity( Model, Node, Depth ) RESULT(porosity)
       USE types
       !------------ external variables ---------------------------
       TYPE(Model_t), TARGET :: Model
       INTEGER :: Node
       REAL(KIND=dp) :: Depth, porosity
     END FUNCTION getPorosity
  END INTERFACE
  porosity = getPorosity(Model, Node, Depth)
  heatCondFact = 9.828e00 * porosity
END FUNCTION getHeatConductivityFactor

!*********************************************************************************************************************************
!*
!* pressure melting point
!*
!*********************************************************************************************************************************
FUNCTION getPressureMeltingPoint( Model, n, Pressure ) RESULT(PressureMeltingPoint)
   USE types
   USE CoordinateSystems
   USE SolverUtils
   USE ElementDescription
!-----------------------------------------------------------
   IMPLICIT NONE
!------------ external variables ---------------------------
   TYPE(Model_t) :: Model
   INTEGER :: n
   REAL(KIND=dp) :: PressureMeltingPoint, Pressure
!------------ internal variables----------------------------
   TYPE(Element_t), POINTER :: Element
   TYPE(ValueList_t),POINTER :: Material
   INTEGER :: body_id, material_id, nodeInElement, istat, elementNodes
   REAL(KIND=dp), ALLOCATABLE :: ClausiusClapeyron(:)
   LOGICAL :: GotIt, FirstTime = .TRUE. 
!------------ remember this -------------------------------
   Save FirstTime, ClausiusClapeyron
  !-------------------------------------------
  ! Allocations 
  !------------------------------------------- 
  IF (FirstTime) THEN
     ALLOCATE(ClausiusClapeyron(Model % MaxElementNodes),&
          STAT=istat)
     IF ( istat /= 0 ) THEN
        CALL FATAL('iceproperties (getPressureMeltingPoint)','Memory allocation error, Aborting.')
     END IF
     FirstTime = .FALSE.
     CALL INFO('iceproperties (getPressureMeltingPoint)','Memory allocation done', level=3)
  END IF
  !-------------------------------------------
  ! get element properties
  !-------------------------------------------   
  IF ( .NOT. ASSOCIATED(Model % CurrentElement) ) THEN
     CALL FATAL('iceproperties (getPressureMeltingPoint)', 'Model % CurrentElement not associated')
  ELSE
     Element => Model % CurrentElement
  END IF
  body_id = Element % BodyId
  material_id = ListGetInteger(Model % Bodies(body_id) % Values, 'Material', GotIt)
  elementNodes = Element % Type % NumberOfNodes
  IF (.NOT. GotIt) THEN
     WRITE(Message,'(a,I2,a,I2,a)') 'No material id for current element of node ',elementNodes,', body ',body_id,' found'
     CALL FATAL('iceproperties (getPressureMeltingPoint)', Message)
  END IF
  DO nodeInElement=1,elementNodes
     IF ( N == Model % CurrentElement % NodeIndexes(nodeInElement) ) EXIT
  END DO
  Material => Model % Materials(material_id) % Values
  !-------------------------
  ! get material parameters
  !-------------------------
  ClausiusClapeyron(1:elementNodes) = &
       ListGetReal( Material, 'Clausius Clapeyron', elementNodes, Element % NodeIndexes, GotIt)
  IF (.NOT. GotIt) THEN
     CALL FATAL('iceproperties (getPressureMeltingPoint)','No value for Clausius Clapeyron parameter found')
  END IF
  !-------------------------------
  ! compute pressure melting point
  !-------------------------------
  PressureMeltingPoint = 2.7316D02 - ClausiusClapeyron(nodeInElement)*MAX(Pressure,0.0d00)
END FUNCTION getPressureMeltingPoint
