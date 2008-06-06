
!/*****************************************************************************/! *
! *  Elmer, A Finite Element Software for Multiphysical Problems
! *
! *  Copyright 1st April 1995 - , CSC - Scientific Computing Ltd., Finland
! *
! *  This program is free software; you can redistribute it and/or
! *  modify it under the terms of the GNU General Public License
! *  as published by the Free Software Foundation; either version 2
! *  of the License, or (at your option) any later version.
! *
! *  This program is distributed in the hope that it will be useful,
! *  but WITHOUT ANY WARRANTY; without even the implied warranty of
! *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
! *  GNU General Public License for more details.
! *
! *  You should have received a copy of the GNU General Public License
! *  along with this program (in file fem/GPL-2); if not, write to the
! *  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
! *  Boston, MA 02110-1301, USA.
! *
! *****************************************************************************/
!
!/******************************************************************************
! *
! *  Module containing solvers and routines for standard ice dynamic problems
! *
! ******************************************************************************
! *
! *  Authors: Thomas Zwinger, Mikko Lyly, Juha Ruokolainen
! *  Email:   Thomas.Zwinger@csc.fi, Juha.Ruokolainen@csc.fi 
! *  Web:     http://www.csc.fi/elmer
! *  Address: CSC - Scientific Computing Ltd.
! *           Keilaranta 14
! *           02101 Espoo, Finland 
! *
! *  Original Date: 14 May 2007
! *
! *****************************************************************************/
!------------------------------------------------------------------------------
   RECURSIVE SUBROUTINE TemperateIceSolver( Model,Solver,Timestep,TransientSimulation )
!DLLEXPORT TemprateIceSolver
!------------------------------------------------------------------------------
!******************************************************************************
!
!  Solve the convection diffusion equation with limiters!
!
!  ARGUMENTS:
!
!  TYPE(Model_t) :: Model,  
!     INPUT: All model information (mesh,materials,BCs,etc...)
!
!  TYPE(Solver_t) :: Solver
!     INPUT: Linear equation solver options
!
!  REAL(KIND=dp) :: Timestep
!     INPUT: Timestep size for time dependent simulations
!
!******************************************************************************
     USE DiffuseConvective
     USE DiffuseConvectiveGeneral
     USE Differentials
     USE MaterialModels
!     USE Adaptive
     USE DefUtils

!------------------------------------------------------------------------------
     IMPLICIT NONE
!------------------------------------------------------------------------------
!------------------------------------------------------------------------------
!    External variables
!------------------------------------------------------------------------------
     TYPE(Model_t)  :: Model
     TYPE(Solver_t), TARGET :: Solver
     LOGICAL :: TransientSimulation
     REAL(KIND=dp) :: Timestep
!------------------------------------------------------------------------------
!    Local variables
!------------------------------------------------------------------------------
     TYPE(Solver_t), POINTER :: PointerToSolver
     TYPE(Matrix_t), POINTER :: Systemmatrix
     TYPE(Nodes_t) :: ElementNodes
     TYPE(Element_t),POINTER :: Element
     TYPE(Variable_t), POINTER :: TempSol,FlowSol,CurrentSol, MeshSol,VarTempHom,VarTempResidual
     TYPE(ValueList_t), POINTER :: Equation,Material,SolverParams,BodyForce,BC,Constants

     INTEGER :: i,j,k,l,m,n,t,iter,body_id,eq_id,material_id, &
          istat, LocalNodes,bf_id, bc_id,  DIM, &
          NSDOFs, NonlinearIter
     INTEGER, POINTER :: NodeIndexes(:), TempPerm(:),FlowPerm(:),CurrentPerm(:),MeshPerm(:)

     CHARACTER(LEN=MAX_NAME_LEN) :: ConvectionFlag, VariableName, SolverName, FlowSolName

     LOGICAL :: Stabilize = .TRUE., Bubbles = .TRUE., UseBubbles, &
          Found, FluxBC, Permeable=.TRUE., IsPeriodicBC=.FALSE.,&
          AllocationsDone = .FALSE.,  SubroutineVisited = .FALSE., FirstTime=.TRUE.,&
          LimitSolution, ApplyDirichlet, FlowSolutionFound
     LOGICAL, ALLOCATABLE ::  LimitedSolution(:), ActiveNode(:)

     REAL(KIND=dp) :: NonlinearTol, LinearTol, Relax, &
          SaveRelax,dt,CumulativeTime, RelativeChange, &
          Norm,PrevNorm,S,C, &
          ReferencePressure=0.0d0, &
          HeatCapacityGradient(3), round = 0.0D00
     REAL(KIND=dp), POINTER :: Temp(:), FlowSolution(:), &
          ForceVector(:), PrevSolution(:), HC(:), Hwrk(:,:,:),&
          PointerToResidualVector(:),&
          ResidualVector(:), TempHomologous(:)
     REAL(KIND=dp), ALLOCATABLE :: MASS(:,:), &
       STIFF(:,:), LOAD(:), HeatConductivity(:,:,:), &
         FORCE(:), Pressure(:),  MeshVelocity(:,:),&
         IceVeloU(:),IceVeloV(:),IceVeloW(:),TimeForce(:), &
         TransferCoeff(:), LocalTemp(:), Work(:), C1(:), C0(:), Zero(:), Viscosity(:),&
         UpperLimit(:), HeatCapacity(:),  Density(:), TempExt(:), &
         StiffVector(:), OldValues(:), OldRHS(:)
     REAL(KIND=dp) :: at,at0,totat,st,totst,t1,CPUTime,RealTime

     SAVE &
          OldValues,             &
          OldRHS,                &
          MeshVelocity,          &
          IceVeloU,             &
          IceVeloV,             &
          IceVeloW,             &
          Pressure,              &
          ElementNodes    ,      &
          Work,Zero,             &
          Viscosity,             &
          HeatCapacity,          &
          Density,               &
          TempExt,               &
          C1,                    &
          C0,                    &
          TransferCoeff,         &
          LocalTemp,             &
          HeatConductivity,      &
          MASS,                  &
          STIFF,LOAD,            &
          FORCE,                 &
          TimeForce,             &
          StiffVector,           &
          ResidualVector,        &
          UpperLimit,            &
          LimitedSolution,       &
          ActiveNode,            &
          AllocationsDone, FirstTime, Hwrk, VariableName, SolverName, NonLinearTol, M, round

!------------------------------------------------------------------------------
!    Get variables needed for solution
!------------------------------------------------------------------------------
     DIM = CoordinateSystemDimension()
     SolverName = 'TemperateIceSolver ('// TRIM(Solver % Variable % Name) // ')'
     VariableName = TRIM(Solver % Variable % Name)

     ! say hello
     CALL INFO(SolverName, 'for variable '//Variablename, level=1 )

     IF ( .NOT. ASSOCIATED( Solver % Matrix ) ) RETURN
     SystemMatrix => Solver % Matrix
     IF ( .NOT. ASSOCIATED( SystemMatrix ) ) &
        CALL FATAL(Solvername,"SystemMatrix not associated")
     ForceVector => Solver % Matrix % RHS
    IF ( .NOT. ASSOCIATED(ForceVector  ) ) &
        CALL FATAL(Solvername,"ForceVector not associated")

     PointerToSolver => Solver

     TempSol => Solver % Variable
     TempPerm  => TempSol % Perm
     Temp => TempSol % Values
     
     LocalNodes = COUNT( TempPerm > 0 )
     IF ( LocalNodes <= 0 ) RETURN

     

!------------------------------------------------------------------------------
!    Allocate some permanent storage, this is done first time only
!------------------------------------------------------------------------------
     IF ( .NOT. AllocationsDone .OR. Solver % Mesh % Changed ) THEN
        N = Solver % Mesh % MaxElementNodes
        M = Model % Mesh % NumberOfNodes
        K = SIZE( SystemMatrix % Values )
        L = SIZE( SystemMatrix % RHS )

        IF ( AllocationsDone ) THEN
           DEALLOCATE(                    &
                OldValues,                &
                OldRHS,                   &
                MeshVelocity,             &
                IceVeloU,                &
                IceVeloV,                &
                IceVeloW,                &
                Pressure,                 &
                ElementNodes % x,         &
                ElementNodes % y,         &
                ElementNodes % z,         &
                Work,Zero,                &
                Viscosity,                &
                HeatCapacity,             &
                Density,                  &
                TempExt,                  &
                C1,                       &
                C0,                       &
                TransferCoeff,            &
                LocalTemp,                &
                HeatConductivity,         &
                MASS,                     &
                STIFF,LOAD,               &
                FORCE,                    &
                TimeForce,                &
                StiffVector,              &
                ResidualVector,           &
                UpperLimit,               &
                LimitedSolution,          &
                ActiveNode)              
        END IF                           
        
        ALLOCATE(                                  &
             OldValues( K ), &
             OldRHS( L ), &
             MeshVelocity( 3,N ),                  &
             IceVeloU( N ),                       &
             IceVeloV( N ),                       &
             IceVeloW( N ),                       &
             Pressure( N ),                        &
             ElementNodes % x( N ),                &
             ElementNodes % y( N ),                &
             ElementNodes % z( N ),                &
             Work( N ), Zero( N ),                 &
             Viscosity( N ),                       &
             HeatCapacity( M ),                    &
             Density( N ),                         &
             TempExt( N ),                         &
             C1( N ),                              &
             C0( N ),                              &
             TransferCoeff( N ),                   &
             LocalTemp( N ),                       &
             HeatConductivity( 3,3,N ),            &
             MASS(  2*N,2*N ),                     &
             STIFF( 2*N,2*N ),LOAD( N ),           &
             FORCE( 2*N ),                         &
             TimeForce( 2*N ),                     &
             StiffVector( L ),                     &
             ResidualVector(L),                    &
             UpperLimit( M ),                      &
             LimitedSolution( M ),                 &
             ActiveNode( M ),                      &
             STAT=istat )

        IF ( istat /= 0 ) THEN
           CALL FATAL( SolverName, 'Memory allocation error' )
        ELSE
           CALL INFO(SolverName, 'Memory allocation done', level=1 )
        END IF

        ActiveNode = .FALSE.
        
        AllocationsDone = .TRUE.

     END IF

!------------------------------------------------------------------------------
!    Say hello
!------------------------------------------------------------------------------
     WRITE(Message,'(A,A)')&
          'Limited diffusion Solver for variable ', VariableName
     CALL INFO(SolverName,Message,Level=1)

!------------------------------------------------------------------------------
!    Read physical and numerical constants and initialize 
!------------------------------------------------------------------------------
     Constants => GetConstants()
     SolverParams => GetSolverParams()

     Stabilize = GetLogical( SolverParams,'Stabilize',Found )
     IF (.NOT. Found) Stabilize = .FALSE.
     UseBubbles = GetLogical( SolverParams,'Bubbles',Found )
     IF ( .NOT.Found .AND. (.NOT.Stabilize)) UseBubbles = .TRUE.

     LinearTol = GetConstReal( SolverParams, &
          'Linear System Convergence Tolerance',    Found )
     IF ( .NOT.Found ) THEN
        CALL FATAL(SolverName, 'No >Linear System Convergence Tolerance< found')
     END IF


     NonlinearIter = GetInteger(   SolverParams, &
                     'Nonlinear System Max Iterations', Found )
     IF ( .NOT.Found ) NonlinearIter = 1


     NonlinearTol  = GetConstReal( SolverParams, &
          'Nonlinear System Convergence Tolerance',    Found )

     Relax = GetConstReal( SolverParams, &
               'Nonlinear System Relaxation Factor',Found )

     IF ( .NOT.Found ) Relax = 1.0D00

     ApplyDirichlet = GetLogical( SolverParams, &
          'Apply Dirichlet', Found)
     IF ( .NOT.Found ) THEN
        ApplyDirichlet = .FALSE.
     END IF

     SaveRelax = Relax
     dt = Timestep
     CumulativeTime = 0.0d0

     ALLOCATE( PrevSolution(LocalNodes) )
!------------------------------------------------------------------------------
!   time stepping loop.
!------------------------------------------------------------------------------
     DO WHILE( CumulativeTime < Timestep-1.0d-12 .OR. .NOT. TransientSimulation )
        round = round +1.0D00
!------------------------------------------------------------------------------
!       The first time around this has been done by the caller...
!------------------------------------------------------------------------------
        IF ( TransientSimulation .AND. .NOT.FirstTime ) THEN
           CALL InitializeTimestep( Solver )
        END IF
        FirstTime = .FALSE.       
!------------------------------------------------------------------------------
!       Save current solution
!------------------------------------------------------------------------------
        PrevSolution = Temp(1:LocalNodes)

        totat = 0.0d0
        totst = 0.0d0


!------------------------------------------------------------------------------
!       Get externally declared DOFs
!------------------------------------------------------------------------------
        IF (.NOT.ApplyDirichlet) ActiveNode = .FALSE.
        VarTempHom => VariableGet( Model % Mesh % Variables, TRIM(Solver % Variable % Name) // ' Homologous' )
        IF (.NOT.ASSOCIATED(VarTempHom)) THEN
           WRITE(Message,'(A)') TRIM(Solver % Variable % Name) // ' Homologous not associated'
           CALL FATAL( SolverName, Message)
        END IF        

        VarTempResidual => VariableGet( Model % Mesh % Variables, TRIM(Solver % Variable % Name) // ' Residual' )
        IF (.NOT.ASSOCIATED(VarTempResidual)) THEN
           WRITE(Message,'(A)') '>' // TRIM(Solver % Variable % Name) // ' Residual< not associated'
           CALL FATAL( SolverName, Message)
        END IF
        PointerToResidualVector => VarTempResidual % Values

        
!------------------------------------------------------------------------------
!       non-linear system iteration loop
!------------------------------------------------------------------------------
        DO iter=1,NonlinearIter
              
           !------------------------------------------------------------------------------
           ! print out some information
           !------------------------------------------------------------------------------
           at  = CPUTime()
           at0 = RealTime()

           CALL Info( SolverName, ' ', Level=4 )
           CALL Info( SolverName, ' ', Level=4 )
           CALL Info( SolverName, '-------------------------------------',Level=4 )
           WRITE( Message,'(A,A,I3,A,I3)') &
                TRIM(Solver % Variable % Name),  ' iteration no.', iter,' of ',NonlinearIter
           CALL Info( SolverName, Message, Level=4 )
           CALL Info( SolverName, '-------------------------------------',Level=4 )
           CALL Info( SolverName, ' ', Level=4 )
           CALL Info( SolverName, 'Starting Assembly...', Level=4 )
           !------------------------------------------------------------------------------
           ! lets start
           !------------------------------------------------------------------------------
           CALL DefaultInitialize()
           !-----------------------------------------------------------------------------
           ! Get lower and Upper limit:
           !-----------------------------------------------------------------------------
           DO t=1,Solver % NumberOfActiveElements
              Element => GetActiveElement(t)
              n = GetElementNOFNodes()
              CALL GetElementNodes( ElementNodes )
              Material => GetMaterial()
              ! upper limit
              !------------
              UpperLimit(Element % Nodeindexes(1:N)) = ListGetReal(Material,TRIM(VariableName) // & 
                   ' Upper Limit',n,Element % NodeIndexes, Found)
              IF (.NOT. Found) THEN
                 LimitedSolution(Element % Nodeindexes(1:N)) = .FALSE.
                 WRITE(Message,'(a,i10)') 'No upper limit of solution for element no. ', t
                 CALL INFO(SolverName, Message, level=10)
              ELSE
                 LimitedSolution(Element % Nodeindexes(1:N)) = .TRUE.
              END IF
           END DO
           !------------------------------------------------------------------------------
           ! write some info on max/min values
           !------------------------------------------------------------------------------
           WRITE(Message,'(a,e13.6,a,e13.6)') &
                'Max/min values Temperature:', MAXVAL( Temp(:)),'/',MINVAL( Temp(:))
           CALL INFO(SolverName,Message,Level=4)
           !------------------------------------------------------------------------------
           body_id = -1
           NULLIFY(Material)
           !------------------------------------------------------------------------------
           ! Bulk elements
           !------------------------------------------------------------------------------
           DO t=1,Solver % NumberOfActiveElements
              !------------------------------------------------------------------------------
              ! write some info on status of assembly
              !------------------------------------------------------------------------------
              IF ( RealTime() - at0 > 1.0 ) THEN
                 WRITE(Message,'(a,i3,a)' ) '   Assembly: ', INT(100.0 - 100.0 * &
                      (Solver % NumberOfActiveElements-t) / &
                      (1.0*Solver % NumberOfActiveElements)), ' % done'

                 CALL Info( SolverName, Message, Level=5 )

                 at0 = RealTime()
              END IF
              !------------------------------------------------------------------------------
              ! Check if this element belongs to a body where scalar equation
              ! should be calculated and (if parallel) it is part of the partition
              !------------------------------------------------------------------------------
              Element => GetActiveElement(t,Solver)
              IF (ParEnv % myPe .NE. Element % partIndex) CYCLE
              IF (.NOT.ASSOCIATED(Element)) CYCLE
              IF ( Element % BodyId /= body_id ) THEN
                 Equation => GetEquation()
                 IF (.NOT.ASSOCIATED(Equation)) THEN
                    WRITE (Message,'(A,I3)') 'No Equation  found for boundary element no. ', t
                    CALL FATAL(SolverName,Message)
                 END IF

                 ConvectionFlag = GetString( Equation, 'Convection', Found )

                 Material => GetMaterial()
                 IF (.NOT.ASSOCIATED(Material)) THEN
                    WRITE (Message,'(A,I3)') 'No Material found for boundary element no. ', t
                    CALL FATAL(SolverName,Message)
                 ELSE
                    material_id = GetMaterialId( Element, Found)
                    IF(.NOT.Found) THEN
                       WRITE (Message,'(A,I3)') 'No Material ID found for boundary element no. ', t
                       CALL FATAL(SolverName,Message)
                    END IF
                 END IF
              END IF


              k = ListGetInteger( Model % Bodies(Element % BodyId) % Values, 'Equation', &
                   minv=1, maxv=Model % NumberOFEquations )

              SELECT CASE( ListGetString( Model % Equations(k) % Values, &
                   'Convection', Found ) )

                 !-----------------
              CASE( 'computed' )
                 !-----------------

                 FlowSolName =  GetString( Model % Equations(k) % Values,'Flow Solution Name', Found)
                 IF(.NOT.Found) THEN        
                    CALL WARN(SolverName,'Keyword >Flow Solution Name< not found in section >Equation<')
                    CALL WARN(SolverName,'Taking default value >Flow Solution<')
                    WRITE(FlowSolName,'(A)') 'Flow Solution'
                 END IF


                 FlowSol => VariableGet( Solver % Mesh % Variables, FlowSolName )
                 IF ( ASSOCIATED( FlowSol ) ) THEN
                    FlowPerm     => FlowSol % Perm
                    NSDOFs       =  FlowSol % DOFs
                    FlowSolution => FlowSol % Values
                    FlowSolutionFound = .TRUE.
                 ELSE
                    CALL INFO(SolverName,'No Flow Solution associated',Level=1)
                    FlowSolutionFound = .FALSE.
                 END IF
              CASE( "none")
                 FlowSolutionFound = .FALSE.

              END SELECT

              !------------------------------------------------------------------------------
              ! Get element material parameters
              !------------------------------------------------------------------------------              
              N = GetElementNOFNodes(Element)
              CALL GetElementNodes( ElementNodes )
              CALL ListGetRealArray( Material,TRIM(Solver % Variable % Name) // &
                   ' Heat Conductivity',Hwrk,n, Element % NodeIndexes )
              HeatConductivity = 0.0d0
              IF ( SIZE(Hwrk,1) == 1 ) THEN
                 DO i=1,3
                    HeatConductivity( i,i,1:N ) = Hwrk( 1,1,1:N)
                 END DO
              ELSE IF ( SIZE(Hwrk,2) == 1 ) THEN
                 DO i=1,MIN(3,SIZE(Hwrk,1))
                    HeatConductivity(i,i,1:N) = Hwrk(i,1,1:N)
                 END DO
              ELSE
                 DO i=1,MIN(3,SIZE(Hwrk,1))
                    DO j=1,MIN(3,SIZE(Hwrk,2))
                       HeatConductivity(i,j,1:N) = Hwrk(i,j,1:N)
                    END DO
                 END DO
              END IF              
              HeatCapacity(1:N) =  ListGetReal( Material,  TRIM(Solver % Variable % Name) // &
                   ' Heat Capacity', n, Element % NodeIndexes, Found )
              IF (.NOT.Found) THEN
                 HeatCapacity = 0.0D00
                 WRITE(Message,'(a,a,a,i5,a,i5,a)') 'Keyword >', TRIM(Solver % Variable % Name) // &
                   ' Heat Capacity', '< not found for element ', t, ' material ', material_id
                 CALL INFO(SolverName,Message,Level=4)
              END IF

              Density(1:N) = ListGetReal( Material, 'Density',  N, Element % NodeIndexes, Found )
              IF (.NOT.Found) THEN
                 Density = 0.0D00
                 WRITE(Message,'(a,i5,a,i5,a)') 'Keyword >Density< not found for element ',&
                      t, ' material ', material_id
                 CALL FATAL(SolverName,Message)
              END IF
              !------------------------------------------
              ! NB.: viscosity needed for strain heating
              !      but Newtonian flow is assumed
              !------------------------------------------
              Viscosity = 0.0D00
              !------------------------------------------------------------------------------
              ! Get mesh velocity
              !------------------------------------------------------------------------------
              MeshVelocity = 0.0d0
              CALL GetVectorLocalSolution( MeshVelocity, 'Mesh Velocity')
              !------------------------------------------------------------------------------         
              ! asuming convection or ALE mesh contribution by default
              !------------------------------------------------------------------------------         
              DO i=1,N
                 C1(i) = Density(i) * HeatCapacity(i)
!                 C1(i) = HeatCapacity(i)
!                 PRINT *, 'C1(',i,')=',C1(i)
              END DO
              !------------------------------------------------------------------------------
              ! Get scalar velocity
              !------------------------------------------------------------------------------         
              IceVeloU = 0.0d00
              IceVeloV = 0.0d00
              IceVeloW = 0.0d00
              ! constant (i.e., in section Material given) velocity
              !---------------------------------------------------
              IF ( ConvectionFlag == 'constant' ) THEN
                 IceVeloU(1:N) = GetReal( Material, 'Convection Velocity 1', Found )
                 IceVeloV(1:N) = GetReal( Material, 'Convection Velocity 2', Found )
                 IceVeloW(1:N) = GetReal( Material, 'Convection Velocity 3', Found )                 
              ! computed velocity
              !------------------
              ELSE IF (( ConvectionFlag == 'computed' ) .AND. FlowSolutionFound) THEN
                 DO i=1,n
                    k = FlowPerm(Element % NodeIndexes(i))
                    IF ( k > 0 ) THEN
                       Pressure(i) = FlowSolution(NSDOFs*k) + ReferencePressure
                       SELECT CASE( NSDOFs )
                       CASE(3)
                          IceVeloU(i) = FlowSolution( NSDOFs*k-2 )
                          IceVeloV(i) = FlowSolution( NSDOFs*k-1 )
                          IceVeloW(i) = 0.0D0
                       CASE(4)
                          IceVeloU(i) = FlowSolution( NSDOFs*k-3 )
                          IceVeloV(i) = FlowSolution( NSDOFs*k-2 )
                          IceVeloW(i) = FlowSolution( NSDOFs*k-1 )
                       END SELECT
                    END IF
                 END DO
                 WRITE(Message,'(a,i5, a, i5)') 'Convection in element ', t, &
                      ' material ',  material_id
              ELSE  ! Conduction and ALE contribution only
                 IF (ANY( MeshVelocity /= 0.0d0 )) THEN
                    WRITE(Message,'(a,i5, a, i5)') 'Only mesh deformation in element ', t,&
                         ' material ',  material_id
                 ELSE ! neither convection nor ALE mesh deformation contribution -> all C1(1:N)=0
                    C1 = 0.0D0 
                    WRITE(Message,'(a,i5, a, i5)') 'No convection and mesh deformation in element ', t,&
                         ' material ',  material_id
                 END IF                 
              END IF
              CALL INFO(SolverName,Message,Level=10)
              !------------------------------------------------------------------------------
              ! no contribution proportional to temperature by default
              !------------------------------------------------------------------------------
              C0=0.0d00
              !------------------------------------------------------------------------------
              ! Add body forces
              !------------------------------------------------------------------------------
              LOAD = 0.0D00
              BodyForce => GetBodyForce()
              IF ( ASSOCIATED( BodyForce ) ) THEN
                 bf_id = GetBodyForceId()
                 LOAD(1:N) = LOAD(1:N) +   &
                      GetReal( BodyForce, TRIM(Solver % Variable % Name) // ' Volume Source', Found )
                 IF (.NOT.Found) LOAD(1:N) = 0.0D00
              END IF
              !------------------------------------------------------------------------------
              ! dummy input array for faking   heat capacity, density, temperature, 
              !                                enthalpy and viscosity
              !------------------------------------------------------------------------------
              Work = 1.0d00
              Zero = 0.0D00
              !------------------------------------------------------------------------------
              ! Do we really need residual free Bubbles
              !------------------------------------------------------------------------------
              Bubbles = UseBubbles  .AND. &
                   ( ConvectionFlag == 'computed' .OR. ConvectionFlag == 'constant' )         
              !------------------------------------------------------------------------------
              ! Get element local matrices, and RHS vectors
              !------------------------------------------------------------------------------
              MASS = 0.0d00
              STIFF = 0.0d00
              FORCE = 0.0D00
              ! cartesian coords
              !----------------
              IF ( CurrentCoordinateSystem() == Cartesian ) THEN
                 CALL DiffuseConvectiveCompose( &
                      MASS, STIFF, FORCE, LOAD, &
                      HeatCapacity, C0, C1(1:N), HeatConductivity, &
                      .FALSE., Zero, Zero, IceVeloU, IceVeloV, IceVeloW, &
                      MeshVelocity(1,1:N),MeshVelocity(2,1:N),MeshVelocity(3,1:N),&
                      Viscosity, Density, Pressure, Zero, Zero,&
                      .FALSE., Stabilize, Bubbles, Element, n, ElementNodes )
              ! special coords (account for metric)
              !-----------------------------------
              ELSE
                 CALL DiffuseConvectiveGenCompose( &
                      MASS, STIFF, FORCE, LOAD, &
                      HeatCapacity, C0, C1(1:N), HeatConductivity, &
                      .FALSE., Zero, Zero, IceVeloU, IceVeloV, IceVeloW, &
                      MeshVelocity(1,1:N),MeshVelocity(2,1:N),MeshVelocity(3,1:N), Viscosity,&
                      Density, Pressure, Zero, Zero,.FALSE.,&
                      Stabilize, Element, n, ElementNodes )

              END IF              
              !------------------------------------------------------------------------------
              ! If time dependent simulation add mass matrix to stiff matrix
              !------------------------------------------------------------------------------
              TimeForce  = FORCE
              IF ( TransientSimulation ) THEN
                 IF ( Bubbles ) FORCE = 0.0d0
                 CALL Default1stOrderTime( MASS,STIFF,FORCE )
              END IF
              !------------------------------------------------------------------------------
              !  Update global matrices from local matrices
              !------------------------------------------------------------------------------
              IF (  Bubbles ) THEN
                 CALL Condensate( N, STIFF, FORCE, TimeForce )
                 IF (TransientSimulation) CALL DefaultUpdateForce( TimeForce )
              END IF

              CALL DefaultUpdateEquations( STIFF, FORCE )
           END DO     !  Bulk elements


           !------------------------------------------------------------------------------
           ! Neumann & Newton boundary conditions
           !------------------------------------------------------------------------------
           DO t=1, Solver % Mesh % NumberOfBoundaryElements

              ! get element information
              Element => GetBoundaryElement(t)
              IF (ParEnv % myPe .NE. Element % partIndex) CYCLE
              IF ( .NOT.ActiveBoundaryElement() ) CYCLE
              n = GetElementNOFNodes()
              IF ( GetElementFamily() == 1 ) CYCLE
              BC => GetBC()
              bc_id = GetBCId( Element )
              CALL GetElementNodes( ElementNodes )


              IF ( ASSOCIATED( BC ) ) THEN            
                 ! Check that we are on the correct boundary part!
                 STIFF=0.0D00
                 FORCE=0.0D00
                 MASS=0.0D00
                 LOAD=0.0D00
                 TransferCoeff = 0.0D00
                 TempExt = 0.0D00
                 FluxBC = .FALSE.
                 FluxBC =  GetLogical(BC,TRIM(Solver % Variable % Name) // ' Flux BC', Found)

                 IF (FluxBC) THEN
                    !------------------------------
                    !BC: -k@T/@n = \alpha(T - TempExt)
                    !------------------------------
                    TransferCoeff(1:N) = GetReal( BC, TRIM(Solver % Variable % Name) //  ' Transfer Coefficient',Found )
                    IF ( ANY(TransferCoeff(1:N) /= 0.0d0) ) THEN
                       TempExt(1:N) = GetReal( BC, TRIM(Solver % Variable % Name) // ' External Value',Found )   
                       DO j=1,n
                          LOAD(j) = LOAD(j) +  TransferCoeff(j) * TempExt(j)
                       END DO
                    END IF
                    !---------------
                    !BC: -k@T/@n = q
                    !---------------
                    LOAD(1:N)  = LOAD(1:N) + &
                         GetReal( BC, TRIM(Solver % Variable % Name) // ' Heat Flux', Found )
                    ! -------------------------------------
                    ! set boundary due to coordinate system
                    ! -------------------------------------
                    IF ( CurrentCoordinateSystem() == Cartesian ) THEN
                       CALL DiffuseConvectiveBoundary( STIFF,FORCE, &
                            LOAD,TransferCoeff,Element,n,ElementNodes )
                    ELSE
                       CALL DiffuseConvectiveGenBoundary(STIFF,FORCE,&
                            LOAD,TransferCoeff,Element,n,ElementNodes ) 
                    END IF
                 END IF
              END IF

              !------------------------------------------------------------------------------
              ! Update global matrices from local matrices
              !------------------------------------------------------------------------------
              IF ( TransientSimulation ) THEN
                 MASS = 0.d0
                 CALL Default1stOrderTime( MASS, STIFF, FORCE )
              END IF
          
              CALL DefaultUpdateEquations( STIFF, FORCE )
           END DO   ! Neumann & Newton BCs
           !------------------------------------------------------------------------------

           CALL DefaultFinishAssembly()
           CALL DefaultDirichletBCs()


           OldValues = SystemMatrix % Values
           OldRHS = ForceVector

           !------------------------------------------------------------------------------
           ! Dirichlet method - matrix and force-vector manipulation
           !------------------------------------------------------------------------------
           IF (ApplyDirichlet) THEN
              ! manipulation of the matrix
              !---------------------------
              DO i=1,Model % Mesh % NumberOfNodes
                 k = TempPerm(i)           
                 IF (ActiveNode(i) .AND. (k > 0)) THEN
                    CALL ZeroRow( SystemMatrix, k ) 
                    CALL SetMatrixElement( SystemMatrix, k, k, 1.0d0 )
                    SystemMatrix % RHS(k) = UpperLimit(i)
                 END IF
              END DO
           END IF


           CALL Info( SolverName, 'Assembly done', Level=4 )

           !------------------------------------------------------------------------------
           !     Solve the system and check for convergence
           !------------------------------------------------------------------------------
           at = CPUTime() - at
           st = CPUTime()

           PrevNorm = Solver % Variable % Norm

           Norm = DefaultSolve()

           st = CPUTime()-st
           totat = totat + at
           totst = totst + st
           WRITE(Message,'(a,i4,a,F8.2,F8.2)') 'iter: ',iter,' Assembly: (s)', at, totat
           CALL Info( SolverName, Message, Level=4 )
           WRITE(Message,'(a,i4,a,F8.2,F8.2)') 'iter: ',iter,' Solve:    (s)', st, totst
           CALL Info( SolverName, Message, Level=4 )


           IF ( PrevNorm + Norm /= 0.0d0 ) THEN
              RelativeChange = 2.0d0 * ABS( PrevNorm-Norm ) / (PrevNorm + Norm)
           ELSE
              RelativeChange = 0.0d0
           END IF
           WRITE( Message, * ) 'Result Norm   : ',Norm
           CALL Info( SolverName, Message, Level=4 )
           WRITE( Message, * ) 'Relative Change : ',RelativeChange
           CALL Info( SolverName, Message, Level=4 )

           SystemMatrix % Values = OldValues
           ForceVector = OldRHS

           !------------------------------------------------------------------------------
           ! compute residual
           !------------------------------------------------------------------------------ 
           IF ( ApplyDirichlet .AND. (ParEnv % PEs > 1) ) THEN !!!!!!!!!!!!!!!!!!!!!! we have a parallel run
             CALL ParallelInitSolve( SystemMatrix, Temp, ForceVector, ResidualVector )
             CALL ParallelMatrixVector( SystemMatrix, Temp, StiffVector, .TRUE. )
             ResidualVector =  StiffVector - ForceVector
             CALL ParallelSumVector( SystemMatrix, ResidualVector )
        
           ELSE IF (ParEnv % PEs == 1) THEN!!!!!!!!!!!!!!!!!!!!!! serial run 
              CALL CRS_MatrixVectorMultiply( SystemMatrix, Temp, StiffVector)
              ResidualVector =  StiffVector - ForceVector
           ELSE
              ResidualVector =  0.0_dp
           END IF

           !-----------------------------
           ! determine "active" nodes set
           !-----------------------------
           IF (ASSOCIATED(VarTempHom)) THEN
              TempHomologous => VarTempHom % Values
              DO i=1,Model % Mesh % NumberOfNodes ! <______________IS THIS OK IN PARALLEL????????



                 k = VarTempHom % Perm(i)
                 l= TempPerm(i)
                 TempHomologous(k) = Temp(l) - UpperLimit(i)
                 IF (ApplyDirichlet) THEN
                    !---------------------------------------------------------
                    ! if upper limit is exceeded, manipulate matrix in any case
                    !----------------------------------------------------------
                    IF (TempHomologous(k) >= 0.0 ) THEN
                       ActiveNode(i) = .TRUE.
                       TempHomologous(k) = LinearTol
                    END IF
                    !---------------------------------------------------
                    ! if there is "heating", don't manipulate the matrix
                    !---------------------------------------------------
                    IF (ResidualVector(l) > - LinearTol &
                         .AND. iter>1) ActiveNode(i) = .FALSE.
                 END IF
                 IF( .NOT.ActiveNode(i) ) THEN
                    PointerToResidualVector(VarTempResidual % Perm(i)) = 0.0D00
                 ELSE
                    PointerToResidualVector(VarTempResidual % Perm(i)) = ResidualVector(l)
                 END IF
              END DO
           ELSE
              WRITE(Message,'(A)') TRIM(Solver % Variable % Name) // ' Homologous not associated'
              CALL WARN( SolverName, Message)
           END IF
           !------------------------------------------
           ! special treatment for periodic boundaries
           !------------------------------------------
           k=0
           IF ( ApplyDirichlet ) THEN
              DO t=1, Solver % Mesh % NumberOfBoundaryElements

                 ! get element information
                 Element => GetBoundaryElement(t)
                 IF (ParEnv % myPe .NE. Element % partIndex) CYCLE
                 IF ( .NOT.ActiveBoundaryElement() ) CYCLE
                 n = GetElementNOFNodes()
                 IF ( GetElementFamily() == 1 ) CYCLE
                 BC => GetBC()
                 bc_id = GetBCId( Element )
                 CALL GetElementNodes( ElementNodes )


                 IF ( ASSOCIATED( BC ) ) THEN    
                    IsPeriodicBC = GetLogical(BC,'Periodic BC ' // TRIM(Solver % Variable % Name),Found)
                    IF (.NOT.Found) IsPeriodicBC = .FALSE.
                    IF (IsPeriodicBC) THEN 
                       DO i=1,N
                          IF  (ActiveNode(Element % NodeIndexes(i))) THEN
                             k = k + 1
                             ActiveNode(Element % NodeIndexes(i)) = .FALSE.
                          END IF
                       END DO
                    END IF
                 END IF
              END DO
           END IF
           !----------------------
           ! check for convergence
           !----------------------
           IF ( RelativeChange < NonlinearTol ) THEN
              EXIT
           ELSE
              IF (ApplyDirichlet) THEN
                 WRITE(Message,'(a,i10)') 'Deactivated Periodic BC nodes:', k
                 CALL INFO(SolverName,Message,Level=1)
                 WRITE(Message,'(a,i10)') 'Number of constrained points:', COUNT(ActiveNode)
                 CALL INFO(SolverName,Message,Level=1)
              END IF
           END IF
        END DO ! of the nonlinear iteration
        !------------------------------------------------------------------------------

        
        !------------------------------------------------------------------------------
        !   Compute cumulative time done by now and time remaining
        !------------------------------------------------------------------------------
        IF ( .NOT. TransientSimulation ) EXIT
        CumulativeTime = CumulativeTime + dt
        dt = Timestep - CumulativeTime
     END DO ! time interval
     !------------------------------------------------------------------------------
     DEALLOCATE( PrevSolution )

     SubroutineVisited = .TRUE.

!------------------------------------------------------------------------------
   END SUBROUTINE TemperateIceSolver
!------------------------------------------------------------------------------



