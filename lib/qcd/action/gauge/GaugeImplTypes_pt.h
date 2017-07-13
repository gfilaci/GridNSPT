/*************************************************************************************

Grid physics library, www.github.com/paboyle/Grid

Source file: ./lib/qcd/action/gauge/GaugeImpl_pt.h

Copyright (C) 2015

Author: paboyle <paboyle@ph.ed.ac.uk>
Author: Gianluca Filaci <g.filaci@ed.ac.uk>

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

See the full license in the file "LICENSE" in the top level distribution
directory
*************************************************************************************/
/*  END LEGAL */
#ifndef GRID_GAUGE_IMPL_TYPES_PT_H
#define GRID_GAUGE_IMPL_TYPES_PT_H

namespace Grid {
namespace QCD {
namespace QCDpt{
////////////////////////////////////////////////////////////////////////
// Implementation dependent gauge types
////////////////////////////////////////////////////////////////////////

#define INHERIT_GIMPL_TYPES(GImpl)                  \
  typedef typename GImpl::Simd Simd;                \
  typedef typename GImpl::LinkField GaugeLinkField; \
  typedef typename GImpl::Field GaugeField;         \
  typedef typename GImpl::ComplexField ComplexField;\
  typedef typename GImpl::SiteField SiteGaugeField; \
  typedef typename GImpl::SiteComplex SiteComplex;  \
  typedef typename GImpl::SiteLink SiteGaugeLink;

#define INHERIT_FIELD_TYPES(Impl)		    \
  typedef typename Impl::Simd Simd;		    \
  typedef typename Impl::ComplexField ComplexField; \
  typedef typename Impl::SiteField SiteField;	    \
  typedef typename Impl::Field Field;

// hardcodes the exponential approximation in the template
template <class S, int Nrepresentation = Nc> class GaugeImplTypes_pt {
public:
  typedef S Simd;

  template <typename vtype> using iImplScalar     = iScalar<iScalar<iPert<iScalar<vtype>, Np> > >;
  template <typename vtype> using iImplGaugeLink  = iScalar<iScalar<iPert<iMatrix<vtype, Nrepresentation>, Np> > >;
  template <typename vtype> using iImplGaugeField = iVector<iScalar<iPert<iMatrix<vtype, Nrepresentation>, Np> >, Nd>;

  typedef iImplScalar<Simd>     SiteComplex;
  typedef iImplGaugeLink<Simd>  SiteLink;
  typedef iImplGaugeField<Simd> SiteField;

  typedef Lattice<SiteComplex> ComplexField;
  typedef Lattice<SiteLink>    LinkField; 
  typedef Lattice<SiteField>   Field;

  // Guido: we can probably separate the types from the HMC functions
  // this will create 2 kind of implementations
  // probably confusing the users
  // Now keeping only one class


  // Move this elsewhere? FIXME
  static inline void AddLink(Field &U, LinkField &W,
                                  int mu) { // U[mu] += W
    PARALLEL_FOR_LOOP
    for (auto ss = 0; ss < U._grid->oSites(); ss++) {
      U._odata[ss]._internal[mu] =
          U._odata[ss]._internal[mu] + W._odata[ss]._internal;
    }
  }

  ///////////////////////////////////////////////////////////
  // Move these to another class
  // HMC auxiliary functions
  static inline void generate_momenta(Field &P, GridParallelRNG &pRNG) {
    // specific for SU gauge fields
    LinkField Pmu(P._grid);
    Pmu = zero;
    for (int mu = 0; mu < Nd; mu++) {
      SU<Nrepresentation>::GaussianFundamentalLieAlgebraMatrix(pRNG, Pmu);
      PokeIndex<LorentzIndex>(P, Pmu, mu);
    }
  }

  static inline Field projectForce(Field &P) { return Ta(P); }
//$//
//  static inline void update_field(Field& P, Field& U, double ep){
//    //static std::chrono::duration<double> diff;
//
//    //auto start = std::chrono::high_resolution_clock::now();
//    parallel_for(int ss=0;ss<P._grid->oSites();ss++){
//      for (int mu = 0; mu < Nd; mu++) 
//        U[ss]._internal[mu] = ProjectOnGroup(Exponentiate(P[ss]._internal[mu], ep, Nexp) * U[ss]._internal[mu]);
//    }
//    
//    //auto end = std::chrono::high_resolution_clock::now();
//   // diff += end - start;
//   // std::cout << "Time to exponentiate matrix " << diff.count() << " s\n";
//  }

  static inline RealD FieldSquareNorm(Field& U){
    LatticeComplex Hloc(U._grid);
    Hloc = zero;
    for (int mu = 0; mu < Nd; mu++) {
      auto Umu = PeekIndex<LorentzIndex>(U, mu);
      Hloc += trace(Umu * Umu);
    }
    Complex Hsum = sum(Hloc);
    return Hsum.real();
  }

  static inline void HotConfiguration(GridParallelRNG &pRNG, Field &U) {
    SU<Nc>::HotConfiguration(pRNG, U);
  }

  static inline void TepidConfiguration(GridParallelRNG &pRNG, Field &U) {
    SU<Nc>::TepidConfiguration(pRNG, U);
  }

  static inline void ColdConfiguration(GridParallelRNG &pRNG, Field &U) {
    SU<Nc>::ColdConfiguration(pRNG, U);
  }
  
};

// perturbative implementation types
typedef GaugeImplTypes_pt<vComplex, Nc> GimplTypes_ptR;

} // QCDpt

// give perturbative trait to the field of a perturbative implementation type
template<> struct isPerturbative<QCDpt::GimplTypes_ptR::Field > {
  static const bool value = true;
  static const bool notvalue = false;
};

} // QCD
} // Grid

#endif // GRID_GAUGE_IMPL_TYPES_H
