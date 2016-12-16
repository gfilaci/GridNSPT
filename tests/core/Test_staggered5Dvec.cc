    /*************************************************************************************

    Grid physics library, www.github.com/paboyle/Grid 

    Source file: ./benchmarks/Benchmark_wilson.cc

    Copyright (C) 2015

Author: Peter Boyle <paboyle@ph.ed.ac.uk>
Author: paboyle <paboyle@ph.ed.ac.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

    See the full license in the file "LICENSE" in the top level distribution directory
    *************************************************************************************/
    /*  END LEGAL */
#include <Grid/Grid.h>

using namespace std;
using namespace Grid;
using namespace Grid::QCD;

int main (int argc, char ** argv)
{
  Grid_init(&argc,&argv);

  std::vector<int> latt_size   = GridDefaultLatt();
  std::vector<int> simd_layout = GridDefaultSimd(Nd,vComplex::Nsimd());
  std::vector<int> mpi_layout  = GridDefaultMpi();

  const int Ls=16;
  GridCartesian         * UGrid   = SpaceTimeGrid::makeFourDimGrid(GridDefaultLatt(), GridDefaultSimd(Nd,vComplex::Nsimd()),GridDefaultMpi());
  GridRedBlackCartesian * UrbGrid = SpaceTimeGrid::makeFourDimRedBlackGrid(UGrid);
  GridCartesian         * FGrid   = SpaceTimeGrid::makeFiveDimGrid(Ls,UGrid);
  GridRedBlackCartesian * FrbGrid = SpaceTimeGrid::makeFiveDimRedBlackGrid(Ls,UGrid);

  std::cout << GridLogMessage << "Making s innermost grids"<<std::endl;
  GridCartesian         * sUGrid   = SpaceTimeGrid::makeFourDimDWFGrid(GridDefaultLatt(),GridDefaultMpi());
  GridRedBlackCartesian * sUrbGrid = SpaceTimeGrid::makeFourDimRedBlackGrid(sUGrid);
  GridCartesian         * sFGrid   = SpaceTimeGrid::makeFiveDimDWFGrid(Ls,UGrid);
  GridRedBlackCartesian * sFrbGrid = SpaceTimeGrid::makeFiveDimDWFRedBlackGrid(Ls,UGrid);

  int threads = GridThread::GetThreads();

  std::cout<<GridLogMessage << "Grid is setup to use "<<threads<<" threads"<<std::endl;

  std::vector<int> seeds({1,2,3,4});
  GridParallelRNG          pRNG4(UGrid);
  GridParallelRNG          pRNG5(FGrid);
  pRNG4.SeedFixedIntegers(seeds);
  pRNG5.SeedFixedIntegers(seeds);

  typedef typename ImprovedStaggeredFermion5DR::FermionField FermionField; 
  typedef typename ImprovedStaggeredFermion5DR::ComplexField ComplexField; 
  typename ImprovedStaggeredFermion5DR::ImplParams params; 

  FermionField src   (FGrid);

  //random(pRNG5,src);
  std::vector<int> site({0,0,0,0,0});
  ColourVector cv = zero;
  cv()()(0)=1.0;
  src = zero;
  pokeSite(cv,src,site);

  FermionField result(FGrid); result=zero;
  FermionField    tmp(FGrid);    tmp=zero;
  FermionField    err(FGrid);    tmp=zero;
  FermionField phi   (FGrid); random(pRNG5,phi);
  FermionField chi   (FGrid); random(pRNG5,chi);

  LatticeGaugeField Umu(UGrid); SU3::ColdConfiguration(pRNG4,Umu);

  double volume=Ls;
  for(int mu=0;mu<Nd;mu++){
    volume=volume*latt_size[mu];
  }  

  RealD mass=0.1;
  RealD c1=9.0/8.0;
  RealD c2=-1.0/24.0;
  RealD u0=1.0;

  ImprovedStaggeredFermion5DR  Ds(Umu,Umu,*FGrid,*FrbGrid,*UGrid,*UrbGrid,mass,c1,c2,u0,params);
  ImprovedStaggeredFermionVec5dR sDs(Umu,Umu,*sFGrid,*sFrbGrid,*sUGrid,*sUrbGrid,mass,c1,c2,u0,params);

  std::cout<<GridLogMessage<<"=========================================================="<<std::endl;
  std::cout<<GridLogMessage<<"= Testing Dhop against cshift implementation         "<<std::endl;
  std::cout<<GridLogMessage<<"=========================================================="<<std::endl;

  std::cout<<GridLogMessage << "Calling staggered operator"<<std::endl;
  int ncall=1000;
  double t0=usecond();
  for(int i=0;i<ncall;i++){
    Ds.Dhop(src,result,0);
  }
  double t1=usecond();

  double flops=(16*(3*(6+8+8)) + 15*3*2)*volume*ncall; // == 66*16 +  == 1146
  
  std::cout<<GridLogMessage << "Called Ds"<<std::endl;
  std::cout<<GridLogMessage << "norm result "<< norm2(result)<<std::endl;
  std::cout<<GridLogMessage << "mflop/s =   "<< flops/(t1-t0)<<std::endl;


  std::cout<<GridLogMessage << "Calling vectorised staggered operator"<<std::endl;


  FermionField ssrc  (sFGrid);  localConvert(src,ssrc);
  FermionField sresult(sFGrid); sresult=zero;

  QCD::StaggeredKernelsStatic::Opt=QCD::StaggeredKernelsStatic::OptHandUnroll;
  t0=usecond();
  for(int i=0;i<ncall;i++){
    sDs.Dhop(ssrc,sresult,0);
  }
  t1=usecond();
  localConvert(sresult,tmp);
 
  std::cout<<GridLogMessage << "Called sDs"<<std::endl;
  std::cout<<GridLogMessage << "norm result "<< norm2(sresult)<<std::endl;
  std::cout<<GridLogMessage << "mflop/s =   "<< flops/(t1-t0)<<std::endl;

  QCD::StaggeredKernelsStatic::Opt=QCD::StaggeredKernelsStatic::OptInlineAsm;

  err = tmp-result; 
  std::cout<<GridLogMessage << "norm diff   "<< norm2(err)<<std::endl;

  t0=usecond();
  for(int i=0;i<ncall;i++){
    sDs.Dhop(ssrc,sresult,0);
  }
  t1=usecond();
  localConvert(sresult,tmp);
 
  std::cout<<GridLogMessage << "Called sDs asm"<<std::endl;
  std::cout<<GridLogMessage << "norm result "<< norm2(sresult)<<std::endl;
  std::cout<<GridLogMessage << "mflop/s =   "<< flops/(t1-t0)<<std::endl;

  err = tmp-result; 
  std::cout<<GridLogMessage << "norm diff   "<< norm2(err)<<std::endl;


  Grid_finalize();
}