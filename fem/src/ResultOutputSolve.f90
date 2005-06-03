SUBROUTINE ResultOutputSolver( Model,Solver,dt,TransientSimulation )
!DEC$ATTRIBUTES DLLEXPORT :: PoissonSolver
!------------------------------------------------------------------------------
!******************************************************************************
!
!  Exports data to other FE-software.
!
!  ARGUMENTS:
!
!  TYPE(Model_t) :: Model,  
!     INPUT: All model information (mesh, materials, BCs, etc...)
!
!  TYPE(Solver_t) :: Solver
!     INPUT: Linear & nonlinear equation solver options
!
!  REAL(KIND=dp) :: dt,
!     INPUT: Timestep size for time dependent simulations
!
!  LOGICAL :: TransientSimulation
!     INPUT: Steady state or transient simulation
!
!******************************************************************************
  USE DefUtils

  IMPLICIT NONE
!------------------------------------------------------------------------------
  TYPE(Solver_t) :: Solver
  TYPE(Model_t) :: Model

  REAL(KIND=dp) :: dt
  LOGICAL :: TransientSimulation
!------------------------------------------------------------------------------
! Local variables
!------------------------------------------------------------------------------
  TYPE(Element_t),POINTER :: Element
  TYPE(Variable_t), POINTER :: Solution
  INTEGER, POINTER :: Perm(:)
  REAL(KIND=dp), POINTER :: Values(:)
  COMPLEX(KIND=dp), POINTER :: CValues(:)
  TYPE(Variable_t), POINTER :: TimeVariable
  TYPE(ValueList_t), POINTER :: SolverParams

  LOGICAL :: AllocationsDone = .FALSE., Found, CoordinatesWritten = .FALSE.
  LOGICAL :: FirstTimeStep = .TRUE., EigenAnalysis = .FALSE.

  INTEGER :: i,j,k,m,n,dim, Code, body_id, ElementCounter, Nloop, Loop
  INTEGER :: ScalarFields, VectorFields, TensorFields, tensorComponents
  INTEGER, PARAMETER :: MaxElemCode = 1000
  INTEGER :: ListElemTypes(MaxElemCode)
  REAL(KIND=dp) :: Norm


  CHARACTER(LEN=1024) :: OutputFile, ResFile, MshFile, Txt, Family, &
       ScalarFieldName, VectorFieldName, TensorFieldName, CompName
  CHARACTER(LEN=1024) :: Txt2, Txt3

  INTEGER :: PyramidMap(14,4)

  SAVE AllocationsDone, FirstTimeStep
!------------------------------------------------------------------------------
  PyramidMap(1,:)  = (/ 7, 8, 3, 12 /)
  PyramidMap(2,:)  = (/ 10, 11, 12, 5 /)
  PyramidMap(3,:)  = (/ 10, 13, 12, 5 /)
  PyramidMap(4,:)  = (/ 9, 10, 13, 11 /)
  PyramidMap(5,:)  = (/ 9, 13, 11, 12 /)
  PyramidMap(6,:)  = (/ 9, 10, 11, 1 /)
  PyramidMap(7,:)  = (/ 9, 6, 11, 1 /)
  PyramidMap(8,:)  = (/ 9, 6, 11, 12 /)
  PyramidMap(9,:)  = (/ 9, 8, 12, 4 /)
  PyramidMap(10,:) = (/ 9, 13, 12, 4 /)
  PyramidMap(11,:) = (/ 7, 9, 8, 12 /)
  PyramidMap(12,:) = (/ 7, 9, 6, 12 /)
  PyramidMap(13,:) = (/ 7, 6, 11, 12 /)
  PyramidMap(14,:) = (/ 7, 6, 11, 2 /)

  SolverParams => GetSolverParams()
  EigenAnalysis = GetLogical( SolverParams, 'Eigen Analysis', Found )

  OutputFile = GetString( Solver % Values, 'Output File Name', Found )
  IF( .NOT.Found ) THEN
     PRINT *,'Output File Name undefined'
  ELSE
     PRINT *,'Output File Name = ',TRIM(OutputFile)
     WRITE(ResFile,'(A,A)') TRIM(OutputFile),'.flavia.res'
     WRITE(MshFile,'(A,A)') TRIM(OutputFile),'.flavia.msh'
     PRINT *,'res-file = ',TRIM(ResFile)
     PRINT *,'msh-file = ',TRIM(MshFile)
  END IF

  ! Write the GiD msh-file:
  !------------------------
  dim = CoordinateSystemDimension()
  IF( CoordinatesWritten ) GOTO 10

  OPEN( UNIT=10, FILE=MshFile )

  ! First check how many element types are involved in the analysis:
  !-----------------------------------------------------------------
  ListElemTypes = 0
  ElementCounter = 0
!  DO i = 1, Solver % NumberOfActiveElements
!     Element => GetActiveElement(i)
  DO i = 1, Model % NumberOfBulkElements !+ Model % NumberOfBoundaryElements
     Element => Model % Mesh % Elements(i)

     Code = Element % Type % ElementCode
     ListElemTypes(Code) = ListElemTypes(Code)+1
  END DO
  PRINT *,'Total number of elements =',SUM(ListElemTypes)

  ! Write the different element types in different blocks:
  !-------------------------------------------------------
  DO i = 1,MaxElemCode
     IF(ListElemTypes(i) == 0) CYCLE
     PRINT *,ListElemTypes(i),'elements of type',i
     n = MOD(i,100)
     IF( INT(i/100) == 1 ) Family = 'Point'
     IF( INT(i/100) == 2 ) Family = 'Linear'
     IF( INT(i/100) == 3 ) Family = 'Triangle'
     IF( INT(i/100) == 4 ) Family = 'Quadrilateral'
     IF( INT(i/100) == 5 ) Family = 'Tetrahedra'
     IF( INT(i/100) == 6 ) THEN
        Family = 'Tetrahedra' ! PYRAMIDS WILL BE SPLITTED
        n = 4                 ! INTO LINEAR TETRAHEDRA
     END IF
     IF( INT(i/100) == 8 ) Family = 'Hexahedra'

     IF( n < 10 ) THEN
        WRITE(Txt,'(A,I1,A,A,A,I2)') 'MESH "Elmer Mesh" dimension ',&
             dim,' ElemType ', TRIM(Family),' Nnode', n
     ELSE
        WRITE(Txt,'(A,I1,A,A,A,I3)') 'MESH "Elmer Mesh" dimension ',&
             dim,' ElemType ', TRIM(Family),' Nnode', n
     END IF

     WRITE(10,'(A)') TRIM(Txt)

     ! Write all node coordinates in the first block:
     !-----------------------------------------------
     IF( .NOT.CoordinatesWritten ) THEN
        WRITE(10,'(A)') 'Coordinates'
        DO j = 1, Model % Mesh % NumberOfNodes
           write(10,'(I6,3E16.6)') j, &
                Model % Mesh % Nodes % x(j), &
                Model % Mesh % Nodes % y(j), &
                Model % Mesh % Nodes % z(j)
        END DO
        WRITE(10,'(A)') 'end coordinates'
        WRITE(10,'(A)') ' '
        CoordinatesWritten = .TRUE.
     END IF

     ! Write the element connectivity tables:
     !---------------------------------------
     WRITE(10,'(A)') 'Elements'
!     DO j = 1, Solver % NumberOfActiveElements
!        Element => GetActiveElement( j )
     DO j = 1, Model % NumberOfBulkElements !+ Model % NumberOfBoundaryElements
        Element => Model % Mesh % Elements(j)

        Code = Element % Type % ElementCode
        IF( Code /= i ) CYCLE
        body_id = Element % BodyId
        IF( Code == 613 ) THEN
           ! 13 noded pyramids will be splitted into 14 linear tetraheda
           DO m = 1,14
              ElementCounter = ElementCounter + 1
              WRITE(10,'(100I10)') ElementCounter, &
                   Element % NodeIndexes(PyramidMap(m,:)), body_id
           END DO
        ELSE
           ! Standard elements for GiD
           ElementCounter = ElementCounter + 1 
           WRITE(10,'(100I10)') ElementCounter, Element % NodeIndexes, body_id
        END IF

     END DO
     WRITE(10,'(A)') 'end elements'
     WRITE(10,'(A)') ' '
  END DO

10 CONTINUE
  
  ! Write the GiD res-file:
  !------------------------
  IF( TransientSimulation .AND. FirstTimeStep ) THEN
     OPEN(UNIT=10, FILE=ResFile )
     WRITE(10,'(A)') 'GiD Post Result File 1.0'
     FirstTimeStep = .FALSE.
  ELSEIF( TransientSimulation .AND. (.NOT.FirstTimeStep ) ) THEN
     OPEN(UNIT=10, FILE=ResFile, POSITION='APPEND' )
  ELSE
     OPEN(UNIT=10, FILE=ResFile )
     WRITE(10,'(A)') 'GiD Post Result File 1.0'
  END IF

  Nloop = 1
  IF( EigenAnalysis ) THEN
     Nloop = GetInteger( Solver % Values, 'Eigen System Values', Found )
     IF( .NOT.Found ) Nloop = 1
  END IF
  DO Loop = 1, Nloop
     PRINT *,'------------'

  ! First scalar fields:
  !----------------------
  ScalarFields = ListGetInteger( Solver % Values, 'Scalar Fields', Found )
  PRINT *,'Number of scalar fields =',ScalarFields
 
  DO i = 1, ScalarFields
     IF( i<10 ) THEN
        WRITE(Txt,'(A,I2)') 'Scalar Field',i
     ELSEIF( i<100 ) THEN
        WRITE(Txt,'(A,I3)') 'Scalar Field',i
     ELSEIF( i<1000 ) THEN
        WRITE(Txt,'(A,I4)') 'Scalar Field',i
     END IF

     ScalarFieldName = GetString( Solver % Values, TRIM(Txt), Found )
     Solution => VariableGet( Solver % Mesh % Variables, ScalarFieldName )
     IF( .NOT.ASSOCIATED( Solution ) ) THEN
        PRINT *,'Scalar field "',TRIM(ScalarFieldName),'" not found'
     ELSE
        PRINT *,'Scarar field',i,'= "',TRIM(ScalarFieldName),'"'
        Perm => Solution % Perm

        IF( .NOT.EigenAnalysis ) THEN
           Values => Solution % Values
        ELSE
           Cvalues => Solution % EigenVectors(Loop,:)
        END IF

        IF( TransientSimulation ) THEN
           TimeVariable => VariableGet( Solver % Mesh % Variables, 'Time' )
           PRINT *,'Current time=',TimeVariable % Values(1)
           WRITE(10,'(A,A,A,E16.6,A)') 'Result "',&
                TRIM(ScalarFieldName),'" "Transient analysis" ', &
                TimeVariable % Values(1) ,' Scalar OnNodes'
        ELSE
           IF( .NOT.EigenAnalysis ) THEN
              WRITE(10,'(A,A,A,I2,A)') 'Result "',&
                   TRIM(ScalarFieldName),'" "Steady analysis"',Loop,' Scalar OnNodes'
           ELSE
              WRITE(10,'(A,A,A,I2,A)') 'Result "',&
                   TRIM(ScalarFieldName),'" "Eigen analysis"',Loop,' Scalar OnNodes'
           END IF
        END IF

        WRITE(10,'(A,A,A)') 'ComponentNames "',TRIM(ScalarFieldName),'"'
        WRITE(10,'(A)') 'Values'
        DO j = 1, Model % Mesh % NumberOfNodes
           k = Perm(j)
           IF( .NOT.EigenAnalysis ) THEN
              WRITE(10,'(I6,E16.6)') j, Values(k)
           ELSE
              WRITE(10,'(I6,E16.6)') j, REAL(CValues(k))
           END IF
        END DO
        WRITE(10,'(A)') 'end values'
        WRITE(10,'(A)') ' '
     END IF
  END DO

  ! Then vector fields:
  !--------------------
  VectorFields = ListGetInteger( Solver % Values, 'Vector Fields', Found )
  PRINT *,'Number of vector fields =',VectorFields

  DO i = 1, VectorFields
     IF( i<10 ) THEN
        WRITE(Txt,'(A,I2)') 'Vector Field',i
     ELSEIF( i<100 ) THEN
        WRITE(Txt,'(A,I3)') 'Vector Field',i
     ELSEIF( i<1000 ) THEN
        WRITE(Txt,'(A,I4)') 'Vector Field',i
     END IF

     VectorFieldName = GetString( Solver % Values, TRIM(Txt), Found )
     PRINT *,'Vector field',i,'= "',TRIM(VectorFieldName),'"'

     IF( TransientSimulation ) THEN
        TimeVariable => VariableGet( Solver % Mesh % Variables, 'Time' )
        print *,'Current time=',TimeVariable % Values(1)
        WRITE(10,'(A,A,A,E16.6,A)') 'Result "',&
             TRIM(VectorFieldName),'" "Transient analysis" ', &
             TimeVariable % Values(1) ,' Vector OnNodes'
     ELSE
        IF( .NOT.EigenAnalysis ) THEN
           WRITE(10,'(A,A,A,I2,A)') 'Result "',&
                TRIM(VectorFieldName),'" "Steady analysis"',Loop,' Vector OnNodes'
        ELSE
           WRITE(10,'(A,A,A,I2,A)') 'Result "',&
                TRIM(VectorFieldName),'" "Eigen analysis"',Loop,' Vector OnNodes'
        END IF
     END IF

!     WRITE(10,'(A,A,A)') 'Result "',&
!          TRIM(VectorFieldName),'" "Steady analysis" 0.0e-8 Vector OnNodes'

     WRITE(Txt,'(A)') 'ComponentNames '
     DO j = 1, dim
        IF(j<Dim) THEN
           WRITE(Txt,'(A,A,A,I2,A)' ) &
                TRIM(Txt), ' "', TRIM(VectorFieldName),j,'",'
        ELSE
           WRITE(Txt,'(A,A,A,I2,A)' ) &
                TRIM(Txt), ' "', TRIM(VectorFieldName),j,'"'
        END IF
     END DO
     WRITE(10,'(A)') TRIM(Txt)
     WRITE(10,'(A)') 'Values'

     DO j = 1, Model % Mesh % NumberOfNodes
        WRITE(Txt2,'(I10)') j
        DO k = 1,dim

           ! Check if vector field components have been defined explicitely:
           !----------------------------------------------------------------
           WRITE(Txt3,'(A,I1,A,I1)') 'Vector Field ',i,' component ',k
           CompName = GetString( Solver % Values, TRIM(Txt3), Found )
           IF( Found ) THEN
              WRITE(Txt,'(A)') TRIM(CompName)
           ELSE
              WRITE(Txt,'(A,A,I1)') TRIM(VectorFieldName), ' ', k
           END IF

           IF( j==1 ) PRINT *, TRIM(Txt3),' = "', TRIM(Txt),'"'

!           WRITE(Txt,'(A,A,I1)') TRIM(VectorFieldName), ' ', k

           Solution => VariableGet( Solver % Mesh % Variables, TRIM(Txt) )
           IF( .NOT.ASSOCIATED( Solution ) ) THEN
              PRINT *,'Vector field component',k,' not found'
           ELSE
              Perm => Solution % Perm
              
              IF( .NOT.EigenAnalysis ) THEN
                 Values => Solution % Values
                 WRITE(Txt2,'(A,E16.6)') TRIM(Txt2), Values( Perm(j) )
              ELSE
                 CValues => Solution % Eigenvectors(Loop,:)
                 WRITE(Txt2,'(A,E16.6)') TRIM(Txt2), REAL(CValues( Perm(j) ) )

              END IF


           END IF
        END DO
        WRITE(10,'(A)') TRIM(Txt2)

     END DO
     WRITE(10,'(A)') 'end values'
  END DO


  ! Finally tensor fields:
  !-----------------------
  TensorFields = ListGetInteger( Solver % Values, 'Tensor Fields', Found )
  PRINT *,'Number of tensor fields =',TensorFields

  DO i = 1, TensorFields
     IF( i<10 ) THEN
        WRITE(Txt,'(A,I2)') 'Tensor Field',i
     ELSEIF( i<100 ) THEN
        WRITE(Txt,'(A,I3)') 'Tensor Field',i
     ELSEIF( i<1000 ) THEN
        WRITE(Txt,'(A,I4)') 'Tensor Field',i
     END IF

     TensorFieldName = GetString( Solver % Values, TRIM(Txt), Found )
     PRINT *,'Tensor field',i,'= "',TRIM(TensorFieldName),'"'

     IF( TransientSimulation ) THEN
        TimeVariable => VariableGet( Solver % Mesh % Variables, 'Time' )
        print *,'Current time=',TimeVariable % Values(1)
        WRITE(10,'(A,A,A,E16.6,A)') 'Result "',&
             TRIM(TensorFieldName),'" "Transient analysis" ', &
             TimeVariable % Values(1) ,' Matrix OnNodes'
     ELSE
        IF( .NOT.EigenAnalysis ) THEN
           WRITE(10,'(A,A,A,I2,A)') 'Result "',&
                TRIM(TensorFieldName),'" "Steady analysis"',Loop,' Matrix OnNodes'
        ELSE
           WRITE(10,'(A,A,A,I2,A)') 'Result "',&
                TRIM(TensorFieldName),'" "Eigen analysis"',Loop,' Matrix OnNodes'
        END IF

     END IF

!     WRITE(10,'(A,A,A)') 'Result "',&
!          TRIM(TensorFieldName),'" "Steady analysis" 0.0e-8 Matrix OnNodes'

     WRITE(Txt,'(A)') 'ComponentNames '
     IF( dim == 2 ) THEN
        TensorComponents = 3
     ELSE
        TensorComponents = 6
     END IF

     DO j = 1, TensorComponents
        IF(j<Dim) THEN
           WRITE(Txt,'(A,A,A,I2,A)' ) &
                TRIM(Txt), ' "', TRIM(TensorFieldName),j,'",'
        ELSE
           WRITE(Txt,'(A,A,A,I2,A)' ) &
                TRIM(Txt), ' "', TRIM(TensorFieldName),j,'"'
        END IF
     END DO
     WRITE(10,'(A)') TRIM(Txt)
     WRITE(10,'(A)') 'Values'

     DO j = 1, Model % Mesh % NumberOfNodes
        WRITE(Txt2,'(I10)') j
        DO k = 1,TensorComponents

           ! Check if tensor field components have been defined explicitely:
           !----------------------------------------------------------------
           WRITE(Txt3,'(A,I1,A,I1)') 'Tensor Field ',i,' component ',k
           CompName = GetString( Solver % Values, TRIM(Txt3), Found )
           IF( Found ) THEN
              WRITE(Txt,'(A)') TRIM(CompName)
           ELSE
              WRITE(Txt,'(A,A,I1)') TRIM(TensorFieldName), ' ', k
           END IF

           IF( j==1 ) PRINT *, TRIM(Txt3),' = "', TRIM(Txt),'"'

!           WRITE(Txt,'(A,A,I1)') TRIM(TensorFieldName), ' ', k

           Solution => VariableGet( Solver % Mesh % Variables, TRIM(Txt) )
           IF( .NOT.ASSOCIATED( Solution ) ) THEN
              PRINT *,'Tensor field component',k,' not found'
           ELSE
              Perm => Solution % Perm
              !Values => Solution % Values
              !WRITE(Txt2,'(A,E16.6)') TRIM(Txt2), Values( Perm(j) )

              IF( .NOT.EigenAnalysis ) THEN
                 Values => Solution % Values
                 WRITE(Txt2,'(A,E16.6)') TRIM(Txt2), Values( Perm(j) )
              ELSE
                 CValues => Solution % Eigenvectors(Loop,:)
                 WRITE(Txt2,'(A,E16.6)') TRIM(Txt2), REAL(CValues( Perm(j) ) )
              END IF



           END IF
        END DO
        WRITE(10,'(A)') TRIM(Txt2)

     END DO
     WRITE(10,'(A)') 'end values'
  END DO

END DO ! Nloop


  CLOSE(10)
  
  PRINT *,'Output complete.'

!------------------------------------------------------------------------------
END SUBROUTINE ResultOutputSolver
!------------------------------------------------------------------------------
  
