int precon = 1;
typedef struct {
  PetscInt  nx, ny, N;
  PetscReal dx, dy, dxinv, dyinv;
  double fft_factor;
  Vec *dptr, *cxxptr, *cyyptr, *caveptr, *c1ptr, *c1uptr;
  double *cxx_, *cyy_, *c1_, *c1u_;
  Mat * Fptr;
} Ctx;

double exact(double x,double y){
  return exp(x*y);
}
double c_xx(double x, double y){
  return 1;
}
double c_yy(double x, double y){
  return 1;
}
double c_1(double x, double y){
  return exp(x*y);
}
double rhs(double x, double y){
  return (x*x*c_xx(x,y)+y*y*c_yy(x,y)+c_1(x,y))*exact(x,y);
}
int Bflag(int i, int j, int nx, int ny){
  if(i==0 || i==nx-1) return 1;
  if(j==0 || j==ny-1) return 1;
  return 0;
}
PetscErrorCode multA(Mat A, Vec a, Vec b){
  PetscErrorCode ierr;
  PetscInt idx;
  Ctx *aa = NULL;
  ierr = MatShellGetContext(A,&aa);
  PetscInt nx = aa->nx; PetscInt ny = aa->ny; 
  double dxinv = aa->dxinv; double dyinv = aa->dyinv;
  const PetscReal * a_ = NULL; PetscReal * b_ = NULL;
  ierr = VecGetArrayRead(a,&a_); CHKERRQ(ierr);
  ierr = VecGetArray(b,&b_); CHKERRQ(ierr);
  PetscReal xm1, xp1, ym1, yp1;
  ierr = VecCopy(a,b);
  ierr = VecPointwiseMult(*(aa->c1uptr),a,*(aa->c1ptr)); // c1u = c1 .* u
  for(int j=0;j<ny;j++){
    for(int i=0;i<nx;i++){
      idx = j*nx + i;
      if(!Bflag(i,j,nx,ny)){
	xm1 = a_[idx-1];
	xp1 = a_[idx+1];
	ym1 = a_[idx-nx];
	yp1 = a_[idx+nx];
	b_[idx] = aa->cxx_[idx] * dxinv*dxinv*(xm1-2*a_[idx]+xp1);
	b_[idx] += aa->cyy_[idx] * dyinv*dyinv*(ym1-2*a_[idx]+yp1);
      }else{
	aa->c1u_[idx] = 0;
      }
    }
  }
  ierr = VecAXPY(b,1,*(aa->c1uptr)); // b += c1u
  ierr = VecRestoreArrayRead(a,&a_);
  ierr = VecRestoreArray(b,&b_);
  return ierr;
}

PetscErrorCode multATransp(Mat A, Vec a, Vec b){
  PetscErrorCode ierr;
  PetscInt idx;
  Ctx *aa = NULL;
  ierr = MatShellGetContext(A,&aa);
  PetscInt nx = aa->nx; PetscInt ny = aa->ny; 
  double dxinv = aa->dxinv; double dyinv = aa->dyinv;
  const PetscReal * a_ = NULL; PetscReal * b_ = NULL;
  ierr = VecGetArrayRead(a,&a_); CHKERRQ(ierr);
  ierr = VecGetArray(b,&b_); CHKERRQ(ierr);
  ierr = VecPointwiseMult(*(aa->c1uptr),a,*(aa->c1ptr)); // c1u = c1 .* u 
  PetscReal xm1, xp1, ym1, yp1;
  for(int j=0;j<ny;j++){
    for(int i=0;i<nx;i++){
      idx = j*nx + i;
      if(!Bflag(i,j,nx,ny)){
	xm1 = a_[idx-1] * aa->cxx_[idx-1];
	xp1 = a_[idx+1] * aa->cxx_[idx+1];
	ym1 = a_[idx-nx] * aa->cyy_[idx-nx];
	yp1 = a_[idx+nx] * aa->cyy_[idx+nx];
	b_[idx] = dxinv*dxinv*(xm1-2*a_[idx]*aa->cxx_[idx]+xp1);
	b_[idx] += dyinv*dyinv*(ym1-2*a_[idx]*aa->cyy_[idx]+yp1);
      }else{
	aa->c1u_[idx] = 0;
      }

    }
  }
  ierr = VecAXPY(b,1,*(aa->c1uptr)); // b += c1u
  ierr = VecRestoreArrayRead(a,&a_);
  ierr = VecRestoreArray(b,&b_);
  return ierr;
}

PetscErrorCode multF(Mat F, Vec a, Vec b){
  PetscErrorCode ierr;
  Ctx *aa = NULL; ierr = MatShellGetContext(F,&aa);
  double * b_; ierr = VecGetArray(b,&b_);
  fftw_plan p;
  p = fftw_plan_r2r_2d(aa->ny,aa->nx,b_,b_,FFTW_RODFT00,FFTW_RODFT00,FFTW_ESTIMATE);

  ierr = VecCopy(a,b);
  ierr = VecPointwiseMult(b,b,*(aa->caveptr));
  fftw_execute(p);
  ierr = VecPointwiseMult(b,b,*(aa->dptr));
  fftw_execute(p);

  fftw_destroy_plan(p);
  int idx = 0;
  for(int j=0;j<aa->ny;j++){
    for(int i=0;i<aa->nx;i++){
      idx = i +j*aa->nx;
	if(Bflag(i,j,aa->nx,aa->ny)) b_[idx] = 0;
    }
  }
  ierr = VecRestoreArray(b,&b_);
  ierr = VecScale(b, aa->fft_factor);

  return ierr;
}

PetscErrorCode multFTransp(Mat F, Vec a, Vec b){
  PetscErrorCode ierr;
  Ctx *aa = NULL; ierr = MatShellGetContext(F,&aa);
  double * b_; ierr = VecGetArray(b,&b_);
  fftw_plan p;
  p = fftw_plan_r2r_2d(aa->ny,aa->nx,b_,b_,FFTW_RODFT00,FFTW_RODFT00,FFTW_ESTIMATE);

  ierr = VecCopy(a,b);
  fftw_execute(p);
  ierr = VecPointwiseMult(b,b,*(aa->dptr));
  fftw_execute(p);
  ierr = VecPointwiseMult(b,b,*(aa->caveptr));

  fftw_destroy_plan(p);
  int idx = 0;
  for(int j=0;j<aa->ny;j++){
    for(int i=0;i<aa->nx;i++){
      idx = i +j*aa->nx;
	if(Bflag(i,j,aa->nx,aa->ny)) b_[idx] = 0;
    }
  }
  ierr = VecRestoreArray(b,&b_);
  ierr = VecScale(b, aa->fft_factor);
  return ierr;
}

PetscErrorCode precondition(PC pc, Vec a, Vec b){
  PetscErrorCode ierr;
  void *vptr = NULL; PCShellGetContext(pc,&vptr);
  Ctx * aa = vptr;
  Mat *Fptr = aa->Fptr;
  ierr = MatMult(*Fptr,a,b);

  return 0;
}
