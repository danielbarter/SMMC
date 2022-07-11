#include "lattice.h"
#include <cassert> 
#include "stdio.h"
#include "stdlib.h"
#include <string>
#include "math.h"
#include <iostream>

using namespace LGMC_NS;

#define DELTALOCAL 10000
#define DELTA 32768
#define EPSILON 0.0001


/* ---------------------------------------------------------------------- */

Lattice::Lattice(float latconst_in, 
        int boxxlo_in, int boxxhi_in, int boxylo_in,
        int boxyhi_in, int boxzlo_in, int boxzhi_in,
        bool is_xperiodic_in, bool is_yperiodic_in, bool is_zperiodic_in)  {
    // TODO: implement error handling
    
    memory = new Memory();

    latconst = latconst_in;
    
    // region of simulation input * lattice spacing
    boxxlo = boxxlo_in * latconst;
    boxxhi = boxxhi_in * latconst;
    boxylo = boxylo_in * latconst;
    boxyhi = boxyhi_in * latconst;
    boxzlo = boxzlo_in * latconst;
    boxzhi = boxzhi_in * latconst;
    
    // 0 = non-periodic, 1 = periodic
    is_xperiodic = is_xperiodic_in;
    is_yperiodic = is_yperiodic_in;
    is_zperiodic = is_zperiodic_in;
    
    nsites = nmax = 0;
    sites = NULL;
    
    maxneigh = 6;
    numneigh = NULL;
    idneigh = NULL;
    
    // create sites on lattice
    structured_lattice();
    
    // set neighbors of each site
    structured_connectivity();
    
    /*for(int n = 0; n < nsites; ++n) {
        std::cout << "id: " << n << " [" << xyz[n][0] << ", " <<
        xyz[n][1] << ", " << xyz[n][2] << "]" << std::endl;
    }*/

} // Lattice()

/* ---------------------------------------------------------------------- */

Lattice::~Lattice() {

    memory->destroy(sites);
    
    memory->destroy(numneigh);
    memory->destroy(idneigh);

    delete memory;
} // ~Lattice()


/* ---------------------------------------------------------------------- */

void Lattice::structured_lattice() {
    
    // if not fully periodic IDs may be non-contiguous and/or ordered irregularly
    uint32_t nx, ny, nz;
    nx = (boxxhi - boxxlo / latconst);
    ny = (boxyhi - boxylo / latconst);
    nz = (boxzhi - boxzlo / latconst);

    // if dim is periodic:
    //    lattice origin = lower box boundary
    //    loop bounds = 0 to N-1
    // if dim is non-periodic:
    //   lattice origin = 0.0
    //   loop bounds = enough to tile box completely, with all basis atoms
    
    if (is_xperiodic) {
        xlo = 0;
        xhi = nx-1;
      }
    else {
        xlo = (boxxlo / latconst);
        while ((xlo+1)*latconst > boxxlo) xlo--;
        xlo++;
        xhi = (boxxhi / latconst);
        while (xhi*latconst <= boxxhi) xhi++;
        xhi--;
      }

    if (is_yperiodic) {
        ylo = 0;
        yhi = ny-1;
    }
    else {
        ylo = (boxylo / latconst);
        while ((ylo+1)*latconst > boxylo) ylo--;
        ylo++;
        yhi = (boxyhi / latconst);
        while (yhi*latconst <= boxyhi) yhi++;
        yhi--;
    }

    if (is_zperiodic) {
        zlo = 0;
        zhi = nz-1;
    }
    else {
        zlo = (boxzlo / latconst);
        while ((zlo+1)*latconst > boxzlo) zlo--;
        zlo++;
        zhi = (boxzhi / latconst);
        while (zhi*latconst <= boxzhi) zhi++;
        zhi--;
    }

    
    // generate xyz coords and store them with site ID
    // tile the simulation box from origin, respecting PBC
    // site IDs should be contiguous if style = BOX and fully periodic
    // for non-periodic dims, check if site is within global box
    // for style = REGION, check if site is within region
    // if non-periodic or style = REGION, IDs may not be contiguous

    uint32_t x,y,z;

    uint32_t n = 0;
    for (int k = zlo; k <= zhi; k++)
        for (int j = ylo; j <= yhi; j++)
            for (int i = xlo; i <= xhi; i++) {
                n++;
                x = i*latconst;
                y = j*latconst;
                z = k*latconst;

                add_site(n,x,y,z);

    }


} // structered_lattice()

/* ----------------------------------------------------------------------
   generate site connectivity for on-lattice applications
 ------------------------------------------------------------------------- */

void Lattice::structured_connectivity() {

    int ineigh,jneigh,kneigh;
    uint32_t gid;
    int xneigh,yneigh,zneigh;
    
    int xprd = boxxhi - boxxlo;
    int yprd = boxyhi - boxylo;
    int zprd = boxzhi - boxzlo;
    
    int nx = xprd / latconst;
    int ny = yprd / latconst;
    int nz = zprd / latconst;
    
    memory->create(idneigh,nsites,maxneigh,"create:idneigh");
    memory->create(numneigh,nmax,"create:numneigh");
    
    // create connectivity offsets
    
    int **cmap;                 // connectivity map for regular lattices
                                // cmap[maxneigh][3]
                                // 0,1,2 = i,j,k lattice unit cell offsets
    
    memory->create(cmap,maxneigh,3,"create:cmap");

    offsets_3d(cmap);
    
    // generate global lattice connectivity for each site
    for (int i = 0; i < nsites; i++) {
        numneigh[i] = 0;
      
        for (int neigh = 0; neigh < maxneigh; neigh++) {

            // ijkm neigh = indices of neighbor site
            // calculated from siteijk and cmap offsets

            ineigh = static_cast<int> (sites[i].x/latconst) + cmap[neigh][0];
            jneigh = static_cast<int> (sites[i].y/latconst) + cmap[neigh][1];
            kneigh = static_cast<int> (sites[i].z/latconst) + cmap[neigh][2];

            // xyz neigh = coords of neighbor site
            // calculated in same manner that structured_lattice() generated coords
            
            xneigh = ineigh * static_cast<int> (latconst);
            yneigh = jneigh * static_cast<int> (latconst);
            zneigh = kneigh * static_cast<int> (latconst);
            
            // remap neighbor coords and indices into periodic box via ijk neigh
            // remap neighbor coords and indices into periodic box via ijk neigh

            if (is_xperiodic) {
                if (ineigh < 0) {
                    xneigh += xprd;
                    ineigh += nx;
                }
                if (ineigh >= nx) {
                    xneigh -= xprd;
                    xneigh = MAX(xneigh, static_cast<int> (boxxlo));
                    ineigh -= nx;
                }
            }
            if (is_yperiodic) {
                if (jneigh < 0) {
                    yneigh += yprd;
                    jneigh += ny;
                }
                if (jneigh >= ny) {
                    yneigh -= yprd;
                    yneigh = MAX(yneigh, static_cast<int> (boxylo));
                    jneigh -= ny;
                }
            }
            if (is_zperiodic) {
                if (kneigh < 0) {
                    zneigh += zprd;
                    kneigh += nz;
                }
                if (kneigh >= nz) {
                    zneigh -= zprd;
                    zneigh = MAX(zneigh, static_cast<int> (boxzlo));
                    kneigh -= nz;
                }
            }

            // discard neighs that are outside non-periodic box or region
            if (!is_xperiodic && (xneigh < boxxlo || xneigh > boxxhi)) continue;
            if (!is_yperiodic && (yneigh < boxylo || yneigh > boxyhi)) continue;
            if (!is_zperiodic && (zneigh < boxzlo || zneigh > boxzhi)) continue;

            // gid = global ID of neighbor
            // calculated in same manner that structured_lattice() generated IDs
            gid = uint32_t((kneigh-zlo)*(yhi-ylo+1)*(xhi-xlo+1)) +
                  uint32_t((jneigh-ylo)*(xhi-xlo+1)) + uint32_t((ineigh-xlo));
            
            
        /*std::cout << "neighbor: (" << sites[gid].x << ", " << sites[gid].y << ", " 
                  << sites[gid].z << ") for: " << "[" << sites[i].x << ", " << 
                  sites[i].y << ", " << sites[i].z << "]" << std::endl;*/
            
        // add gid to neigh list of site i
        idneigh[i][numneigh[i]++] = gid;
      }
    }

    std::cout << "numneigh" << std::endl;
    for(int i = 0; i < nsites; i++) {
        std::cout << "[" << sites[i].x << ", " <<
        sites[i].y << ", " << sites[i].z << "]" << ",";
        std::cout << "num: " << numneigh[i] << std::endl;
    }
    
    std::cout << "idneigh" << std::endl;
    for(int i = 0; i < nsites; i++) {
        std::cout << "neighbors: ";
        for(int j = 0; j < maxneigh; j++) {
            std::cout << idneigh[i][j] << ", ";
        }
        std::cout << std::endl;
    }
    
    memory->destroy(cmap);
} // structured_connectivity()

/* ---------------------------------------------------------------------- */

void Lattice::offsets_3d(int **cmapin) {
    
    int n = 0;
    double delx,dely,delz,r;
    double cutoff = 1*latconst;

    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            for (int k = -1; k <= 1; k++) {
                delx = i * latconst;
                dely = j * latconst;
                delz = k * latconst;
                r = sqrt(delx * delx + dely * dely + delz * delz);
                if (r > cutoff - EPSILON && r < cutoff + EPSILON) {
                    assert(n != maxneigh);
                    cmapin[n][0] = i;
                    cmapin[n][1] = j;
                    cmapin[n][2] = k;
                    n++;
                }
            }
        }
    }
  assert(n == maxneigh);
} // offsets_3d

/* ---------------------------------------------------------------------- */

void Lattice::add_site(uint32_t n, uint32_t x, uint32_t y, uint32_t z) {
    if (nsites == nmax) grow(0);

    // initially empty site, species = -1
    sites[n] = Site{x, y, z, -1};

    nsites++;
} // add_site()

/* ---------------------------------------------------------------------- */

void Lattice::grow(uint32_t n) {
    if (n == 0) nmax += DELTA;
    else nmax = n;

    memory->grow(sites,nmax,"grow:sites");
    
} // grow()
