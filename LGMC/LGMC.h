
#ifndef LGMC_H
#define LGMC_H

#include "stdio.h"
#include "memory.h"
#include "lattice.h"
#include "../GMC/reaction_network.h"
#include "../core/sampler.h"
#include "../core/simulation.h"
#include "LatSolver.h"

struct LGMCParameters {
    float latconst;                               
    float boxxlo,boxxhi,boxylo,                   
          boxyhi,boxzlo,boxzhi;                       
    bool xperiodic, yperiodic, zperiodic;
    float temperature;
    float potential;
};

namespace LGMC_NS {

class LGMC {
    public: 
        LGMC(SqlConnection &reaction_network_database, SqlConnection &initial_state_database, 
            LGMCParameters parameters);
        
        ~LGMC();

        /* -------------------------------- Updates LGMC ----------------------------- */

        void update_state(Lattice *lattice, std::unordered_map<std::string,                     
                        std::vector< std::pair<double, int> > > &props,
                        std::vector<int> &state, int next_reaction, 
                        std::optional<int> site_one, std::optional<int> site_two, 
                        double prop_sum, int active_indices);

        void update_propensities(Lattice *lattice, std::vector<int> &state, std::function<void(Update update)> update_function, 
                                        std::function<void(LatticeUpdate lattice_update)> lattice_update_function, 
                                        int next_reaction, std::optional<int> site_one, std::optional<int> site_two);

        void update_adsorp_state(Lattice *lattice, std::unordered_map<std::string, std::vector< std::pair<double, int> > > &props,
                                double prop_sum, int active_indices); 

        void update_adsorp_props(Lattice *lattice, std::function<void(LatticeUpdate lattice_update)> lattice_update_function, std::vector<int> &state);

        /* -------------------------------- Updates Lattice ----------------------------- */

        bool update_state(Lattice *lattice, std::unordered_map<std::string,                     
                        std::vector< std::pair<double, int> > > &props, 
                        int next_reaction, int site_one, int site_two, 
                        double prop_sum, int active_indices);

        void clear_site(Lattice *lattice, std::unordered_map<std::string,                     
                        std::vector< std::pair<double, int> > > &props,
                        int site, std::optional<int> ignore_neighbor, 
                        double prop_sum, int active_indices);

        void clear_site_helper(std::unordered_map<std::string,                     
                        std::vector< std::pair<double, int> > > &props,
                        int site_one, int site_two, double &prop_sum, 
                        int active_indices);

        void relevant_react(Lattice *lattice, std::function<void(LatticeUpdate lattice_update)> update_function,
                                            int site, std::optional<int> ignore_neighbor);

        double compute_propensity(int num_one, int num_two, int react_id);

        bool update_propensities(Lattice *lattice, std::vector<int> &state,
                        std::function<void(LatticeUpdate lattice_update)> 
                        update_function, int next_reaction, int site_one, int site_two);

        std::string make_string(int site_one, int site_two);

        double sum_row(std::string hash, std::unordered_map<std::string,                     
                        std::vector< std::pair<double, int> > > &props);


        /* -------------------------------------------------------------------------------- */
        // convert a history element as found a simulation to history
        // to a SQL type.
        TrajectoriesSql history_element_to_sql(
            int seed,
            HistoryElement history_element);
        
        std::vector<double> initial_propensities;
        std::vector<int> initial_state;
        Lattice *initial_lattice;                          // lattice for SEI

        // model compatibility 
        void update_state(std::vector<int> &state,
        int reaction_index) {assert(false);};

        void update_propensities(
        std::function<void(Update update)> update_function,
        std::vector<int> &state,
        int next_reaction) {assert(false);};

    private:                                                          
                           
        LatticeReactionNetwork *react_net;                   // pointer to gillespie reaction network

        Sampler sampler;

};

}

#endif
