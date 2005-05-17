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
! *
! * Module containing a band matrix solver for linear system given in
! * CRS format.
! *
! ******************************************************************************
! *
! *                     Author:       Juha Ruokolainen
! *
! *                    Address: Center for Scientific Computing
! *                                Tietotie 6, P.O. BOX 405
! *                                  02101 Espoo, Finland
! *                                  Tel. +358 0 457 2723
! *                                Telefax: +358 0 457 2302
! *                              EMail: Juha.Ruokolainen@csc.fi
! *
! *                       Date: 08 Jun 1997
! *
! *                Modified by:
! *
! *       Date of modification:
! *
! *****************************************************************************/

MODULE DirectSolve

   USE CRSMatrix
   USE BandMatrix

   USE Lists

   IMPLICIT NONE

CONTAINS


!------------------------------------------------------------------------------
   SUBROUTINE ComplexBandSolver( A,x,b )
DLLEXPORT BandSolver
!------------------------------------------------------------------------------

     REAL(KIND=dp) :: x(:),b(:)
     TYPE(Matrix_t), POINTER :: A
!------------------------------------------------------------------------------

   
     INTEGER :: i,j,k,istat,Subband,N
     COMPLEX(KIND=dp), ALLOCATABLE :: BA(:,:)

     SAVE BA
!------------------------------------------------------------------------------

     n = A % NumberOfRows
     x(1:n) = b(1:n)
     n = n / 2

     IF ( A % Format == MATRIX_CRS .AND. .NOT. A % Symmetric ) THEN
       Subband = 0
       DO i=1,N
         DO j=A % Rows(2*i-1),A % Rows(2*i)-1,2
           Subband = MAX(Subband,ABS((A % Cols(j)+1)/2-i))
         END DO
       END DO

       IF ( .NOT.ALLOCATED( BA ) ) THEN

         ALLOCATE( BA(3*SubBand+1,N),stat=istat )

         IF ( istat /= 0 ) THEN
           CALL Fatal( 'ComplexBandSolver', 'Memory allocation error.' )
         END IF

       ELSE IF ( SIZE(BA,1) /= 3*Subband+1 .OR. SIZE(BA,2) /= N ) THEN

         DEALLOCATE( BA )
         ALLOCATE( BA(3*SubBand+1,N),stat=istat )

         IF ( istat /= 0 ) THEN
           CALL Fatal( 'ComplexBandSolver', 'Memory allocation error.' )
         END IF

       END IF

       BA = 0.0D0
       DO i=1,N
         DO j=A % Rows(2*i-1),A % Rows(2*i)-1,2
           k = i - (A % Cols(j)+1)/2 + 2*Subband + 1
           BA(k,(A % Cols(j)+1)/2) = DCMPLX( A % Values(j), -A % Values(j+1) )
         END DO
       END DO

       CALL SolveComplexBandLapack( N,1,BA,x,Subband,3*Subband+1 )

     ELSE IF ( A % Format == MATRIX_CRS ) THEN

       Subband = 0
       DO i=1,N
         DO j=A % Rows(2*i-1),A % Diag(2*i-1)
           Subband = MAX(Subband,ABS((A % Cols(j)+1)/2-i))
         END DO
       END DO

       IF ( .NOT.ALLOCATED( BA ) ) THEN

         ALLOCATE( BA(SubBand+1,N),stat=istat )

         IF ( istat /= 0 ) THEN
           CALL Fatal( 'ComplexBandSolver', 'Memory allocation error.' )
         END IF

       ELSE IF ( SIZE(BA,1) /= Subband+1 .OR. SIZE(BA,2) /= N ) THEN

         DEALLOCATE( BA )
         ALLOCATE( BA(SubBand+1,N),stat=istat )

         IF ( istat /= 0 ) THEN
           CALL Fatal( 'ComplexBandSolver', 'Direct solver memory allocation error.' )
         END IF

       END IF

       BA = 0.0D0
       DO i=1,N
         DO j=A % Rows(2*i-1),A % Diag(2*i-1)
           k = i - (A % Cols(j)+1)/2 + 1
           BA(k,(A % Cols(j)+1)/2) = DCMPLX( A % Values(j), -A % Values(j+1) )
         END DO
       END DO

       CALL SolveComplexSBandLapack( N,1,BA,x,Subband,Subband+1 )

     END IF
!------------------------------------------------------------------------------
  END SUBROUTINE ComplexBandSolver 
!------------------------------------------------------------------------------


!------------------------------------------------------------------------------
   SUBROUTINE BandSolver( A,x,b )
DLLEXPORT BandSolver
!------------------------------------------------------------------------------

     REAL(KIND=dp) :: x(:),b(:)
     TYPE(Matrix_t), POINTER :: A
!------------------------------------------------------------------------------

     INTEGER :: i,j,k,istat,Subband,N
     REAL(KIND=dp), ALLOCATABLE :: BA(:,:)

     SAVE BA
!------------------------------------------------------------------------------
     N = A % NumberOfRows

     x(1:n) = b(1:n)

     IF ( A % Format == MATRIX_CRS ) THEN ! .AND. .NOT. A % Symmetric ) THEN
        Subband = 0
        DO i=1,N
          DO j=A % Rows(i),A % Rows(i+1)-1
            Subband = MAX(Subband,ABS(A % Cols(j)-i))
          END DO
        END DO

        IF ( .NOT.ALLOCATED( BA ) ) THEN

          ALLOCATE( BA(3*SubBand+1,N),stat=istat )

          IF ( istat /= 0 ) THEN
            CALL Fatal( 'BandSolver', 'Memory allocation error.' )
          END IF

        ELSE IF ( SIZE(BA,1) /= 3*Subband+1 .OR. SIZE(BA,2) /= N ) THEN

          DEALLOCATE( BA )
          ALLOCATE( BA(3*SubBand+1,N),stat=istat )

          IF ( istat /= 0 ) THEN
            CALL Fatal( 'BandSolver', 'Memory allocation error.' )
          END IF

       END IF

       BA = 0.0D0
       DO i=1,N
         DO j=A % Rows(i),A % Rows(i+1)-1
           k = i - A % Cols(j) + 2*Subband + 1
           BA(k,A % Cols(j)) = A % Values(j)
         END DO
       END DO

       CALL SolveBandLapack( N,1,BA,x,Subband,3*Subband+1 )

     ELSE IF ( A % Format == MATRIX_CRS ) THEN

       Subband = 0
       DO i=1,N
         DO j=A % Rows(i),A % Diag(i)
           Subband = MAX(Subband,ABS(A % Cols(j)-i))
         END DO
       END DO

       IF ( .NOT.ALLOCATED( BA ) ) THEN

         ALLOCATE( BA(SubBand+1,N),stat=istat )

         IF ( istat /= 0 ) THEN
           CALL Fatal( 'BandSolver', 'Memory allocation error.' )
         END IF

       ELSE IF ( SIZE(BA,1) /= Subband+1 .OR. SIZE(BA,2) /= N ) THEN

         DEALLOCATE( BA )
         ALLOCATE( BA(SubBand+1,N),stat=istat )

         IF ( istat /= 0 ) THEN
           CALL Fatal( 'BandSolver', 'Memory allocation error.' )
         END IF

       END IF

       BA = 0.0D0
       DO i=1,N
         DO j=A % Rows(i),A % Diag(i)
           k = i - A % Cols(j) + 1
           BA(k,A % Cols(j)) = A % Values(j)
         END DO
       END DO

       CALL SolveSBandLapack( N,1,BA,x,Subband,Subband+1 )

     ELSE IF ( A % Format == MATRIX_BAND ) THEN

       CALL SolveBandLapack( N,1,A % Values(1:N*(3*A % Subband+1)), &
                     x,A % Subband,3*A % Subband+1 )

     ELSE IF ( A % Format == MATRIX_SBAND ) THEN

       CALL SolveSBandLapack( N,1,A % Values,x,A % Subband,A % Subband+1 )

     END IF

!------------------------------------------------------------------------------
  END SUBROUTINE BandSolver 
!------------------------------------------------------------------------------

!------------------------------------------------------------------------------
  SUBROUTINE UMFPack_SolveSystem( Solver,A,x,b )
!------------------------------------------------------------------------------
#ifdef HAVE_UMFPACK
  INTERFACE
    SUBROUTINE umf4def( control )
       USE Types
       REAL(KIND=dp) :: control(*) 
    END SUBROUTINE umf4def

    SUBROUTINE umf4sym( m,n,rows,cols,values,symbolic,control,iinfo )
       USE Types
       INTEGER :: m,n,rows(*),cols(*)
       INTEGER(KIND=AddrInt) ::  symbolic
       REAL(KIND=dp) :: Values(*), control(*),iinfo(*)
    END SUBROUTINE umf4sym

    SUBROUTINE umf4num( rows,cols,values,symbolic,numeric, control,iinfo )
       USE Types
       INTEGER :: rows(*),cols(*)
       INTEGER(KIND=AddrInt) ::  numeric, symbolic
       REAL(KIND=dp) :: Values(*), control(*),iinfo(*)
    END SUBROUTINE umf4num

    SUBROUTINE umf4sol( sys, x, b, numeric, control, iinfo )
       USE Types
       INTEGER :: sys
       INTEGER(KIND=AddrInt) :: numeric
       REAL(KIND=dp) :: x(*), b(*), control(*), iinfo(*)
    END SUBROUTINE umf4sol
  END INTERFACE
#endif


  TYPE(Matrix_t), POINTER :: A
  TYPE(Solver_t) :: Solver
  INTEGER :: n
  REAL(KIND=dp) :: x(*), b(*)

#ifdef HAVE_UMPFACK
  INTEGER :: status, sys
  INTEGER(KIND=AddrInt) :: numeric, symbolic
  REAL(KIND=dp) :: Control(20), iInfo(90)

  SAVE Control, iInfo, Symbolic, Numeric
 
  LOGICAL :: Factorize, stat

  Factorize = ListGetLogical( Solver % Values, 'UMF Factorize', stat )
  IF ( .NOT. stat ) Factorize = .TRUE.

  IF ( Factorize ) THEN
    IF ( Numeric  /= 0 ) THEN
       CALL umf4fnum( Numeric )
       Numeric = 0
    END IF
    IF ( Symbolic /= 0 ) THEN
       CALL umf4fsym( Symbolic )
       Symbolic = 0
    END IF
  END IF

  n = a % numberofrows

  A % Rows = A % Rows-1
  A % Cols = A % Cols-1

  IF ( Factorize ) THEN
    CALL umf4def( Control )
    CALL umf4sym( n,n, A % Rows, A % Cols, A % Values, Symbolic,Control, iinfo )
    IF (iinfo(1) .lt. 0) THEN
        print *, 'Error occurred in umf4sym: ', iinfo(1)
        stop
    END IF
    CALL umf4num( A % Rows, A % Cols, A % Values, Symbolic, Numeric, Control, iinfo )
    IF (iinfo(1) .lt. 0) THEN
        print *, 'Error occurred in umf4num: ', iinfo(1)
        stop
    ENDIF
  END IF
  sys = 2
  CALL umf4sol( sys, x, b, Numeric, Control, iinfo )
  IF (iinfo(1) .lt. 0) THEN
      print *, 'Error occurred in umf4sol: ', iinfo(1)
      stop
   END IF

   A % Rows = A % Rows+1
   A % Cols = A % Cols+1
#else
   CALL Fatal( 'UMFPack_SolveSystem', 'UMFPACK Solver has not been installed.' )
#endif
!------------------------------------------------------------------------------
  END SUBROUTINE UMFPack_SolveSystem
!------------------------------------------------------------------------------


!------------------------------------------------------------------------------
  SUBROUTINE DirectSolver( A,x,b,Solver )
DLLEXPORT DirectSolver
!------------------------------------------------------------------------------

    TYPE(Solver_t) :: Solver

    REAL(KIND=dp), DIMENSION(:) :: x,b
    TYPE(Matrix_t), POINTER :: A
!------------------------------------------------------------------------------

    LOGICAL :: GotIt
    CHARACTER(LEN=MAX_NAME_LEN) :: Method

!------------------------------------------------------------------------------

#if 0
    SELECT CASE( A % Format )

    CASE( MATRIX_BAND, MATRIX_SBAND, MATRIX_CRS )
      CALL BandSolver( A, x, b )

    CASE DEFAULT
       CALL Fatal( 'DirectSolver', 'Unknown matrix format for directsolver.' )

    END SELECT
#else
    Method = ListGetString( Solver % Values, 'Linear System Direct Method',GotIt )
    IF ( .NOT. GotIt ) Method = 'banded'

    SELECT CASE( Method )
      CASE( 'banded', 'symmetric banded' )
        IF ( .NOT. A % Complex ) THEN
           CALL BandSolver( A, x, b )
        ELSE
           CALL ComplexBandSolver( A, x, b )
        END IF

      CASE( 'umfpack' )
        CALL Umfpack_SolveSystem( Solver, A, x, b )

      CASE DEFAULT
        CALL Fatal( 'DirectSolver', 'Unknown direct solver method.' )
    END SELECT
#endif

!------------------------------------------------------------------------------
  END SUBROUTINE DirectSolver
!------------------------------------------------------------------------------

END MODULE DirectSolve
